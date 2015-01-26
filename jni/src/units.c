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

#include "units.h"
#include "shots.h"
#include "bfield.h"

extern SDL_Renderer *renderer;
extern SDL_Texture *img_units;
extern SDL_Sound *wav_expl3;
extern int player_score;
extern int audio_on;
extern BField bfield;

Unit units_anchor;
Unit *units_tail = &units_anchor;
int unit_count = 0; /* number of active units */

int strip_blocked[STRIP_COUNT]; /* number of units in each strip; -1 means the strip is
                                   not accessable */

static int pos2dir( int cx, int cy, int x, int y ) {
    int dir;
    int slope;

    /* Translate the problem to (0,0) and compute the direction
     * 0-7 which is closest to (x,y) when cx,cy is the position
     * of the observer.
     *
     * Therefore the slope is approximated as an integer
     * value multiplied by 100 thus the borders for the 45 degree
     * segments have a slope of 241 (upper border at 67,5 deg) and
     * 41 (lower border at 22,5 deg) in the 1. quadrant for the segement
     * with direction1. As to the symmetry all other segments are
     * handled analouge.
     */

    x -= cx; y -= cy;
    if ( x == 0 && y == 0 ) return 0;
    if ( x == 0 ) slope = (y>0)?300:-300; /* prevent floating point exception */
    else          slope = 100 * y / x;
    if ( x >= 0 ) {
        if ( y < 0 ) {
            /* 1. quadrant */
            if ( slope < -241 ) return 0; else
                if ( slope >  -41 ) return 2; else
                    return 1;
        } else {
            /* 2.quadrant */
            if ( slope <   41 ) return 2; else
                if ( slope >  241 ) return 4; else
                    return 3;
        }
    } else {
        if ( y >= 0 ) {
            /* 3. quadrant */
            if ( slope < -241 ) return 4; else
                if ( slope >  -41 ) return 6; else
                    return 5;
        } else {
            /* 4. quadrant */
            if ( slope <   41 ) return 6; else
                if ( slope >  241 ) return 0; else
                    return 7;
        }
    }
    return dir;
}

static void unit_add( Unit *unit ) {
    units_tail->next = unit;
    unit->prev = units_tail;
    units_tail = unit;

    unit_count++;
}

static void unit_delete( Unit *unit ) {
    /* each unit has a previous one as we use an anchor which may not be deleted */
    if ( unit == &units_anchor ) return;
    unit->prev->next = unit->next;

    if ( unit->next )
        unit->next->prev = unit->prev;
    else
        units_tail = unit->prev;

    free( unit );

    unit_count--;
}

static Unit* unit_create_basic(
        int type,
        SDL_Texture *img, int img_xoff, int img_yoff, int w, int h,
        int frame_count, double frame_change, double speed, Vector *pos ) {
    Unit *unit = calloc( 1, sizeof( Unit ) );

    if ( unit == 0 ) { LOG( "out of memory\n" ); return 0; }

    unit->alive = 1;
    unit->type = type;
    unit->img = img;
    unit->img_x_offset = img_xoff; unit->img_y_offset = img_yoff;
    unit->w = w; unit->h = h;
    unit->frame_count = frame_count;
    unit->frame_change = frame_change;
    unit->speed = speed;
    unit->pos = *pos;

    return unit;
}

void units_draw( int camera_x, int camera_y ) {
    SDL_Rect srect, drect;
    Unit *unit = units_anchor.next;

    while ( unit ) {
        /* add in camera check? */

        srect.x = unit->img_x_offset + (int)unit->frame*unit->w;
        srect.y = unit->img_y_offset + unit->frame_y_offset;
        drect.x = unit->pos.x - camera_x; drect.y = unit->pos.y - camera_y;
        srect.w = drect.w = unit->w;
        srect.h = drect.h = unit->h;
        SDL_RenderCopy(renderer, unit->img, &srect, &drect);

        unit = unit->next;
    }
}

