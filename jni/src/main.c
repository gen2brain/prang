/***************************************************************************
    copyright            : (C) 2003 by Michael Speck
    email                : http://lgames.sf.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include "defs.h"
#include "shots.h"
#include "units.h"
#include "data.h"
#include "chart.h"
#include "bfield.h"
#include "menu.h"

int window_w = 640;
int window_h = 480;

SDL_Window *window;
SDL_Renderer *renderer;

SDL_PixelFormat *format;
Uint32 pformat;

int quit = 0;
int delay = 0;
int fullscreen = 1;

int audio_on = 1;
int audio_freq = 22050;
int audio_format = AUDIO_S16LSB;
int audio_channels = 2;
int audio_mix_channel_count = 0;

char player_text[128];

extern char *optarg;
extern SDL_Texture *img_ammo, *background, *img_ground, *img_logo, *img_icons, *img_black;
extern SDL_Texture *img_cursors, *img_gun_reload, *img_small_grenade, *img_large_grenade;
extern SFont_Font *ft_main, *ft_chart, *ft_chart_highlight, *ft_help;
extern SDL_Sound *wav_empty;
extern SDL_Cursor *cr_empty;
extern int ammo_w, ammo_h;
extern int cursor_w, cursor_x_offset;
extern int gun_reload_w, gun_reload_h;
extern BField bfield;

Menu main_menu;
enum {
    ID_GAME = 101,
    ID_HELP,
    ID_CHART,
    ID_QUIT
};

Vector gun_base = { 20, 240, 0 };
Vector gun_img_offset = { -9, -20, 0 };

int player_score = 0;
int player_ammo = 0;

int game_start = 0, game_end = 0; /* time in global secs when game started and has to end */

int timer_blink_delay = 0; /* timer starts to blink when time runs out */
int timer_visible = 1; /* used to blink timer */

enum {
    STATE_MENU,
    STATE_HELP,
    STATE_CHART,
    STATE_GAME,
    STATE_SCORE
};

int state = STATE_MENU; /* state of game */

static void draw_score() {
    char str[8];

    if ( player_score < 0 ) player_score = 0;
    snprintf( str, 8, "%06i", player_score ); str[7] = 0;
    SDL_WriteText( ft_main, 10, 10, str );
}

static void draw_time(int ms) {
    static int old_secs = 0;
    char str[8];
    int secs = game_end - time(0);

    /* blink timer when time is running out in such a way
     * that it becomes visible everytime a new second started */
    if ( secs < 30 ) {
        if ( timer_blink_delay )
            if ( (timer_blink_delay-=ms) <= 0 )
                timer_visible = 0;
        if ( secs != old_secs ) {
            timer_visible = 1;
            timer_blink_delay = 500;
            old_secs = secs;
        }
    }

    if ( !timer_visible ) return;
    snprintf( str, 8, "%i: %02i", secs/60, secs%60 ); str[7] = 0;
    SDL_WriteTextRight( ft_main, 630, 10, str );
}

void draw_reload_button() {
    SDL_Rect rect;
    rect.w = gun_reload_w;
    rect.h = gun_reload_h;
    rect.y = 160 - 10 - rect.h;
    rect.x = 10;
    SDL_RenderCopy( renderer, img_gun_reload, NULL, &rect );
}

int reload_button_clicked( int x, int y ) {
    SDL_Rect rect;
    rect.w = gun_reload_w;
    rect.h = gun_reload_h;
    rect.y = 160 - 10 - rect.h;
    rect.x = 10;

    if ( ( x >= rect.x && x < rect.x + rect.w + 29 )
            && ( y >= rect.y && y < rect.y + rect.h ) )
        return 1;
    return 0;
}

void draw_toggle_grenade(int large) {
    SDL_Texture *img;
    SDL_Rect rect;
    rect.h = 21;
    rect.y = 100 - 10 - rect.h;
    rect.x = 10;

    if ( large ) {
        img = img_large_grenade;
        rect.w = 53;
    } else {
        img = img_small_grenade;
        rect.w = 8;
    }

    SDL_RenderCopy( renderer, img, NULL, &rect );
}

int toggle_grenade_clicked( int x, int y ) {
    SDL_Rect rect;
    rect.w = 53;
    rect.h = 21;
    rect.y = 100 - 10 - rect.h;
    rect.x = 10;

    if ( ( x >= rect.x && x < rect.x + rect.w )
            && ( y >= rect.y && y < rect.y + rect.h ) )
        return 1;
    return 0;
}

