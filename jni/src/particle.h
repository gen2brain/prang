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

#ifndef PARTICLE_H
#define PARTICLE_H

#include "defs.h"

enum {
    PT_SMOKE = 0,	/* animate if frame_change is set and fade out by modifying the
                       alpha by alpha_change*ms. remove the particle when it becomes
                       invisible. uses wind vector. */
    PT_EXPLOSION, 	/* animate through all frames with defined alpha but do not change it.
                       wind and gravity do not have influence. when animation is finished
                       type is set to smoke and set direction vector to a very slow random
                       movement. */
    PT_FIRE,	    /* first part as PT_EXPLOSION except that wind has always influence.
                       when animation is done type is set to PT_SMOKE but the direction
                       vector is not changed */
    PT_BALLISTIC,	/* move particle under influence of wind and gravity and remove
                       it when the particle has reached the ground again (pos.z==0).
                       if alpha_change is set the particle is removed when invisible. */
    PT_WASTE	    /* inanimate/unmoved particles that fade away */
};

typedef struct {
    int	type; /* type of particle */
    SDL_Texture	*img; /* resource image */
    int	img_x_offset;
    int	img_y_offset; /* image may contain different particles */
    int	w, h; /* size of the particle */
    double frame; /* if particle is animated, this is the floating point
                     frame id to apply frame_change correctly */
    int frame_id; /* the integer frame_id (int)frame used to display the current frame */
    double frame_change;	/* change in frame / ms */
    double frame_count;	/* number of frames in the horizontal strip. it is of
                                   type double to save a type conversion when checking this */
    double alpha; /* alpha of particle. the old alpha of image is not restored */
    double alpha_change; /* change of alpha / ms */
    Vector pos;	/* coordinates of the particle */
    Vector dir;	/* direction particle is moving to:
                   change of coordinates / ms */
    Vector *wind; /* the wind vector (direction change/ms) is used to modify
                     the direction. it is assumed that w == h (at least it
                     should not differ too much) and the effect of wind is
                     proportional to the particle's width (representing mass) */
    Vector *grav; /* gravity vector (direction change/ms). both vectors do only
                     have effect when the particle's type require them and they
                     are not NULL. */
} Particle;

typedef struct {
    int	type; /* smoke or fire */
    int	x,y; /* coordinates */
    int	radius; /* radius of emitting area */
    int	lifespan; /* time in ms emitter will be active */
    double rate; /* particles / ms */
    double count; /* number of particles to be emitted next */
} Emitter;

void particles_draw( int camera_x, int camera_y );

void particles_update( int ms );

void particles_add_emitter( int type, int x, int y, int radius, double rate, int lifespan );

void particles_clear(); /* clear emitters and particle */

/* customized particle creation */

void particles_set_fire( int x, int y, int radius, int density );

void particles_set_smoke( int x, int y, int radius, int density );

void particles_set_muzzle_fire( int x, int y, int density );

void particles_explode_human( int x, int y, int sx, int sy );

void particles_explode_bomb( int x, int y );

void particles_explode_grenade( int x, int y );

void particles_explode_clusterbomb( int x, int y );

#endif