void units_update( int ms ) {
    Unit *unit = units_anchor.next, *next;

    while ( unit ) {
        /* dead? */
        if ( !unit->alive ) {
            if ( unit->move_time ) {
                unit->move_time -= ms; /* used as wrack time */
                if ( unit->move_time <= 0 ) {
                    next = unit->next;
                    unit_delete( unit );
                    unit = next;
                    continue;
                }
            }
            unit = unit->next;
            continue;
        }

        /* move */
        if ( unit->move_time > 0 ) {
            /* animate */
            if ( unit->frame_count > 1 ) {
                unit->frame += unit->frame_change * ms;
                if ( unit->frame >= unit->frame_count ) {
                    unit->frame = 1; /* start at the beginning, frame 0
                                        is standing unit */
                }
            }

            /* position */
            unit->pos.x += unit->dir.x * ms;
            unit->pos.y += unit->dir.y * ms;
            unit->pos.z += unit->dir.z * ms;
            unit->move_time -= ms;
            if ( unit->move_time <= 0 ) {
                /* stop */
                unit->move_time = 0;
                unit->frame = 0;
                /* units that stop at the moment are saved */
                switch ( unit->type ) {
                    case UT_SOLDIER:
                        if ( !bfield.demo )
                            player_score += SCORE_HUMAN_SAVE;
                        break;
                    case UT_TANK:
                        if ( !bfield.demo )
                            player_score += SCORE_TANK_SAVE;
                        strip_blocked[unit->strip_id] = 0;
                        break;
                    case UT_RECON:
                        if ( !bfield.demo )
                            player_score += SCORE_RECON_SAVE;
                        strip_blocked[unit->strip_id] = 0;
                        break;
                }
                next = unit->next;
                unit_delete( unit );
                unit = next;
                continue;
            }
        }

        unit = unit->next;
    }

}

void unit_start_move( Unit *unit, Vector *dest ) {
    double len;

    unit->dest = *dest;
    unit->dir.x = dest->x - unit->pos.x;
    unit->dir.y = dest->y - unit->pos.y;
    unit->dir.z = 0;
    len = sqrt(unit->dir.x*unit->dir.x + unit->dir.y*unit->dir.y);
    unit->move_time = len / unit->speed;
    len /= unit->speed;
    unit->dir.x /= len; unit->dir.y /= len;

    /* soldier has 8 looking directions in theory; practically it only
     * looks up or down; other units only move in one fixed direction */
    if ( unit->type == UT_SOLDIER )
        unit->frame_y_offset =
            unit->h * pos2dir( unit->pos.x, unit->pos.y, dest->x, dest->y );

    /* animation starts at frame 1 as frame 0 is the standing unit */
    if ( unit->frame_count > 1)
        unit->frame = 1;
}

void units_delete() {
    while ( units_anchor.next )
        unit_delete( units_anchor.next );
}

void units_add_soldier( int strip_id, int x, int y, int move_up ) {
    Unit *unit;
    Vector pos, dest;

    pos.x = STRIP_OFFSET + strip_id * STRIP_SIZE + x; pos.y = y; pos.z = 0;
    unit = unit_create_basic(
            UT_SOLDIER,
            img_units, 0, 0, 12, 12, 1, 0.0, 0.012, &pos );
    if ( !unit ) return;
    unit->frame_y_offset = move_up?0:(unit->h*4);
    unit->hx = 5; unit->hy = 9;
    unit_add( unit );

    dest.x = unit->pos.x; dest.y = move_up?-20:480;
    unit_start_move( unit, &dest );
}

void units_add_dummy_tank( int strip_id, int x, int y, int speed ) {
    Vector pos, dest;
    Unit *unit;

    pos.x = STRIP_OFFSET + strip_id * STRIP_SIZE + x; pos.y = y; pos.z = 0;
    unit = unit_create_basic(
            UT_TANK,
            img_units, 0, 128, 13, 22, 1, 0.0, (double)speed/1000, &pos );
    if ( unit == 0 ) return;
    unit->strip_id = strip_id; strip_blocked[strip_id] = 1;
    unit->hx = 5; unit->hy = 13;
    unit->armor = 30;
    unit_add( unit );

    dest.x = pos.x; dest.y = -40;
    unit_start_move( unit, &dest );
}

