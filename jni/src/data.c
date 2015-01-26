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

#include "defs.h"
#include "data.h"

SDL_Texture *img_particles = 0;
SDL_Texture *img_ground = 0;
SDL_Texture *img_crater = 0;
SDL_Texture *img_small_crater = 0;
SDL_Texture *img_units = 0;
SDL_Texture *img_trees = 0;
SDL_Texture *img_shots = 0;
SDL_Texture *img_ammo = 0;
SDL_Texture *img_logo = 0;
SDL_Texture *img_icons = 0;
SDL_Texture *img_black = 0;
SDL_Texture *background = 0;
SDL_Texture *img_cursors = 0;
SDL_Texture *img_gun = 0;
SDL_Texture *img_gun_reload = 0;
SDL_Texture *img_small_grenade = 0;
SDL_Texture *img_large_grenade = 0;
SDL_Texture *back = 0;

SFont_Font *ft_main = 0;
SFont_Font *ft_chart = 0;
SFont_Font *ft_chart_highlight = 0;
SFont_Font *ft_menu = 0;
SFont_Font *ft_menu_highlight = 0;
SFont_Font *ft_help = 0;

SDL_Sound *wav_expl1 = 0; /* grenade explosion */
SDL_Sound *wav_expl2 = 0; /* (cluster)bomb explosion */
SDL_Sound *wav_expl3 = 0; /* tank explosion */
SDL_Sound *wav_cannon1 = 0; /* grenade autocannon */
SDL_Sound *wav_cannon2 = 0;
SDL_Sound *wav_click = 0;
SDL_Sound *wav_highlight = 0;
SDL_Sound *wav_empty = 0;
SDL_Sound *wav_reload = 0;

SDL_Cursor *cr_empty = 0;

int img_count = 0, wav_count = 0, ft_count = 0;
int ammo_w = 0, ammo_h = 0; /* size of an ammo tile */
int cursor_w = 0, cursor_x_offset = 0;
int gun_w = 0, gun_h = 0;
int gun_reload_w = 0, gun_reload_h = 0;

extern int audio_on;

extern SDL_Renderer *renderer;
extern SDL_Window *window; /* display */

extern int window_w, window_h;
extern SDL_PixelFormat *format;
extern Uint32 pformat;

static void data_free_cursor( SDL_Cursor **cursor ) {
    if ( *cursor ) {
        SDL_FreeCursor( *cursor );
        *cursor = 0;
    }
}

static SDL_Cursor* data_create_cursor( int width, int height, int hot_x, int hot_y, char *source ) {
    unsigned char *mask = 0, *data = 0;
    SDL_Cursor *cursor = 0;
    int i, j, k;
    unsigned char data_byte, mask_byte;
    int pot;

    /* meaning of char from source: b : black, w: white, ' ':transparent */

    mask = malloc( width * height * sizeof ( char ) / 8 );
    data = malloc( width * height * sizeof ( char ) / 8 );

    k = 0;
    for ( j = 0; j < width * height; j += 8, k++ ) {
        pot = 1;
        data_byte = mask_byte = 0;
        /* create byte */
        for ( i = 7; i >= 0; i-- ) {
            switch ( source[j + i] ) {
                case 'b': data_byte += pot;
                case 'w': mask_byte += pot; break;
            }
            pot *= 2;
        }
        /* add to mask */
        data[k] = data_byte;
        mask[k] = mask_byte;

    }
    /* create and return cursor */
    cursor = SDL_CreateCursor( data, mask, width, height, hot_x, hot_y );
    free( mask );
    free( data );
    return cursor;
}

static void data_free_image( SDL_Texture **text ) {
    if ( *text != NULL) {
        SDL_DestroyTexture( *text );
    }
}

static SDL_Texture *data_create_image( int w, int h, int access ) {
    LOG( "creating %ix%i bitmap ... ", w, h );

    SDL_Texture *text = 0;
    text = SDL_CreateTexture( renderer, pformat, access, w, h );
    if ( text == 0 ) {
        LOG( "%s\n", SDL_GetError() );
        exit(1);
    }
    img_count++; /* statistics */
    LOG( "ok\n" );
    return text;
}

static SDL_Texture *data_load_image( char *name ) {
    char path[128];
    SDL_Texture *text = 0;
    snprintf( path, 128, "%sgfx/%s", ASSETS_DIR, name ); path[127] = 0;
    LOG( "loading %s ... ", path );
    text = IMG_LoadTexture( renderer, path );
    if ( text == 0 ) {
        LOG( "%s\n", SDL_GetError() );
        exit(1);
    }
    img_count++; /* statistics */
    LOG( "ok\n" );
    return text;
}

static void data_free_font( SFont_Font **font ) {
    if ( *font ) {
        SFont_FreeFont( *font );
    }
}

static SFont_Font* data_load_font( char *name ) {
    char path[128];
    SDL_Surface *tmp;
    snprintf( path, 128, "%sgfx/%s", ASSETS_DIR, name ); path[127] = 0;
    LOG( "loading %s ... ", path );
    tmp = IMG_Load( path );
    if ( tmp == 0 ) {
        LOG( "%s\n", SDL_GetError() );
        exit(1);
    }

    SFont_Font* font = SFont_InitFont( tmp );
    ft_count++; /* statistics */
    LOG( "ok\n" );
    return font;
}

#ifdef AUDIO_ENABLED
static void data_free_sound( SDL_Sound **wav ) {
    if ( *wav ) {
        Mix_FreeChunk( *wav );
        *wav = 0;
    }
}