static void draw_ammo() {
    int i, full, rest;
    SDL_Rect drect, srect;

    full = player_ammo / AMMO_BOMB; /* one tile represents a bomb ammo which
                                       contains of AMMO_BOMB single grenade shots */
    rest = player_ammo % AMMO_BOMB; /* first ammo may contain less than AMMO_BOMB ammo */

    drect.w = srect.w = ammo_w;
    drect.h = srect.h = ammo_h;
    srect.x = 0; srect.y = 0;
    drect.y = 480 - 10 - drect.h;
    drect.x = 10;

    for ( i = 0; i < full; i++ ) {
        SDL_RenderCopy( renderer, img_ammo, &srect, &drect );
        drect.y -= drect.h;
    }

    if ( rest > 0 ) {
        srect.y = ( AMMO_BOMB - rest ) * ammo_h;
        SDL_RenderCopy( renderer, img_ammo, &srect, &drect );
    }

#ifdef __ANDROID__
    if ( player_ammo != MAX_AMMO )
        draw_reload_button();
#endif
}

/* draw cursor centered at x,y */
static void draw_cursor( int x, int y ) {
#ifdef __ANDROID__
    if ( reload_button_clicked( x, y ) || toggle_grenade_clicked( x, y ) )
        return;
#endif

    SDL_Rect srect,drect;

    int w, h;
    SDL_QueryTexture( img_cursors, NULL, NULL, &w, &h );

    drect.w = srect.w = cursor_w;
    drect.h = srect.h = h;
    srect.x = cursor_x_offset; srect.y = 0;
    /* XXX: within gun strip, gun cannot be fired; to indicate this use
     * disabled cursor instead of current selection. (bad hack, state
     * should be properly set somewhere) */
    if ( x < STRIP_OFFSET && !bfield.demo )
        srect.x = CURSOR_DISABLED * cursor_w;
    drect.x = x - (cursor_w>>1); drect.y = y - (h>>1);
    SDL_RenderCopy( renderer, img_cursors, &srect, &drect );
}

static void draw_logo_centered( int y ) {
    SDL_Rect drect;

    int img_logo_w, img_logo_h;
    SDL_QueryTexture( img_logo, NULL, NULL, &img_logo_w, &img_logo_h );

    drect.x = (window_w-img_logo_w)/2;
    drect.y = y;
    drect.w = img_logo_w; drect.h = img_logo_h;

    SDL_RenderCopy( renderer, img_logo, NULL, &drect );
}

#define LINE(y,text) SDL_WriteText(ft_help,x,y,text)
static void draw_help() {
    int x = 70, target_y = 90, gun_y = 200, hint_y = 360;
    SDL_Rect srect, drect;

    SDL_RenderCopy( renderer, img_black, NULL, NULL );

    /* objective */
    LINE( 30, "Your objective is to destroy as many dummy" );
    LINE( 50, "targets as possible within 3 minutes." );

    /* icons and points */
    SDL_WriteText( ft_help, x,target_y, "Target Scores:" );
    srect.w = drect.w = 30;
    srect.h = drect.h = 30;
    srect.y = 0; drect.y = target_y+20;
    srect.x = 0; drect.x = x;
    SDL_RenderCopy(renderer, img_icons, &srect, &drect);
    SDL_WriteText( ft_help, x + 34,target_y+24, "Soldier: 5 Pts" );
    srect.x = 30; drect.x = x+180;
    SDL_RenderCopy(renderer, img_icons, &srect, &drect);
    SDL_WriteText( ft_help, x+214,target_y+24, "Jeep: 20 Pts" );
    srect.x = 60; drect.x = x+350;
    SDL_RenderCopy(renderer, img_icons, &srect, &drect);
    SDL_WriteText( ft_help, x+384,target_y+24, "Tank: 50 Pts" );
    SDL_WriteText( ft_help, x,target_y+50,
            "If a jeep makes it safe through the screen you'll" );
    SDL_WriteText( ft_help, x,target_y+70,
            "loose 10 pts. For a tank you'll loose 25 pts. For" );
    SDL_WriteText( ft_help, x,target_y+90,
            "soldiers there is no such penalty." );

    /* gun handling */
#ifdef __ANDROID__
    gun_y += 20;
    LINE( gun_y, "Gun Handling:");
    LINE( gun_y+20, "Tap on screen to fire a grenade. You can toggle" );
    LINE( gun_y+40, "between a large (costs 6 rounds, 0.5s fire rate)" );
    LINE( gun_y+60, "and a small grenade (1 round). Tap the button to" );
    LINE( gun_y+80, "reload your gun to the full 36 rounds. Reloading" );
    LINE( gun_y+100, "costs 36 points so do this as late as possible." );
#else
    LINE( gun_y, "Gun Handling:");
    LINE( gun_y+20, "The gun is controlled by the mouse. Move the" );
    LINE( gun_y+40, "crosshair at the target and left-click to fire a" );
    LINE( gun_y+60, "large grenade. (costs 6 rounds, 0.5 secs fire rate)" );
    LINE( gun_y+80, "Middle-click to fire a small grenade. (1 round)" );
    LINE( gun_y+100, "Use the right mouse button to reload your gun" );
    LINE( gun_y+120, "to the full 36 rounds. Reloading always costs" );
    LINE( gun_y+140, "36 points so do this as late as possible." );
#endif

    /* hint */
    LINE( hint_y,    "NOTE: A tank cannot be damaged by small" );
    LINE( hint_y+20, "grenades which you should only use at soldiers" );
    LINE( hint_y+40, "and jeeps when you are experienced enough in" );
    LINE( hint_y+60, "aiming." );
}

