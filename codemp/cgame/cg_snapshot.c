// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_snapshot.c -- things that happen on snapshot transition,
// not necessarily every single rendered frame

#include "cg_local.h"

#define _LOGTRAILS 1
#if _LOGTRAILS
static void CG_LogStrafeTrail(void);
#endif

/*
==================
CG_ResetEntity
==================
*/
static void CG_ResetEntity( centity_t *cent ) {
	// if the previous snapshot this entity was updated in is at least
	// an event window back in time then we can reset the previous event
	if ( cent->snapShotTime < cg.time - EVENT_VALID_MSEC ) {
		cent->previousEvent = 0;
	}

	cent->trailTime = cg.snap->serverTime;

	VectorCopy (cent->currentState.origin, cent->lerpOrigin);
	VectorCopy (cent->currentState.angles, cent->lerpAngles);

	if (cent->currentState.eFlags & EF_G2ANIMATING)
	{ //reset the animation state
		cent->pe.torso.animationNumber = -1;
		cent->pe.legs.animationNumber = -1;
	}

#if 0
	if (cent->isRagging && (cent->currentState.eFlags & EF_DEAD))
	{
		VectorAdd(cent->lerpOrigin, cent->lerpOriginOffset, cent->lerpOrigin);
	}
#endif

	if ( cent->currentState.eType == ET_PLAYER || cent->currentState.eType == ET_NPC ) {
		CG_ResetPlayerEntity( cent );
	}
}

/*
===============
CG_TransitionEntity

cent->nextState is moved to cent->currentState and events are fired
===============
*/
static void CG_TransitionEntity( centity_t *cent ) {
	cent->currentState = cent->nextState;
	cent->currentValid = qtrue;

	// reset if the entity wasn't in the last frame or was teleported
	if ( !cent->interpolate ) {
		CG_ResetEntity( cent );
	} else {	//mme
		int newHeight;
		int maxs = ((cent->currentState.solid >> 16) & 255) - 32;
		if ( maxs > 16 )
			newHeight = DEFAULT_VIEWHEIGHT;
		else
			newHeight = CROUCH_VIEWHEIGHT;

		if ( newHeight != cent->pe.viewHeight ) {
			cent->pe.duckTime = cg.snap->serverTime;
			cent->pe.duckChange = newHeight - cent->pe.viewHeight;
			cent->pe.viewHeight = newHeight;
		}
	}

	// clear the next state.  if will be set by the next CG_SetNextSnap
	cent->interpolate = qfalse;

	// check for events
	CG_CheckEvents( cent );
}

