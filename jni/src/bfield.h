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

#ifndef BFIELD_H
#define BFIELD_H

enum {
    BFIELD_DEMO = 0,
    BFIELD_GAME
};

typedef struct {
    SDL_Texture *img;
    int	img_x_offset, img_y_offset; /* position of the living tree img_y_offset+h
                                       is the position of the dead tree */
    int	x, y; /* battlefield position */
    int w, h;
    int	hx, hy;	/* hotspot for impacts */
    int	burn_time; /* if >0 tree is burning down thus when this
                      becomes 0 the tree is dead */
    int	dead; /* hits doesn't start burning */
} Tree;

typedef struct {
    Vector 	gun; /* base point of gun */
    Vector  gun_img_offset; /* offset added to base point to draw image */
    int	gun_dir; /* 0 - 8 in 20° steps */
    int	img_gun_x_offset; /* offset in gun image indicating the looking direction */
    Tree trees[MAX_TREES]; /* living and burned down trees */
    int	tree_count;
    int	gun_delay; /* while > 0 gun is reloaded */
    int	demo; /* wether bfield_update fires gun automatically */
    int	demo_delay; /* demo does not shoot all the time */
    int demo_hunt_soldiers; /* while >0 decrease and fire at soldiers when 0
                               decide 50-50 wether to fire at soldiers or tanks */
    int	delays[UT_LAST]; /* if any gets 0 this unit type enteres the playing field
                               and the delay is reset to the settings below */
    Range delay_ranges[UT_LAST]; /* the time until next unit type will enter the battlefield */
    Range group_sizes[UT_LAST];	/* number of units that will enter the battlefield */
} BField;

void bfield_init( int type, Vector *gun, Vector *gun_img_offset );

void bfield_finalize();

void bfield_draw( int camera_x, int camera_y );

void bfield_update( int ms );

int bfield_gun_is_ready();

void bfield_fire_gun( int type /* ST_... */, Vector *dest, double alpha, double power );

void bfield_reload_gun();

void bfield_handle_impact( Shot *shot );

void bfield_update_gun_dir( int mx, int my );

#endif