static void draw_result( int rank ) {
    char buf[64];

    /* check wether highscore was entered */
    if ( rank >= 0 ) {
        SDL_WriteTextCenter( ft_chart, 320, 50, "The time is up!" );
        sprintf( buf, "Your result is %i points.", player_score );
        SDL_WriteTextCenter( ft_chart, 320, 80, buf );

        sprintf( buf, "This makes you Topgunner #%i", rank+1 );
        SDL_WriteTextCenter( ft_chart, 320, 120, buf );
        SDL_WriteTextCenter( ft_chart, 320, 160, "Sign here:" );
        SDL_WriteTextCenter( ft_chart, 320, 190, player_text );
    }
    else {
        SDL_WriteTextCenter( ft_chart, 320, 130, "The time is up!" );
        sprintf( buf, "Your result is %i points.", player_score );
        SDL_WriteTextCenter( ft_chart, 320, 160, buf );

        SDL_WriteTextCenter( ft_chart, 320, 290, "Not enough to be a topgunner!" );
        SDL_WriteTextCenter( ft_chart, 320, 320, "Try again! Dismissed." );
    }
}

static void game_init() {
    bfield_finalize(); /* clear battlefield from menu */
    bfield_init( BFIELD_GAME, &gun_base, &gun_img_offset );
    player_score = 0;
    player_ammo = MAX_AMMO;

    SET_CURSOR( CURSOR_CROSSHAIR );
    SDL_RenderCopy( renderer, background, NULL, NULL );

    game_start = time(0); game_end = game_start + GAME_TIME; timer_visible = 1;
}

static void game_finalize() {
    bfield_finalize();
    SET_CURSOR( CURSOR_ARROW );

    bfield_init( BFIELD_DEMO, &gun_base, &gun_img_offset ); /* new battlefield for menu */
    bfield_draw( 0, 0 );
}

static void toggle_fullscreen() {
    Uint32 flags = SDL_GetWindowFlags( window );
    flags ^= SDL_WINDOW_FULLSCREEN_DESKTOP;

    if ( SDL_SetWindowFullscreen( window, flags ) < 0 ) {
        LOG( "%s\n", SDL_GetError() );
    }

    if ( (flags & SDL_WINDOW_FULLSCREEN_DESKTOP) != 0 ) {
        SDL_SetHint( SDL_HINT_RENDER_SCALE_QUALITY, "linear" );
        SDL_RenderSetLogicalSize( renderer, 640, 480 );
    } else {
        SDL_SetWindowSize( window, 640, 480 );
    }
}