void CG_AddToHistory( int serverTime, entityState_t *state, centity_t *cent ) {
	timedEntityState_t *tstate = &cent->stateHistory.states[cent->stateHistory.nextSlot % MAX_STATE_HISTORY], *prev = NULL;
	if ( cg.demoPlayback != 2 ) {
		return;
	}
	if ( state->number >= MAX_CLIENTS && state->eType != ET_MOVER ) {
		return;
	}
	if ( cent->stateHistory.nextSlot > 0 ) {
		prev = &cent->stateHistory.states[(cent->stateHistory.nextSlot - 1) % MAX_STATE_HISTORY];
		if ( prev->serverTime == serverTime ) {
			// already have this snap recorded
			return;
		}
		if ( prev->serverTime > serverTime ) {
			// jumped back in time
			cent->currentStateHistory = cent->stateHistory.nextSlot;
		}
	}
	tstate->es = *state;
	tstate->serverTime = serverTime;
	tstate->isTeleport = cent->stateHistory.nextSlot <= 0 ||
		( prev && prev->serverTime > serverTime ) ||
		( ( cent->stateHistory.states[(cent->stateHistory.nextSlot - 1) % MAX_STATE_HISTORY].es.eFlags ^ state->eFlags ) & EF_TELEPORT_BIT ) ? qtrue : qfalse;
	if ( prev && state->pos.trType == TR_LINEAR_STOP ) {
		if ( cent->stateHistory.nextSlot == 0 ) {
			// base case
			tstate->time = serverTime;
		} else {
			// determine correct lerp time for new snap
			if (tstate->isTeleport) {
				// if frame is a teleport, reset as best we can
				tstate->time = Q_max(prev->time, tstate->serverTime);
				if ( prev->serverTime > tstate->serverTime ) {
					tstate->time = tstate->serverTime;
				}
			} else {
				// so for smoothest experience, time should be cg.nextSnap->ps.commandTime
				// but, we shouldn't drift time too far off or else player will move weird (will appear to be further ahead depending on ping)
				// so, prevent to drift further than 1 frame away in future, or 2 in past
				int bestNewTime = state->pos.trTime - prev->es.pos.trTime + prev->time;
				int deltaTime = tstate->serverTime - prev->serverTime;
				// allow  to drift ahead or behind by 1 frame only
				int newTime = Q_min(tstate->serverTime + deltaTime, Q_max(prev->serverTime, bestNewTime ) );
				tstate->time = newTime;
		
#ifdef _DEBUG
				if ( tstate->time < prev->time ) {
					Com_Printf( "WARNING: Drifted backwards\n" );
				}
#endif

				//Com_Printf("Corrected time %d ms (commandDiff %d, framedelta %d)\n", tps->time - tps->serverTime, tps->ps.commandTime - lasttps->ps.commandTime, deltaTime);
			}
		}
	} else {
		// command time unknown
#ifdef _DEBUG
		if (cg_commandSmooth.integer)
			Com_Printf( "commandTime unknown for client %d\n", state->number );
		tstate->time = serverTime;
#endif
	}
	if ( ( cent->stateHistory.nextSlot % MAX_STATE_HISTORY ) == ( cent->currentStateHistory % MAX_STATE_HISTORY ) && cent->currentStateHistory < cent->stateHistory.nextSlot ) {
		// buffer overflowed, force advance
#ifdef _DEBUG
		if (cg_commandSmooth.integer)
			Com_Printf( "centity buffer overflow for client %d\n", state->number );
#endif
		cent->currentStateHistory = cent->stateHistory.nextSlot - MAX_STATE_HISTORY + 1;
	}
	cent->stateHistory.nextSlot++;
}

void CG_UpdateTps( snapshot_t *snap, qboolean isTeleport ) {
	timedPlayerState_t	*tps, *lasttps = NULL;

	tps = &cg.psHistory.states[cg.psHistory.nextSlot % MAX_STATE_HISTORY];
	if ( cg.psHistory.nextSlot > 0 ) {
		lasttps = &cg.psHistory.states[( cg.psHistory.nextSlot - 1 ) % MAX_STATE_HISTORY];
		if ( lasttps->serverTime == snap->serverTime ) {
			// already have this snap recorded
			return;
		}
		if ( lasttps->serverTime > snap->serverTime ) {
			// jumped back in time
			cg.currentPsHistory = cg.psHistory.nextSlot;
		}
	}
	
	tps->ps = snap->ps;
	tps->serverTime = snap->serverTime;
	tps->isTeleport = isTeleport;
	if ( lasttps ) {
		// determine correct lerp time for new snap
		if ( isTeleport ) {
			// if frame is a teleport, reset as best we can
			tps->time = Q_max(lasttps->time, tps->serverTime );
			if ( lasttps->serverTime > tps->serverTime ) {
				tps->time = tps->serverTime;
			}
		}
		else {
			// so for smoothest experience, time should be cg.nextSnap->ps.commandTime
			// but, we shouldn't drift time too far off or else player will move weird (will appear to be further ahead depending on ping)
			// so, prevent to drift further than 1 frame away in future, or 2 in past
			int bestNewTime = tps->ps.commandTime - lasttps->ps.commandTime + lasttps->time;
			int deltaTime = tps->serverTime - lasttps->serverTime;
			// allow  to drift ahead or behind by 1 frame only
			int newTime = Q_min(tps->serverTime + deltaTime, Q_max(lasttps->serverTime, bestNewTime ) );
			if ( lasttps->serverTime != 0 ) {
				tps->time = newTime;
			}
			else {
				tps->time = tps->serverTime;
			}

#ifdef _DEBUG
			if ( tps->time < lasttps->time ) {
				Com_Printf( "WARNING: Drifted backwards\n" );
			}
#endif

			//Com_Printf("Corrected time %d ms (commandDiff %d, framedelta %d)\n", tps->time - tps->serverTime, tps->ps.commandTime - lasttps->ps.commandTime, deltaTime);
		}
	} else {
		tps->time = tps->serverTime;
	}
	if ( ( cg.psHistory.nextSlot % MAX_STATE_HISTORY ) == ( cg.currentPsHistory % MAX_STATE_HISTORY ) && cg.currentPsHistory < cg.psHistory.nextSlot ) {
		// buffer overflowed, force advance
#ifdef _DEBUG
		if (cg_commandSmooth.integer)
			Com_Printf( "ps buffer overflow\n" );
#endif
		cg.currentPsHistory = cg.psHistory.nextSlot - MAX_STATE_HISTORY + 1;
	}
	cg.psHistory.nextSlot++;
}

