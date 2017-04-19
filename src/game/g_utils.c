/*
 * Copyright (C) 1997-2001 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * =======================================================================
 *
 * Misc. utility functions for the game logic.
 *
 * =======================================================================
 */

#include "header/local.h"

#define MAXCHOICES 8

/*
 * Searches all active entities for the next
 * one that holds the matching string at fieldofs
 * (use the FOFS() macro) in the structure.ind q
 *
 * Searches beginning at the edict after from, or
 * the beginning. If NULL, NULL will be returned
 * if the end of the list is reached.
 */
edict_t *G_Find( edict_t *from, int fieldofs, char *match ) {
	char *s;

	if ( !from ) {
		from = g_edicts;
	} else {
		from++;
	}

	if ( !match ) {
		return NULL;
	}

	for ( ; from < &g_edicts[globals.num_edicts]; from++ ) {
		if ( !from->inuse ) {
			continue;
		}

		s = *( char ** )( ( byte * )from + fieldofs );

		if ( !s ) {
			continue;
		}

		if ( !Q_stricmp( s, match ) ) {
			return from;
		}
	}

	return NULL;
}

/*
 * Searches all active entities for
 * the next one that holds the matching
 * string at fieldofs (use the FOFS() macro)
 * in the structure.
 *
 * Searches beginning at the edict after from,
 * or the beginning. If NULL, NULL will be
 * returned if the end of the list is reached.
 */
edict_t *G_PickTarget( char *targetname ) {
	edict_t *ent = NULL;
	int num_choices = 0;
	edict_t *choice[MAXCHOICES];

	if ( !targetname ) {
		gi.dprintf( "G_PickTarget called with NULL targetname\n" );
		return NULL;
	}

	while ( 1 ) {
		ent = G_Find( ent, FOFS( targetname ), targetname );

		if ( !ent ) {
			break;
		}

		choice[num_choices++] = ent;

		if ( num_choices == MAXCHOICES ) {
			break;
		}
	}

	if ( !num_choices ) {
		gi.dprintf( "G_PickTarget: target %s not found\n", targetname );
		return NULL;
	}

	return choice[randk() % num_choices];
}

void Think_Delay( edict_t *ent ) {
	if ( !ent ) {
		return;
	}

	G_UseTargets( ent, ent->activator );
	G_FreeEdict( ent );
}

/*
 * The global "activator" should be set to
 * the entity that initiated the firing.
 *
 * If self.delay is set, a DelayedUse entity
 * will be created that will actually do the
 * SUB_UseTargets after that many seconds have passed.
 *
 * Centerprints any self.message to the activator.
 *
 * Search for (string)targetname in all entities that
 * match (string)self.target and call their .use function
 */
void G_UseTargets( edict_t *ent, edict_t *activator ) {
	edict_t *t;

	if ( !ent || !activator ) {
		return;
	}

	/* check for a delay */
	if ( ent->delay ) {
		/* create a temp object to fire at a later time */
		t = G_Spawn();
		t->classname = "DelayedUse";
		t->nextthink = level.time + ent->delay;
		t->think = Think_Delay;
		t->activator = activator;

		if ( !activator ) {
			gi.dprintf( "Think_Delay with no activator\n" );
		}

		t->message = ent->message;
		t->target = ent->target;
		t->killtarget = ent->killtarget;
		return;
	}

	/* print the message */
	if ( ( ent->message ) && !( activator->svflags & SVF_MONSTER ) ) {
		gi.centerprintf( activator, "%s", ent->message );

		if ( ent->noise_index ) {
			gi.sound( activator, CHAN_AUTO, ent->noise_index, 1, ATTN_NORM, 0 );
		} else {
			gi.sound( activator, CHAN_AUTO, gi.soundindex(
			              "misc/talk1.wav" ), 1, ATTN_NORM, 0 );
		}
	}

	/* kill killtargets */
	if ( ent->killtarget ) {
		t = NULL;

		while ( ( t = G_Find( t, FOFS( targetname ), ent->killtarget ) ) ) {
			G_FreeEdict( t );

			if ( !ent->inuse ) {
				gi.dprintf( "entity was removed while using killtargets\n" );
				return;
			}
		}
	}

	/* fire targets */
	if ( ent->target ) {
		t = NULL;

		while ( ( t = G_Find( t, FOFS( targetname ), ent->target ) ) ) {
			/* doors fire area portals in a specific way */
			if ( !Q_stricmp( t->classname, "func_areaportal" ) &&
			        ( !Q_stricmp( ent->classname, "func_door" ) ||
			          !Q_stricmp( ent->classname, "func_door_rotating" ) ) ) {
				continue;
			}

			if ( t == ent ) {
				gi.dprintf( "WARNING: Entity used itself.\n" );
			} else {
				if ( t->use ) {
					t->use( t, ent, activator );
				}
			}

			if ( !ent->inuse ) {
				gi.dprintf( "entity was removed while using targets\n" );
				return;
			}
		}
	}
}

