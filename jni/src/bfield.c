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
#include "shots.h"
#include "units.h"
#include "bfield.h"

extern SDL_Renderer *renderer;
extern SDL_Texture *img_trees, *background, *img_ground, *img_small_crater, *img_crater, *img_gun;
extern void vec2xy( Vector *v, int *sx, int *sy ); /* from shots.c */
extern SDL_Sound *wav_expl1, *wav_expl2, *wav_cannon1, *wav_cannon2, *wav_reload, *wav_empty;
extern SDL_Cursor *cr_reload, *cr_hourglass, *cr_crosshair;
extern int player_ammo, player_score;
extern int strip_blocked[STRIP_COUNT];
extern int audio_on;
extern int cursor_x_offset, cursor_w;
extern int gun_w, gun_h;

BField bfield;

extern void vec2xy( Vector *v, int *sx, int *sy );

/* fire positions relative to the gun base point for each gun sprite */
Vector fire_offsets[9] = {
    {2, -6, 28},
    {5, -5, 30},
    {7, -3, 30},
    {10,-3, 34},
    {11, 0, 36},
    {10, 1, 34},
    {8,  2, 36},
    {7,  3, 34},
    {3,  4, 34}
};

static void set_tree( Tree *tree, int x, int y, int dead ) {
    tree->img = img_trees;
    tree->img_x_offset = 0;
    tree->img_y_offset = dead*23;

    tree->w = 38; tree->h = 23;
    tree->x = x; tree->y = y;
    tree->hx = 17; tree->hy = 8; /* hotspot: center for impacts */
    tree->burn_time = 0;
    tree->dead = dead;
}

void bfield_init( int type, Vector *gun, Vector *gun_img_offset ) {
    int tree_strips[] = { 1, 7, 11, 17, -1 };
    int i;

    memset( &bfield, 0, sizeof( bfield ) );
    bfield.gun = *gun; bfield.gun_img_offset = *gun_img_offset;
    bfield.img_gun_x_offset = 4 * gun_w;
    bfield.demo = ( type==BFIELD_DEMO );
    memset( strip_blocked, 0, sizeof( strip_blocked ) );

    /* basic ground */
    SDL_SetRenderTarget( renderer, background );
    SDL_RenderCopy( renderer, img_ground, NULL, NULL );
    SDL_SetRenderTarget( renderer, NULL );

    /* add burned trees to some strips and mark them unuseable */
    i = 0;
    while ( tree_strips[i] != -1 ) {
        strip_blocked[tree_strips[i]] = 1;
        set_tree( &bfield.trees[bfield.tree_count++],
                STRIP_OFFSET +
                tree_strips[i]*STRIP_SIZE +
                RAND( 0, STRIP_SIZE-17 ) - 10 /* shadow must be in strip */,
                rand() % 230, 1 );
        set_tree( &bfield.trees[bfield.tree_count++],
                STRIP_OFFSET +
                tree_strips[i]*STRIP_SIZE +
                RAND( 0, STRIP_SIZE-17 ) - 10 /*shadow must be in strip */,
                230 + rand() % 230, 1 );
        i++;
    }
    /* add normal trees to gun strip */
    for ( i = 0; i < 4; i++ )
        set_tree( &bfield.trees[bfield.tree_count++],
                RAND(0, STRIP_OFFSET-38), RAND(i*50, (i+1)*50-20), 0 );
    for ( i = 0; i < 4; i++ )
        set_tree( &bfield.trees[bfield.tree_count++],
                RAND(0, STRIP_OFFSET-38), RAND(280+i*50, 280+(i+1)*50-20), 0 );

    /* set the ranges */
    SET_RANGE( bfield.delay_ranges[UT_SOLDIER], 2000, 4000 );
    SET_RANGE( bfield.delay_ranges[UT_RECON],   2000, 4000 );
    SET_RANGE( bfield.delay_ranges[UT_TANK],    2000, 5000 );
    SET_RANGE( bfield.group_sizes[UT_SOLDIER], 2, 4 );
    SET_RANGE( bfield.group_sizes[UT_TANK],    1, 1 );
    SET_RANGE( bfield.group_sizes[UT_RECON],   1, 1 );

    /* use bfield_impact_handler for exploding projectiles */
    shots_set_impact_handler( bfield_handle_impact );
}