/*
==================
CG_SetInitialSnapshot

This will only happen on the very first snapshot, or
on tourney restarts.  All other times will use 
CG_TransitionSnapshot instead.

FIXME: Also called by map_restart?
==================
*/
void CG_SetInitialSnapshot( snapshot_t *snap ) {
	int				i;
	centity_t		*cent;
	entityState_t	*state, pes;

	cg.snap = snap; 

	if ((cg_entities[snap->ps.clientNum].ghoul2 == NULL) && trap_G2_HaveWeGhoul2Models(cgs.clientinfo[snap->ps.clientNum].ghoul2Model))
	{
		trap_G2API_DuplicateGhoul2Instance(cgs.clientinfo[snap->ps.clientNum].ghoul2Model, &cg_entities[snap->ps.clientNum].ghoul2);
		CG_CopyG2WeaponInstance(&cg_entities[snap->ps.clientNum], FIRST_WEAPON, cg_entities[snap->ps.clientNum].ghoul2);
		
		if (trap_G2API_AddBolt(cg_entities[snap->ps.clientNum].ghoul2, 0, "face") == -1)
		{ //check now to see if we have this bone for setting anims and such
			cg_entities[snap->ps.clientNum].noFace = qtrue;
		}
	}
	BG_PlayerStateToEntityState( &snap->ps, &cg_entities[ snap->ps.clientNum ].currentState, qfalse );

	// sort out solid entities
	CG_BuildSolidList();

	CG_ExecuteNewServerCommands( snap->serverCommandSequence );

	// set our local weapon selection pointer to
	// what the server has indicated the current weapon is
	CG_Respawn();

	if (cg.demoPlayback == 2) {
		CG_UpdateTps(snap, qtrue);
		BG_PlayerStateToEntityStateExtraPolate(&snap->ps, &pes, snap->ps.commandTime, qfalse);
		CG_AddToHistory(snap->serverTime, &pes, &cg_entities[snap->ps.clientNum]);
	} else if (!cg.demoPlayback) {
		// init force selection to SPEED
		cg.forceSelect = cg.snap->ps.fd.forcePowerSelected = FP_SPEED;
		trap_SetUserCmdValue(cg.weaponSelect, cg.zoomSensitivity, 0.0f, 0.0f, 0.0f, cg.forceSelect, cg.itemSelect, qfalse);
	}

	for ( i = 0 ; i < cg.snap->numEntities ; i++ ) {
		state = &cg.snap->entities[ i ];
		cent = &cg_entities[ state->number ];

		memcpy(&cent->currentState, state, sizeof(entityState_t));
		//cent->currentState = *state;
		cent->interpolate = qfalse;
		cent->currentValid = qtrue;

		if (cg.demoPlayback == 2)
			CG_AddToHistory( snap->serverTime, state, cent );

		CG_ResetEntity( cent );

		// check for events
		CG_CheckEvents( cent );
	}
}

