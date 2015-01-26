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

#ifndef UNITS_H
#define UNITS_H

#include "particle.h"

#define WRACK_TIME	20000

enum {
    UT_SOLDIER = 0, /* must start at 0 ! */
    UT_TANK,
    UT_RECON,
    UT_LAST
};

typedef struct _Unit {
    int	type; /* as above */
    int	alive; /* some units are not deleted but became dead */
    SDL_Texture	*img; /* may contain various graphics */
    int	img_x_offset, img_y_offset; /* basic offsets */
    int	w, h; /* size of a single frame */
    int	hx, hy; /* hotspot in all frames that defines the units
                   center (is used to check if explosions took effect) */
    int	armor; /* only impacts with a bigger radius can damage the unit
                  and must be closer than expl_radius-armor therefore */
    double frame; /* (int)frame*w is current x_offset */
    double frame_change; /* frames / ms */
    int frame_count; /* number of frames per moving direction, when
                        exceeded animation starts at the beginning */
    int frame_y_offset;	/* depends on the moving direction, a soldier
                           image has 8 horizontal strips in a vertical order
                           but all have the same amount of frames, the first
                           image is not used when moving as it represents
                           the standing soldier looking into the direction */
    Vector pos; /* coordinates of unit */
    Vector dest; /* destination for movement which is approached
                    when move_time is greater than 0 */
    Vector dir; /* direction of movement */
    double speed; /* speed per ms and the length of direction vector */
    int move_time; /* time until destination is reached. if unit is no
                      longer alive this is used to count down wrack time */
    int strip_id; /* the battlefield is tiled in strips and tanks and recons
                     block their strip to prevent driving each other to death */

    struct _Unit *next, *prev; /* dynamically linked */
} Unit;


void units_draw( int camera_x, int camera_y );

void units_update( int ms );

void unit_start_move( Unit *unit, Vector *dest );

void units_delete();


/* x value is local in strip: global_x = STRIP_OFFSET + strip_id*STRIP_SIZE + x*/
void units_add_soldier( int strip_id, int x, int y, int move_up );
/* speed is pixels per second */
void units_add_dummy_tank( int strip_id, int x, int y, int speed );
void units_add_dummy_recon( int strip_id, int x, int y, int speed );

void units_check_impact( int x, int y, int radius );

/* return id of a free strip that the passed unit type may use */
int units_get_rand_strip( int type );

/* return first vehicle that is in the screen or 0 if no available */
Unit *units_get_first_vehicle();

/* return first soldier that is in the screen or 0 if no available */
Unit *units_get_first_soldier();

#endif