void bfield_finalize() {
    units_delete();
    shots_delete();
    particles_clear();

}

static void bfield_add_units( int ms ) {
    int num, x, y, dir, i, strip;

    /* decrease delays */
    for ( i = 0; i < UT_LAST; i++ )
        if ( (bfield.delays[i] -= ms) < 0 )
            bfield.delays[i] = 0;

    /* check wether new units appear. as soldiers may interfere
     * they use only even id'd strips to prevent them from blocking
     * all strips. */
    if ( bfield.delays[UT_SOLDIER] == 0 ) {
        bfield.delays[UT_SOLDIER] = RAND_IN_RANGE( bfield.delay_ranges[UT_SOLDIER] );
        strip = units_get_rand_strip( UT_SOLDIER ); /* works always */
        dir = rand() % 2;
        num = RAND_IN_RANGE( bfield.group_sizes[UT_SOLDIER] );
        for ( i = 0, y=0; i < num; i++ ) {
            x = RAND( 0, STRIP_SIZE-12 );
            units_add_soldier( strip, x, ((dir)?480:-50)+y, dir );
            y += RAND( 10, 20 );
        }
    }
    if ( bfield.delays[UT_TANK] == 0 ) {
        strip = units_get_rand_strip( UT_TANK );
        if ( strip != -1 ) {
            bfield.delays[UT_TANK] = RAND_IN_RANGE( bfield.delay_ranges[UT_TANK] );
            x = RAND( 0, STRIP_SIZE-13 );
            y = RAND( 480, 530 );
            units_add_dummy_tank( strip, x, y, RAND(30, 40) );
        }
    }
    if ( bfield.delays[UT_RECON] == 0 ) {
        strip = units_get_rand_strip( UT_RECON );
        if ( strip != -1 ) {
            bfield.delays[UT_RECON] = RAND_IN_RANGE( bfield.delay_ranges[UT_RECON] );
            x = RAND( 0, STRIP_SIZE-12 );
            y = RAND( -50, -20 );
            units_add_dummy_recon( strip, x, y, RAND(40,50) );
        }
    }
}

/* let gun look towards mouse position */
static void bfield_draw_gun( int camera_x, int camera_y ) {
    SDL_Rect drect, srect;

    srect.w = drect.w = gun_w; srect.h = drect.h = gun_h;
    srect.x = bfield.img_gun_x_offset; srect.y = 0;
    drect.x = bfield.gun.x + bfield.gun_img_offset.x;
    drect.y = bfield.gun.y + bfield.gun_img_offset.y;
    SDL_RenderCopy( renderer, img_gun, &srect, &drect );
}

void bfield_draw( int camera_x, int camera_y ) {
    Tree *tree;
    int i;
    SDL_Rect drect, srect;

    SDL_RenderCopy( renderer, background, NULL, NULL );
    bfield_draw_gun( camera_x, camera_y );

    units_draw( 0, 0 );

    bfield_draw_gun( camera_x, camera_y );

    for ( i = 0; i < bfield.tree_count; i++ ) {
        tree = &bfield.trees[i];
        drect.x = tree->x; drect.y = tree->y;
        srect.w = drect.w = tree->w;
        srect.h = drect.h = tree->h;
        srect.x = tree->img_x_offset;
        srect.y = tree->img_y_offset;
        SDL_RenderCopy( renderer, tree->img, &srect, &drect );
    }

    particles_draw( 0, 0 );
    shots_draw( 0, 0 );
}