static qboolean CG_IsTeleport( snapshot_t *lastSnap, snapshot_t *snap ) {
	// if the next frame is a teleport for the playerstate, we
	// can't interpolate during demos
	if ( lastSnap && ( ( snap->ps.eFlags ^ lastSnap->ps.eFlags ) & EF_TELEPORT_BIT ) ) {
		return qtrue;
	}

	// if changing follow mode, don't interpolate
	if ( snap->ps.clientNum != lastSnap->ps.clientNum ) {
		return qtrue;
	}

	// if changing server restarts, don't interpolate
	if ( ( snap->snapFlags ^ lastSnap->snapFlags ) & SNAPFLAG_SERVERCOUNT ) {
		return qtrue;
	}

	return qfalse;
}

/*
===================
CG_SetNextSnap

A new snapshot has just been read in from the client system.
===================
*/
void CG_SetNextSnap( snapshot_t *snap ) {
	int					num;
	entityState_t		*es, pes;
	centity_t			*cent;

	cg.nextSnap = snap;

	//CG_CheckPlayerG2Weapons(&cg.snap->ps, &cg_entities[cg.snap->ps.clientNum]);
	BG_PlayerStateToEntityState( &snap->ps, &cg_entities[ snap->ps.clientNum ].nextState, qfalse );
	//cg_entities[ cg.snap->ps.clientNum ].interpolate = qtrue;
	//No longer want to do this, as the cg_entities[clnum] and cg.predictedPlayerEntity are one in the same.
	if (cg.demoPlayback == 2) {
		BG_PlayerStateToEntityStateExtraPolate(&snap->ps, &pes, snap->ps.commandTime, qfalse);
		CG_AddToHistory(snap->serverTime, &pes, &cg_entities[snap->ps.clientNum]);
	}
	//cg_entities[snap->ps.clientNum].interpolate = qtrue;

	// check for extrapolation errors
	for ( num = 0 ; num < snap->numEntities ; num++ ) 
	{
		es = &snap->entities[num];
		cent = &cg_entities[ es->number ];

		memcpy(&cent->nextState, es, sizeof(entityState_t));
		//cent->nextState = *es;

		// if this frame is a teleport, or the entity wasn't in the
		// previous frame, don't interpolate
		if ( !cent->currentValid || ( ( cent->currentState.eFlags ^ es->eFlags ) & EF_TELEPORT_BIT )  ) {
			cent->interpolate = qfalse;
		} else {
			cent->interpolate = qtrue;
		}

		if (cg.demoPlayback == 2)
			CG_AddToHistory( snap->serverTime, es, cent );
	}

	cg.nextFrameTeleport = CG_IsTeleport( cg.snap, snap );

	if ( cg.nextNextSnap == NULL && cg.demoPlayback == 2 ) {
		CG_UpdateTps( snap, cg.nextFrameTeleport );
	}

	// sort out solid entities
	CG_BuildSolidList();

#if _LOGTRAILS
	CG_LogStrafeTrail();
#endif
}

/*
===================
CG_SetNextNextSnap

A new snapshot has just been read in from the client system.
===================
*/
void CG_SetNextNextSnap( snapshot_t *snap ) {
	int num;
	entityState_t *es, pes;
	centity_t *cent;
	cg.nextNextSnap = snap;
	CG_UpdateTps( snap, CG_IsTeleport( cg.nextSnap, snap ) );
	BG_PlayerStateToEntityStateExtraPolate( &snap->ps, &pes, snap->ps.commandTime, qfalse );
	//cg_entities[ cg.snap->ps.clientNum ].interpolate = qtrue;
	//No longer want to do this, as the cg_entities[clnum] and cg.predictedPlayerEntity are one in the same.
	CG_AddToHistory( snap->serverTime, &pes, &cg_entities[ snap->ps.clientNum ] );

	for ( num = 0 ; num < snap->numEntities ; num++ ) 
	{
		es = &snap->entities[num];
		cent = &cg_entities[ es->number ];

		CG_AddToHistory( snap->serverTime, es, cent );
	}
}

