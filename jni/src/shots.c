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
#include "units.h"
#include "shots.h"

extern SDL_Renderer *renderer;
extern SDL_Texture *img_shots;

Shot shots_anchor;
Shot *shots_tail = &shots_anchor;

static Vector grav = { 0, 0, -GRAV }; /* for all projectiles that are influenced by gravity */
static void (*impact_handler)(Shot *shot) = 0;

/* used by other modules as well */
void vec2xy( Vector *v, int *sx, int *sy ) {
    /* convert to screen */
    *sx = (int)v->x;
    *sy = (int)v->y - (((int)v->z)>>1);
}

static void shot_add( Shot *shot ) {
    shots_tail->next = shot;
    shot->prev = shots_tail;
    shots_tail = shot;
}

static void shot_delete( Shot *shot ) {
    /* each shot has a previous one as we use an anchor which may not be deleted */
    if ( shot == &shots_anchor ) return;
    shot->prev->next = shot->next;

    if ( shot->next )
        shot->next->prev = shot->prev;
    else
        shots_tail = shot->prev;

    free( shot );
}

void shots_draw( int camera_x, int camera_y ) {
    int sx, sy;
    SDL_Rect srect, drect;
    Shot *shot = shots_anchor.next;

    while ( shot ) {
        vec2xy( &shot->pos, &sx, &sy );
        /* add in camera check? */

        /* the position refers to the center of of the projectile
         * but we have to start drawing at the upper left pixel */
        sx -= shot->w>>1;
        sy -= shot->h>>1; /* center */

        /* projectile */
        srect.x = shot->img_x_offset;
        srect.y = shot->img_y_offset;
        drect.x = sx - camera_x; drect.y = sy - camera_y;
        srect.w = drect.w = shot->w;
        srect.h = drect.h = shot->h;
        SDL_SetTextureBlendMode( shot->img, SDL_BLENDMODE_NONE );
        SDL_SetTextureAlphaMod( shot->img, 0 );
        SDL_RenderCopy( renderer, shot->img, &srect, &drect );

        /* shadow */
        if ( shot->with_shadow ) {
            srect.x = shot->img_x_offset;
            srect.y = shot->img_y_offset + shot->h;
            drect.x = shot->pos.x - (shot->w>>1);
            drect.x += ((int)shot->pos.z)>>3;
            drect.y = shot->pos.y - (shot->h>>1);
            srect.w = drect.w = shot->w;
            srect.h = drect.h = shot->h;
            SDL_SetTextureBlendMode( shot->img, SDL_BLENDMODE_BLEND );
            SDL_SetTextureAlphaMod( shot->img, 128 );
            SDL_RenderCopy( renderer, shot->img, &srect, &drect );
        }

        shot = shot->next;
    }
}

void shots_update( int ms ) {
    int i;
    Shot *shot = shots_anchor.next, *next;

    while ( shot ) {
        /* update position at ground */
        shot->pos.x += shot->dir.x * ms;
        shot->pos.y += shot->dir.y * ms;
        /* as we approach the projectile curve by vectors
         * the frame rate is essentially for the accuracy of the
         * height as with each call the tangential speed vector
         * is added which is instantly out of sync due to
         * a non-linear movement */
        for ( i = 0; i < ms; i++ ) {
            shot->pos.z += shot->dir.z;
            shot->dir.z += grav.z; /* apply gravity */
        }


        if ( shot->dir.z < 0 ) /* must be coming down */
            if ( shot->pos.z <= shot->dest.z ) {
                shot->pos.z = shot->dest.z;
                if ( impact_handler )
                    impact_handler( shot );
                next = shot->next;
                shot_delete( shot );
                shot = next;
                continue;
            }

        shot = shot->next;
    }
}

void shots_delete() {
    while ( shots_anchor.next )
        shot_delete( shots_anchor.next );
}

void shots_set_impact_handler( void (*callback)(Shot* shot) ) {
    impact_handler = callback;
}

void shots_fire_ballistic(
        int type,
        SDL_Texture *img, int img_xoff, int img_yoff, int w, int h,
        Vector *src, Vector *dest, double alpha, double power ) {
    double len, height, r;
    Shot *shot = calloc( 1, sizeof( Shot ) );

    if ( shot == 0 ) { LOG( "out of memory\n" ); return; }

    /* graphics and type */
    shot->type = type;
    if ( img == 0 )
        shot->img = img_shots;
    else
        shot->img = img;
    shot->img_x_offset = img_xoff; shot->img_y_offset = img_yoff;
    shot->w = w; shot->h = h;
    shot->with_shadow = 1;

    /* convert alpha from degree to radiant */
    alpha = 3.14 * alpha / 180;

    /* position and destination */
    shot->pos = *src;
    shot->dest = *dest;

    shot->dir.x = dest->x - src->x; shot->dir.y = dest->y - src->y;
    len = sqrt(shot->dir.x*shot->dir.x + shot->dir.y*shot->dir.y);
    height = dest->z - src->z;
    if ( power == 0 ) {
        /* if not specified compute by angle and hit destination exactly */
        r = -0.5 * GRAV / ( height - len * tan(alpha) );
        if ( r < 0 ) {
            LOG( "impossible movement for ballistic projectile!\n" );
            free( shot );
            return;
        }
        else {
            power = sqrt(r) * len / cos(alpha);
            shot->is_precise = 1;
        }
    }
    shot->dir.z = len * tan(alpha);
    len = sqrt(shot->dir.x*shot->dir.x + shot->dir.y*shot->dir.y + shot->dir.z*shot->dir.z);
    shot->dir.x /= len; shot->dir.y /= len; shot->dir.z /= len;
    shot->dir.x *= power; shot->dir.y *= power; shot->dir.z *= power;

    shot_add( shot );
}