void bfield_update( int ms ) {
    int i;
    Vector dest;
    Unit *unit;

    /* add units */
    bfield_add_units( ms );

    /* move things */
    units_update( ms );
    shots_update( ms );
    particles_update( ms );

    /* check wether gun may fire */
    if ( bfield.gun_delay > 0 )
        if ( (bfield.gun_delay-=ms) <= 0 ) {
            bfield.gun_delay = 0;
            if ( !bfield.demo )
                SET_CURSOR( CURSOR_CROSSHAIR );
        }

    /* fire cannon automatically in demo mode */
    if ( bfield.demo && bfield.gun_delay == 0 && (bfield.demo_delay-=ms) <= 0 ) {
        /* soldiers or tanks? */
        if ( bfield.demo_hunt_soldiers == 0 )
            if ( (rand() % 3) == 0  )
                bfield.demo_hunt_soldiers = 4;
        /* select target */
        if ( bfield.demo_hunt_soldiers > 0 ) {
            unit = units_get_first_soldier();
            if ( unit ) {
                bfield.demo_hunt_soldiers--;
                if ( bfield.demo_hunt_soldiers == 0 )
                    bfield.demo_delay = 2500; /* take a short break */
            }
        }
        else
            unit = units_get_first_vehicle();
        /* fire! */
        if ( unit ) {
            if ( unit->type != UT_SOLDIER )
                bfield.demo_delay = 2500;
            dest.x = unit->pos.x + unit->hx + RAND( -10, 10 );
            dest.y = unit->pos.y + unit->hy + RAND( -20, 20 );
            if ( unit->type != UT_SOLDIER )
                dest.y += ((unit->dir.y<0)?-30:40);
            dest.z = 0;
            bfield_update_gun_dir( (int)dest.x, (int)dest.y );
            bfield_fire_gun( (unit->type == UT_SOLDIER) ? ST_GRENADE : ST_BOMB, &dest, 60, 0 );
        }
    }

    /* check burning trees */
    for ( i = 0; i < bfield.tree_count; i++ )
        if ( bfield.trees[i].burn_time > 0 )
            if ( (bfield.trees[i].burn_time-=ms) <=0 ) {
                bfield.trees[i].burn_time = 0;
                bfield.trees[i].dead = 1;
                bfield.trees[i].img_y_offset += bfield.trees[i].h;
            }
}

int bfield_gun_is_ready() {
    return ( bfield.gun_delay == 0 );
}

void bfield_fire_gun( int type /* ST_... */, Vector *dest, double alpha, double power ) {
    int sx, sy;
    Vector pos;

    /* must not be too near to gun */
    if ( dest->x < STRIP_OFFSET )
        return;

    pos = bfield.gun;
    pos.x += fire_offsets[bfield.gun_dir].x;
    pos.y += fire_offsets[bfield.gun_dir].y;
    pos.z += fire_offsets[bfield.gun_dir].z;
    switch ( type ) {
        case ST_GRENADE:
            if ( player_ammo >= AMMO_GRENADE || bfield.demo ) {
                SET_DELAY( bfield.gun_delay, GRENADE_DELAY );
                shots_fire_grenade( &pos, dest, alpha, power );
                SDL_PlaySound( wav_cannon1 );
                if ( !bfield.demo )
                    player_ammo -= AMMO_GRENADE;
                vec2xy( &pos, &sx, &sy );
                particles_set_muzzle_fire( sx, sy, 10 );
            }
            else
                if ( !bfield.demo ) {
                    SDL_PlaySound( wav_empty );
                    SET_CURSOR( CURSOR_RELOAD );
                }
            break;
        case ST_BOMB:
            if ( player_ammo >= AMMO_BOMB || bfield.demo ) {
                SET_DELAY( bfield.gun_delay, BOMB_DELAY );
                shots_fire_bomb( &pos, dest, alpha, power );
                SDL_PlaySound( wav_cannon2 );
                if ( !bfield.demo ) {
                    player_ammo -= AMMO_BOMB;
                    SET_CURSOR( CURSOR_WAIT );
                }
                vec2xy( &pos, &sx, &sy );
                particles_set_muzzle_fire( sx, sy, 30 );
            }
            else
                if ( !bfield.demo ) {
                    SDL_PlaySound( wav_empty );
                    SET_CURSOR( CURSOR_RELOAD );
                }
            break;
        case ST_CLUSTER:
            if ( player_ammo >= AMMO_CLUSTER || bfield.demo ) {
                SET_DELAY( bfield.gun_delay, CLUSTER_DELAY );
                dest->z = 50; /* detonate above target */
                SDL_PlaySound( wav_cannon2 );
                shots_fire_cluster( &pos, dest, 70, 0 );
                if ( !bfield.demo ) {
                    player_ammo -= AMMO_CLUSTER;
                    SET_CURSOR( CURSOR_WAIT );
                }
            }
            else
                if ( !bfield.demo ) {
                    SDL_PlaySound( wav_empty );
                    SET_CURSOR( CURSOR_RELOAD );
                }

            break;
        case ST_NAPALM:
            if ( player_ammo >= AMMO_NAPALM || bfield.demo ) {
                SET_DELAY( bfield.gun_delay, NAPALM_DELAY );
                SDL_PlaySound( wav_cannon2 );
                shots_fire_napalm( &pos, dest, 70, 0 );
                if ( !bfield.demo ) {
                    player_ammo -= AMMO_BOMB;
                    SET_CURSOR( CURSOR_WAIT );
                }
            }
            else
                if ( !bfield.demo ) {
                    SDL_PlaySound( wav_empty );
                    SET_CURSOR( CURSOR_RELOAD );
                }
            break;
    }
}