/*
===================
CG_TransitionSnapshot

The transition point from snap to nextSnap has passed
===================
*/
extern qboolean CG_UsingEWeb(void); //cg_predict.c
void CG_TransitionSnapshot( void ) {
	centity_t			*cent;
	snapshot_t			*oldFrame;
	int					i;

	if ( !cg.snap ) {
		CG_Error( "CG_TransitionSnapshot: NULL cg.snap" );
	}
	if ( !cg.nextSnap ) {
		CG_Error( "CG_TransitionSnapshot: NULL cg.nextSnap" );
	}

	// execute any server string commands before transitioning entities
	CG_ExecuteNewServerCommands( cg.nextSnap->serverCommandSequence );

	// if we had a map_restart, set everthing with initial
	if ( !cg.snap ) {
	}

	// clear the currentValid flag for all entities in the existing snapshot
	for ( i = 0 ; i < cg.snap->numEntities ; i++ ) {
		cent = &cg_entities[ cg.snap->entities[ i ].number ];
		cent->currentValid = qfalse;
	}

	// move nextSnap to snap and do the transitions
	oldFrame = cg.snap;
	cg.snap = cg.nextSnap;

	//CG_CheckPlayerG2Weapons(&cg.snap->ps, &cg_entities[cg.snap->ps.clientNum]);
	//CG_CheckPlayerG2Weapons(&cg.snap->ps, &cg.predictedPlayerEntity);
	BG_PlayerStateToEntityState( &cg.snap->ps, &cg_entities[ cg.snap->ps.clientNum ].currentState, qfalse );
	cg_entities[ cg.snap->ps.clientNum ].interpolate = qfalse;

	for ( i = 0 ; i < cg.snap->numEntities ; i++ ) {
		cent = &cg_entities[ cg.snap->entities[ i ].number ];
		CG_TransitionEntity( cent );

		// remember time of snapshot this entity was last updated in
		cent->snapShotTime = cg.snap->serverTime;
	}

	if ( !cg.nextNextSnap ) {
		cg.nextSnap = NULL;	
	} else {
		CG_SetNextSnap( cg.nextNextSnap );
		cg.nextNextSnap = NULL;
	}

	// check for playerstate transition events
	if ( oldFrame ) {
		playerState_t	*ops, *ps;

		ops = &oldFrame->ps;
		ps = &cg.snap->ps;
		// teleporting checks are irrespective of prediction
		if ( ( ps->eFlags ^ ops->eFlags ) & EF_TELEPORT_BIT ) {
			cg.thisFrameTeleport = qtrue;	// will be cleared by prediction code
		}

		// if we are not doing client side movement prediction for any
		// reason, then the client events and view changes will be issued now
		if ( cg.demoPlayback || (cg.snap->ps.pm_flags & PMF_FOLLOW)
			|| cg_noPredict.integer || g_synchronousClients.integer || CG_UsingEWeb() ) {
			CG_TransitionPlayerState( ps, ops );
		}
	}

}

