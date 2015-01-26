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
#include "menu.h"

extern SDL_Window *window;
extern SDL_Font *ft_menu, *ft_menu_highlight;
extern SDL_Sound *wav_click, *wav_highlight;
extern int audio_on;
extern int window_w, window_h;

void menu_init( Menu *menu, int y, int y_offset ) {
    memset( menu, 0, sizeof( Menu ) );
    menu->y = y;
    menu->y_offset = y_offset;
    menu->cur_entry_id = -1;
}

void menu_add_entry( Menu *menu, char *name, int id ) {
    MenuEntry *entry;

    if ( menu->entry_count >= MAX_MENU_ENTRIES ) return;

    int ft_menu_w, ft_menu_h;
    SDL_QueryTexture( ft_menu->Texture, NULL, NULL, &ft_menu_w, &ft_menu_h );

    entry = &menu->entries[menu->entry_count++];
    snprintf( entry->name, 24, "%s", name ); entry->name[23] = 0;
    entry->w = SDL_TextWidth( ft_menu, entry->name );
    entry->h = ft_menu_h;
    entry->x = (window_w - entry->w) / 2; /* always centered */
    entry->id = id;
}

void menu_draw( Menu *menu ) {
    int	i, y;
    SDL_Font *font;

    for ( i = 0, y = menu->y; i < menu->entry_count; i++, y += menu->y_offset ) {
        font = (i == menu->cur_entry_id)?ft_menu_highlight:ft_menu;
        SDL_WriteText( font, menu->entries[i].x, y, menu->entries[i].name );
    }
}

void menu_handle_motion( Menu *menu, int mx, int my ) {
    MenuEntry *entry;
    int	i, y, old = menu->cur_entry_id;

    menu->cur_entry_id = -1;
    for ( i = 0, y = menu->y; i < menu->entry_count; i++, y += menu->y_offset ) {
        entry = &menu->entries[i];
        if ( mx >= entry->x && mx < entry->x + entry->w )
            if ( my >= y && my < y + entry->h ) {
                menu->cur_entry_id = i;
                break;
            }
    }
    if ( old != menu->cur_entry_id && menu->cur_entry_id != -1 )
        SDL_PlaySound( wav_highlight );
}

int menu_handle_click( Menu *menu, int x, int y ) {
    menu_handle_motion( menu, x, y );
    if ( menu->cur_entry_id == -1 )
        return 0;
    else {
        SDL_PlaySound( wav_click );
        return menu->entries[menu->cur_entry_id].id;
    }
}