void bfield_reload_gun() {
    if ( player_ammo < MAX_AMMO ) {
        player_ammo = MAX_AMMO;
        player_score += SCORE_RELOAD;
        if ( bfield.gun_delay == 0 )
            SDL_PlaySound( wav_reload );
            SET_CURSOR( CURSOR_CROSSHAIR );
    }
}

void bfield_handle_impact( Shot *shot ) {
    SDL_Rect drect;
    Vector dest;
    int i;
    int sx, sy;

    if ( shot->is_precise )
        shot->pos = shot->dest; /* impact at exact position */
    vec2xy( &shot->pos, &sx, &sy ); /* get screen position */

    switch ( shot->type ) {
        case ST_GRENADE:
            particles_explode_grenade( sx, sy );

            int img_small_crater_w, img_small_crater_h;
            SDL_QueryTexture(img_small_crater, NULL, NULL, &img_small_crater_w, &img_small_crater_h);

            drect.x = sx - (img_small_crater_w>>1);
            drect.y = sy - (img_small_crater_h>>1);
            drect.w = img_small_crater_w; drect.h = img_small_crater_h;
            SDL_SetRenderTarget( renderer, background );
            SDL_RenderCopy( renderer, img_small_crater, NULL, &drect );
            SDL_SetRenderTarget( renderer, NULL );

            SDL_PlaySound( wav_expl1 );

            units_check_impact( sx, sy, 20 );
            break;
        case ST_BOMB:
            particles_explode_bomb( sx, sy );

            int img_crater_w, img_crater_h;
            SDL_QueryTexture( img_crater, NULL, NULL, &img_crater_w, &img_crater_h );

            drect.x = sx - (img_crater_w>>1);
            drect.y = sy - (img_crater_h>>1);
            drect.w = img_crater_w; drect.h = img_crater_h;
            SDL_SetRenderTarget( renderer, background );
            SDL_RenderCopy( renderer, img_crater, NULL, &drect );
            SDL_SetRenderTarget( renderer, NULL );

            SDL_PlaySound( wav_expl2 );

            units_check_impact( sx, sy, 40 );
            break;
        case ST_CLUSTER:
            particles_explode_clusterbomb( sx, sy );
            for ( i = 0; i < 16; i++ ) {
                dest.x = shot->dest.x + RAND( -50, 50 );
                dest.y = shot->dest.y + RAND( -50, 50 );
                dest.z = 0;
                shots_fire_grenade(
                        &shot->pos, &dest,
                        RAND( -40, 0 ), 0.06 + (double)RAND( 0, 20 )/100 );
            }
            break;
        case ST_NAPALM:
            for ( i = 0; i < 10; i++ ) {
                dest.x = shot->dest.x + RAND( -50, 50 );
                dest.y = shot->dest.y + RAND( -50, 50 );
                dest.z = 0;
                shots_fire_oil(
                        &shot->pos, &dest,
                        RAND( 20, 60 ), 0.01 + (double)RAND( 0, 20 )/100 );

                units_check_impact( sx, sy, 40 );
            }
            break;
        case ST_OIL:
            particles_add_emitter( PT_FIRE, sx, sy, 5, 0.1, 6000 );
            break;
    }
}

void bfield_update_gun_dir( int mx, int my ) {
    double alpha;

    if ( mx <= bfield.gun.x ) {
        if ( my <= bfield.gun.y )
            bfield.gun_dir = 0;
        else
            bfield.gun_dir = 8;
    }
    else {
        alpha = atan( (my - bfield.gun.y) / (mx - bfield.gun.x ) );
        alpha = alpha * 180 / 3.14;
        bfield.gun_dir = ((alpha + 10) / 20) + 4;
    }

    bfield.img_gun_x_offset = gun_w * bfield.gun_dir;
}