static void main_loop() {
    Uint8 *keystate, empty_keys[SDL_NUM_SCANCODES];
    int x = 0, y = 0;
    int cur_ticks, prev_ticks;
    int ms;
    int reload_clicked = 0;
    int reload_key = SDL_SCANCODE_SPACE;
    int frame_time = 0, frames = 0;
    Vector dest;
    int input_delay = 0; /* while >0 no clicks are accepted */
    int cursor_delay = 0; /* show cursor on android after fire gun */
    SDL_Keymod modstate = 0;
    int shot_type;
    int rank = -1;
#ifdef __ANDROID__
    int large_grenade = 1;
#endif

    frame_time = SDL_GetTicks(); /* for frame rate */
    memset( empty_keys, 0, sizeof( empty_keys ) ); /* to block input */

    state = STATE_MENU;
    bfield_init( BFIELD_DEMO, &gun_base, &gun_img_offset );

    cur_ticks = prev_ticks = SDL_GetTicks();
    while ( !quit ) {
        if ( delay > 0 ) SDL_Delay(delay);

        prev_ticks = cur_ticks;
        cur_ticks = SDL_GetTicks();
        ms = cur_ticks - prev_ticks;

        keystate = (Uint8 *) SDL_GetKeyboardState( NULL );
        modstate = SDL_GetModState();

        /* update the battefield (particles,units,new cannonfodder) */
        if ( state != STATE_SCORE )
            bfield_update( ms );

        /* update input_delay */
        if ( input_delay > 0 )
            if ( (input_delay-=ms) < 0 ) input_delay = 0;

        /* update cursor delay */
        if (  cursor_delay > 0 )
            if ( (cursor_delay-=ms) < 0 ) cursor_delay = 0;

        /* draw ground, trees, units, particles */
        bfield_draw( 0, 0 );

        /* according to state draw add-ons */
        switch ( state ) {
            case STATE_GAME:
                draw_score();
                draw_time( ms );
                draw_ammo();
#ifdef __ANDROID__
                draw_toggle_grenade( large_grenade );
#endif
                break;
            case STATE_MENU:
                draw_logo_centered( 70 );
                menu_draw( &main_menu );
                break;
            case STATE_CHART:
                chart_draw();
                break;
            case STATE_HELP:
                draw_help();
                break;
            case STATE_SCORE:
                draw_result( rank );
                break;
        }

        /* add cursor */
#ifdef __ANDROID__
        if ( cursor_delay > 0 )
            draw_cursor( x, y );
#else
        draw_cursor( x, y );
#endif

        /* update screen */
        SDL_RenderPresent( renderer ); frames++;

        /* end game? */
        if ( state == STATE_GAME )
            if ( time(0) >= game_end ) {
                game_finalize();

                rank = chart_get_rank( player_score ); /* check highscore */
                state = STATE_SCORE;
                player_text[0]=0x00;
                if( rank >= 0 )
                    SDL_StartTextInput();

                input_delay = INPUT_DELAY;
                cur_ticks = prev_ticks = SDL_GetTicks(); /* reset timer */
            }

        /* after this point only input is handled */
        if ( input_delay > 0 ) continue;

        SDL_Event e;
        while ( SDL_PollEvent(&e) ){
            if ( e.type == SDL_QUIT ){
                quit = 1;
            }

            if ( e.type == SDL_MOUSEMOTION ){
                x = e.motion.x;
                y = e.motion.y;
                menu_handle_motion( &main_menu, x, y );
            }

            if ( e.type == SDL_KEYDOWN ){

                Uint8 key = e.key.keysym.scancode;

                /* handle input */
                switch ( state ) {
                    case STATE_HELP:
                    case STATE_CHART:
                        if ( key == SDL_SCANCODE_ESCAPE ||
                                keystate[SDL_SCANCODE_AC_BACK] ) {
                            input_delay = INPUT_DELAY;
                            state = STATE_MENU;
                            /* clear the selection of the menu entry */
                            menu_handle_motion( &main_menu, -1, -1 );
                        }
                        break;
                    case STATE_GAME:
                        /* abort game */
                        if ( key == SDL_SCANCODE_ESCAPE ||
                                keystate[SDL_SCANCODE_AC_BACK] ) {
                            game_finalize();
                            cur_ticks = prev_ticks = SDL_GetTicks(); /* reset timer */
                            state = STATE_CHART;
                            input_delay = INPUT_DELAY;
                        }
                        /* reload */
                        if ( key == reload_key ) {
                            if ( !reload_clicked ) {
                                reload_clicked = 1;
                                bfield_reload_gun();
                            }
                        }
                        else
                            reload_clicked = 0;
                        break;
                    case STATE_MENU:
                        if ( key == SDL_SCANCODE_ESCAPE ||
                                keystate[SDL_SCANCODE_AC_BACK] )
                            quit = 1;
                        break;
                    case STATE_SCORE:
                        if ( key == SDL_SCANCODE_ESCAPE ||
                                keystate[SDL_SCANCODE_AC_BACK] ) {
                            state = STATE_CHART;
                        }

                        if ( key == SDL_SCANCODE_RETURN ) {
                            SDL_StopTextInput();
                            chart_add_entry( player_text, player_score );
                            chart_save();
                            state = STATE_CHART;
                        }

                        if ( key == SDL_SCANCODE_BACKSPACE ) {
                             int textlen=SDL_strlen( player_text );

                             do {
                                 if ( textlen==0 ) {
                                     break;
                                 }
                                 if ( (player_text[textlen-1] & 0x80) == 0x00 ) {
                                     /* One byte */
                                     player_text[textlen-1]=0x00;
                                     break;
                                 }
                                 if ( (player_text[textlen-1] & 0xC0) == 0x80 ) {
                                     /* Byte from the multibyte sequence */
                                     player_text[textlen-1]=0x00;
                                     textlen--;
                                 }
                                 if ( (player_text[textlen-1] & 0xC0) == 0xC0 ) {
                                     /* First byte of multibyte sequence */
                                     player_text[textlen-1]=0x00;
                                     break;
                                 }
                             } while(1);
                        }
                        break;
                }

                /* switch fullscreen/window anywhere */
                if ( key == SDL_SCANCODE_F ) {
                    fullscreen = !fullscreen;
                    toggle_fullscreen();
                    input_delay = INPUT_DELAY;
                }
                /* enabe/disable sound anywhere */
                if ( key == SDL_SCANCODE_S && audio_on != -1 ) {
                    audio_on = !audio_on;
                    input_delay = INPUT_DELAY;
                }
            }

            if( e.type == SDL_TEXTINPUT ) {
                if (SDL_strlen( e.text.text ) == 0 || e.text.text[0] == '\n')
                    break;

                if ( SDL_strlen( player_text ) + SDL_strlen( e.text.text ) < sizeof( player_text ))
                    SDL_strlcat( player_text, e.text.text, sizeof( player_text ) );
            }

            if ( e.type == SDL_MOUSEBUTTONDOWN ){

                Uint8 button = e.button.button;
                x = e.button.x;
                y = e.button.y;

                switch ( state ) {
                    case STATE_HELP:
                    case STATE_CHART:
                    case STATE_SCORE:
                        if ( button == SDL_BUTTON_LEFT ) {
                            input_delay = INPUT_DELAY;
                            state = STATE_MENU;
                            /* clear the selection of the menu entry */
                            menu_handle_motion( &main_menu, -1, -1 );
                        }
                        break;
                    case STATE_GAME:
                        /* set direction of gun */
                        bfield_update_gun_dir( x, y );

#ifdef __ANDROID__
                        if ( toggle_grenade_clicked( x, y ) ) {
                            large_grenade = !large_grenade;
                            SDL_PlaySound( wav_empty );
                        }
#endif

                        /* fire gun */
                        if ( bfield_gun_is_ready() ) {
                            dest.x = x; dest.y = y; dest.z = 0;
                            shot_type = ST_NONE;
                            if ( button == SDL_BUTTON_LEFT ) {
                                if ( modstate & (KMOD_LCTRL|KMOD_RCTRL)
#ifdef __ANDROID__
                                        || !large_grenade
#endif
                                    )
                                    shot_type = ST_GRENADE;
                                else
                                    shot_type = ST_BOMB;
                            }
                            if ( button == SDL_BUTTON_MIDDLE )
                                shot_type = ST_GRENADE;
                            if ( shot_type != ST_NONE )
                                bfield_fire_gun( shot_type, &dest, 60, 0 );
                                cursor_delay = CURSOR_DELAY;
                        }
                        /* reload */
                        if ( button == SDL_BUTTON_RIGHT
#ifdef __ANDROID__
                                || reload_button_clicked(x, y)
#endif
                                ) {
                            if ( !reload_clicked ) {
                                reload_clicked = 1;
                                bfield_reload_gun();
                            }
                        }
                        else
                            reload_clicked = 0;
                        break;
                    case STATE_MENU:
                        if ( button == SDL_BUTTON_LEFT ) {
                            switch ( menu_handle_click( &main_menu, x, y ) ) {
                                case ID_QUIT:
                                    quit = 1;
                                    break;
                                case ID_HELP:
                                    state = STATE_HELP;
                                    input_delay = INPUT_DELAY;
                                    break;
                                case ID_CHART:
                                    state = STATE_CHART;
                                    input_delay = INPUT_DELAY;
                                    break;
                                case ID_GAME:
                                    state = STATE_GAME;
                                    game_init();
                                    break;
                            }
                        }
                        break;
                }
            }
        }
    }

    frame_time = SDL_GetTicks() - frame_time;
    LOG( "Time: %.2f, Frames: %i\n", (double)frame_time/1000, frames );
    LOG( "FPS: %.2f\n", 1000.0 * frames / frame_time );

}