void units_add_dummy_recon( int strip_id, int x, int y, int speed ) {
    Vector pos, dest;
    Unit *unit;

    pos.x = STRIP_OFFSET + strip_id * STRIP_SIZE + x; pos.y = y; pos.z = 0;
    unit = unit_create_basic(
            UT_RECON,
            img_units, 13, 128, 12, 18, 1, 0.0, (double)speed/1000, &pos );
    if ( unit == 0 ) return;
    unit->strip_id = strip_id; strip_blocked[strip_id] = 1;
    unit->hx = 5; unit->hy = 10;
    unit->armor = 12;
    unit_add( unit );

    dest.x = pos.x; dest.y = 480;
    unit_start_move( unit, &dest );
}

void units_check_impact( int x, int y, int radius ) {
    Unit *unit = units_anchor.next;
    Unit *next;

    while ( unit ) {
        if ( unit->alive )
            if ( DIST(x,y,unit->pos.x+unit->hx,unit->pos.y+unit->hy) <= (radius-unit->armor) ) {
                if ( unit->type == UT_SOLDIER ) {
                    particles_explode_human(
                            unit->pos.x+unit->hx,
                            unit->pos.y+unit->hy,
                            x,y );
                    next = unit->next;
                    unit_delete( unit );
                    unit = next;
                    if ( !bfield.demo )
                        player_score += SCORE_HUMAN_HIT;
                    continue;
                } else {
                    particles_add_emitter( PT_FIRE,
                            unit->pos.x+unit->hx,unit->pos.y+unit->hy,
                            3,
                            0.05,
                            WRACK_TIME );
                    SDL_PlaySound( wav_expl3 );
                    unit->alive = 0;
                    unit->move_time = WRACK_TIME;
                    if ( !bfield.demo ) {
                        if ( unit->type == UT_TANK )
                            player_score += SCORE_TANK_HIT;
                        else
                            player_score += SCORE_RECON_HIT;
                    }
                    strip_blocked[unit->strip_id] = 0;
                }
            }

        unit = unit->next;
    }
}

/* return id of a free strip */
int units_get_rand_strip( int type ) {
    int i, start;

    /* soldiers may walk in strips with uneven numbers */
    if ( type == UT_SOLDIER ) {
        i = rand() % STRIP_COUNT;
        if ( !(i&1) ) i++;
        while ( strip_blocked[i] ) {
            i += 2;
            if ( i >= STRIP_COUNT )
                i = 1;
        }
        return i;
    }

    /* tanks may drive in even numbered strips */
    i = rand() % STRIP_COUNT;
    if ( i&1) i--;
    start = i;
    while ( strip_blocked[i] ) {
        i+=2; if ( i >= STRIP_COUNT ) i = 0;
        if ( i == start ) return -1; /* dont loop endlessly */
    }
    return i;
}

/* return a random vehicle that is in the screen or 0 if no available */
Unit *units_get_first_vehicle() {
    Unit *unit = units_anchor.next;

    while ( unit ) {
        if ( unit->type != UT_SOLDIER )
            if ( unit->alive )
                if ( unit->pos.y > 0 && unit->pos.y < 470 )
                    return unit;
        unit = unit->next;
    }

    return 0;
}

/* return first soldier that is in the screen or 0 if no available */
Unit *units_get_first_soldier() {
    Unit *unit = units_anchor.next;

    while ( unit ) {
        if ( unit->type == UT_SOLDIER )
            if ( unit->alive )
                if ( unit->pos.y > 0 && unit->pos.y < 470 )
                    return unit;
        unit = unit->next;
    }

    return 0;
}
