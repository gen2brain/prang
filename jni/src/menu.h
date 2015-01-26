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

#ifndef MENU_H
#define MENU_H

typedef struct {
    char name[24]; 	/* name */
    int x, w, h; 	/* size and x position; y depends on the position
                           in the menu and is computed dynamically */
    int	id;		/* unique id */
} MenuEntry;

typedef struct {
    MenuEntry entries[MAX_MENU_ENTRIES];
    int	entry_count;	/* menu entries */
    int y;		/* y of first menu entry */
    int y_offset;	/* offset to next menu entry's y */
    int cur_entry_id;	/* index in entries of currently highlighted entry */
} Menu;

void menu_init( Menu *menu, int y, int y_offset );

void menu_add_entry( Menu *menu, char *name, int id );

void menu_draw( Menu *menu );

void menu_handle_motion( Menu *menu, int x, int y );

/* returns global id or 0 if no selection */
int menu_handle_click( Menu *menu, int x, int y );

#endif
