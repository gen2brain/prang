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

#include "particle.h"

Particle particles[MAX_PARTICLES];
int particle_count = 0;
Emitter emitters[MAX_EMITTERS];
int emitter_count = 0;

extern SDL_Renderer *renderer;
extern SDL_Texture *img_particles;

static Vector wind = { -0.005, 0, 0 };
static Vector grav = { 0, 0, -GRAV };


static void particle_add( Particle *particle ) {
    if ( particle_count >= MAX_PARTICLES ) return;

    particles[particle_count] = *particle;
    particle_count++;
}

static void particle_delete( int index ) {
    if ( particle_count == 0 ) return;

    particles[index] = particles[particle_count-1];
    particle_count--;
}

static void emitter_delete( int index ) {
    if ( emitter_count == 0 ) return;

    emitters[index] = emitters[emitter_count-1];
    emitter_count--;
}

void particles_clear() {
    emitter_count = 0;
    particle_count = 0;
}

void particles_draw( int camera_x, int camera_y ) {
    Particle *part;
    SDL_Rect srect, drect;
    int i;

    for ( i = 0; i < particle_count; i++ ) {
        part = &particles[i];

        srect.x = part->img_x_offset + part->frame_id*part->w;
        srect.y = part->img_y_offset;
        drect.x = (int)part->pos.x - camera_x;
        drect.y = (int)part->pos.y - ((int)part->pos.z>>1) - camera_y;
        srect.w = drect.w = part->w;
        srect.h = drect.h = part->h;

        /* add in camera check? */

        SDL_SetTextureAlphaMod( part->img, (Uint8)part->alpha );

        SDL_RenderCopy( renderer, part->img, &srect, &drect );
    }
}

void particles_update( int ms ) {
    Emitter *emit;
    Particle *part;
    int i, dens;

    /* emitters */
    for ( i = 0; i < emitter_count; i++ ) {
        emit = &emitters[i];

        emit->count += emit->rate * ms;
        dens = emit->count;
        if ( dens > 0 ) {
            emit->count -= dens;
            switch (emit->type ) {
                case PT_FIRE:
                    particles_set_fire( emit->x, emit->y, emit->radius, dens );
                    break;
                case PT_SMOKE:
                    particles_set_smoke( emit->x, emit->y, emit->radius, dens );
                    break;
            }
        }

        if ( emit->lifespan > 0 ) {
            emit->lifespan -= ms;
            if ( emit->lifespan <= 0 ) {
                emitter_delete( i );
                i--; /* last tile is copied to this one's position */
            }
        }
    }

    /* particles */
    for ( i = 0; i < particle_count; i++ ) {
        part = &particles[i];

        if ( part->type != PT_EXPLOSION && part->type != PT_FIRE  ) {
            part->alpha += part->alpha_change * ms;
            if ( part->alpha < 32.0 ) {
                particle_delete( i );
                i--; /* last tile is copied to this one's position */
                continue;
            }
        }

        part->pos.x += part->dir.x * ms;
        part->pos.y += part->dir.y * ms;
        part->pos.z += part->dir.z * ms;

        if ( part->frame_id < part->frame_count-1 ) {
            part->frame += part->frame_change * ms;
            part->frame_id = part->frame;
            if ( part->frame_id >= part->frame_count-1 ) {
                part->frame_id = part->frame_count-1;
                /* explosion turns to smoke */
                if ( part->type == PT_EXPLOSION ) {
                    part->dir.x = (double)RAND(-10,10)/1000;
                    part->dir.y = (double)RAND(-10,10)/1000;
                    part->dir.z = 0.0;
                    part->type = PT_SMOKE;
                }
                /* fire turns to smoke */
                if ( part->type == PT_FIRE )
                    part->type = PT_SMOKE;
            }
        }

        if ( part->wind )
            if ( part->type != PT_WASTE )
                if ( part->type != PT_EXPLOSION ) {
                    part->pos.x += part->wind->x * ms;
                    part->pos.y += part->wind->y * ms;
                    part->pos.z += part->wind->z * ms;
                }
        if ( part->grav && part->type == PT_BALLISTIC ) {
            part->dir.x += part->grav->x * ms;
            part->dir.y += part->grav->y * ms;
            part->dir.z += part->grav->z * ms;
        }

        if ( part->type == PT_BALLISTIC && part->pos.z <= 0 ) {
            part->dir.x = part->dir.y = part->dir.z = 0;
            part->alpha_change = -0.20;
            part->type = PT_WASTE;
        }

    }
}

static void vector_scale( Vector *vector, double scale ) {
    double norm;
    norm = sqrt( vector->x*vector->x + vector->y*vector->y + vector->z*vector->z );
    norm /= scale;
    vector->x /= norm; vector->y /= norm; vector->z /= norm;

}