/*
========================
CG_ReadNextSnapshot

This is the only place new snapshots are requested
This may increment cgs.processedSnapshotNum multiple
times if the client system fails to return a
valid snapshot.
========================
*/
snapshot_t *CG_ReadNextSnapshot( void ) {
	qboolean	r;
	snapshot_t	*dest;

	if ( cg.latestSnapshotNum > cgs.processedSnapshotNum + 1000 ) {
		CG_Printf( "WARNING: CG_ReadNextSnapshot: way out of range, %i > %i\n", 
			cg.latestSnapshotNum, cgs.processedSnapshotNum );
	}

	while ( cgs.processedSnapshotNum < cg.latestSnapshotNum ) {
		// decide which of the two slots to load it into
		if ( cg.demoPlayback != 2 && cg.snap == &cg.activeSnapshots[0] ) {
			dest = &cg.activeSnapshots[1];
		} else if ( cg.demoPlayback != 2 ) {
			dest = &cg.activeSnapshots[0];
		} else if ( !cg.snap ) {
			dest = &cg.activeSnapshots[0];
		} else {
			// pick slot not already used
			int curOffset;
			for ( curOffset = 0; curOffset < 3; curOffset++ ) {
				if ( ( !cg.snap || cg.snap != &cg.activeSnapshots[curOffset] ) &&
						( !cg.nextSnap || cg.nextSnap != &cg.activeSnapshots[curOffset] ) ) {
					break;
				}
			}
			if ( curOffset == 3 ) {
				Com_Printf( "WARNING: Couldn't find unused activeSnapshot\n" );
			}
			dest = &cg.activeSnapshots[curOffset % 3];
		}

		// try to read the snapshot from the client system
		cgs.processedSnapshotNum++;
		r = trap_GetSnapshot( cgs.processedSnapshotNum, dest );

		// FIXME: why would trap_GetSnapshot return a snapshot with the same server time
		if ( cg.snap && r && dest->serverTime == cg.snap->serverTime ) {
			//[BugFix30]
			//According to dumbledore, this situation occurs when you're playing back a demo that was record when
			//the game was running in local mode.  As such, we need to skip those snaps or the demo looks laggy.
			if ( cg.demoPlayback ) 
			{
				continue;
			}
			//[/BugFix30]
		}

		// if it succeeded, return
		if ( r ) {
			CG_AddLagometerSnapshotInfo( dest );
			return dest;
		}

		// a GetSnapshot will return failure if the snapshot
		// never arrived, or  is so old that its entities
		// have been shoved off the end of the circular
		// buffer in the client system.

		// record as a dropped packet
		CG_AddLagometerSnapshotInfo( NULL );

		// If there are additional snapshots, continue trying to
		// read them.
	}

	// nothing left to read
	return NULL;
}


/*
============
CG_ProcessSnapshots

We are trying to set up a renderable view, so determine
what the simulated time is, and try to get snapshots
both before and after that time if available.

If we don't have a valid cg.snap after exiting this function,
then a 3D game view cannot be rendered.  This should only happen
right after the initial connection.  After cg.snap has been valid
once, it will never turn invalid.

Even if cg.snap is valid, cg.nextSnap may not be, if the snapshot
hasn't arrived yet (it becomes an extrapolating situation instead
of an interpolating one)

============
*/
void CG_ProcessSnapshots( void ) {
	snapshot_t		*snap;
	int				n;

	// see what the latest snapshot the client system has is
	trap_GetCurrentSnapshotNumber( &n, &cg.latestSnapshotTime );
	if ( n != cg.latestSnapshotNum ) {
		if ( n < cg.latestSnapshotNum ) {
			//do we need those?
			cg.snap = 0;
			cg.nextSnap = 0;
			cgs.processedSnapshotNum = -2;
//			// this should never happen
//			CG_Error( "CG_ProcessSnapshots: n < cg.latestSnapshotNum" );
		}
		cg.latestSnapshotNum = n;
	}

	// If we have yet to receive a snapshot, check for it.
	// Once we have gotten the first snapshot, cg.snap will
	// always have valid data for the rest of the game
	while ( !cg.snap ) {
		snap = CG_ReadNextSnapshot();
		if ( !snap ) {
			// we can't continue until we get a snapshot
			return;
		}

		// set our weapon selection to what
		// the playerstate is currently using
		if ( !( snap->snapFlags & SNAPFLAG_NOT_ACTIVE ) ) {
			CG_SetInitialSnapshot( snap );
		}
	}

	// loop until we either have a valid nextSnap with a serverTime
	// greater than cg.time to interpolate towards, or we run
	// out of available snapshots
	do {
		// if we don't have a nextframe, try and read a new one in
		if ( !cg.nextSnap ) {
			snap = CG_ReadNextSnapshot();

			// if we still don't have a nextframe, we will just have to
			// extrapolate
			if ( !snap ) {
				break;
			}

			CG_SetNextSnap( snap );


			// if time went backwards, we have a level restart
			if ( cg.nextSnap->serverTime < cg.snap->serverTime ) {
				CG_Error( "CG_ProcessSnapshots: Server time went backwards" );
			}
		}
		if ( cg.demoPlayback == 2 && !cg.nextNextSnap ) {
			snap = CG_ReadNextSnapshot();

			// if we still don't have a nextframe, we will just have to
			// extrapolate
			if ( snap ) {
				CG_SetNextNextSnap( snap );


				// if time went backwards, we have a level restart
				if ( cg.nextNextSnap->serverTime < cg.nextSnap->serverTime ) {
					CG_Error( "CG_ProcessSnapshots: Server time went backwards" );
				}
			}
		}

		// if our time is < nextFrame's, we have a nice interpolating state
		if ( cg.time >= cg.snap->serverTime && cg.time < cg.nextSnap->serverTime ) {
			break;
		}

		// we have passed the transition from nextFrame to frame
		CG_TransitionSnapshot();
	} while ( 1 );

	// assert our valid conditions upon exiting
	if ( cg.snap == NULL ) {
		CG_Error( "CG_ProcessSnapshots: cg.snap == NULL" );
	}
	if ( cg.time < cg.snap->serverTime ) {
		// this can happen right after a vid_restart
		cg.time = cg.snap->serverTime;
	}
	if ( cg.nextSnap != NULL && cg.nextSnap->serverTime <= cg.time ) {
		CG_Error( "CG_ProcessSnapshots: cg.nextSnap->serverTime <= cg.time" );
	}

}