static SDL_Sound *data_load_sound( char *name ) {
    char path[128];
    SDL_Sound *wav = 0;
    snprintf( path, 128, "%ssounds/%s", ASSETS_DIR, name ); path[127] = 0;
    LOG( "loading %s ... ", path );
    wav = Mix_LoadWAV( path ); /* convets already */
    if ( wav == 0 ) {
        printf( "%s\n", SDL_GetError() );
        exit(1);
    }
    wav_count++; /* statistics */
    LOG( "ok\n" );
    return wav;
}
#endif

void data_load() {
    img_ground = data_load_image( "soil.png" );

    img_particles = data_load_image( "particles.png" );
    SDL_SetTextureBlendMode( img_particles, SDL_BLENDMODE_BLEND );

    img_crater = data_load_image( "crater.png" );
    SDL_SetTextureBlendMode( img_crater, SDL_BLENDMODE_BLEND );
    SDL_SetTextureAlphaMod( img_crater, 196 );

    img_small_crater = data_load_image( "small_crater.png" );
    SDL_SetTextureBlendMode( img_small_crater, SDL_BLENDMODE_BLEND );
    SDL_SetTextureAlphaMod( img_small_crater, 196 );

    img_units = data_load_image( "units.png" );
    img_trees = data_load_image( "trees.png" );

    img_shots = data_load_image( "shots.png" );

    img_ammo = data_load_image( "ammo.png" );
    int img_ammo_w, img_ammo_h;
    SDL_QueryTexture(img_ammo, NULL, NULL, &img_ammo_w, &img_ammo_h);
    ammo_w = img_ammo_w; ammo_h = img_ammo_h / AMMO_BOMB;

    img_logo = data_load_image( "logo.png" );
    img_icons = data_load_image( "icons.png" );

    img_black = data_create_image( window_w, window_h, SDL_TEXTUREACCESS_TARGET );
    SDL_SetTextureBlendMode( img_black, SDL_BLENDMODE_BLEND );
    SDL_SetTextureAlphaMod( img_black, 128 );

    background = data_create_image( window_w, window_h, SDL_TEXTUREACCESS_TARGET );
    SDL_SetTextureBlendMode( background, SDL_BLENDMODE_BLEND );

    back = data_create_image( window_w, 36, SDL_TEXTUREACCESS_STATIC );

    img_cursors = data_load_image( "cursors.png" );
    int img_cursors_w, img_cursors_h;
    SDL_QueryTexture(img_cursors, NULL, NULL, &img_cursors_w, &img_cursors_h);
    cursor_w = img_cursors_w / CURSOR_COUNT;

    img_gun = data_load_image( "gun.png" );
    int img_gun_w, img_gun_h;
    SDL_QueryTexture(img_gun, NULL, NULL, &img_gun_w, &img_gun_h);
    gun_w = img_gun_w / 9; gun_h = img_gun_h;

    img_gun_reload = data_load_image( "reload.png" );
    SDL_QueryTexture(img_gun_reload, NULL, NULL, &gun_reload_w, &gun_reload_h);

    img_small_grenade = data_load_image( "small_grenade.png" );
    img_large_grenade = data_load_image( "large_grenade.png" );

    ft_main = data_load_font( "ft_red.png" );
    ft_chart = data_load_font( "ft_red.png" );
    ft_chart_highlight = data_load_font( "ft_yellow.png" );
    ft_help = data_load_font( "ft_red14.png" );
    /* use the same fonts for menu */
    ft_menu = ft_chart; ft_menu_highlight = ft_chart_highlight;

#ifdef AUDIO_ENABLED
    if ( audio_on != -1 ) {
        wav_expl1 = data_load_sound( "expl1.wav" );
        wav_expl2 = data_load_sound( "expl2.wav" );
        wav_expl3 = data_load_sound( "expl3.wav" );
        wav_cannon1 = data_load_sound( "autocannon.wav" );
        wav_cannon2 = data_load_sound( "gunfire.wav" );
        wav_click = data_load_sound( "click.wav" );
        wav_highlight = data_load_sound( "highlight.wav" );
        wav_empty = data_load_sound( "empty.wav" );
        wav_reload = data_load_sound( "reload.wav" );
    }
#endif

    cr_empty = data_create_cursor( 16, 16, 8, 8,
            "                "
            "                "
            "                "
            "                "
            "                "
            "                "
            "                "
            "                "
            "                "
            "                "
            "                "
            "                "
            "                "
            "                "
            "                "
            "                " );
}

void data_delete() {
    data_free_image( &img_particles );
    data_free_image( &img_ground );
    data_free_image( &img_crater );
    data_free_image( &img_small_crater );
    data_free_image( &img_units );
    data_free_image( &img_trees );
    data_free_image( &img_shots );
    data_free_image( &img_ammo );
    data_free_image( &img_logo );
    data_free_image( &img_icons );
    data_free_image( &img_black );
    data_free_image( &back );
    data_free_image( &background );
    data_free_image( &img_cursors );
    data_free_image( &img_gun );
    data_free_image( &img_gun_reload );
    LOG( "%i images deleted\n", img_count );

    data_free_font( &ft_main );
    data_free_font( &ft_chart );
    data_free_font( &ft_chart_highlight );
    data_free_font( &ft_help );
    LOG( "%i fonts deleted\n", ft_count );

#ifdef AUDIO_ENABLED
    if ( audio_on != -1 ) {
        data_free_sound( &wav_expl1 );
        data_free_sound( &wav_expl2 );
        data_free_sound( &wav_expl3 );
        data_free_sound( &wav_cannon1 );
        data_free_sound( &wav_click );
        data_free_sound( &wav_highlight );
        data_free_sound( &wav_empty );
        data_free_sound( &wav_reload );
        LOG( "%i sounds deleted\n", wav_count );
    }
#endif

    data_free_cursor( &cr_empty );
}