void particles_explode_bomb( int x, int y ) {
    int i, yoff;
    Particle part;
    double energy;

    memset( &part, 0, sizeof( part ) );
    part.type = PT_BALLISTIC;
    part.img = img_particles;
    part.frame_count = 1;
    part.alpha = 200;
    part.grav = &grav;
    part.wind = &wind;
    part.pos.x = x;
    part.pos.y = y;
    for ( energy = 0.2, i = 0; i < 40; i++, energy += 0.01 ) {
        part.w = part.h = 4;
        part.dir.x = (double)RAND( -12, 12 );
        part.dir.y = (double)RAND( -12, 12 );
        part.dir.z = (double)RAND( 100, 400 );
        vector_scale( &part.dir, energy );
        particle_add( &part );
    }
    for ( energy = 0.2, i = 0; i < 100; i++, energy += 0.004 ) {
        part.w = part.h = 2;
        part.img_x_offset = 4;
        part.dir.x = (double)RAND( -10, 10 );
        part.dir.y = (double)RAND( -10, 10 );
        part.dir.z = (double)RAND( 100, 400 );
        vector_scale( &part.dir, energy );
        particle_add( &part );
    }

    memset( &part, 0, sizeof( part ) );
    part.type = PT_EXPLOSION;
    part.img = img_particles;
    part.frame_count = 8;
    part.frame_change = 0.1;
    part.alpha = 200;
    part.alpha_change = -0.2;
    part.wind = &wind;
    part.pos.x = x;
    part.pos.y = y;
    part.w = part.h = 6;
    for ( yoff = 0, energy = 0.4, i = 0; i < 100; i++, energy += 0.002, yoff += 6 ) {
        if ( yoff == 18 ) yoff = 0;
        part.img_y_offset = 18 + yoff;
        part.dir.x = (double)RAND( -80, 80 );
        part.dir.y = (double)RAND( -80, 80 );
        part.dir.z = (double)RAND( 200, 500 );
        vector_scale( &part.dir, energy );
        particle_add( &part );
    }
}

void particles_explode_clusterbomb( int x, int y ) {
    int i, yoff;
    Particle part;

    memset( &part, 0, sizeof( part ) );
    part.type = PT_EXPLOSION;
    part.img = img_particles;
    part.frame_count = 8;
    part.frame_change = 0.1;
    part.alpha = 200;
    part.alpha_change = -0.2;
    part.wind = &wind;
    part.pos.x = x;
    part.pos.y = y;
    part.w = part.h = 6;
    for ( yoff = 0, i = 0; i < 100; i++, yoff += 6 ) {
        if ( yoff == 18 ) yoff = 0;
        part.img_y_offset = 18 + yoff;
        part.dir.x = (double)RAND( -80, 80 );
        part.dir.y = (double)RAND( -80, 80 );
        part.dir.z = (double)RAND( 200, 500 );
        vector_scale( &part.dir, 0.4 );
        particle_add( &part );
    }
}

void particles_explode_grenade( int x, int y ) {
    int i;
    Particle part;
    double energy;

    memset( &part, 0, sizeof( part ) );
    part.type = PT_BALLISTIC;
    part.img = img_particles;
    part.frame_count = 1;
    part.alpha = 200;
    part.grav = &grav;
    part.wind = &wind;
    part.pos.x = x;
    part.pos.y = y;
    for ( energy = 0.2, i = 0; i < 10; i++, energy += 0.02 ) {
        part.w = part.h = 4;
        part.dir.x = (double)RAND( -10, 10 );
        part.dir.y = (double)RAND( -10, 10 );
        part.dir.z = (double)RAND( 100, 400 );
        vector_scale( &part.dir, energy );
        particle_add( &part );
    }
    for ( energy = 0.2, i = 0; i < 50; i++, energy += 0.004 ) {
        part.w = part.h = 2;
        part.img_x_offset = 4;
        part.dir.x = (double)RAND( -10, 10 );
        part.dir.y = (double)RAND( -10, 10 );
        part.dir.z = (double)RAND( 100, 400 );
        vector_scale( &part.dir, energy );
        particle_add( &part );
    }

    memset( &part, 0, sizeof( part ) );
    part.type = PT_EXPLOSION;
    part.img = img_particles;
    part.frame_count = 8;
    part.frame_change = 0.1;
    part.alpha = 200;
    part.alpha_change = -0.2;
    part.wind = &wind;
    part.pos.x = x;
    part.pos.y = y;
    part.w = part.h = 4;
    part.img_y_offset = 14;
    for ( energy = 0.2, i = 0; i < 40; i++, energy += 0.002 ) {
        part.dir.x = (double)RAND( -40, 40 );
        part.dir.y = (double)RAND( -40, 40 );
        part.dir.z = (double)RAND( 200, 500 );
        vector_scale( &part.dir, energy );
        particle_add( &part );
    }
}