void cleanup() {
    data_delete();
    SDL_DestroyRenderer( renderer );
    SDL_DestroyWindow( window );
    window = NULL;
    renderer = NULL;

#ifdef AUDIO_ENABLED
    Mix_CloseAudio();
#endif
    SDL_Quit();
}

void init() {
    if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_TIMER ) < 0 ) {
        LOG( "%s\n", SDL_GetError() );
        exit(1);
    }
    else if ( SDL_InitSubSystem( SDL_INIT_AUDIO ) < 0 ) {
        LOG( "%s\n", SDL_GetError() );
        LOG( "Disabling sound and continuing...\n" );
        audio_on = -1;
    }

    if( !( IMG_Init( IMG_INIT_PNG ) & IMG_INIT_PNG ) )
    {
        LOG( "%s\n", SDL_GetError() );
        exit(1);
    }

    Uint32 wflags = ( fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0 );
    wflags |= SDL_RENDERER_PRESENTVSYNC;

    window = SDL_CreateWindow( "Prang",
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            fullscreen ? 0 : window_w,
            fullscreen ? 0 : window_h,
            wflags );

    if ( window == NULL ) {
        LOG( "%s\n", SDL_GetError() );
        exit(1);
    }

    Uint32 rflags = SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC|SDL_RENDERER_TARGETTEXTURE;

    renderer = SDL_CreateRenderer( window, -1, rflags );
    if ( renderer == NULL ) {
        LOG( "%s\n", SDL_GetError() );
        exit(1);
    }

    SDL_SetHint( SDL_HINT_RENDER_SCALE_QUALITY, "nearest" );
    SDL_RenderSetLogicalSize( renderer, window_w, window_h );

    pformat = SDL_GetWindowPixelFormat( window );
    format = SDL_AllocFormat( pformat );

    SDL_SetRenderDrawColor( renderer, 0, 0, 0, 255 );
    SDL_RenderClear( renderer );

