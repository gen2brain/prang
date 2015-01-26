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
#include "chart.h"

extern SFont_Font *ft_chart_highlight, *ft_chart;

ChartEntry chart[CHART_SIZE+1]; /* extra space for new entry */

#define CHART_FILE ".prang.hscr"

static void chart_reset() {
    int i;
    memset( chart, 0, sizeof( chart ) );
    for ( i = 0; i < CHART_SIZE; i++ )
        chart_add_entry( "............",  200 * ( i + 1 ) );
    for ( i = 0; i < CHART_SIZE; i++ )
        chart[i].highlight = 0;
}

void chart_load() {
    char buf[128], *ptr;
    int i;
    FILE *file;

    chart_reset();
#ifdef __ANDROID__
    const char* apath = SDL_AndroidGetInternalStoragePath();
    snprintf( buf, 128, "%s/%s", apath, CHART_FILE ); buf[127] = 0;
#else
    snprintf( buf, 128, "%s/%s", getenv("HOME"), CHART_FILE ); buf[127] = 0;
#endif
    file = fopen( buf, "r" );
    if ( file == 0 ) {
        LOG( "couldn't read highscore file: %s\n", buf );
        return;
    }
    else
        LOG( "loading highscores: %s\n", buf );
    for ( i = 0; i < CHART_SIZE; i++ ) {
        buf[0] = 0; /* if file is corrupted the name will be empty */
        if ( fgets( buf, 32, file ) != NULL) {
            buf[31] = 0;
        }
        ptr = strchr( buf, '\n' ); if ( ptr ) ptr[0] = 0; /* kill newline */
        snprintf( chart[i].name, 19, "%s", buf );
        if ( fgets( buf, 32, file ) != NULL) {
            buf[31] = 0;
        }
        chart[i].score = atoi( buf );
    }
    fclose( file );
}

void chart_save() {
    char path[128];
    int i;
    FILE *file;

#ifdef __ANDROID__
    const char* apath = SDL_AndroidGetInternalStoragePath();
    snprintf( path, 128, "%s/%s", apath, CHART_FILE ); path[127] = 0;
#else
    snprintf( path, 128, "%s/%s", getenv("HOME"), CHART_FILE ); path[127] = 0;
#endif
    file = fopen( path, "w" );
    if ( file == 0 ) {
        LOG( "couldn't open highscore file: %s\n", path );
        return;
    }
    for ( i = 0; i < CHART_SIZE; i++ )
        fprintf( file, "%s\n%i\n", chart[i].name, chart[i].score );
    fclose( file );
}

int chart_get_rank( int score ) {
    int i;
    for ( i = 0; i < CHART_SIZE; i++ ) {
        if ( chart[i].score >= score )
            continue;
        return i;
    }
    return -1;
}

static void chart_swap( ChartEntry *entry1, ChartEntry *entry2 ) {
    ChartEntry dummy;
    dummy = *entry1;
    *entry1 = *entry2;
    *entry2 = dummy;
}

static void chart_sort() {
    int j;
    int changed = 0;
    /* use entry dummy as well so count is CHART_SIZE + 1 */
    do {
        changed = 0;
        for ( j = 0; j < CHART_SIZE; j++ )
            if ( chart[j].score < chart[j + 1].score ) {
                chart_swap( &chart[j], &chart[j + 1] );
                changed = 1;
            }
    } while ( changed );
}

void chart_add_entry( char *name, int score ) {
    /* add new entry at blind end of chart */
    snprintf( chart[CHART_SIZE].name, 19, "%s", name );
    chart[CHART_SIZE].name[18] = 0;
    chart[CHART_SIZE].score = score;
    chart[CHART_SIZE].highlight = 1;

    chart_sort();
}

void chart_clear_highlights() {
    int i;
    for ( i = 0; i < CHART_SIZE; i++ )
        chart[i].highlight = 0;
}

void chart_draw() {
    SFont_Font *font;
    int i, y = 110, h = 30;
    char score_str[8];

    SDL_WriteTextCenter( ft_chart, 320, 50, "Topgunners" );

    for ( i = 0; i < CHART_SIZE; i++, y+= h ) {
        if ( chart[i].highlight )
            font = ft_chart_highlight;
        else
            font = ft_chart;
        snprintf( score_str, 8, "%6i", chart[i].score ); score_str[7] = 0;

        SDL_WriteText( font, 100, y, chart[i].name );
        SDL_WriteTextRight( font, 540, y, score_str );
    }
}