void particles_set_fire( int x, int y, int radius, int density ) {
    int i, frame;
    Particle part;
    double energy;

    memset( &part, 0, sizeof( part ) );
    part.type = PT_FIRE;
    part.img = img_particles;
    part.img_y_offset = 6;
    part.w = part.h = 2;
    part.frame_count = 10;
    part.frame_change = 0.01;
    part.alpha = 200;
    part.alpha_change = -0.2;
    part.wind = &wind;
    for ( frame = 0, energy = 0.03, i = 0; i < density; i++, frame++, energy += 0.001 ) {
        if ( frame == 6 ) frame = 0;
        part.frame = frame;
        part.pos.x = x + RAND(-radius, radius);
        part.pos.y = y + RAND(-(radius>>1),(radius>>1));
        part.dir.x = (double)RAND( -20, 20 );
        part.dir.y = (double)RAND( -20, 20 );
        part.dir.z = (double)RAND( 100, 400 );
        vector_scale( &part.dir, energy );
        particle_add( &part );
    }
}

void particles_set_smoke( int x, int y, int radius, int density ) {
    int i, xoff;
    Particle part;
    double energy;

    memset( &part, 0, sizeof( part ) );
    part.type = PT_SMOKE;
    part.img = img_particles;
    part.img_y_offset = 8;
    part.w = part.h = 6;
    part.frame_count = 1;
    part.alpha = 128;
    part.alpha_change = -0.1;
    part.wind = &wind;
    for ( xoff = 0, energy = 0.04, i = 0; i < density; i++, energy += 0.01, xoff += 6 ) {
        if ( xoff == 18 ) xoff = 0;
        part.img_x_offset = xoff;
        part.pos.x = x + RAND(-radius, radius);
        part.pos.y = y + RAND(-(radius>>1),(radius>>1));
        part.dir.x = (double)RAND( -20, 20 );
        part.dir.y = (double)RAND( -20, 20 );
        part.dir.z = (double)RAND( 100, 400 );
        vector_scale( &part.dir, energy );
        particle_add( &part );
    }
}

void particles_set_muzzle_fire( int x, int y, int density ) {
    int i;
    Particle part;

    memset( &part, 0, sizeof( part ) );
    part.type = PT_EXPLOSION;
    part.img = img_particles;
    part.frame_count = 8;
    part.frame_change = 0.1;
    part.alpha = 200;
    part.alpha_change = -0.2;
    part.wind = &wind;
    part.pos.x = x;
    part.pos.y = y;
    part.w = part.h = 4;
    part.img_y_offset = 14;
    for ( i = 0; i < density; i++ ) {
        part.dir.x = (double)RAND( -80, 80 );
        part.dir.y = (double)RAND( -80, 80 );
        part.dir.z = (double)RAND( 200, 500 );
        vector_scale( &part.dir, 0.2 );
        particle_add( &part );
    }
}

void particles_explode_human( int x, int y, int sx, int sy ) {
    int i;
    double dx = x - sx, dy = y - sy;
    Particle part;
    double energy;

    energy = sqrt(dx*dx+dy*dy); dx /= energy; dy /= energy;

    memset( &part, 0, sizeof( part ) );
    part.type = PT_BALLISTIC;
    part.img = img_particles;
    part.img_y_offset = 4;
    part.frame_count = 1;
    part.alpha = 200;
    part.wind = &wind;
    part.grav = &grav;
    for ( energy = 0.1, i = 0; i < 10; i++, energy += 0.01 ) {
        part.w = part.h = 2;
        part.img_x_offset = 1;
        part.pos.x = x + RAND( -2, 2 );
        part.pos.y = y + RAND( -2, 2 );
        part.dir.x = dx;
        part.dir.y = dy;
        part.dir.z = 3+ rand() % 3;
        vector_scale( &part.dir, energy );
        particle_add( &part );
    }
    for ( energy = 0.1, i = 0; i < 20; i++, energy += 0.01 ) {
        part.w = part.h = 1;
        part.pos.x = x + RAND( -2, 2 );
        part.pos.y = y + RAND( -2, 2 );
        part.dir.x = dx;
        part.dir.y = dy;
        part.dir.z = 3 + rand() % 3;
        vector_scale( &part.dir, energy );
        particle_add( &part );
    }

}

void particles_add_emitter( int type, int x, int y, int radius, double rate, int lifespan ) {
    Emitter emitter;

    if ( emitter_count >= MAX_EMITTERS ) return;

    memset( &emitter, 0, sizeof( emitter ) );
    emitter.type = type;
    emitter.x = x;
    emitter.y = y;
    emitter.radius = radius;
    emitter.lifespan = lifespan;
    emitter.rate = rate;

    emitters[emitter_count] = emitter; emitter_count++;
}