#ifdef AUDIO_ENABLED
    if ( Mix_OpenAudio( audio_freq, audio_format, audio_channels, 1024 ) < 0 ) {
        LOG( "%s\n", SDL_GetError() );
        LOG( "Disabling sound and continuing\n" );
        audio_on = -1;
    }
    audio_mix_channel_count = Mix_AllocateChannels( 16 );
#endif
}

int main( int argc, char *argv[] ) {
    int c;
    LOG( "main\n" );

    while ( ( c = getopt( argc, argv, "d:ws" ) ) != -1 ) {
        switch (c) {
            case 'd': delay = atoi(optarg); break;
            case 'w': fullscreen=0; break;
            case 's': audio_on = 0; break;
        }
    }
    LOG( "main loop delay: %d ms\n", delay );

    srand( time(0) );

    init();

    data_load();
    chart_load();

#ifndef __ANDROID__
    SDL_SetCursor( cr_empty );
#endif

    /* create menu */
    menu_init( &main_menu, 230, 40 );
    menu_add_entry( &main_menu, "Enter Shooting Range", ID_GAME );
    menu_add_entry( &main_menu, "", 0 );
    menu_add_entry( &main_menu, "Receive Briefing", ID_HELP );
    menu_add_entry( &main_menu, "Topgunners", ID_CHART );
#ifndef __ANDROID__
    menu_add_entry( &main_menu, "Quit", ID_QUIT );
#endif

    /* run game */
    main_loop();

    cleanup();

    exit(0);
}