#if _LOGTRAILS
static void CG_LogStrafeTrail(void) {
	if (Q_stricmp("0", cg_logStrafeTrail.string) && cg.snap->ps.stats[11] && cg.snap->ps.duelTime) {
		centity_t *player = &cg_entities[cg.snap->ps.clientNum];
		vec3_t diff;
		char *str;
		char input[MAX_QPATH], fileName[MAX_QPATH];

		if (!VectorCompare(player->lastOrigin, player->lerpOrigin) && VectorLengthSquared(diff) < 192 * 192) {
			Q_strncpyz(input, cg_logStrafeTrail.string, sizeof(input));

			Q_strstrip(input, "\n\r;:?*<>|\"\\/ ", NULL);
			Q_CleanStr(input, qfalse);
			Q_strlwr(fileName);//dat linux

			Com_sprintf(fileName, sizeof(fileName), "strafetrails/%s.cfg", input);

			if (!cg.japro.loggingStrafeTrail) {
				trap_FS_FOpenFile(fileName, &cg.japro.strafeTrailFileHandle, FS_APPEND);
			}
			else { //Filename exists
				if (Q_stricmp(cg.japro.logStrafeTrailFilename, fileName)) { //If we changed filename.. close old file?
					trap_FS_FCloseFile(cg.japro.strafeTrailFileHandle);
				}
			}

			Q_strncpyz(cg.japro.logStrafeTrailFilename, fileName, sizeof(cg.japro.logStrafeTrailFilename));

			str = va("%i %i %i\n", (int)cg.snap->ps.origin[0], (int)cg.snap->ps.origin[1], (int)cg.snap->ps.origin[2]);

			if (cg.japro.strafeTrailFileHandle) {
				trap_FS_Write(str, strlen(str), cg.japro.strafeTrailFileHandle);
			}
			cg.japro.loggingStrafeTrail = qtrue;
		}

		VectorCopy(player->lerpOrigin, player->lastOrigin);
		return;
	}

	if (cg.japro.loggingStrafeTrail && (!Q_stricmp("0", cg_logStrafeTrail.string) || (cg.snap->ps.stats[11] && !cg.snap->ps.duelTime)))
	{
		//Com_Printf("closing to %s\n", cg.japro.logStrafeTrailFilename);
		trap_FS_FCloseFile(cg.japro.strafeTrailFileHandle);
		cg.japro.loggingStrafeTrail = qfalse;
	}
}
#endif
