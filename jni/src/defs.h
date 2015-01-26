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

#ifndef GAME_DEFS_H
#define GAME_DEFS_H

#define VERSION "1.0.4"

#include <SDL.h>
#include <SDL_image.h>
#ifdef AUDIO_ENABLED
#include <SDL_mixer.h>
typedef Mix_Chunk SDL_Sound;
#define SDL_PlaySound( chunk ) if( audio_on==1 ) Mix_PlayChannelTimed( -1, chunk, 0, -1 )
#else
typedef void SDL_Sound;
#define SDL_PlaySound( chunk )
#endif
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <getopt.h>

/* SDL font library created by Karl Bartel <karlb@gmx.net> */
#include "SFont.h"
typedef SFont_Font SDL_Font;
#define SDL_WriteText( font, posx, posy, text ) \
    SFont_Write( font, posx, posy, text )
#define SDL_WriteTextCenter( font, posx, posy, text ) \
    SFont_WriteCenter( font, posy, text )
#define SDL_WriteTextRight( font, posx, posy, text ) \
    SFont_Write( font, posx-SFont_TextWidth(font,text), posy, text )
#define SDL_TextWidth( font, text ) \
    SFont_TextWidth( font, text )

#ifdef __ANDROID__

#include <android/log.h>
#define LOG_TAG "SDL"
#define LOG(...) __android_log_print( ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__ );
#define ASSETS_DIR ""

#else

#define  LOG(...)   printf( __VA_ARGS__ );
#define ASSETS_DIR SRC_DIR "/"

#endif

#define INPUT_DELAY 500

#define CURSOR_DELAY 1000

#define GRAV 0.001

#define MAX_PARTICLES    10000
#define MAX_EMITTERS     100
#define MAX_MENU_ENTRIES 10
#define MAX_TREES        50

#define RAND( low, high ) ( (rand() % ((high)-(low)+1))+(low) )

typedef struct {
    int min, max;
} Range;
#define SET_RANGE( range, mini, maxi ) { range.min=mini; range.max=maxi; }
#define RAND_IN_RANGE( range ) ( RAND( range.min, range.max ) )

#define DIST( x1, y1, x2, y2 ) \
    ( sqrt(((x1)-(x2))*((x1)-(x2))+((y1)-(y2))*((y1)-(y2))) )

#define INRANGE( x1, y1, x2, y2, dist ) ( DIST( x1, y1, x2, y2 ) <= dist )

typedef struct {
    double x, y, z;
} Vector;

#define STRIP_OFFSET 120
#define STRIP_SIZE    20
#define STRIP_COUNT   26

#define GAME_TIME     180

#define MAX_AMMO     36
#define AMMO_GRENADE 1
#define AMMO_BOMB    6
#define AMMO_CLUSTER 12
#define AMMO_NAPALM  12

#define BOMB_DELAY     500
#define GRENADE_DELAY   80
#define NAPALM_DELAY  2000
#define CLUSTER_DELAY 1000

#define SCORE_RELOAD      -36
#define SCORE_HUMAN_HIT    5
#define SCORE_RECON_HIT   20
#define SCORE_TANK_HIT    50
#define SCORE_HUMAN_SAVE   0
#define SCORE_RECON_SAVE -10
#define SCORE_TANK_SAVE  -25

#define SET_DELAY( delay, value ) delay = value
#define UPDATE_DELAY( delay, ms ) if ( (delay -= ms) < 0 ) delay = 0

enum {
    CURSOR_ARROW,
    CURSOR_CROSSHAIR,
    CURSOR_WAIT,
    CURSOR_RELOAD,
    CURSOR_DISABLED,
    CURSOR_COUNT = 5
};
#define SET_CURSOR( type ) cursor_x_offset = type*cursor_w

#endif
