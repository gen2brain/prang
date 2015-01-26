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

#ifndef SHOT_H
#define SHOT_H

#include "particle.h"

#define MAX_SHOT_VECTORS 3

enum {
    ST_NONE = 0,
    ST_GRENADE,
    ST_BOMB,
    ST_CLUSTER,
    ST_NAPALM,
    ST_OIL
};

typedef struct _Shot {
    int	type;
    SDL_Texture *img; /* depending on the type there may be multiple graphics
                             required which have to be stored horizontally (e.g.
                             a grenade only has one sprite but a missile 8) vertically
                             below each tile an equally sized shadow sprite is required
                             if shadow is used */
    int img_x_offset;
    int	img_y_offset; /* offset in image where first sprite is located */
    int	w, h; /* size of projectile */
    int	with_shadow; /* use shadow */
    Vector pos; /* coordinates of projectile */
    Vector dest; /* destination of projectile at ground */
    Vector dir; /* direction of projectile */
    int is_precise;	/* wether to hit destination exactly; is used
                       to workaround a little targeting inaccuracy
                       due to vectorized update which is only noticably
                       when a passed destination has to be hit exactly.
                       if power and angle were passed the inaccuracy is
                       not noticable. */
    double speed; /* if projectile has a constant speed/ms (missile)
                     the direction vector must be renormed to it
                     every time it is changed */

    struct _Shot *next;	/* the number of shots is not limited so they are stored
                           as a linked list with a dummy shot as anchor */
    struct _Shot *prev;
} Shot;

void shots_draw( int camera_x, int camera_y );

void shots_update( int ms );

void shots_delete();

void shots_set_impact_handler( void (*callback)(Shot*) );

void shots_fire_ballistic(
        int type,
        SDL_Texture *img, int img_xoff, int img_yoff, int w, int h,
        Vector *src, Vector *dest, double alpha, double power );
#define shots_fire_grenade( pos, dest, alpha, power ) \
    shots_fire_ballistic( ST_GRENADE, 0, 0,0, 4,4, pos, dest, alpha, power )
#define shots_fire_bomb( pos, dest, alpha, power ) \
    shots_fire_ballistic( ST_BOMB, 0, 4,0, 6,6, pos, dest, alpha, power )
#define shots_fire_cluster( pos, dest, alpha, power ) \
    shots_fire_ballistic( ST_CLUSTER, 0, 4,0, 6,6, pos, dest, alpha, power )
#define shots_fire_napalm( pos, dest, alpha, power ) \
    shots_fire_ballistic( ST_NAPALM, 0, 4,0, 6,6, pos, dest, alpha, power )
#define shots_fire_oil( pos, dest, alpha, power ) \
    shots_fire_ballistic( ST_OIL, 0, 10,0, 4,4, pos, dest, alpha, power )

#endif