/*
 * This is just a convenience function
 * for printing vectors
 */
char *vtos( vec3_t v ) {
	static int index;
	static char str[8][32];
	char *s;

	/* use an array so that multiple vtos won't collide */
	s = str[index];
	index = ( index + 1 ) & 7;

	Com_sprintf( s, 32, "(%i %i %i)", ( int )v[0], ( int )v[1], ( int )v[2] );

	return s;
}

vec3_t VEC_UP = {0, -1, 0};
vec3_t MOVEDIR_UP = {0, 0, 1};
vec3_t VEC_DOWN = {0, -2, 0};
vec3_t MOVEDIR_DOWN = {0, 0, -1};

void G_SetMovedir( vec3_t angles, vec3_t movedir ) {
	if ( VectorCompare( angles, VEC_UP ) ) {
		VectorCopy( MOVEDIR_UP, movedir );
	} else if ( VectorCompare( angles, VEC_DOWN ) ) {
		VectorCopy( MOVEDIR_DOWN, movedir );
	} else {
		AngleVectors( angles, movedir, NULL, NULL );
	}

	VectorClear( angles );
}

float vectoyaw( vec3_t vec ) {
	float yaw;

	if ( vec[PITCH] == 0 ) {
		yaw = 0;

		if ( vec[YAW] > 0 ) {
			yaw = 90;
		} else if ( vec[YAW] < 0 ) {
			yaw = -90;
		}
	} else {
		yaw = ( int )( atan2( vec[YAW], vec[PITCH] ) * 180 / M_PI );

		if ( yaw < 0 ) {
			yaw += 360;
		}
	}

	return yaw;
}

void G_InitEdict( edict_t *e ) {
	e->inuse = true;
	e->classname = "noclass";
	e->gravity = 1.0;
	e->s.number = e - g_edicts;
}

/*
 * Either finds a free edict, or allocates a
 * new one.  Try to avoid reusing an entity
 * that was recently freed, because it can
 * cause the client to think the entity
 * morphed into something else instead of
 * being removed and recreated, which can
 * cause interpolated angles and bad trails.
 */
edict_t *G_Spawn( void ) {
	int i;
	edict_t *e;

	e = &g_edicts[( int )maxclients->value + 1];

	for ( i = maxclients->value + 1; i < globals.num_edicts; i++, e++ ) {
		/* the first couple seconds of
		   server time can involve a lot of
		   freeing and allocating, so relax
		   the replacement policy */
		if ( !e->inuse && ( ( e->freetime < 2 ) || ( level.time - e->freetime > 0.5 ) ) ) {
			G_InitEdict( e );
			return e;
		}
	}

	if ( i == game.maxentities ) {
		gi.error( "ED_Alloc: no free edicts" );
	}

	globals.num_edicts++;
	G_InitEdict( e );
	return e;
}

/*
 * Marks the edict as free
 */
void G_FreeEdict( edict_t *ed ) {
	gi.unlinkentity( ed ); /* unlink from world */

	if ( ( ed - g_edicts ) <= ( maxclients->value ) ) {
		return;
	}

	memset( ed, 0, sizeof( *ed ) );
	ed->classname = "freed";
	ed->freetime = level.time;
	ed->inuse = false;
}

void G_TouchTriggers( edict_t *ent ) {
	int i, num;
	edict_t *touch[MAX_EDICTS], *hit;

	if ( !ent ) {
		return;
	}

	/* dead things don't activate triggers! */
	if ( ( ent->client || ( ent->svflags & SVF_MONSTER ) ) && ( ent->health <= 0 ) ) {
		return;
	}

	num = gi.BoxEdicts( ent->absmin, ent->absmax, touch,
	                    MAX_EDICTS, AREA_TRIGGERS );

	/* be careful, it is possible to have an entity in this
	   list removed before we get to it (killtriggered) */
	for ( i = 0; i < num; i++ ) {
		hit = touch[i];

		if ( !hit->inuse ) {
			continue;
		}

		if ( !hit->touch ) {
			continue;
		}

		hit->touch( hit, ent, NULL, NULL );
	}
}

/*
 * Kills all entities that would touch the
 * proposed new positioning of ent. Ent s
 * hould be unlinked before calling this!
 */
qboolean KillBox( edict_t *ent ) {
	return false;
}
