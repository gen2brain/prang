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

#ifndef CHART_H
#define CHART_H

#define CHART_SIZE 10

typedef struct {
    char name[20];
    int score;
    int highlight; /* highlight in chart */
} ChartEntry;

void chart_load();

void chart_save();

int chart_get_rank( int score ); /* returns -1 if not in chart */

void chart_add_entry( char *name, int score ); /* enter chart */

void chart_clear_highlights();

/* draw centered */
void chart_draw();

#endif

