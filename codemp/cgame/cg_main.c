// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_main.c -- initialization and primary entry point for cgame
#include "cg_local.h"

#include "../ui/ui_shared.h"
// display context for new ui stuff
displayContextDef_t cgDC;

#include "cg_lights.h"
#include "cg_multispec.h"

extern int cgSiegeRoundState;
extern int cgSiegeRoundTime;
/*
Ghoul2 Insert Start
*/
void CG_InitItems(void);
/*
Ghoul2 Insert End
*/

void CG_InitJetpackGhoul2(void);
void CG_CleanJetpackGhoul2(void);

vec4_t colorTable[CT_MAX] = 
{
{0, 0, 0, 0},			// CT_NONE
{0, 0, 0, 1},			// CT_BLACK
{1, 0, 0, 1},			// CT_RED
{0, 1, 0, 1},			// CT_GREEN
{0, 0, 1, 1},			// CT_BLUE
{1, 1, 0, 1},			// CT_YELLOW
{1, 0, 1, 1},			// CT_MAGENTA
{0, 1, 1, 1},			// CT_CYAN
{1, 1, 1, 1},			// CT_WHITE
{0.75f, 0.75f, 0.75f, 1},	// CT_LTGREY
{0.50f, 0.50f, 0.50f, 1},	// CT_MDGREY
{0.25f, 0.25f, 0.25f, 1},	// CT_DKGREY
{0.15f, 0.15f, 0.15f, 1},	// CT_DKGREY2

{0.810f, 0.530f, 0.0f,  1},	// CT_VLTORANGE -- needs values
{0.810f, 0.530f, 0.0f,  1},	// CT_LTORANGE
{0.610f, 0.330f, 0.0f,  1},	// CT_DKORANGE
{0.402f, 0.265f, 0.0f,  1},	// CT_VDKORANGE

{0.503f, 0.375f, 0.996f, 1},	// CT_VLTBLUE1
{0.367f, 0.261f, 0.722f, 1},	// CT_LTBLUE1
{0.199f, 0.0f,   0.398f, 1},	// CT_DKBLUE1
{0.160f, 0.117f, 0.324f, 1},	// CT_VDKBLUE1

{0.300f, 0.628f, 0.816f, 1},	// CT_VLTBLUE2 -- needs values
{0.300f, 0.628f, 0.816f, 1},	// CT_LTBLUE2
{0.191f, 0.289f, 0.457f, 1},	// CT_DKBLUE2
{0.125f, 0.250f, 0.324f, 1},	// CT_VDKBLUE2

{0.796f, 0.398f, 0.199f, 1},	// CT_VLTBROWN1 -- needs values
{0.796f, 0.398f, 0.199f, 1},	// CT_LTBROWN1
{0.558f, 0.207f, 0.027f, 1},	// CT_DKBROWN1
{0.328f, 0.125f, 0.035f, 1},	// CT_VDKBROWN1

{0.996f, 0.796f, 0.398f, 1},	// CT_VLTGOLD1 -- needs values
{0.996f, 0.796f, 0.398f, 1},	// CT_LTGOLD1
{0.605f, 0.441f, 0.113f, 1},	// CT_DKGOLD1
{0.386f, 0.308f, 0.148f, 1},	// CT_VDKGOLD1

{0.648f, 0.562f, 0.784f, 1},	// CT_VLTPURPLE1 -- needs values
{0.648f, 0.562f, 0.784f, 1},	// CT_LTPURPLE1
{0.437f, 0.335f, 0.597f, 1},	// CT_DKPURPLE1
{0.308f, 0.269f, 0.375f, 1},	// CT_VDKPURPLE1

{0.816f, 0.531f, 0.710f, 1},	// CT_VLTPURPLE2 -- needs values
{0.816f, 0.531f, 0.710f, 1},	// CT_LTPURPLE2
{0.566f, 0.269f, 0.457f, 1},	// CT_DKPURPLE2
{0.343f, 0.226f, 0.316f, 1},	// CT_VDKPURPLE2

{0.929f, 0.597f, 0.929f, 1},	// CT_VLTPURPLE3
{0.570f, 0.371f, 0.570f, 1},	// CT_LTPURPLE3
{0.355f, 0.199f, 0.355f, 1},	// CT_DKPURPLE3
{0.285f, 0.136f, 0.230f, 1},	// CT_VDKPURPLE3

{0.953f, 0.378f, 0.250f, 1},	// CT_VLTRED1
{0.953f, 0.378f, 0.250f, 1},	// CT_LTRED1
{0.593f, 0.121f, 0.109f, 1},	// CT_DKRED1
{0.429f, 0.171f, 0.113f, 1},	// CT_VDKRED1
{.25f, 0, 0, 1},					// CT_VDKRED
{.70f, 0, 0, 1},					// CT_DKRED
	
{0.717f, 0.902f, 1.0f,   1},		// CT_VLTAQUA
{0.574f, 0.722f, 0.804f, 1},		// CT_LTAQUA
{0.287f, 0.361f, 0.402f, 1},		// CT_DKAQUA
{0.143f, 0.180f, 0.201f, 1},		// CT_VDKAQUA

{0.871f, 0.386f, 0.375f, 1},		// CT_LTPINK
{0.435f, 0.193f, 0.187f, 1},		// CT_DKPINK
{	  0,    .5f,    .5f, 1},		// CT_LTCYAN
{	  0,   .25f,   .25f, 1},		// CT_DKCYAN
{   .179f, .51f,   .92f, 1},		// CT_LTBLUE3
{   .199f, .71f,   .92f, 1},		// CT_LTBLUE3
{   .5f,   .05f,    .4f, 1},		// CT_DKBLUE3

{   0.0f,   .613f,  .097f, 1},		// CT_HUD_GREEN
{   0.835f, .015f,  .015f, 1},		// CT_HUD_RED
{	.567f,	.685f,	1.0f,	.75f},	// CT_ICON_BLUE
{	.515f,	.406f,	.507f,	1},		// CT_NO_AMMO_RED
{   1.0f,   .658f,  .062f, 1},		// CT_HUD_ORANGE

};

#include "holocronicons.h"

int cgWeatherOverride = 0;

void CG_Init( int serverMessageNum, int serverCommandSequence, int clientNum );
void CG_Shutdown( void );

//void CG_CalcEntityLerpPositions( centity_t *cent );	//mme: brought to cg_local.h
void CG_ROFF_NotetrackCallback( centity_t *cent, const char *notetrack);

void UI_CleanupGhoul2(void);

static int	C_PointContents(void);
static void C_GetLerpOrigin(void);
static void C_GetLerpData(void);
static void C_Trace(void);
static void C_G2Trace(void);
static void C_G2Mark(void);
static int	CG_RagCallback(int callType);
static void C_GetBoltPos(void);
static void C_ImpactMark(void);

extern autoMapInput_t cg_autoMapInput; //cg_view.c
extern int cg_autoMapInputTime;
extern vec3_t cg_autoMapAngle;

void CG_MiscEnt(void);
void CG_DoCameraShake( vec3_t origin, float intensity, int radius, int time );

//do we have any force powers that we would normally need to cycle to?
qboolean CG_NoUseableForce(void)
{
	int i = FP_HEAL;
	while (i < NUM_FORCE_POWERS)
	{
		if (i != FP_SABERTHROW &&
			i != FP_SABER_OFFENSE &&
			i != FP_SABER_DEFENSE &&
			i != FP_LEVITATION)
		{ //valid selectable power
			if (cg.predictedPlayerState.fd.forcePowersKnown & (1 << i))
			{ //we have it
				return qfalse;
			}
		}
		i++;
	}

	//no useable force powers, I guess.
	return qtrue;
}

/*
================
vmMain

This is the only way control passes into the module.
This must be the very first function compiled into the .q3vm file
================
*/
extern void CG_DemosDrawActiveFrame( int serverTime, stereoFrame_t stereoView );
extern qboolean CG_DemosConsoleCommand( void );
extern qboolean CG_DemosKeyEvent(int key, qboolean down);
extern void CG_DemosMouseEvent(int dx, int dy);
Q_EXPORT intptr_t vmMain(int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11) {
	switch (command) {
	case CG_INIT:
		CG_Init(arg0, arg1, arg2);
		return 0;
	case CG_SHUTDOWN:
		CG_Shutdown();
		return 0;
	case CG_CONSOLE_COMMAND:
		if (cg.demoPlayback == 2)
			return CG_DemosConsoleCommand();
		else
			return CG_ConsoleCommand();
	case CG_DRAW_ACTIVE_FRAME:
		if (arg2 == 2) {
			CG_DemosDrawActiveFrame(arg0, arg1);
		} else {
			CG_DrawActiveFrame(arg0, arg1, arg2);
		}
		return 0;
	case CG_CROSSHAIR_PLAYER:
		return CG_CrosshairPlayer();
	case CG_LAST_ATTACKER:
		return CG_LastAttacker();
	case CG_KEY_EVENT:
		if (CG_MultiSpecEditing())
			return CG_MultiSpecKeyEvent(arg0, arg1);
		else if (cg.demoPlayback == 2)
			return CG_DemosKeyEvent(arg0, arg1);
		else
			return CG_KeyEvent(arg0, arg1);
	case CG_MOUSE_EVENT:
		cgDC.cursorx = cgs.cursorX;
		cgDC.cursory = cgs.cursorY;
		if (CG_MultiSpecEditing())
			CG_MultiSpecMouseEvent(arg0, arg1);
		else if (cg.demoPlayback == 2)
			CG_DemosMouseEvent(arg0, arg1);
		else
			CG_MouseEvent(arg0, arg1);
		return 0;
	case CG_EVENT_HANDLING:
		CG_EventHandling(arg0);
		return 0;
	case CG_POINT_CONTENTS:
		return C_PointContents();
	case CG_GET_LERP_ORIGIN:
		C_GetLerpOrigin();
		return 0;
	case CG_GET_LERP_DATA:
		C_GetLerpData();
		return 0;
	case CG_GET_GHOUL2:
		return (int)cg_entities[arg0].ghoul2; //NOTE: This is used by the effect bolting which is actually not used at all.
											  //I'm fairly sure if you try to use it with vm's it will just give you total
											  //garbage. In other words, use at your own risk.
	case CG_GET_MODEL_LIST:
		return (int)cgs.gameModels;
	case CG_CALC_LERP_POSITIONS:
		CG_CalcEntityLerpPositions(&cg_entities[arg0]);
		return 0;
	case CG_TRACE:
		C_Trace();
		return 0;
	case CG_GET_SORTED_FORCE_POWER:
		return forcePowerSorted[arg0];
	case CG_G2TRACE:
		C_G2Trace();
		return 0;
	case CG_G2MARK:
		C_G2Mark();
		return 0;
	case CG_RAG_CALLBACK:
		return CG_RagCallback(arg0);
	case CG_INCOMING_CONSOLE_COMMAND:
		//rww - let mod authors filter client console messages so they can cut them off if they want.
		//return 1 if the command is ok. Otherwise, you can set char 0 on the command str to 0 and return
		//0 to not execute anything, or you can fill conCommand in with something valid and return 0
		//in order to have that string executed in place. Some example code:
#if 0
		{
			TCGIncomingConsoleCommand	*icc = (TCGIncomingConsoleCommand *)cg.sharedBuffer;
			if (strstr(icc->conCommand, "wait")) {
			//filter out commands contaning wait
				Com_Printf("You can't use commands containing the string wait with MyMod v1.0\n");
				icc->conCommand[0] = 0;
				return 0;
			} else if (strstr(icc->conCommand, "blah")) {
			//any command containing the string "blah" is redirected to "quit"
				strcpy(icc->conCommand, "quit");
				return 0;
			}
		}
#endif
		return 1;
	case CG_GET_USEABLE_FORCE:
		return CG_NoUseableForce();
	case CG_GET_ORIGIN:
		VectorCopy(cg_entities[arg0].currentState.pos.trBase, (float *)arg1);
		return 0;
	case CG_GET_ANGLES:
		VectorCopy(cg_entities[arg0].currentState.apos.trBase, (float *)arg1);
		return 0;
	case CG_GET_ORIGIN_TRAJECTORY:
		return (int)&cg_entities[arg0].nextState.pos;
	case CG_GET_ANGLE_TRAJECTORY:
		return (int)&cg_entities[arg0].nextState.apos;
	case CG_ROFF_NOTETRACK_CALLBACK:
		CG_ROFF_NotetrackCallback(&cg_entities[arg0], (const char *)arg1);
		return 0;
	case CG_IMPACT_MARK:
		C_ImpactMark();
		return 0;
	case CG_MAP_CHANGE:
		// this trap map be called more than once for a given map change, as the
		// server is going to attempt to send out multiple broadcasts in hopes that
		// the client will receive one of them
		cg.mMapChange = qtrue;
		return 0;
	case CG_AUTOMAP_INPUT:
		//special input during automap mode -rww
		{
			autoMapInput_t *autoInput = (autoMapInput_t *)cg.sharedBuffer;
			memcpy(&cg_autoMapInput, autoInput, sizeof(autoMapInput_t));
			if (!arg0) {
			//if this is non-0, it's actually a one-frame mouse event
				cg_autoMapInputTime = cg.time + 1000;
			} else {
				if (cg_autoMapInput.yaw) {
					cg_autoMapAngle[YAW] += cg_autoMapInput.yaw;
				}
				if (cg_autoMapInput.pitch) {
					cg_autoMapAngle[PITCH] += cg_autoMapInput.pitch;
				}
				cg_autoMapInput.yaw = 0.0f;
				cg_autoMapInput.pitch = 0.0f;
			}
		}
		return 0;
	case CG_MISC_ENT:
		CG_MiscEnt();
		return 0;
	case CG_FX_CAMERASHAKE:
		{
			TCGCameraShake *data = (TCGCameraShake *)cg.sharedBuffer;
			CG_DoCameraShake(data->mOrigin, data->mIntensity, data->mRadius, data->mTime);
		}
		return 0;
	default:
		CG_Error("vmMain: unknown command %i", command);
		break;
	}
	return -1;
}

int fxT;
qboolean doFX = qfalse;

static int C_PointContents(void)
{
	TCGPointContents	*data = (TCGPointContents *)cg.sharedBuffer;

	return CG_PointContents( data->mPoint, data->mPassEntityNum );
}

static void C_GetLerpOrigin(void)
{
	TCGVectorData		*data = (TCGVectorData *)cg.sharedBuffer;

	VectorCopy(cg_entities[data->mEntityNum].lerpOrigin, data->mPoint);
}

static void C_GetLerpData(void)
{//only used by FX system to pass to getboltmat
	TCGGetBoltData		*data = (TCGGetBoltData *)cg.sharedBuffer;

	VectorCopy(cg_entities[data->mEntityNum].lerpOrigin, data->mOrigin);
	VectorCopy(cg_entities[data->mEntityNum].modelScale, data->mScale);
	VectorCopy(cg_entities[data->mEntityNum].lerpAngles, data->mAngles);
	if (cg_entities[data->mEntityNum].currentState.eType == ET_PLAYER)
	{ //normal player
		data->mAngles[PITCH] = 0.0f;
		data->mAngles[ROLL] = 0.0f;
	}
	else if (cg_entities[data->mEntityNum].currentState.eType == ET_NPC)
	{ //an NPC
		Vehicle_t *pVeh = cg_entities[data->mEntityNum].m_pVehicle;
		if (!pVeh)
		{ //for vehicles, we may or may not want to 0 out pitch and roll
			data->mAngles[PITCH] = 0.0f;
			data->mAngles[ROLL] = 0.0f;
		}
		else if (pVeh->m_pVehicleInfo->type == VH_SPEEDER)
		{ //speeder wants no pitch but a roll
			data->mAngles[PITCH] = 0.0f;
		}
		else if (pVeh->m_pVehicleInfo->type != VH_FIGHTER)
		{ //fighters want all angles
			data->mAngles[PITCH] = 0.0f;
			data->mAngles[ROLL] = 0.0f;
		}
	}
}

static void C_Trace(void)
{
	TCGTrace	*td = (TCGTrace *)cg.sharedBuffer;

	CG_Trace(&td->mResult, td->mStart, td->mMins, td->mMaxs, td->mEnd, td->mSkipNumber, td->mMask);
}

static void C_G2Trace(void)
{
	TCGTrace	*td = (TCGTrace *)cg.sharedBuffer;

	CG_G2Trace(&td->mResult, td->mStart, td->mMins, td->mMaxs, td->mEnd, td->mSkipNumber, td->mMask);
}

static void C_G2Mark(void)
{
	TCGG2Mark	*td = (TCGG2Mark *)cg.sharedBuffer;
	trace_t		tr;
	vec3_t		end;

	VectorMA(td->start, 64, td->dir, end);
	CG_G2Trace(&tr, td->start, NULL, NULL, end, ENTITYNUM_NONE, MASK_PLAYERSOLID);

	if (tr.entityNum < ENTITYNUM_WORLD &&
		cg_entities[tr.entityNum].ghoul2)
	{ //hit someone with a ghoul2 instance, let's project the decal on them then.
		centity_t *cent = &cg_entities[tr.entityNum];

		//CG_TestLine(tr.endpos, end, 2000, 0x0000ff, 1);

		CG_AddGhoul2Mark(td->shader, td->size, tr.endpos, end, tr.entityNum,
			cent->lerpOrigin, cent->lerpAngles[YAW], cent->ghoul2, cent->modelScale,
			Q_irand(2000, 4000));
		//I'm making fx system decals have a very short lifetime.
	}
}

static void CG_DebugBoxLines(vec3_t mins, vec3_t maxs, int duration)
{
	vec3_t start;
	vec3_t end;
	vec3_t vert;

	float x = maxs[0] - mins[0];
	float y = maxs[1] - mins[1];

	start[2] = maxs[2];
	vert[2] = mins[2];

	vert[0] = mins[0];
	vert[1] = mins[1];
	start[0] = vert[0];
	start[1] = vert[1];
	CG_TestLine(start, vert, duration, 0x00000ff, 1);

	vert[0] = mins[0];
	vert[1] = maxs[1];
	start[0] = vert[0];
	start[1] = vert[1];
	CG_TestLine(start, vert, duration, 0x00000ff, 1);

	vert[0] = maxs[0];
	vert[1] = mins[1];
	start[0] = vert[0];
	start[1] = vert[1];
	CG_TestLine(start, vert, duration, 0x00000ff, 1);

	vert[0] = maxs[0];
	vert[1] = maxs[1];
	start[0] = vert[0];
	start[1] = vert[1];
	CG_TestLine(start, vert, duration, 0x00000ff, 1);

	// top of box
	VectorCopy(maxs, start);
	VectorCopy(maxs, end);
	start[0] -= x;
	CG_TestLine(start, end, duration, 0x00000ff, 1);
	end[0] = start[0];
	end[1] -= y;
	CG_TestLine(start, end, duration, 0x00000ff, 1);
	start[1] = end[1];
	start[0] += x;
	CG_TestLine(start, end, duration, 0x00000ff, 1);
	CG_TestLine(start, maxs, duration, 0x00000ff, 1);
	// bottom of box
	VectorCopy(mins, start);
	VectorCopy(mins, end);
	start[0] += x;
	CG_TestLine(start, end, duration, 0x00000ff, 1);
	end[0] = start[0];
	end[1] += y;
	CG_TestLine(start, end, duration, 0x00000ff, 1);
	start[1] = end[1];
	start[0] -= x;
	CG_TestLine(start, end, duration, 0x00000ff, 1);
	CG_TestLine(start, mins, duration, 0x00000ff, 1);
}

//handle ragdoll callbacks, for events and debugging -rww
static int CG_RagCallback(int callType)
{
	switch(callType)
	{
	case RAG_CALLBACK_DEBUGBOX:
		{
			ragCallbackDebugBox_t *callData = (ragCallbackDebugBox_t *)cg.sharedBuffer;

			CG_DebugBoxLines(callData->mins, callData->maxs, callData->duration);
		}
		break;
	case RAG_CALLBACK_DEBUGLINE:
		{
			ragCallbackDebugLine_t *callData = (ragCallbackDebugLine_t *)cg.sharedBuffer;

			CG_TestLine(callData->start, callData->end, callData->time, callData->color, callData->radius);
		}
		break;
	case RAG_CALLBACK_BONESNAP:
		{
			ragCallbackBoneSnap_t *callData = (ragCallbackBoneSnap_t *)cg.sharedBuffer;
			centity_t *cent = &cg_entities[callData->entNum];
			int snapSound = trap_S_RegisterSound(va("sound/player/bodyfall_human%i.wav", Q_irand(1, 3)));

			trap_S_StartSound(cent->lerpOrigin, callData->entNum, CHAN_AUTO, snapSound);
		}
	case RAG_CALLBACK_BONEIMPACT:
		break;
	case RAG_CALLBACK_BONEINSOLID:
#if 0
		{
			ragCallbackBoneInSolid_t *callData = (ragCallbackBoneInSolid_t *)cg.sharedBuffer;

			if (callData->solidCount > 16)
			{ //don't bother if we're just tapping into solidity, we'll probably recover on our own
				centity_t *cent = &cg_entities[callData->entNum];
				vec3_t slideDir;

				VectorSubtract(cent->lerpOrigin, callData->bonePos, slideDir);
				VectorAdd(cent->ragOffsets, slideDir, cent->ragOffsets);

				cent->hasRagOffset = qtrue;
			}
		}
#endif
		break;
	case RAG_CALLBACK_TRACELINE:
		{
			ragCallbackTraceLine_t *callData = (ragCallbackTraceLine_t *)cg.sharedBuffer;

			CG_Trace(&callData->tr, callData->start, callData->mins, callData->maxs,
				callData->end, callData->ignore, callData->mask);
		}
		break;
	default:
		Com_Error(ERR_DROP, "Invalid callType in CG_RagCallback");
		break;
	}

	return 0;
}

static void C_ImpactMark(void)
{
	TCGImpactMark	*data = (TCGImpactMark *)cg.sharedBuffer;

	/*
	CG_ImpactMark((int)arg0, (const float *)arg1, (const float *)arg2, (float)arg3,
		(float)arg4, (float)arg5, (float)arg6, (float)arg7, qtrue, (float)arg8, qfalse);
	*/
	CG_ImpactMark(data->mHandle, data->mPoint, data->mAngle, data->mRotation,
		data->mRed, data->mGreen, data->mBlue, data->mAlphaStart, qtrue, data->mSizeStart, qfalse);
}

void CG_MiscEnt(void)
{
	int i;
	int modelIndex;
	TCGMiscEnt *data = (TCGMiscEnt *)cg.sharedBuffer;
	cg_staticmodel_t *staticmodel;

	if( cgs.numMiscStaticModels >= MAX_STATIC_MODELS ) {
		CG_Error( "^1MAX_STATIC_MODELS(%i) hit", MAX_STATIC_MODELS );
	}

	modelIndex = trap_R_RegisterModel(data->mModel);
	if (modelIndex == 0) {
		CG_Error( "client_model failed to load model '%s'", data->mModel );
		return;
	}

	staticmodel = &cgs.miscStaticModels[cgs.numMiscStaticModels++];
	staticmodel->model = modelIndex;
	AnglesToAxis( data->mAngles, staticmodel->axes );
	for ( i = 0; i < 3; i++ ) {
		VectorScale( staticmodel->axes[i], data->mScale[i], staticmodel->axes[i] );
	}

	VectorCopy( data->mOrigin, staticmodel->org );
	staticmodel->zoffset = 0.f;

	if( staticmodel->model ) {
		vec3_t mins, maxs;

		trap_R_ModelBounds( staticmodel->model, mins, maxs );

		VectorScaleVector(mins, data->mScale, mins);
		VectorScaleVector(maxs, data->mScale, maxs);

		staticmodel->radius = RadiusFromBounds( mins, maxs );
	} else {
		staticmodel->radius = 0;
	}
}

/*
Ghoul2 Insert Start
*/
/*
void CG_ResizeG2Bolt(boltInfo_v *bolt, int newCount)
{
	bolt->resize(newCount);
}

void CG_ResizeG2Surface(surfaceInfo_v *surface, int newCount)
{
	surface->resize(newCount);
}

void CG_ResizeG2Bone(boneInfo_v *bone, int newCount)
{
	bone->resize(newCount);
}

void CG_ResizeG2(CGhoul2Info_v *ghoul2, int newCount)
{
	ghoul2->resize(newCount);
}

void CG_ResizeG2TempBone(mdxaBone_v *tempBone, int newCount)
{
	tempBone->resize(newCount);
}
*/
/*
Ghoul2 Insert End
*/
cg_t				cg;
cgs_t				cgs;
centity_t			cg_entities[MAX_GENTITIES];

centity_t			*cg_permanents[MAX_GENTITIES]; //rwwRMG - added
int					cg_numpermanents = 0;

weaponInfo_t		cg_weapons[MAX_WEAPONS];
itemInfo_t			cg_items[MAX_ITEMS];

extern void CG_CheckNotification(void);

extern void trap_MME_FontRatioFix( float ratio );
static void CG_Set2DRatio(void) {
	if (mov_ratioFix.integer)
		cgs.widthRatioCoef = (float)(SCREEN_WIDTH * cgs.glconfig.vidHeight) / (float)(SCREEN_HEIGHT * cgs.glconfig.vidWidth);
	else
		cgs.widthRatioCoef = 1.0f;
	if (mov_ratioFix.integer == 2)
		trap_MME_FontRatioFix(1.0f);
	else
		trap_MME_FontRatioFix(cgs.widthRatioCoef);
}

colorTable_t CG_SwitchColorTable(void) {
	colorTable_t cTable;
	if (cg.rpmod.detected) {
		cTable = CT_RPMOD;
	} else if (cg.uag.newColors) {
		cTable = CT_UAG;
	} else {
		cTable = CT_DEFAULT;
	}
	return cTable;
}

extern void trap_MME_ExtendedColors( colorTable_t cTable );
void CG_SetExtendedColours(void) {
	if (cg_UAGColours.integer == 2)
		cg.uag.newColors = qtrue;
	else if (cg_UAGColours.integer)
		cg.uag.newColors = cg.uag.detected;
	else
		cg.uag.newColors = qfalse;
	trap_MME_ExtendedColors(CG_SwitchColorTable());
}

//Strafehelper colors
static void CG_CrosshairColorChange(void) {
    int i;
    if (sscanf(cg_crosshairColor.string, "%f %f %f %f", &cg.crosshairColor[0], &cg.crosshairColor[1], &cg.crosshairColor[2], &cg.crosshairColor[3]) != 4) {
        cg.crosshairColor[0] = 0;
        cg.crosshairColor[1] = 0;
        cg.crosshairColor[2] = 0;
        cg.crosshairColor[3] = 255;
    }

    for (i = 0; i < 4; i++) {
        if (cg.crosshairColor[i] < 1)
            cg.crosshairColor[i] = 0;
        else if (cg.crosshairColor[i] > 255)
            cg.crosshairColor[i] = 255;
    }

    cg.crosshairColor[0] /= 255.0f;
    cg.crosshairColor[1] /= 255.0f;
    cg.crosshairColor[2] /= 255.0f;
    cg.crosshairColor[3] /= 255.0f;

    //Com_Printf("New color is %f, %f, %f, %f\n", cg.crosshairColor[0], cg.crosshairColor[1], cg.crosshairColor[2], cg.crosshairColor[3]);
}

static void CVU_StrafeHelper (void) {
    trap_Cvar_Set( "cg_strafeHelperActiveColor", va("%i %i %i %i", ui_sha_r.integer, ui_sha_g.integer, ui_sha_b.integer, ui_sha_a.integer) );
}

static void CG_StrafeHelperActiveColorChange(void) {
    int i;
    if (sscanf(cg_strafeHelperActiveColor.string, "%f %f %f %f", &cg.strafeHelperActiveColor[0], &cg.strafeHelperActiveColor[1], &cg.strafeHelperActiveColor[2], &cg.strafeHelperActiveColor[3]) != 4) {
        cg.strafeHelperActiveColor[0] = 0;
        cg.strafeHelperActiveColor[1] = 255;
        cg.strafeHelperActiveColor[2] = 0;
        cg.strafeHelperActiveColor[3] = 100;
    }

    for (i = 0; i < 4; i++) {
        if (cg.strafeHelperActiveColor[i] < 0)
            cg.strafeHelperActiveColor[i] = 0;
        else if (cg.strafeHelperActiveColor[i] > 255)
            cg.strafeHelperActiveColor[i] = 255;
    }

    trap_Cvar_Set("ui_sha_r", va("%f", cg.strafeHelperActiveColor[0]));
    trap_Cvar_Set("ui_sha_g", va("%f", cg.strafeHelperActiveColor[1]));
    trap_Cvar_Set("ui_sha_b", va("%f", cg.strafeHelperActiveColor[2]));
    trap_Cvar_Set("ui_sha_a", va("%f", cg.strafeHelperActiveColor[3]));

    cg.strafeHelperActiveColor[0] /= 255.0f;
    cg.strafeHelperActiveColor[1] /= 255.0f;
    cg.strafeHelperActiveColor[2] /= 255.0f;
    cg.strafeHelperActiveColor[3] /= 255.0f;

    //Com_Printf("New color is %f, %f, %f, %f\n", cg.strafeHelperActiveColor[0], cg.strafeHelperActiveColor[1], cg.strafeHelperActiveColor[2], cg.strafeHelperActiveColor[3]);
}

static void CG_SetMovementKeysPos( void ) {
	if ( sscanf( cg_drawMovementKeysPos.string, "%f %f", &cg.moveKeysPos[0], &cg.moveKeysPos[1] ) != 2 ) {
        if(cg_drawMovementKeys.integer == 4) {
            cg.moveKeysPos[0] = (SCREEN_WIDTH / 2);
            cg.moveKeysPos[1] = (SCREEN_HEIGHT / 2);
        } else if (cg_drawMovementKeys.integer){
            cg.moveKeysPos[0] = 465;
            cg.moveKeysPos[1] = 432;
        }
	}
}

static void CG_SVRunningChange( void ) {
	cgs.localServer = sv_running.integer;
}

static void CG_ForceModelChange( void ) {
	int i;

	for ( i=0; i<MAX_CLIENTS; i++ ) {
		const char *clientInfo;
		void *oldGhoul2;

		clientInfo = CG_ConfigString( CS_PLAYERS+i );
		if ( !VALIDSTRING( clientInfo ) )
			continue;

		oldGhoul2 = cgs.clientinfo[i].ghoul2Model;
		CG_NewClientInfo( i, qtrue );
	}
}

static void CG_TeamOverlayChange( void ) {
	// If team overlay is on, ask for updates from the server.  If its off,
	// let the server know so we don't receive it
	if ( cg_drawTeamOverlay.integer > 0 && cgs.gametype >= GT_SINGLE_PLAYER)
		trap_Cvar_Set( "teamoverlay", "1" );
	else
		trap_Cvar_Set( "teamoverlay", "0" );
}

typedef struct {
	vmCvar_t	*vmCvar;
	char		*cvarName;
	char		*defaultString;
	void		(*update)( void );
	int			cvarFlags;
} cvarTable_t;

#define XCVAR_DECL
	#include "cg_xcvar.h"
#undef XCVAR_DECL

static cvarTable_t cvarTable[] = {
	#define XCVAR_LIST
		#include "cg_xcvar.h"
	#undef XCVAR_LIST
};
static int cvarTableSize = ARRAY_LEN( cvarTable );

/*
=================
CG_RegisterCvars
=================
*/
void CG_RegisterCvars( void ) {
	int			i;
	cvarTable_t	*cv;

	for ( i=0, cv=cvarTable; i<cvarTableSize; i++, cv++ ) {
		trap_Cvar_Register( cv->vmCvar, cv->cvarName, cv->defaultString, cv->cvarFlags );
		if ( cv->update )
			cv->update();
	}
}

/*
=================
CG_UpdateCvars
=================
*/
void CG_UpdateCvars( void ) {
	int			i;
	cvarTable_t	*cv;

	for ( i=0, cv=cvarTable; i<cvarTableSize; i++, cv++ ) {
		if ( cv->vmCvar ) {
			int modCount = cv->vmCvar->modificationCount;
			trap_Cvar_Update( cv->vmCvar );
			if ( cv->vmCvar->modificationCount > modCount ) {
				if ( cv->update )
					cv->update();
			}
		}
	}
}

int CG_CrosshairPlayer( void ) {
	if ( cg.time > (cg.crosshairClientTime + 1000) )
		return -1;

	if ( cg.crosshairClientNum >= MAX_CLIENTS )
		return -1;

	return cg.crosshairClientNum;
}

int CG_LastAttacker( void ) {
	if ( !cg.attackerTime )
		return -1;

	return cg.snap->ps.persistant[PERS_ATTACKER];
}

void QDECL CG_Printf( const char *msg, ... ) {
	va_list		argptr;
	char		text[1024] = {0};

	va_start( argptr, msg );
	Q_vsnprintf( text, sizeof( text ), msg, argptr );
	va_end( argptr );

	trap_Print( text );
}

void QDECL CG_Error( const char *msg, ... ) {
	va_list		argptr;
	char		text[1024] = {0};

	va_start (argptr, msg);
	Q_vsnprintf( text, sizeof( text ), msg, argptr );
	va_end (argptr);

	trap_Error( text );
}

void QDECL Com_Error( int level, const char *error, ... ) {
	va_list		argptr;
	char		text[1024] = {0};

	va_start( argptr, error );
	Q_vsnprintf( text, sizeof( text ), error, argptr );
	va_end( argptr );

	trap_Error(text);
}

void QDECL Com_Printf( const char *msg, ... ) {
	va_list		argptr;
	char		text[1024] = {0};

	va_start( argptr, msg );
	Q_vsnprintf( text, sizeof( text ), msg, argptr );
	va_end( argptr );

	trap_Print(text);
}

/*
================
CG_Argv
================
*/
const char *CG_Argv( int arg ) {
	static char	buffer[MAX_STRING_CHARS] = {0};

	trap_Argv( arg, buffer, sizeof( buffer ) );

	return buffer;
}


//========================================================================

//so shared code can get the local time depending on the side it's executed on
int BG_GetTime(void)
{
	return cg.time;
}

/*
=================
CG_RegisterItemSounds

The server says this item is used on this level
=================
*/
static void CG_RegisterItemSounds( int itemNum ) {
	gitem_t			*item;
	char			data[MAX_QPATH];
	char			*s, *start;
	int				len;

	item = &bg_itemlist[ itemNum ];

	if( item->pickup_sound ) {
		trap_S_RegisterSound( item->pickup_sound );
	}

	// parse the space seperated precache string for other media
	s = item->sounds;
	if (!s || !s[0])
		return;

	while (*s) {
		start = s;
		while (*s && *s != ' ') {
			s++;
		}

		len = s-start;
		if (len >= MAX_QPATH || len < 5) {
			CG_Error( "PrecacheItem: %s has bad precache string", 
				item->classname);
			return;
		}
		memcpy (data, start, len);
		data[len] = 0;
		if ( *s ) {
			s++;
		}

		trap_S_RegisterSound( data );
	}

	// parse the space seperated precache string for other media
	s = item->precaches;
	if (!s || !s[0])
		return;

	while (*s) {
		start = s;
		while (*s && *s != ' ') {
			s++;
		}

		len = s-start;
		if (len >= MAX_QPATH || len < 5) {
			CG_Error( "PrecacheItem: %s has bad precache string", 
				item->classname);
			return;
		}
		memcpy (data, start, len);
		data[len] = 0;
		if ( *s ) {
			s++;
		}

		if ( !strcmp(data+len-3, "efx" )) {
			trap_FX_RegisterEffect( data );
		}
	}
}

static void CG_AS_Register(void)
{
	const char *soundName;
	int i;

//	CG_LoadingString( "ambient sound sets" );

	//Load the ambient sets
#if 0 //as_preCacheMap was game-side.. that is evil.
	trap_AS_AddPrecacheEntry( "#clear" );	// ;-)
	//FIXME: Don't ask... I had to get around a really nasty MS error in the templates with this...
	namePrecache_m::iterator	pi;
	STL_ITERATE( pi, as_preCacheMap )
	{
		cgi_AS_AddPrecacheEntry( ((*pi).first).c_str() );
	}
#else
	trap_AS_AddPrecacheEntry( "#clear" );

	for ( i = 1 ; i < MAX_AMBIENT_SETS ; i++ ) {
		soundName = CG_ConfigString( CS_AMBIENT_SET+i );
		if ( !soundName || !soundName[0] )
		{
			break;
		}

		trap_AS_AddPrecacheEntry(soundName);
	}
	soundName = CG_ConfigString( CS_GLOBAL_AMBIENT_SET );
	if (soundName && soundName[0] && Q_stricmp(soundName, "default"))
	{ //global soundset
		trap_AS_AddPrecacheEntry(soundName);
	}
#endif

	trap_AS_ParseSets();
}

//a global weather effect (rain, snow, etc)
void CG_ParseWeatherEffect(const char *str)
{
	char *sptr = (char *)str;
	sptr++; //pass the '*'
	trap_R_WorldEffectCommand(sptr);
}

extern int cgSiegeRoundBeganTime;
void CG_ParseSiegeState(const char *str)
{
	int i = 0;
	int j = 0;
//	int prevState = cgSiegeRoundState;
	char b[1024];

	while (str[i] && str[i] != '|')
	{
		b[j] = str[i];
		i++;
		j++;
	}
	b[j] = 0;
	cgSiegeRoundState = atoi(b);

	if (str[i] == '|')
	{
		j = 0;
		i++;
		while (str[i])
		{
			b[j] = str[i];
			i++;
			j++;
		}
		b[j] = 0;
//		if (cgSiegeRoundState != prevState)
		{ //it changed
			cgSiegeRoundTime = atoi(b);
			if (cgSiegeRoundState == 0 || cgSiegeRoundState == 2)
			{
				cgSiegeRoundBeganTime = cgSiegeRoundTime;
			}
		}
	}
	else
	{
	    cgSiegeRoundTime = cg.time;
	}
}

/*
=================
CG_RegisterSounds

called during a precache command
=================
*/
void CG_PrecacheNPCSounds(const char *str);
void CG_ParseSiegeObjectiveStatus(const char *str);
extern int cg_beatingSiegeTime;
extern int cg_siegeWinTeam;
static void CG_RegisterSounds( void ) {
	int		i;
	char	items[MAX_ITEMS+1];
	char	name[MAX_QPATH];
	const char	*soundName;

	CG_AS_Register();

//	CG_LoadingString( "sounds" );

	trap_S_RegisterSound( "sound/weapons/melee/punch1.mp3" );
	trap_S_RegisterSound( "sound/weapons/melee/punch2.mp3" );
	trap_S_RegisterSound( "sound/weapons/melee/punch3.mp3" );
	trap_S_RegisterSound( "sound/weapons/melee/punch4.mp3" );
	trap_S_RegisterSound("sound/movers/objects/saber_slam.mp3");

	trap_S_RegisterSound("sound/player/bodyfall_human1.wav");
	trap_S_RegisterSound("sound/player/bodyfall_human2.wav");
	trap_S_RegisterSound("sound/player/bodyfall_human3.wav");

	//test effects
	trap_FX_RegisterEffect("effects/mp/test_sparks.efx");
	trap_FX_RegisterEffect("effects/mp/test_wall_impact.efx");

	cgs.media.oneMinuteSound = trap_S_RegisterSound( "sound/chars/protocol/misc/40MOM004.wav" );
	cgs.media.fiveMinuteSound = trap_S_RegisterSound( "sound/chars/protocol/misc/40MOM005.wav" );
	cgs.media.oneFragSound = trap_S_RegisterSound( "sound/chars/protocol/misc/40MOM001.wav" );
	cgs.media.twoFragSound = trap_S_RegisterSound( "sound/chars/protocol/misc/40MOM002.wav" );
	cgs.media.threeFragSound = trap_S_RegisterSound( "sound/chars/protocol/misc/40MOM003.wav");
	cgs.media.count3Sound = trap_S_RegisterSound( "sound/chars/protocol/misc/40MOM035.wav" );
	cgs.media.count2Sound = trap_S_RegisterSound( "sound/chars/protocol/misc/40MOM036.wav" );
	cgs.media.count1Sound = trap_S_RegisterSound( "sound/chars/protocol/misc/40MOM037.wav" );
	cgs.media.countFightSound = trap_S_RegisterSound( "sound/chars/protocol/misc/40MOM038.wav" );

	cgs.media.hackerIconShader			= trap_R_RegisterShaderNoMip("gfx/mp/c_icon_tech");

	cgs.media.redSaberGlowShader		= trap_R_RegisterShader( "gfx/effects/sabers/red_glow" );
	cgs.media.redSaberCoreShader		= trap_R_RegisterShader( "gfx/effects/sabers/red_line" );
	cgs.media.orangeSaberGlowShader		= trap_R_RegisterShader( "gfx/effects/sabers/orange_glow" );
	cgs.media.orangeSaberCoreShader		= trap_R_RegisterShader( "gfx/effects/sabers/orange_line" );
	cgs.media.yellowSaberGlowShader		= trap_R_RegisterShader( "gfx/effects/sabers/yellow_glow" );
	cgs.media.yellowSaberCoreShader		= trap_R_RegisterShader( "gfx/effects/sabers/yellow_line" );
	cgs.media.greenSaberGlowShader		= trap_R_RegisterShader( "gfx/effects/sabers/green_glow" );
	cgs.media.greenSaberCoreShader		= trap_R_RegisterShader( "gfx/effects/sabers/green_line" );
	cgs.media.blueSaberGlowShader		= trap_R_RegisterShader( "gfx/effects/sabers/blue_glow" );
	cgs.media.blueSaberCoreShader		= trap_R_RegisterShader( "gfx/effects/sabers/blue_line" );
	cgs.media.purpleSaberGlowShader		= trap_R_RegisterShader( "gfx/effects/sabers/purple_glow" );
	cgs.media.purpleSaberCoreShader		= trap_R_RegisterShader( "gfx/effects/sabers/purple_line" );


	//[RGBSabers]
	//if no ja++ shader
		//then try ja+ shader
	cgs.media.rgbSaberGlowShader		= trap_R_RegisterShaderNoMip( "gfx/effects/sabers/RGBglow1" );
	if (!cgs.media.rgbSaberGlowShader)
		cgs.media.rgbSaberGlowShader	= trap_R_RegisterShaderNoMip( "gfx/effects/sabers/RGBGlow" );
	cgs.media.rgbSaberCoreShader		= trap_R_RegisterShaderNoMip( "gfx/effects/sabers/RGBcore1" );
	if (!cgs.media.rgbSaberCoreShader)
		cgs.media.rgbSaberCoreShader	= trap_R_RegisterShaderNoMip( "gfx/effects/sabers/RGBLine" );

	//Flame 1
	cgs.media.rgbSaberGlow2Shader		= trap_R_RegisterShaderNoMip( "gfx/effects/sabers/RGBglow2" );
	if (!cgs.media.rgbSaberGlow2Shader)
		cgs.media.rgbSaberGlow2Shader	= trap_R_RegisterShaderNoMip( "gfx/effects/sabers/RGBGlow" );
	cgs.media.rgbSaberCore2Shader		= trap_R_RegisterShaderNoMip( "gfx/effects/sabers/RGBcore2" );
	cgs.media.rgbSaberTrail2Shader		= trap_R_RegisterShaderNoMip( "gfx/effects/sabers/RGBtrail2" );

	//Electric 1
	cgs.media.rgbSaberGlow3Shader		= trap_R_RegisterShaderNoMip( "gfx/effects/sabers/RGBglow3" );
	if (!cgs.media.rgbSaberGlow3Shader)
		cgs.media.rgbSaberGlow3Shader	= trap_R_RegisterShaderNoMip( "gfx/effects/sabers/RGBGlow" );
	cgs.media.rgbSaberCore3Shader		= trap_R_RegisterShaderNoMip( "gfx/effects/sabers/RGBcore3" );
	cgs.media.rgbSaberTrail3Shader		= trap_R_RegisterShaderNoMip( "gfx/effects/sabers/RGBtrail3" );

	//Flame 2
	cgs.media.rgbSaberGlow4Shader		= trap_R_RegisterShaderNoMip( "gfx/effects/sabers/RGBglow4" );
	cgs.media.rgbSaberCore4Shader		= trap_R_RegisterShaderNoMip( "gfx/effects/sabers/RGBcore4" );
	cgs.media.rgbSaberTrail4Shader		= trap_R_RegisterShaderNoMip( "gfx/effects/sabers/RGBtrail4" );

	//Electric 2
	cgs.media.rgbSaberGlow5Shader		= trap_R_RegisterShaderNoMip( "gfx/effects/sabers/RGBglow5" );
	if (!cgs.media.rgbSaberGlow5Shader)
		cgs.media.rgbSaberGlow5Shader	= trap_R_RegisterShaderNoMip( "gfx/effects/sabers/RGBGlow" );
	cgs.media.rgbSaberCore5Shader		= trap_R_RegisterShaderNoMip( "gfx/effects/sabers/RGBcore5" );
	cgs.media.rgbSaberTrail5Shader		= trap_R_RegisterShaderNoMip( "gfx/effects/sabers/swordTrail" );

	cgs.media.grappleShader = trap_R_RegisterShader("gfx/effects/grapple_line");//japro

	//Black
	cgs.media.blackSaberGlowShader		= trap_R_RegisterShaderNoMip( "gfx/effects/sabers/blackglow" );
	if (!cgs.media.blackSaberGlowShader)
		cgs.media.blackSaberGlowShader	= trap_R_RegisterShaderNoMip( "gfx/effects/sabers/black_glow" );
	cgs.media.blackSaberCoreShader		= trap_R_RegisterShaderNoMip( "gfx/effects/sabers/blackcore" );
	if (!cgs.media.blackSaberCoreShader)
		cgs.media.blackSaberCoreShader	= trap_R_RegisterShaderNoMip( "gfx/effects/sabers/black_line" );
	cgs.media.blackBlurShader			= trap_R_RegisterShaderNoMip( "gfx/effects/sabers/blacktrail" );
	if (!cgs.media.blackBlurShader)
		cgs.media.blackBlurShader		= trap_R_RegisterShaderNoMip( "gfx/effects/sabers/blacksaberBlur" );
	//[/RGBSabers]	
	
	//[SFXSabers]
	cgs.media.sfxSaberBladeShader		= trap_R_RegisterShaderNoMip( "gfx/sfx_sabers/saber_blade" );
	cgs.media.sfxSaberBlade2Shader		= trap_R_RegisterShaderNoMip( "gfx/sfx_sabers/saber_blade_rgb" );
	cgs.media.sfxSaberEndShader			= trap_R_RegisterShaderNoMip( "gfx/sfx_sabers/saber_end" );
	cgs.media.sfxSaberEnd2Shader		= trap_R_RegisterShaderNoMip( "gfx/sfx_sabers/saber_end_rgb" );
	cgs.media.sfxSaberTrailShader		= trap_R_RegisterShaderNoMip( "gfx/sfx_sabers/saber_trail" );
	//[/SFXSabers]

	cgs.media.saberBlurShader			= trap_R_RegisterShader( "gfx/effects/sabers/saberBlur" );
	cgs.media.swordTrailShader			= trap_R_RegisterShader( "gfx/effects/sabers/swordTrail" );

	cgs.media.forceCoronaShader			= trap_R_RegisterShaderNoMip( "gfx/hud/force_swirl" );

	cgs.media.yellowDroppedSaberShader	= trap_R_RegisterShader("gfx/effects/yellow_glow");

	cgs.media.rivetMarkShader			= trap_R_RegisterShader( "gfx/damage/rivetmark" );

	cgs.media.saberFlare				= trap_R_RegisterShader( "gfx/effects/saberFlare" );

	trap_R_RegisterShader( "powerups/ysalimarishell" );
	
	trap_R_RegisterShader( "gfx/effects/forcePush" );

	trap_R_RegisterShader( "gfx/misc/red_dmgshield" );
	trap_R_RegisterShader( "gfx/misc/red_portashield" );
	trap_R_RegisterShader( "gfx/misc/blue_dmgshield" );
	trap_R_RegisterShader( "gfx/misc/blue_portashield" );

	trap_R_RegisterShader( "models/map_objects/imp_mine/turret_chair_dmg.tga" );

	for (i=1 ; i<9 ; i++)
	{
		trap_S_RegisterSound(va("sound/weapons/saber/saberhup%i.wav", i));
	}

	for (i=1 ; i<10 ; i++)
	{
		trap_S_RegisterSound(va("sound/weapons/saber/saberblock%i.wav", i));
	}

	for (i=1 ; i<4 ; i++)
	{
		trap_S_RegisterSound(va("sound/weapons/saber/bounce%i.wav", i));
	}

	trap_S_RegisterSound( "sound/weapons/saber/enemy_saber_on.wav" );
	trap_S_RegisterSound( "sound/weapons/saber/enemy_saber_off.wav" );

	trap_S_RegisterSound( "sound/weapons/saber/saberhum1.wav" );
	trap_S_RegisterSound( "sound/weapons/saber/saberon.wav" );
	trap_S_RegisterSound( "sound/weapons/saber/saberoffquick.wav" );
	trap_S_RegisterSound( "sound/weapons/saber/saberhitwall1.mp3" );
	trap_S_RegisterSound( "sound/weapons/saber/saberhitwall2.mp3" );
	trap_S_RegisterSound( "sound/weapons/saber/saberhitwall3.mp3" );
	trap_S_RegisterSound("sound/weapons/saber/saberhit.wav");
	trap_S_RegisterSound("sound/weapons/saber/saberhit1.wav");
	trap_S_RegisterSound("sound/weapons/saber/saberhit2.wav");
	trap_S_RegisterSound("sound/weapons/saber/saberhit3.wav");

	trap_S_RegisterSound("sound/weapons/saber/saber_catch.wav");

	cgs.media.teamHealSound = trap_S_RegisterSound("sound/weapons/force/teamheal.wav");
	cgs.media.teamRegenSound = trap_S_RegisterSound("sound/weapons/force/teamforce.wav");

	trap_S_RegisterSound("sound/weapons/force/heal.wav");
	trap_S_RegisterSound("sound/weapons/force/speed.wav");
	trap_S_RegisterSound("sound/weapons/force/see.wav");
	trap_S_RegisterSound("sound/weapons/force/rage.wav");
	trap_S_RegisterSound("sound/weapons/force/lightning.mp3");
	trap_S_RegisterSound("sound/weapons/force/lightninghit1.mp3");
	trap_S_RegisterSound("sound/weapons/force/lightninghit2.mp3");
	trap_S_RegisterSound("sound/weapons/force/lightninghit3.mp3");
	trap_S_RegisterSound("sound/weapons/force/drain.wav");
	trap_S_RegisterSound("sound/weapons/force/jumpbuild.wav");
	trap_S_RegisterSound("sound/weapons/force/distract.wav");
	trap_S_RegisterSound("sound/weapons/force/distractstop.wav");
	trap_S_RegisterSound("sound/weapons/force/pull.wav");
	trap_S_RegisterSound("sound/weapons/force/push.wav");

	for (i=1 ; i<3 ; i++)
	{
		trap_S_RegisterSound(va("sound/weapons/thermal/bounce%i.wav", i));
	}

	trap_S_RegisterSound("sound/movers/switches/switch2.wav");
	trap_S_RegisterSound("sound/movers/switches/switch3.wav");
	trap_S_RegisterSound("sound/ambience/spark5.wav");
	trap_S_RegisterSound("sound/chars/turret/ping.wav");
	trap_S_RegisterSound("sound/chars/turret/startup.wav");
	trap_S_RegisterSound("sound/chars/turret/shutdown.wav");
	trap_S_RegisterSound("sound/chars/turret/move.wav");
	trap_S_RegisterSound("sound/player/pickuphealth.wav");
	trap_S_RegisterSound("sound/player/pickupshield.wav");

	trap_S_RegisterSound("sound/effects/glassbreak1.wav");

	trap_S_RegisterSound( "sound/weapons/rocket/tick.wav" );
	trap_S_RegisterSound( "sound/weapons/rocket/lock.wav" );
	
	cgs.media.speedLoopSound = trap_S_RegisterSound("sound/weapons/force/speedloop.wav");
	cgs.media.protectLoopSound = trap_S_RegisterSound("sound/weapons/force/protectloop.wav");
	cgs.media.absorbLoopSound = trap_S_RegisterSound("sound/weapons/force/absorbloop.wav");
	cgs.media.rageLoopSound = trap_S_RegisterSound("sound/weapons/force/rageloop.wav");
	cgs.media.seeLoopSound = trap_S_RegisterSound("sound/weapons/force/seeloop.wav");

	trap_S_RegisterSound("sound/weapons/force/protecthit.mp3"); //PDSOUND_PROTECTHIT
	trap_S_RegisterSound("sound/weapons/force/protect.mp3"); //PDSOUND_PROTECT
	trap_S_RegisterSound("sound/weapons/force/absorbhit.mp3"); //PDSOUND_ABSORBHIT
	trap_S_RegisterSound("sound/weapons/force/absorb.mp3"); //PDSOUND_ABSORB
	trap_S_RegisterSound("sound/weapons/force/jump.mp3"); //PDSOUND_FORCEJUMP
	trap_S_RegisterSound("sound/weapons/force/grip.mp3"); //PDSOUND_FORCEGRIP

	if ( cgs.gametype >= GT_TEAM || com_buildScript.integer ) {

#ifdef JK2AWARDS
		cgs.media.captureAwardSound = trap_S_RegisterSound( "sound/chars/protocol/misc/capture.wav" );
#endif
		cgs.media.redLeadsSound = trap_S_RegisterSound( "sound/chars/protocol/misc/40MOM046.wav");
		cgs.media.blueLeadsSound = trap_S_RegisterSound( "sound/chars/protocol/misc/40MOM045.wav");
		cgs.media.teamsTiedSound = trap_S_RegisterSound( "sound/chars/protocol/misc/40MOM032.wav" );

		cgs.media.redScoredSound = trap_S_RegisterSound( "sound/chars/protocol/misc/40MOM044.wav");
		cgs.media.blueScoredSound = trap_S_RegisterSound( "sound/chars/protocol/misc/40MOM043.wav" );

		if ( cgs.gametype == GT_CTF || com_buildScript.integer ) {
			cgs.media.redFlagReturnedSound = trap_S_RegisterSound( "sound/chars/protocol/misc/40MOM042.wav" );
			cgs.media.blueFlagReturnedSound = trap_S_RegisterSound( "sound/chars/protocol/misc/40MOM041.wav" );
			cgs.media.redTookFlagSound = trap_S_RegisterSound( "sound/chars/protocol/misc/40MOM040.wav" );
			cgs.media.blueTookFlagSound = trap_S_RegisterSound( "sound/chars/protocol/misc/40MOM039.wav" );
		}
		if ( cgs.gametype == GT_CTY /*|| com_buildScript.integer*/ ) {	//we don't hav them in JA assets
			cgs.media.redYsalReturnedSound = trap_S_RegisterSound( "sound/chars/protocol/misc/40MOM050.wav" );
			cgs.media.blueYsalReturnedSound = trap_S_RegisterSound( "sound/chars/protocol/misc/40MOM049.wav" );
			cgs.media.redTookYsalSound = trap_S_RegisterSound( "sound/chars/protocol/misc/40MOM048.wav" );
			cgs.media.blueTookYsalSound = trap_S_RegisterSound( "sound/chars/protocol/misc/40MOM047.wav" );
		}
	}

	cgs.media.drainSound = trap_S_RegisterSound("sound/weapons/force/drained.mp3");

	cgs.media.happyMusic = trap_S_RegisterSound("music/goodsmall.mp3");
	cgs.media.dramaticFailure = trap_S_RegisterSound("music/badsmall.mp3");

	//PRECACHE ALL MUSIC HERE (don't need to precache normally because it's streamed off the disk)
	if (com_buildScript.integer) {
		trap_S_StartBackgroundTrack( "music/mp/duel.mp3", "music/mp/duel.mp3", qfalse );
	}

	cg.loadLCARSStage = 1;

	cgs.media.selectSound = trap_S_RegisterSound( "sound/weapons/change.wav" );

	cgs.media.teleInSound = trap_S_RegisterSound( "sound/player/telein.wav" );
	cgs.media.teleOutSound = trap_S_RegisterSound( "sound/player/teleout.wav" );
	cgs.media.respawnSound = trap_S_RegisterSound( "sound/items/respawn1.wav" );

	trap_S_RegisterSound( "sound/movers/objects/objectHit.wav" );

	cgs.media.talkSound = trap_S_RegisterSound( "sound/player/talk.wav" );
	cgs.media.landSound = trap_S_RegisterSound( "sound/player/land1.wav");
	cgs.media.fallSound = trap_S_RegisterSound( "sound/player/fallsplat.wav");

	cgs.media.crackleSound = trap_S_RegisterSound( "sound/effects/energy_crackle.wav" );

//JAPRO - Clientside - Hitsounds Start
	cgs.media.hitSound			= trap_S_RegisterSound( "sound/effects/hitsound.wav" ); 
	cgs.media.hitSound2			= trap_S_RegisterSound( "sound/effects/hitsound2.wav" );
	cgs.media.hitSound3			= trap_S_RegisterSound( "sound/effects/hitsound3.wav" );
	cgs.media.hitSound4			= trap_S_RegisterSound( "sound/effects/hitsound4.wav" );
	cgs.media.hitTeamSound		= trap_S_RegisterSound( "sound/effects/hitsoundteam.wav" );

/*	cgs.media.gibSound			= trap_S_RegisterSound( "sound/player/gibsplt1.wav" );
	cgs.media.gibBounce1Sound	= trap_S_RegisterSound( "sound/player/gibimp1.wav" );
	cgs.media.gibBounce2Sound	= trap_S_RegisterSound( "sound/player/gibimp2.wav" );
	cgs.media.gibBounce3Sound	= trap_S_RegisterSound( "sound/player/gibimp3.wav" );*/
//JAPRO - Clientside - Hitsounds End

#ifdef JK2AWARDS
	cgs.media.firstImpressiveSound = trap_S_RegisterSound( "sound/chars/protocol/misc/first_impressive.wav" );
	cgs.media.impressiveSound = trap_S_RegisterSound( "sound/chars/protocol/misc/impressive.wav" );
	cgs.media.firstExcellentSound = trap_S_RegisterSound( "sound/chars/protocol/misc/first_excellent.wav" );
	cgs.media.excellentSound = trap_S_RegisterSound( "sound/chars/protocol/misc/excellent.wav" );
	cgs.media.firstHumiliationSound = trap_S_RegisterSound( "sound/chars/protocol/misc/first_gauntlet.wav" );
	cgs.media.humiliationSound = trap_S_RegisterSound( "sound/chars/protocol/misc/humiliation.wav" );
	cgs.media.deniedSound = trap_S_RegisterSound( "sound/chars/protocol/misc/denied.wav" );
	cgs.media.defendSound = trap_S_RegisterSound( "sound/chars/protocol/misc/defense.wav" );
	cgs.media.assistSound = trap_S_RegisterSound( "sound/chars/protocol/misc/assist.wav" );
#endif

	/*
	cgs.media.takenLeadSound = trap_S_RegisterSound( "sound/chars/protocol/misc/40MOM051");
	cgs.media.tiedLeadSound = trap_S_RegisterSound( "sound/chars/protocol/misc/40MOM032");
	cgs.media.lostLeadSound = trap_S_RegisterSound( "sound/chars/protocol/misc/40MOM052");
	*/

	cgs.media.rollSound					= trap_S_RegisterSound( "sound/player/roll1.wav");

	cgs.media.noforceSound				= trap_S_RegisterSound( "sound/weapons/force/noforce.mp3" );

	cgs.media.watrInSound				= trap_S_RegisterSound( "sound/player/watr_in.wav");
	cgs.media.watrOutSound				= trap_S_RegisterSound( "sound/player/watr_out.wav");
	cgs.media.watrUnSound				= trap_S_RegisterSound( "sound/player/watr_un.wav");

	cgs.media.explosionModel			= trap_R_RegisterModel ( "models/map_objects/mp/sphere.md3" );
	cgs.media.surfaceExplosionShader	= trap_R_RegisterShader( "surfaceExplosion" );

	cgs.media.disruptorShader			= trap_R_RegisterShader( "gfx/effects/burn");

	if (com_buildScript.integer)
	{
		trap_R_RegisterShader( "gfx/effects/turretflashdie" );
	}

	cgs.media.solidWhite = trap_R_RegisterShader( "gfx/effects/solidWhite_cull" );

	trap_R_RegisterShader("gfx/misc/mp_light_enlight_disable");
	trap_R_RegisterShader("gfx/misc/mp_dark_enlight_disable");

	trap_R_RegisterModel ( "models/map_objects/mp/sphere.md3" );
	trap_R_RegisterModel("models/items/remote.md3");

	cgs.media.holocronPickup = trap_S_RegisterSound( "sound/player/holocron.wav" );

	// Zoom
	cgs.media.zoomStart = trap_S_RegisterSound( "sound/interface/zoomstart.wav" );
	cgs.media.zoomLoop	= trap_S_RegisterSound( "sound/interface/zoomloop.wav" );
	cgs.media.zoomEnd	= trap_S_RegisterSound( "sound/interface/zoomend.wav" );

	for (i=0 ; i<4 ; i++) {
		Com_sprintf (name, sizeof(name), "sound/player/footsteps/stone_step%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_STONEWALK][i] = trap_S_RegisterSound (name);
		Com_sprintf (name, sizeof(name), "sound/player/footsteps/stone_run%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_STONERUN][i] = trap_S_RegisterSound (name);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/metal_step%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_METALWALK][i] = trap_S_RegisterSound (name);
		Com_sprintf (name, sizeof(name), "sound/player/footsteps/metal_run%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_METALRUN][i] = trap_S_RegisterSound (name);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/pipe_step%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_PIPEWALK][i] = trap_S_RegisterSound (name);
		Com_sprintf (name, sizeof(name), "sound/player/footsteps/pipe_run%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_PIPERUN][i] = trap_S_RegisterSound (name);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/water_run%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_SPLASH][i] = trap_S_RegisterSound (name);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/water_walk%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_WADE][i] = trap_S_RegisterSound (name);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/water_wade_0%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_SWIM][i] = trap_S_RegisterSound (name);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/snow_step%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_SNOWWALK][i] = trap_S_RegisterSound (name);
		Com_sprintf (name, sizeof(name), "sound/player/footsteps/snow_run%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_SNOWRUN][i] = trap_S_RegisterSound (name);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/sand_walk%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_SANDWALK][i] = trap_S_RegisterSound (name);
		Com_sprintf (name, sizeof(name), "sound/player/footsteps/sand_run%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_SANDRUN][i] = trap_S_RegisterSound (name);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/grass_step%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_GRASSWALK][i] = trap_S_RegisterSound (name);
		Com_sprintf (name, sizeof(name), "sound/player/footsteps/grass_run%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_GRASSRUN][i] = trap_S_RegisterSound (name);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/dirt_step%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_DIRTWALK][i] = trap_S_RegisterSound (name);
		Com_sprintf (name, sizeof(name), "sound/player/footsteps/dirt_run%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_DIRTRUN][i] = trap_S_RegisterSound (name);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/mud_walk%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_MUDWALK][i] = trap_S_RegisterSound (name);
		Com_sprintf (name, sizeof(name), "sound/player/footsteps/mud_run%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_MUDRUN][i] = trap_S_RegisterSound (name);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/gravel_walk%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_GRAVELWALK][i] = trap_S_RegisterSound (name);
		Com_sprintf (name, sizeof(name), "sound/player/footsteps/gravel_run%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_GRAVELRUN][i] = trap_S_RegisterSound (name);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/rug_step%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_RUGWALK][i] = trap_S_RegisterSound (name);
		Com_sprintf (name, sizeof(name), "sound/player/footsteps/rug_run%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_RUGRUN][i] = trap_S_RegisterSound (name);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/wood_walk%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_WOODWALK][i] = trap_S_RegisterSound (name);
		Com_sprintf (name, sizeof(name), "sound/player/footsteps/wood_run%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_WOODRUN][i] = trap_S_RegisterSound (name);
	}

	// only register the items that the server says we need
	//Raz: Fixed buffer overflow
	Q_strncpyz(items, CG_ConfigString(CS_ITEMS), sizeof(items));

	for ( i = 1 ; i < bg_numItems ; i++ ) {
		if ( items[ i ] == '1' || com_buildScript.integer ) {
			CG_RegisterItemSounds( i );
		}
	}

	for ( i = 1 ; i < MAX_SOUNDS ; i++ ) {
		soundName = CG_ConfigString( CS_SOUNDS+i );
		if ( !soundName[0] ) {
			break;
		}
		if ( soundName[0] == '*' )
		{
			if (soundName[1] == '$')
			{ //an NPC soundset
				CG_PrecacheNPCSounds(soundName);
			}
			continue;	// custom sound
		}
		cgs.gameSounds[i] = trap_S_RegisterSound( soundName );
	}

	for ( i = 1 ; i < MAX_FX ; i++ ) {
		soundName = CG_ConfigString( CS_EFFECTS+i );
		if ( !soundName[0] ) {
			break;
		}

		if (soundName[0] == '*')
		{ //it's a special global weather effect
			CG_ParseWeatherEffect(soundName);
			cgs.gameEffects[i] = 0;
		}
		else
		{
			cgs.gameEffects[i] = trap_FX_RegisterEffect( soundName );
		}
	}

	// register all the server specified icons
	for ( i = 1; i < MAX_ICONS; i ++ )
	{
		const char* iconName;

		iconName = CG_ConfigString ( CS_ICONS + i );
		if ( !iconName[0] )
		{
			break;
		}

		cgs.gameIcons[i] = trap_R_RegisterShaderNoMip ( iconName );
	}

	soundName = CG_ConfigString(CS_SIEGE_STATE);

	if (soundName[0])
	{
		CG_ParseSiegeState(soundName);
	}

	soundName = CG_ConfigString(CS_SIEGE_WINTEAM);

	if (soundName[0])
	{
		cg_siegeWinTeam = atoi(soundName);
	}

	if (cgs.gametype == GT_SIEGE)
	{
		CG_ParseSiegeObjectiveStatus(CG_ConfigString(CS_SIEGE_OBJECTIVES));
		cg_beatingSiegeTime = atoi(CG_ConfigString(CS_SIEGE_TIMEOVERRIDE));
		if ( cg_beatingSiegeTime )
		{
			CG_SetSiegeTimerCvar ( cg_beatingSiegeTime );
		}
	}

	cg.loadLCARSStage = 2;

	// FIXME: only needed with item
	cgs.media.deploySeeker = trap_S_RegisterSound ("sound/chars/seeker/misc/hiss.wav");
	cgs.media.medkitSound = trap_S_RegisterSound ("sound/items/use_bacta.wav");
	
	cgs.media.winnerSound = trap_S_RegisterSound( "sound/chars/protocol/misc/40MOM006.wav" );
	cgs.media.loserSound = trap_S_RegisterSound( "sound/chars/protocol/misc/40MOM010.wav" );
}


//-------------------------------------
// CG_RegisterEffects
// 
// Handles precaching all effect files
//	and any shader, model, or sound
//	files an effect may use.
//-------------------------------------
static void CG_RegisterEffects( void )
{
	/*
	const char	*effectName;
	int			i;

	for ( i = 1 ; i < MAX_FX ; i++ ) 
	{
		effectName = CG_ConfigString( CS_EFFECTS + i );

		if ( !effectName[0] ) 
		{
			break;
		}

		trap_FX_RegisterEffect( effectName );
	}
	*/
	//the above was redundant as it's being done in CG_RegisterSounds

	// Set up the glass effects mini-system.
	CG_InitGlass();

	//footstep effects
	cgs.effects.footstepMud = trap_FX_RegisterEffect( "materials/mud" );
	cgs.effects.footstepSand = trap_FX_RegisterEffect( "materials/sand" );
	cgs.effects.footstepSnow = trap_FX_RegisterEffect( "materials/snow" );
	cgs.effects.footstepGravel = trap_FX_RegisterEffect( "materials/gravel" );
	//landing effects
	cgs.effects.landingMud = trap_FX_RegisterEffect( "materials/mud_large" );
	cgs.effects.landingSand = trap_FX_RegisterEffect( "materials/sand_large" );
	cgs.effects.landingDirt = trap_FX_RegisterEffect( "materials/dirt_large" );
	cgs.effects.landingSnow = trap_FX_RegisterEffect( "materials/snow_large" );
	cgs.effects.landingGravel = trap_FX_RegisterEffect( "materials/gravel_large" );
	//splashes
	cgs.effects.waterSplash = trap_FX_RegisterEffect( "env/water_impact" );
	cgs.effects.lavaSplash = trap_FX_RegisterEffect( "env/lava_splash" );
	cgs.effects.acidSplash = trap_FX_RegisterEffect( "env/acid_splash" );
}

//===================================================================================

extern char *forceHolocronModels[];
int CG_HandleAppendedSkin(char *modelName);
void CG_CacheG2AnimInfo(char *modelName);
/*
=================
CG_RegisterGraphics

This function may execute for a couple of minutes with a slow disk.
=================
*/
static void CG_RegisterGraphics( void ) {
	int			i;
	int			breakPoint;
	char		items[MAX_ITEMS+1];
	const char	*terrainInfo;
	int			terrainID;

	static char		*sb_nums[11] = {
		"gfx/2d/numbers/zero",
		"gfx/2d/numbers/one",
		"gfx/2d/numbers/two",
		"gfx/2d/numbers/three",
		"gfx/2d/numbers/four",
		"gfx/2d/numbers/five",
		"gfx/2d/numbers/six",
		"gfx/2d/numbers/seven",
		"gfx/2d/numbers/eight",
		"gfx/2d/numbers/nine",
		"gfx/2d/numbers/minus",
	};

	static char		*sb_t_nums[11] = {
		"gfx/2d/numbers/t_zero",
		"gfx/2d/numbers/t_one",
		"gfx/2d/numbers/t_two",
		"gfx/2d/numbers/t_three",
		"gfx/2d/numbers/t_four",
		"gfx/2d/numbers/t_five",
		"gfx/2d/numbers/t_six",
		"gfx/2d/numbers/t_seven",
		"gfx/2d/numbers/t_eight",
		"gfx/2d/numbers/t_nine",
		"gfx/2d/numbers/t_minus",
	};

	static char		*sb_c_nums[11] = {
		"gfx/2d/numbers/c_zero",
		"gfx/2d/numbers/c_one",
		"gfx/2d/numbers/c_two",
		"gfx/2d/numbers/c_three",
		"gfx/2d/numbers/c_four",
		"gfx/2d/numbers/c_five",
		"gfx/2d/numbers/c_six",
		"gfx/2d/numbers/c_seven",
		"gfx/2d/numbers/c_eight",
		"gfx/2d/numbers/c_nine",
		"gfx/2d/numbers/t_minus", //?????
	};

	// clear any references to old media
	memset( &cg.refdef, 0, sizeof( cg.refdef ) );
	trap_R_ClearScene();

	CG_LoadingString( cgs.mapname );        

	trap_R_LoadWorldMap( cgs.mapname );

	// precache status bar pics
//	CG_LoadingString( "game media" );

	for ( i=0 ; i<11 ; i++) {
		cgs.media.numberShaders[i] = trap_R_RegisterShader( sb_nums[i] );
	}

	cg.loadLCARSStage = 3;

	for ( i=0; i < 11; i++ )
	{
		cgs.media.numberShaders[i]			= trap_R_RegisterShaderNoMip( sb_nums[i] );
		cgs.media.smallnumberShaders[i]		= trap_R_RegisterShaderNoMip( sb_t_nums[i] );
		cgs.media.chunkyNumberShaders[i]	= trap_R_RegisterShaderNoMip( sb_c_nums[i] );
	}

	trap_R_RegisterShaderNoMip ( "gfx/mp/pduel_icon_lone" );
	trap_R_RegisterShaderNoMip ( "gfx/mp/pduel_icon_double" );

	cgs.media.balloonShader = trap_R_RegisterShader( "gfx/mp/chat_icon" );
	cgs.media.vchatShader = trap_R_RegisterShader( "gfx/mp/vchat_icon" );

	cgs.media.deferShader = trap_R_RegisterShaderNoMip( "gfx/2d/defer.tga" );

	cgs.media.radarShader			= trap_R_RegisterShaderNoMip ( "gfx/menus/radar/radar.png" );
	cgs.media.siegeItemShader		= trap_R_RegisterShaderNoMip ( "gfx/menus/radar/goalitem" );
	cgs.media.mAutomapPlayerIcon	= trap_R_RegisterShader( "gfx/menus/radar/arrow_w" );
	cgs.media.mAutomapRocketIcon	= trap_R_RegisterShader( "gfx/menus/radar/rocket" );

	cgs.media.wireframeAutomapFrame_left = trap_R_RegisterShader( "gfx/mp_automap/mpauto_frame_left" );
	cgs.media.wireframeAutomapFrame_right = trap_R_RegisterShader( "gfx/mp_automap/mpauto_frame_right" );
	cgs.media.wireframeAutomapFrame_top = trap_R_RegisterShader( "gfx/mp_automap/mpauto_frame_top" );
	cgs.media.wireframeAutomapFrame_bottom = trap_R_RegisterShader( "gfx/mp_automap/mpauto_frame_bottom" );

	cgs.media.lagometerShader = trap_R_RegisterShaderNoMip("gfx/2d/lag" );
	cgs.media.connectionShader = trap_R_RegisterShaderNoMip( "gfx/2d/net" );

	trap_FX_InitSystem(&cg.refdef);
	CG_RegisterEffects();

	cgs.media.boltShader = trap_R_RegisterShader( "gfx/misc/blueLine" );

	cgs.effects.turretShotEffect = trap_FX_RegisterEffect( "turret/shot" );
	cgs.effects.mEmplacedDeadSmoke = trap_FX_RegisterEffect("emplaced/dead_smoke.efx");
	cgs.effects.mEmplacedExplode = trap_FX_RegisterEffect("emplaced/explode.efx");
	cgs.effects.mTurretExplode = trap_FX_RegisterEffect("turret/explode.efx");
	cgs.effects.mSparkExplosion = trap_FX_RegisterEffect("sparks/spark_explosion.efx");
	cgs.effects.mTripmineExplosion = trap_FX_RegisterEffect("tripMine/explosion.efx");
	cgs.effects.mDetpackExplosion = trap_FX_RegisterEffect("detpack/explosion.efx");
	cgs.effects.mFlechetteAltBlow = trap_FX_RegisterEffect("flechette/alt_blow.efx");
	cgs.effects.mStunBatonFleshImpact = trap_FX_RegisterEffect("stunBaton/flesh_impact.efx");
	cgs.effects.mAltDetonate = trap_FX_RegisterEffect("demp2/altDetonate.efx");
	cgs.effects.mSparksExplodeNoSound = trap_FX_RegisterEffect("sparks/spark_exp_nosnd");
	cgs.effects.mTripMineLaster = trap_FX_RegisterEffect("tripMine/laser.efx");
	cgs.effects.mEmplacedMuzzleFlash = trap_FX_RegisterEffect( "effects/emplaced/muzzle_flash" );
	cgs.effects.mConcussionAltRing = trap_FX_RegisterEffect("concussion/alt_ring");

	cgs.effects.mHyperspaceStars = trap_FX_RegisterEffect("ships/hyperspace_stars");
	cgs.effects.mBlackSmoke = trap_FX_RegisterEffect( "volumetric/black_smoke" );
	cgs.effects.mShipDestDestroyed = trap_FX_RegisterEffect("effects/ships/dest_destroyed.efx");
	cgs.effects.mShipDestBurning = trap_FX_RegisterEffect("effects/ships/dest_burning.efx");
	cgs.effects.mBobaJet = trap_FX_RegisterEffect("effects/boba/jet.efx");


	cgs.effects.itemCone = trap_FX_RegisterEffect("mp/itemcone.efx");
	cgs.effects.mTurretMuzzleFlash = trap_FX_RegisterEffect("effects/turret/muzzle_flash.efx");
	cgs.effects.mSparks = trap_FX_RegisterEffect("sparks/spark_nosnd.efx"); //sparks/spark.efx
	cgs.effects.mSaberCut = trap_FX_RegisterEffect("saber/saber_cut.efx");
	cgs.effects.mSaberBlock = trap_FX_RegisterEffect("saber/saber_block.efx");
	cgs.effects.mSaberBloodSparks = trap_FX_RegisterEffect("saber/blood_sparks_mp.efx");
	cgs.effects.mSaberBloodSparksSmall = trap_FX_RegisterEffect("saber/blood_sparks_25_mp.efx");
	cgs.effects.mSaberBloodSparksMid = trap_FX_RegisterEffect("saber/blood_sparks_50_mp.efx");
	cgs.effects.mSpawn = trap_FX_RegisterEffect("mp/spawn.efx");
	cgs.effects.mJediSpawn = trap_FX_RegisterEffect("mp/jedispawn.efx");
	cgs.effects.mBlasterDeflect = trap_FX_RegisterEffect("blaster/deflect.efx");
	cgs.effects.mBlasterSmoke = trap_FX_RegisterEffect("blaster/smoke_bolton");
	cgs.effects.mForceConfustionOld = trap_FX_RegisterEffect("force/confusion_old.efx");

	cgs.effects.forceLightning		= trap_FX_RegisterEffect( "effects/force/lightning.efx" );
	cgs.effects.forceLightningWide	= trap_FX_RegisterEffect( "effects/force/lightningwide.efx" );
	cgs.effects.forceDrain		= trap_FX_RegisterEffect( "effects/mp/drain.efx" );
	cgs.effects.forceDrainWide	= trap_FX_RegisterEffect( "effects/mp/drainwide.efx" );
	cgs.effects.forceDrained	= trap_FX_RegisterEffect( "effects/mp/drainhit.efx");

	cgs.effects.mDisruptorDeathSmoke = trap_FX_RegisterEffect("disruptor/death_smoke");

	//japro grapple
	if (cg.japro.detected) {
		cgs.effects.grappleHitWall = trap_FX_RegisterEffect("effects/grapple/hit_wall.efx");
		cgs.effects.grappleHitWall = trap_FX_RegisterEffect("effects/grapple/hit_player.efx");
		cgs.media.grappleModel = trap_R_RegisterModel("models/items/grapple.md3");//Grapple model
	}


	for ( i = 0 ; i < NUM_CROSSHAIRS ; i++ ) {
		cgs.media.crosshairShader[i] = trap_R_RegisterShaderNoMip( va("gfx/2d/crosshair%c", 'a'+i) );
	}

	cg.loadLCARSStage = 4;

	cgs.media.backTileShader = trap_R_RegisterShader( "gfx/2d/backtile" );

	//precache the fpls skin
	//trap_R_RegisterSkin("models/players/kyle/model_fpls2.skin");

	cgs.media.itemRespawningPlaceholder = trap_R_RegisterShader("powerups/placeholder");
	cgs.media.itemRespawningRezOut = trap_R_RegisterShader("powerups/rezout");

	cgs.media.playerShieldDamage = trap_R_RegisterShader("gfx/misc/personalshield");
	cgs.media.protectShader = trap_R_RegisterShader("gfx/misc/forceprotect");
	cgs.media.forceSightBubble = trap_R_RegisterShader("gfx/misc/sightbubble");
	cgs.media.forceShell = trap_R_RegisterShader("powerups/forceshell");
	cgs.media.sightShell = trap_R_RegisterShader("powerups/sightshell");

	cgs.media.itemHoloModel = trap_R_RegisterModel("models/map_objects/mp/holo.md3");

	if (cgs.gametype == GT_HOLOCRON || com_buildScript.integer)
	{
		for ( i=0; i < NUM_FORCE_POWERS; i++ )
		{
			if (forceHolocronModels[i] &&
				forceHolocronModels[i][0])
			{
				trap_R_RegisterModel(forceHolocronModels[i]);
			}
		}
	}

	if ( cgs.gametype == GT_CTF || cgs.gametype == GT_CTY || com_buildScript.integer ) {
		if (com_buildScript.integer)
		{
			trap_R_RegisterModel( "models/flags/r_flag.md3" );
			trap_R_RegisterModel( "models/flags/b_flag.md3" );
			trap_R_RegisterModel( "models/flags/r_flag_ysal.md3" );
			trap_R_RegisterModel( "models/flags/b_flag_ysal.md3" );
		}

		if (cgs.gametype == GT_CTF)
		{
			cgs.media.redFlagModel = trap_R_RegisterModel( "models/flags/r_flag.md3" );
			cgs.media.blueFlagModel = trap_R_RegisterModel( "models/flags/b_flag.md3" );
		}
		else
		{
			cgs.media.redFlagModel = trap_R_RegisterModel( "models/flags/r_flag_ysal.md3" );
			cgs.media.blueFlagModel = trap_R_RegisterModel( "models/flags/b_flag_ysal.md3" );
		}

		cgs.media.simpleFlagRed = trap_R_RegisterShaderNoMip( "models/flags/rflag" );
		cgs.media.simpleFlagBlue = trap_R_RegisterShaderNoMip( "models/flags/bflag" );
		cgs.media.simpleFlagModelRed = trap_R_RegisterModel( "models/flags/rflag.md3" );
		cgs.media.simpleFlagModelBlue = trap_R_RegisterModel( "models/flags/bflag.md3" );

		trap_R_RegisterShaderNoMip( "gfx/hud/mpi_rflag_x" );
		trap_R_RegisterShaderNoMip( "gfx/hud/mpi_bflag_x" );

		trap_R_RegisterShaderNoMip( "gfx/hud/mpi_rflag_ys" );
		trap_R_RegisterShaderNoMip( "gfx/hud/mpi_bflag_ys" );

		trap_R_RegisterShaderNoMip( "gfx/hud/mpi_rflag" );
		trap_R_RegisterShaderNoMip( "gfx/hud/mpi_bflag" );

		trap_R_RegisterShaderNoMip("gfx/2d/net.tga");

		cgs.media.flagPoleModel = trap_R_RegisterModel( "models/flag2/flagpole.md3" );
		cgs.media.flagFlapModel = trap_R_RegisterModel( "models/flag2/flagflap3.md3" );

		cgs.media.redFlagBaseModel = trap_R_RegisterModel( "models/mapobjects/flagbase/red_base.md3" );
		cgs.media.blueFlagBaseModel = trap_R_RegisterModel( "models/mapobjects/flagbase/blue_base.md3" );
		cgs.media.neutralFlagBaseModel = trap_R_RegisterModel( "models/mapobjects/flagbase/ntrl_base.md3" );
	}

	if ( cgs.gametype >= GT_TEAM || com_buildScript.integer ) {
		cgs.media.teamRedShader = trap_R_RegisterShader( "sprites/team_red" );
		cgs.media.teamBlueShader = trap_R_RegisterShader( "sprites/team_blue" );
		//cgs.media.redQuadShader = trap_R_RegisterShader("powerups/blueflag" );
		cgs.media.teamStatusBar = trap_R_RegisterShader( "gfx/2d/colorbar.tga" );
	}
	else if ( cgs.gametype == GT_JEDIMASTER )
	{
		cgs.media.teamRedShader = trap_R_RegisterShader( "sprites/team_red" );
	}

	if (cgs.gametype == GT_POWERDUEL || com_buildScript.integer)
	{
		cgs.media.powerDuelAllyShader = trap_R_RegisterShader("gfx/mp/pduel_icon_double");//trap_R_RegisterShader("gfx/mp/pduel_gameicon_ally");
	}

	cgs.media.heartShader			= trap_R_RegisterShaderNoMip( "ui/assets/statusbar/selectedhealth.tga" );

	cgs.media.ysaliredShader		= trap_R_RegisterShader( "powerups/ysaliredshell");
	cgs.media.ysaliblueShader		= trap_R_RegisterShader( "powerups/ysaliblueshell");
	cgs.media.ysalimariShader		= trap_R_RegisterShader( "powerups/ysalimarishell");
	cgs.media.boonShader			= trap_R_RegisterShader( "powerups/boonshell");
	cgs.media.endarkenmentShader	= trap_R_RegisterShader( "powerups/endarkenmentshell");
	cgs.media.enlightenmentShader	= trap_R_RegisterShader( "powerups/enlightenmentshell");
	cgs.media.invulnerabilityShader = trap_R_RegisterShader( "powerups/invulnerabilityshell");

#ifdef JK2AWARDS
	cgs.media.medalImpressive		= trap_R_RegisterShaderNoMip( "medal_impressive" );
	cgs.media.medalExcellent		= trap_R_RegisterShaderNoMip( "medal_excellent" );
	cgs.media.medalGauntlet			= trap_R_RegisterShaderNoMip( "medal_gauntlet" );
	cgs.media.medalDefend			= trap_R_RegisterShaderNoMip( "medal_defend" );
	cgs.media.medalAssist			= trap_R_RegisterShaderNoMip( "medal_assist" );
	cgs.media.medalCapture			= trap_R_RegisterShaderNoMip( "medal_capture" );
#endif

	// Binocular interface
	cgs.media.binocularCircle		= trap_R_RegisterShader( "gfx/2d/binCircle" );
	cgs.media.binocularMask			= trap_R_RegisterShader( "gfx/2d/binMask" );
	cgs.media.binocularArrow		= trap_R_RegisterShader( "gfx/2d/binSideArrow" );
	cgs.media.binocularTri			= trap_R_RegisterShader( "gfx/2d/binTopTri" );
	cgs.media.binocularStatic		= trap_R_RegisterShader( "gfx/2d/binocularWindow" );
	cgs.media.binocularOverlay		= trap_R_RegisterShader( "gfx/2d/binocularNumOverlay" );

	cg.loadLCARSStage = 5;

	// Chunk models
	//FIXME: jfm:? bother to conditionally load these if an ent has this material type?
	for ( i = 0; i < NUM_CHUNK_MODELS; i++ )
	{
		cgs.media.chunkModels[CHUNK_METAL2][i]	= trap_R_RegisterModel( va( "models/chunks/metal/metal1_%i.md3", i+1 ) ); //_ /switched\ _
		cgs.media.chunkModels[CHUNK_METAL1][i]	= trap_R_RegisterModel( va( "models/chunks/metal/metal2_%i.md3", i+1 ) ); //  \switched/
		cgs.media.chunkModels[CHUNK_ROCK1][i]	= trap_R_RegisterModel( va( "models/chunks/rock/rock1_%i.md3", i+1 ) );
		cgs.media.chunkModels[CHUNK_ROCK2][i]	= trap_R_RegisterModel( va( "models/chunks/rock/rock2_%i.md3", i+1 ) );
		cgs.media.chunkModels[CHUNK_ROCK3][i]	= trap_R_RegisterModel( va( "models/chunks/rock/rock3_%i.md3", i+1 ) );
		cgs.media.chunkModels[CHUNK_CRATE1][i]	= trap_R_RegisterModel( va( "models/chunks/crate/crate1_%i.md3", i+1 ) );
		cgs.media.chunkModels[CHUNK_CRATE2][i]	= trap_R_RegisterModel( va( "models/chunks/crate/crate2_%i.md3", i+1 ) );
		cgs.media.chunkModels[CHUNK_WHITE_METAL][i]	= trap_R_RegisterModel( va( "models/chunks/metal/wmetal1_%i.md3", i+1 ) );
	}

	cgs.media.chunkSound			= trap_S_RegisterSound("sound/weapons/explosions/glasslcar.wav");
	cgs.media.grateSound			= trap_S_RegisterSound( "sound/effects/grate_destroy.mp3" );
	cgs.media.rockBreakSound		= trap_S_RegisterSound("sound/effects/wall_smash.mp3");
	cgs.media.rockBounceSound[0]	= trap_S_RegisterSound("sound/effects/stone_bounce.wav");
	cgs.media.rockBounceSound[1]	= trap_S_RegisterSound("sound/effects/stone_bounce2.wav");
	cgs.media.metalBounceSound[0]	= trap_S_RegisterSound("sound/effects/metal_bounce.mp3");
	cgs.media.metalBounceSound[1]	= trap_S_RegisterSound("sound/effects/metal_bounce2.mp3");
	cgs.media.glassChunkSound		= trap_S_RegisterSound("sound/weapons/explosions/glassbreak1.wav");
	cgs.media.crateBreakSound[0]	= trap_S_RegisterSound("sound/weapons/explosions/crateBust1.wav" );
	cgs.media.crateBreakSound[1]	= trap_S_RegisterSound("sound/weapons/explosions/crateBust2.wav" );

/*
Ghoul2 Insert Start
*/
	CG_InitItems();
/*
Ghoul2 Insert End
*/
	memset( cg_weapons, 0, sizeof( cg_weapons ) );

	// only register the items that the server says we need
	Q_strncpyz(items, CG_ConfigString(CS_ITEMS), sizeof(items));

	for ( i = 1 ; i < bg_numItems ; i++ ) {
		if ( items[ i ] == '1' || com_buildScript.integer ) {
			CG_LoadingItem( i );
			CG_RegisterItemVisuals( i );
		}
	}

	cg.loadLCARSStage = 6;

	cgs.media.glassShardShader	= trap_R_RegisterShader( "gfx/misc/test_crackle" );

	// doing one shader just makes it look like a shell.  By using two shaders with different bulge offsets and different texture scales, it has a much more chaotic look
	cgs.media.electricBodyShader			= trap_R_RegisterShader( "gfx/misc/electric" );
	cgs.media.electricBody2Shader			= trap_R_RegisterShader( "gfx/misc/fullbodyelectric2" );

	cgs.media.fsrMarkShader					= trap_R_RegisterShader( "footstep_r" );
	cgs.media.fslMarkShader					= trap_R_RegisterShader( "footstep_l" );
	cgs.media.fshrMarkShader				= trap_R_RegisterShader( "footstep_heavy_r" );
	cgs.media.fshlMarkShader				= trap_R_RegisterShader( "footstep_heavy_l" );

	cgs.media.refractionShader				= trap_R_RegisterShader("effects/refraction");

	cgs.media.cloakedShader					= trap_R_RegisterShader( "gfx/effects/cloakedShader" );

	// wall marks
	cgs.media.shadowMarkShader	= trap_R_RegisterShader( "markShadow" );
	cgs.media.wakeMarkShader	= trap_R_RegisterShader( "wake" );

	cgs.media.viewPainShader					= trap_R_RegisterShader( "gfx/misc/borgeyeflare" );
	cgs.media.viewPainShader_Shields			= trap_R_RegisterShader( "gfx/mp/dmgshader_shields" );
	cgs.media.viewPainShader_ShieldsAndHealth	= trap_R_RegisterShader( "gfx/mp/dmgshader_shieldsandhealth" );

    //jaPRO mod assets - start
	//japro cosmetics
	cgs.media.cosmetics.pumpkin = trap_R_RegisterModel("models/players/hats/pumpkin.md3");
	cgs.media.cosmetics.cap = trap_R_RegisterModel("models/players/hats/cap.md3");
	cgs.media.cosmetics.fedora = trap_R_RegisterModel("models/players/hats/fedora.md3");
	cgs.media.cosmetics.kringekap = trap_R_RegisterModel("models/players/hats/cringe.md3");
	cgs.media.cosmetics.sombrero = trap_R_RegisterModel("models/players/hats/sombrero.md3");
	cgs.media.cosmetics.tophat = trap_R_RegisterModel("models/players/hats/tophat.md3");
	cgs.media.cosmetics.mask = trap_R_RegisterModel("models/players/hats/mask.md3");
	cgs.media.cosmetics.gradcap = trap_R_RegisterModel("models/players/hats/gradcap.md3");
	cgs.media.cosmetics.fedora1 = trap_R_RegisterModel("models/players/hats/fedora1.md3");
	cgs.media.cosmetics.fedora2 = trap_R_RegisterModel("models/players/hats/fedora2.md3");
	cgs.media.cosmetics.fedora3 = trap_R_RegisterModel("models/players/hats/fedora3.md3");
	cgs.media.cosmetics.fedora4 = trap_R_RegisterModel("models/players/hats/fedora4.md3");
	cgs.media.cosmetics.headcrab = trap_R_RegisterModel("models/players/hats/headcrab.md3");
	cgs.media.cosmetics.vadercape = trap_R_RegisterModel("models/players/hats/vadercape.md3");
	cgs.media.cosmetics.yodacape = trap_R_RegisterModel("models/players/hats/yodacape.md3");
	cgs.media.cosmetics.horns = trap_R_RegisterModel("models/players/hats/horns.md3");
	cgs.media.cosmetics.metalhelm = trap_R_RegisterModel("models/players/hats/metalhelm.md3");
	cgs.media.cosmetics.afro = trap_R_RegisterModel("models/players/hats/afro.md3");
	cgs.media.cosmetics.ak47 = trap_R_RegisterModel("models/players/hats/ak47.md3");
	cgs.media.cosmetics.bucket = trap_R_RegisterModel("models/players/hats/bucket.md3");
	cgs.media.cosmetics.crowbar = trap_R_RegisterModel("models/players/hats/crowbar.md3");
	cgs.media.cosmetics.crown = trap_R_RegisterModel("models/players/hats/crown.md3");
	cgs.media.cosmetics.royalcape = trap_R_RegisterModel("models/players/hats/royalcape.md3");
	cgs.media.cosmetics.beard = trap_R_RegisterModel("models/players/hats/beard.md3");
	cgs.media.cosmetics.grogucape = trap_R_RegisterModel("models/players/hats/grogucape.md3");
	cgs.media.cosmetics.plaguemask = trap_R_RegisterModel("models/players/hats/plaguemask.md3");
	cgs.media.cosmetics.glasses = trap_R_RegisterModel("models/players/hats/glasses.md3");
	cgs.media.cosmetics.mario = trap_R_RegisterModel("models/players/hats/mario.md3");
	cgs.media.cosmetics.rpg = trap_R_RegisterModel("models/players/hats/rpg.md3");

    //Movement Keys - Start
    cgs.media.keyCrouchOffShader	= trap_R_RegisterShaderNoMip ( "gfx/hud/keys/crouch_off" );
    cgs.media.keyCrouchOnShader		= trap_R_RegisterShaderNoMip ( "gfx/hud/keys/crouch_on" );
    cgs.media.keyJumpOffShader		= trap_R_RegisterShaderNoMip ( "gfx/hud/keys/jump_off" );
    cgs.media.keyJumpOnShader		= trap_R_RegisterShaderNoMip ( "gfx/hud/keys/jump_on" );
    cgs.media.keyBackOffShader		= trap_R_RegisterShaderNoMip ( "gfx/hud/keys/back_off" );
    cgs.media.keyBackOnShader		= trap_R_RegisterShaderNoMip ( "gfx/hud/keys/back_on" );
    cgs.media.keyForwardOffShader	= trap_R_RegisterShaderNoMip ( "gfx/hud/keys/forward_off" );
    cgs.media.keyForwardOnShader	= trap_R_RegisterShaderNoMip ( "gfx/hud/keys/forward_on" );
    cgs.media.keyLeftOffShader		= trap_R_RegisterShaderNoMip ( "gfx/hud/keys/left_off" );
    cgs.media.keyLeftOnShader		= trap_R_RegisterShaderNoMip ( "gfx/hud/keys/left_on" );
    cgs.media.keyRightOffShader		= trap_R_RegisterShaderNoMip ( "gfx/hud/keys/right_off" );
    cgs.media.keyRightOnShader		= trap_R_RegisterShaderNoMip ( "gfx/hud/keys/right_on" );
    cgs.media.keyAttackOn	    	= trap_R_RegisterShaderNoMip ( "gfx/hud/keys/attack_on" );
    cgs.media.keyAttackOff		    = trap_R_RegisterShaderNoMip ( "gfx/hud/keys/attack_off" );
    cgs.media.keyAltOn	    	    = trap_R_RegisterShaderNoMip ( "gfx/hud/keys/alt_on" );
    cgs.media.keyAltOff		        = trap_R_RegisterShaderNoMip ( "gfx/hud/keys/alt_off" );


    //Movement Keys 2
    cgs.media.keyCrouchOnShader2	= trap_R_RegisterShaderNoMip ( "gfx/hud/keys/crouch_on2" );
    cgs.media.keyJumpOnShader2		= trap_R_RegisterShaderNoMip ( "gfx/hud/keys/jump_on2" );
    cgs.media.keyBackOnShader2		= trap_R_RegisterShaderNoMip ( "gfx/hud/keys/back_on2" );
    cgs.media.keyForwardOnShader2	= trap_R_RegisterShaderNoMip ( "gfx/hud/keys/forward_on2" );
    cgs.media.keyLeftOnShader2		= trap_R_RegisterShaderNoMip ( "gfx/hud/keys/left_on2" );
    cgs.media.keyRightOnShader2		= trap_R_RegisterShaderNoMip ( "gfx/hud/keys/right_on2" );
    cgs.media.keyAttackOn2	    	= trap_R_RegisterShaderNoMip ( "gfx/hud/keys/attack_on2" );
    cgs.media.keyAltOn2	    	    = trap_R_RegisterShaderNoMip ( "gfx/hud/keys/alt_on2" );

    //Movement Keys - End

	// register the inline models
	breakPoint = cgs.numInlineModels = trap_CM_NumInlineModels();
	for ( i = 1 ; i < cgs.numInlineModels ; i++ ) {
		char	name[10];
		vec3_t			mins, maxs;
		int				j;

		Com_sprintf( name, sizeof(name), "*%i", i );
		cgs.inlineDrawModel[i] = trap_R_RegisterModel( name );
		if (!cgs.inlineDrawModel[i])
		{
			breakPoint = i;
			break;
		}

		trap_R_ModelBounds( cgs.inlineDrawModel[i], mins, maxs );
		for ( j = 0 ; j < 3 ; j++ ) {
			cgs.inlineModelMidpoints[i][j] = mins[j] + 0.5 * ( maxs[j] - mins[j] );
		}
	}

	cg.loadLCARSStage = 7;

	// register all the server specified models
	for (i=1 ; i<MAX_MODELS ; i++) {
		const char		*cModelName;
		char modelName[MAX_QPATH];

		cModelName = CG_ConfigString( CS_MODELS+i );
		if ( !cModelName[0] ) {
			break;
		}

		strcpy(modelName, cModelName);
		if (strstr(modelName, ".glm") || modelName[0] == '$')
		{ //Check to see if it has a custom skin attached.
			CG_HandleAppendedSkin(modelName);
			CG_CacheG2AnimInfo(modelName);
		}

		if (modelName[0] != '$' && modelName[0] != '@')
		{ //don't register vehicle names and saber names as models.
			cgs.gameModels[i] = trap_R_RegisterModel( modelName );
		}
		else
		{//FIXME: register here so that stuff gets precached!!!
			cgs.gameModels[i] = 0;
		}
	}
	cg.loadLCARSStage = 8;
/*
Ghoul2 Insert Start
*/


//	CG_LoadingString( "BSP instances" );

	for(i = 1; i < MAX_SUB_BSP; i++)
	{
		const char		*bspName = 0;
		vec3_t			mins, maxs;
		int				j;
		int				sub = 0;
		char			temp[MAX_QPATH];

		bspName = CG_ConfigString( CS_BSP_MODELS+i );
		if ( !bspName[0] ) 
		{
			break;
		}

		trap_CM_LoadMap( bspName, qtrue );
		cgs.inlineDrawModel[breakPoint] = trap_R_RegisterModel( bspName );
		trap_R_ModelBounds( cgs.inlineDrawModel[breakPoint], mins, maxs );
		for ( j = 0 ; j < 3 ; j++ ) 
		{
			cgs.inlineModelMidpoints[breakPoint][j] = mins[j] + 0.5 * ( maxs[j] - mins[j] );
		}
		breakPoint++;
		for(sub=1;sub<MAX_MODELS;sub++)
		{
			Com_sprintf(temp, MAX_QPATH, "*%d-%d", i, sub);
			cgs.inlineDrawModel[breakPoint] = trap_R_RegisterModel( temp );
			if (!cgs.inlineDrawModel[breakPoint])
			{
				break;
			}
			trap_R_ModelBounds( cgs.inlineDrawModel[breakPoint], mins, maxs );
			for ( j = 0 ; j < 3 ; j++ ) 
			{
				cgs.inlineModelMidpoints[breakPoint][j] = mins[j] + 0.5 * ( maxs[j] - mins[j] );
			}
			breakPoint++;
		}
	}

//	CG_LoadingString( "Creating terrain" );
	for(i = 0; i < MAX_TERRAINS; i++)
	{
		terrainInfo = CG_ConfigString( CS_TERRAINS + i );
		if ( !terrainInfo[0] )
		{
			break;
		}

		terrainID = trap_CM_RegisterTerrain(terrainInfo);

		trap_RMG_Init(terrainID, terrainInfo);

		// Send off the terrainInfo to the renderer
		trap_RE_InitRendererTerrain( terrainInfo );
	}

	/*
	CG_LoadingString("skins");
	// register all the server specified models
	for (i=1 ; i<MAX_CHARSKINS ; i++) {
		const char		*modelName;

		modelName = CG_ConfigString( CS_CHARSKINS+i );
		if ( !modelName[0] ) {
			break;
		}
		cgs.skins[i] = trap_R_RegisterSkin( modelName );
	}
	*/
	//rww - removed and replaced with CS_G2BONES. For custom skins
	//the new method is to append a * after an indexed model name and
	//then append the skin name after that (note that this is only
	//used for NPCs)

//	CG_LoadingString("weapons");

	CG_InitG2Weapons();

/*
Ghoul2 Insert End
*/
	cg.loadLCARSStage = 9;


	// new stuff
	cgs.media.patrolShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/patrol.tga");
	cgs.media.assaultShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/assault.tga");
	cgs.media.campShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/camp.tga");
	cgs.media.followShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/follow.tga");
	cgs.media.defendShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/defend.tga");
	cgs.media.teamLeaderShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/team_leader.tga");
	cgs.media.retrieveShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/retrieve.tga");
	cgs.media.escortShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/escort.tga");
	cgs.media.cursor = trap_R_RegisterShaderNoMip( "menu/art/3_cursor2" );
	cgs.media.sizeCursor = trap_R_RegisterShaderNoMip( "ui/assets/sizecursor.tga" );
	cgs.media.selectCursor = trap_R_RegisterShaderNoMip( "ui/assets/selectcursor.tga" );
	cgs.media.flagShaders[0] = trap_R_RegisterShaderNoMip("ui/assets/statusbar/flag_in_base.tga");
	cgs.media.flagShaders[1] = trap_R_RegisterShaderNoMip("ui/assets/statusbar/flag_capture.tga");
	cgs.media.flagShaders[2] = trap_R_RegisterShaderNoMip("ui/assets/statusbar/flag_missing.tga");

	cgs.media.halfShieldModel	= trap_R_RegisterModel ( "models/weaphits/testboom.md3" );
	cgs.media.halfShieldShader	= trap_R_RegisterShader( "halfShieldShell" );

	trap_FX_RegisterEffect("force/force_touch");

	CG_ClearParticles ();
/*
	for (i=1; i<MAX_PARTICLES_AREAS; i++)
	{
		{
			int rval;

			rval = CG_NewParticleArea ( CS_PARTICLES + i);
			if (!rval)
				break;
		}
	}
*/

	//Weather
	cgs.effects.saberFizz = trap_FX_RegisterEffect("saber/fizz.efx");
	cgs.effects.rain = trap_FX_RegisterEffect("effects/rain");
}


const char *CG_GetStringEdString(char *refSection, char *refName)
{
	static char text[2][1024]={0};	//just incase it's nested
	static int		index = 0;

	index ^= 1;
	trap_SP_GetStringTextString(va("%s_%s", refSection, refName), text[index], sizeof(text[0]));
	return text[index];
}

int	CG_GetClassCount(team_t team,int siegeClass );
int CG_GetTeamNonScoreCount(team_t team);

void CG_SiegeCountCvars( void )
{
	int classGfx[6];

	trap_Cvar_Set( "ui_tm1_cnt",va("%d",CG_GetTeamNonScoreCount(TEAM_RED )));
	trap_Cvar_Set( "ui_tm2_cnt",va("%d",CG_GetTeamNonScoreCount(TEAM_BLUE )));
	trap_Cvar_Set( "ui_tm3_cnt",va("%d",CG_GetTeamNonScoreCount(TEAM_SPECTATOR )));
	
	// This is because the only way we can match up classes is by the gfx handle. 
	classGfx[0] = trap_R_RegisterShaderNoMip("gfx/mp/c_icon_infantry");
	classGfx[1] = trap_R_RegisterShaderNoMip("gfx/mp/c_icon_heavy_weapons");
	classGfx[2] = trap_R_RegisterShaderNoMip("gfx/mp/c_icon_demolitionist");
	classGfx[3] = trap_R_RegisterShaderNoMip("gfx/mp/c_icon_vanguard");
	classGfx[4] = trap_R_RegisterShaderNoMip("gfx/mp/c_icon_support");
	classGfx[5] = trap_R_RegisterShaderNoMip("gfx/mp/c_icon_jedi_general");

	trap_Cvar_Set( "ui_tm1_c0_cnt",va("%d",CG_GetClassCount(TEAM_RED,classGfx[0])));
	trap_Cvar_Set( "ui_tm1_c1_cnt",va("%d",CG_GetClassCount(TEAM_RED,classGfx[1])));
	trap_Cvar_Set( "ui_tm1_c2_cnt",va("%d",CG_GetClassCount(TEAM_RED,classGfx[2])));
	trap_Cvar_Set( "ui_tm1_c3_cnt",va("%d",CG_GetClassCount(TEAM_RED,classGfx[3])));
	trap_Cvar_Set( "ui_tm1_c4_cnt",va("%d",CG_GetClassCount(TEAM_RED,classGfx[4])));
	trap_Cvar_Set( "ui_tm1_c5_cnt",va("%d",CG_GetClassCount(TEAM_RED,classGfx[5])));

	trap_Cvar_Set( "ui_tm2_c0_cnt",va("%d",CG_GetClassCount(TEAM_BLUE,classGfx[0])));
	trap_Cvar_Set( "ui_tm2_c1_cnt",va("%d",CG_GetClassCount(TEAM_BLUE,classGfx[1])));
	trap_Cvar_Set( "ui_tm2_c2_cnt",va("%d",CG_GetClassCount(TEAM_BLUE,classGfx[2])));
	trap_Cvar_Set( "ui_tm2_c3_cnt",va("%d",CG_GetClassCount(TEAM_BLUE,classGfx[3])));
	trap_Cvar_Set( "ui_tm2_c4_cnt",va("%d",CG_GetClassCount(TEAM_BLUE,classGfx[4])));
	trap_Cvar_Set( "ui_tm2_c5_cnt",va("%d",CG_GetClassCount(TEAM_BLUE,classGfx[5])));

}

/*																																			
=======================
CG_BuildSpectatorString

=======================
*/
void CG_BuildSpectatorString(void) {
	int i;
	cg.spectatorList[0] = 0;

	// Count up the number of players per team and per class
	CG_SiegeCountCvars();

	for (i = 0; i < MAX_CLIENTS; i++) {
		if (cgs.clientinfo[i].infoValid && cgs.clientinfo[i].team == TEAM_SPECTATOR ) {
			Q_strcat(cg.spectatorList, sizeof(cg.spectatorList), va("%s     ", cgs.clientinfo[i].name));
		}
	}
	i = strlen(cg.spectatorList);
	if (i != cg.spectatorLen) {
		cg.spectatorLen = i;
		cg.spectatorWidth = -1;
	}
}


/*																																			
===================
CG_RegisterClients
===================
*/
static void CG_RegisterClients( void ) {
	int		i;

	CG_LoadingClient(cg.clientNum);
	CG_NewClientInfo(cg.clientNum, qfalse);

	for (i=0 ; i<MAX_CLIENTS ; i++) {
		const char		*clientInfo;

		if (cg.clientNum == i) {
			continue;
		}

		clientInfo = CG_ConfigString( CS_PLAYERS+i );
		if ( !clientInfo[0]) {
			continue;
		}
		CG_LoadingClient( i );
		CG_NewClientInfo( i, qfalse);
	}
	CG_BuildSpectatorString();
}

//===========================================================================

/*
=================
CG_ConfigString
=================
*/
const char *CG_ConfigString( int index ) {
	if ( index < 0 || index >= MAX_CONFIGSTRINGS ) {
		CG_Error( "CG_ConfigString: bad index: %i", index );
	}
	return cgs.gameState.stringData + cgs.gameState.stringOffsets[ index ];
}

//==================================================================

/*
======================
CG_StartMusic

======================
*/
void CG_StartMusic( qboolean bForceStart ) {
	char	*s;
	char	parm1[MAX_QPATH], parm2[MAX_QPATH];

	// start the background music
	s = (char *)CG_ConfigString( CS_MUSIC );
	Q_strncpyz( parm1, COM_Parse( (const char **)&s ), sizeof( parm1 ) );
	Q_strncpyz( parm2, COM_Parse( (const char **)&s ), sizeof( parm2 ) );

	trap_S_StartBackgroundTrack( parm1, parm2, !bForceStart );
}

char *CG_GetMenuBuffer(const char *filename) {
	int	len;
	fileHandle_t	f;
	static char buf[MAX_MENUFILE];

	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if ( !f ) {
		trap_Print( va( S_COLOR_RED "menu file not found: %s, using default\n", filename ) );
		return NULL;
	}
	if ( len >= MAX_MENUFILE ) {
		trap_Print( va( S_COLOR_RED "menu file too large: %s is %i, max allowed is %i\n", filename, len, MAX_MENUFILE ) );
		trap_FS_FCloseFile( f );
		return NULL;
	}

	trap_FS_Read( buf, len, f );
	buf[len] = 0;
	trap_FS_FCloseFile( f );

	return buf;
}

//
// ==============================
// new hud stuff ( mission pack )
// ==============================
//
qboolean CG_Asset_Parse(int handle) {
	pc_token_t token;

	if (!trap_PC_ReadToken(handle, &token))
		return qfalse;
	if (Q_stricmp(token.string, "{") != 0) {
		return qfalse;
	}
    
	while ( 1 ) {
		if (!trap_PC_ReadToken(handle, &token))
			return qfalse;

		if (Q_stricmp(token.string, "}") == 0) {
			return qtrue;
		}

		// font
		if (Q_stricmp(token.string, "font") == 0) {
			int pointSize;
			if (!trap_PC_ReadToken(handle, &token) || !PC_Int_Parse(handle, &pointSize)) {
				return qfalse;
			}

//			cgDC.registerFont(token.string, pointSize, &cgDC.Assets.textFont);
			cgDC.Assets.qhMediumFont = cgDC.RegisterFont(token.string);
			continue;
		}

		// smallFont
		if (Q_stricmp(token.string, "smallFont") == 0) {
			int pointSize;
			if (!trap_PC_ReadToken(handle, &token) || !PC_Int_Parse(handle, &pointSize)) {
				return qfalse;
			}
//			cgDC.registerFont(token.string, pointSize, &cgDC.Assets.smallFont);
			cgDC.Assets.qhSmallFont = cgDC.RegisterFont(token.string);
			continue;
		}

		// smallFont
		if (Q_stricmp(token.string, "small2Font") == 0) {
			int pointSize;
			if (!trap_PC_ReadToken(handle, &token) || !PC_Int_Parse(handle, &pointSize)) {
				return qfalse;
			}
//			cgDC.registerFont(token.string, pointSize, &cgDC.Assets.smallFont);
			cgDC.Assets.qhSmall2Font = cgDC.RegisterFont(token.string);
			continue;
		}

		// font
		if (Q_stricmp(token.string, "bigfont") == 0) {
			int pointSize;
			if (!trap_PC_ReadToken(handle, &token) || !PC_Int_Parse(handle, &pointSize)) {
				return qfalse;
			}
//			cgDC.registerFont(token.string, pointSize, &cgDC.Assets.bigFont);
			cgDC.Assets.qhBigFont = cgDC.RegisterFont(token.string);
			continue;
		}

		// gradientbar
		if (Q_stricmp(token.string, "gradientbar") == 0) {
			if (!trap_PC_ReadToken(handle, &token)) {
				return qfalse;
			}
			cgDC.Assets.gradientBar = trap_R_RegisterShaderNoMip(token.string);
			continue;
		}

		// enterMenuSound
		if (Q_stricmp(token.string, "menuEnterSound") == 0) {
			if (!trap_PC_ReadToken(handle, &token)) {
				return qfalse;
			}
			cgDC.Assets.menuEnterSound = trap_S_RegisterSound( token.string );
			continue;
		}

		// exitMenuSound
		if (Q_stricmp(token.string, "menuExitSound") == 0) {
			if (!trap_PC_ReadToken(handle, &token)) {
				return qfalse;
			}
			cgDC.Assets.menuExitSound = trap_S_RegisterSound( token.string );
			continue;
		}

		// itemFocusSound
		if (Q_stricmp(token.string, "itemFocusSound") == 0) {
			if (!trap_PC_ReadToken(handle, &token)) {
				return qfalse;
			}
			cgDC.Assets.itemFocusSound = trap_S_RegisterSound( token.string );
			continue;
		}

		// menuBuzzSound
		if (Q_stricmp(token.string, "menuBuzzSound") == 0) {
			if (!trap_PC_ReadToken(handle, &token)) {
				return qfalse;
			}
			cgDC.Assets.menuBuzzSound = trap_S_RegisterSound( token.string );
			continue;
		}

		if (Q_stricmp(token.string, "cursor") == 0) {
			if (!PC_String_Parse(handle, &cgDC.Assets.cursorStr)) {
				return qfalse;
			}
			cgDC.Assets.cursor = trap_R_RegisterShaderNoMip( cgDC.Assets.cursorStr);
			continue;
		}

		if (Q_stricmp(token.string, "fadeClamp") == 0) {
			if (!PC_Float_Parse(handle, &cgDC.Assets.fadeClamp)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "fadeCycle") == 0) {
			if (!PC_Int_Parse(handle, &cgDC.Assets.fadeCycle)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "fadeAmount") == 0) {
			if (!PC_Float_Parse(handle, &cgDC.Assets.fadeAmount)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "shadowX") == 0) {
			if (!PC_Float_Parse(handle, &cgDC.Assets.shadowX)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "shadowY") == 0) {
			if (!PC_Float_Parse(handle, &cgDC.Assets.shadowY)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "shadowColor") == 0) {
			if (!PC_Color_Parse(handle, &cgDC.Assets.shadowColor)) {
				return qfalse;
			}
			cgDC.Assets.shadowFadeClamp = cgDC.Assets.shadowColor[3];
			continue;
		}
	}
	return qfalse; // bk001204 - why not?
}

void CG_ParseMenu(const char *menuFile) {
	pc_token_t token;
	int handle;

	handle = trap_PC_LoadSource(menuFile);
	if (!handle)
		handle = trap_PC_LoadSource("ui/testhud.menu");
	if (!handle)
		return;

	while ( 1 ) {
		if (!trap_PC_ReadToken( handle, &token )) {
			break;
		}

		//if ( Q_stricmp( token, "{" ) ) {
		//	Com_Printf( "Missing { in menu file\n" );
		//	break;
		//}

		//if ( menuCount == MAX_MENUS ) {
		//	Com_Printf( "Too many menus!\n" );
		//	break;
		//}

		if ( token.string[0] == '}' ) {
			break;
		}

		if (Q_stricmp(token.string, "assetGlobalDef") == 0) {
			if (CG_Asset_Parse(handle)) {
				continue;
			} else {
				break;
			}
		}


		if (Q_stricmp(token.string, "menudef") == 0) {
			// start a new menu
			Menu_New(handle);
		}
	}
	trap_PC_FreeSource(handle);
}


qboolean CG_Load_Menu(const char **p) 
{

	char *token;

	token = COM_ParseExt((const char **)p, qtrue);

	if (token[0] != '{') {
		return qfalse;
	}

	while ( 1 ) {

		token = COM_ParseExt((const char **)p, qtrue);
    
		if (Q_stricmp(token, "}") == 0) {
			return qtrue;
		}

		if ( !token || token[0] == 0 ) {
			return qfalse;
		}

		CG_ParseMenu(token); 
	}
	return qfalse;
}


static qboolean CG_OwnerDrawHandleKey(int ownerDraw, int flags, float *special, int key) {
	return qfalse;
}


static int CG_FeederCount(float feederID) {
	int i, count;
	count = 0;
	if (feederID == FEEDER_REDTEAM_LIST) {
		for (i = 0; i < cg.numScores; i++) {
			if (cg.scores[i].team == TEAM_RED) {
				count++;
			}
		}
	} else if (feederID == FEEDER_BLUETEAM_LIST) {
		for (i = 0; i < cg.numScores; i++) {
			if (cg.scores[i].team == TEAM_BLUE) {
				count++;
			}
		}
	} else if (feederID == FEEDER_SCOREBOARD) {
		return cg.numScores;
	}
	return count;
}


void CG_SetScoreSelection(void *p) {
	menuDef_t *menu = (menuDef_t*)p;
	playerState_t *ps = &cg.snap->ps;
	int i, red, blue;
	red = blue = 0;
	for (i = 0; i < cg.numScores; i++) {
		if (cg.scores[i].team == TEAM_RED) {
			red++;
		} else if (cg.scores[i].team == TEAM_BLUE) {
			blue++;
		}
		if (ps->clientNum == cg.scores[i].client) {
			cg.selectedScore = i;
		}
	}

	if (menu == NULL) {
		// just interested in setting the selected score
		return;
	}

	if ( cgs.gametype >= GT_TEAM ) {
		int feeder = FEEDER_REDTEAM_LIST;
		i = red;
		if (cg.scores[cg.selectedScore].team == TEAM_BLUE) {
			feeder = FEEDER_BLUETEAM_LIST;
			i = blue;
		}
		Menu_SetFeederSelection(menu, feeder, i, NULL);
	} else {
		Menu_SetFeederSelection(menu, FEEDER_SCOREBOARD, cg.selectedScore, NULL);
	}
}

// FIXME: might need to cache this info
static clientInfo_t * CG_InfoFromScoreIndex(int index, int team, int *scoreIndex) {
	int i, count;
	if ( cgs.gametype >= GT_TEAM ) {
		count = 0;
		for (i = 0; i < cg.numScores; i++) {
			if (cg.scores[i].team == team) {
				if (count == index) {
					*scoreIndex = i;
					return &cgs.clientinfo[cg.scores[i].client];
				}
				count++;
			}
		}
	}
	*scoreIndex = index;
	return &cgs.clientinfo[ cg.scores[index].client ];
}

static const char *CG_FeederItemText(float feederID, int index, int column,
									 qhandle_t *handle1, qhandle_t *handle2, qhandle_t *handle3) {
	gitem_t *item;
	int scoreIndex = 0;
	clientInfo_t *info = NULL;
	int team = -1;
	score_t *sp = NULL;

	*handle1 = *handle2 = *handle3 = -1;

	if (feederID == FEEDER_REDTEAM_LIST) {
		team = TEAM_RED;
	} else if (feederID == FEEDER_BLUETEAM_LIST) {
		team = TEAM_BLUE;
	}

	info = CG_InfoFromScoreIndex(index, team, &scoreIndex);
	sp = &cg.scores[scoreIndex];

	if (info && info->infoValid) {
		switch (column) {
			case 0:
				if ( info->powerups & ( 1 << PW_NEUTRALFLAG ) ) {
					item = BG_FindItemForPowerup( PW_NEUTRALFLAG );
					*handle1 = cg_items[ ITEM_INDEX(item) ].icon;
				} else if ( info->powerups & ( 1 << PW_REDFLAG ) ) {
					item = BG_FindItemForPowerup( PW_REDFLAG );
					*handle1 = cg_items[ ITEM_INDEX(item) ].icon;
				} else if ( info->powerups & ( 1 << PW_BLUEFLAG ) ) {
					item = BG_FindItemForPowerup( PW_BLUEFLAG );
					*handle1 = cg_items[ ITEM_INDEX(item) ].icon;
				} else {
					/*	
					if ( info->botSkill > 0 && info->botSkill <= 5 ) {
						*handle1 = cgs.media.botSkillShaders[ info->botSkill - 1 ];
					} else if ( info->handicap < 100 ) {
					return va("%i", info->handicap );
					}
					*/
				}
			break;
			case 1:
				if (team == -1) {
					return "";
				} else {
					*handle1 = CG_StatusHandle(info->teamTask);
				}
		  break;
			case 2:
				if ( cg.snap->ps.stats[ STAT_CLIENTS_READY ] & ( 1 << sp->client ) ) {
					return "Ready";
				}
				if (team == -1) {
					if (cgs.gametype == GT_DUEL || cgs.gametype == GT_POWERDUEL) {
						return va("%i/%i", info->wins, info->losses);
					} else if (info->infoValid && info->team == TEAM_SPECTATOR ) {
						return "Spectator";
					} else {
						return "";
					}
				} else {
					if (info->teamLeader) {
						return "Leader";
					}
				}
			break;
			case 3:
				return info->name;
			break;
			case 4:
				return va("%i", info->score);
			break;
			case 5:
				return va("%4i", sp->time);
			break;
			case 6:
				if ( sp->ping == -1 ) {
					return "connecting";
				} 
				return va("%4i", sp->ping);
			break;
		}
	}

	return "";
}

static qhandle_t CG_FeederItemImage(float feederID, int index) {
	return 0;
}

static qboolean CG_FeederSelection(float feederID, int index, itemDef_t *item) {
	if ( cgs.gametype >= GT_TEAM ) {
		int i, count;
		int team = (feederID == FEEDER_REDTEAM_LIST) ? TEAM_RED : TEAM_BLUE;
		count = 0;
		for (i = 0; i < cg.numScores; i++) {
			if (cg.scores[i].team == team) {
				if (index == count) {
					cg.selectedScore = i;
				}
				count++;
			}
		}
	} else {
		cg.selectedScore = index;
	}

	return qtrue;
}

float CG_Cvar_Get(const char *cvar) {
	char buff[128];
	memset(buff, 0, sizeof(buff));
	trap_Cvar_VariableStringBuffer(cvar, buff, sizeof(buff));
	return atof(buff);
}

void CG_Text_PaintWithCursor(float x, float y, float scale, vec4_t color, const char *text, int cursorPos, char cursor, int limit, int style, int iMenuFont) {
	CG_Text_Paint(x, y, scale, color, text, 0, limit, style, iMenuFont);
}

static int CG_OwnerDrawWidth(int ownerDraw, float scale) {
	switch (ownerDraw) {
	  case CG_GAME_TYPE:
			return CG_Text_Width(BG_GetGametypeString( cgs.gametype ), scale, FONT_MEDIUM);
	  case CG_GAME_STATUS:
			return CG_Text_Width(CG_GetGameStatusText(), scale, FONT_MEDIUM);
			break;
	  case CG_KILLER:
			return CG_Text_Width(CG_GetKillerText(), scale, FONT_MEDIUM);
			break;
	  case CG_RED_NAME:
			return CG_Text_Width(DEFAULT_REDTEAM_NAME/*cg_redTeamName.string*/, scale, FONT_MEDIUM);
			break;
	  case CG_BLUE_NAME:
			return CG_Text_Width(DEFAULT_BLUETEAM_NAME/*cg_blueTeamName.string*/, scale, FONT_MEDIUM);
			break;


	}
	return 0;
}

static int CG_PlayCinematic(const char *name, float x, float y, float w, float h) {
  return trap_CIN_PlayCinematic(name, x, y, w, h, CIN_loop);
}

static void CG_StopCinematic(int handle) {
  trap_CIN_StopCinematic(handle);
}

static void CG_DrawCinematic(int handle, float x, float y, float w, float h) {
  trap_CIN_SetExtents(handle, x, y, w, h);
  trap_CIN_DrawCinematic(handle);
}

static void CG_RunCinematicFrame(int handle) {
  trap_CIN_RunCinematic(handle);
}

/*
=================
CG_LoadMenus();

=================
*/
void CG_LoadMenus(const char *menuFile) 
{
	const char	*token;
	const char	*p;
	int	len;
	fileHandle_t	f;
	static char buf[MAX_MENUDEFFILE];

	len = trap_FS_FOpenFile( menuFile, &f, FS_READ );

	if ( !f )
	{
		if( Q_isanumber( menuFile ) ) // cg_hudFiles 1
			trap_Print( S_COLOR_GREEN "hud menu file skipped, using default\n" );
		else
			CG_Printf( S_COLOR_YELLOW "hud menu file not found: %s, using default\n", menuFile );

		len = trap_FS_FOpenFile( "ui/jahud.txt", &f, FS_READ );
		if (!f)
		{
			trap_FS_FCloseFile( f );
			CG_Error( S_COLOR_RED "default hud menu file not found: ui/jahud.txt, unable to continue!" );
		}
	}

	if ( len >= MAX_MENUDEFFILE ) 
	{
		trap_FS_FCloseFile( f );
		CG_Error( S_COLOR_RED "menu file too large: %s is %i, max allowed is %i", menuFile, len, MAX_MENUDEFFILE );
	}

	trap_FS_Read( buf, len, f );
	buf[len] = 0;
	trap_FS_FCloseFile( f );
	
	p = buf;

	while ( 1 ) 
	{
		token = COM_ParseExt( &p, qtrue );
		if( !token || token[0] == 0 || token[0] == '}') 
		{
			break;
		}

		if ( Q_stricmp( token, "}" ) == 0 ) 
		{
			break;
		}

		if (Q_stricmp(token, "loadmenu") == 0) 
		{
			if (CG_Load_Menu(&p)) 
			{
				continue;
			} 
			else 
			{
				break;
			}
		}
	}

	//Com_Printf("UI menu load time = %d milli seconds\n", cgi_Milliseconds() - start);
}

/*
=================
CG_LoadHudMenu();

=================
*/
void CG_LoadHudMenu() 
{
	const char *hudSet;

	cgDC.registerShaderNoMip = &trap_R_RegisterShaderNoMip;
	cgDC.setColor = &trap_R_SetColor;
	cgDC.drawHandlePic = &CG_DrawPic;
	cgDC.drawStretchPic = &trap_R_DrawStretchPic;
	cgDC.drawText = &CG_Text_Paint;
	cgDC.textWidth = &CG_Text_Width;
	cgDC.textHeight = &CG_Text_Height;
	cgDC.registerModel = &trap_R_RegisterModel;
	cgDC.modelBounds = &trap_R_ModelBounds;
	cgDC.fillRect = &CG_FillRect;
	cgDC.drawRect = &CG_DrawRect;   
	cgDC.drawSides = &CG_DrawSides;
	cgDC.drawTopBottom = &CG_DrawTopBottom;
	cgDC.clearScene = &trap_R_ClearScene;
	cgDC.addRefEntityToScene = &trap_R_AddRefEntityToScene;
	cgDC.renderScene = &trap_R_RenderScene;
	cgDC.RegisterFont = &trap_R_RegisterFont;
	cgDC.Font_StrLenPixels = &trap_R_Font_StrLenPixels;
	cgDC.Font_StrLenChars = &trap_R_Font_StrLenChars;
	cgDC.Font_HeightPixels = &trap_R_Font_HeightPixels;
	cgDC.Font_DrawString = &trap_R_Font_DrawString;
	cgDC.Language_IsAsian = &trap_Language_IsAsian;
	cgDC.Language_UsesSpaces = &trap_Language_UsesSpaces;
	cgDC.AnyLanguage_ReadCharFromString = &trap_AnyLanguage_ReadCharFromString;
	cgDC.ownerDrawItem = &CG_OwnerDraw;
	cgDC.getValue = &CG_GetValue;
	cgDC.ownerDrawVisible = &CG_OwnerDrawVisible;
	cgDC.runScript = &CG_RunMenuScript;
	cgDC.deferScript = &CG_DeferMenuScript;
	cgDC.getTeamColor = &CG_GetTeamColor;
	cgDC.setCVar = trap_Cvar_Set;
	cgDC.getCVarString = trap_Cvar_VariableStringBuffer;
	cgDC.getCVarValue = CG_Cvar_Get;
	cgDC.drawTextWithCursor = &CG_Text_PaintWithCursor;
	//cgDC.setOverstrikeMode = &trap_Key_SetOverstrikeMode;
	//cgDC.getOverstrikeMode = &trap_Key_GetOverstrikeMode;
	cgDC.startLocalSound = &trap_S_StartLocalSound;
	cgDC.ownerDrawHandleKey = &CG_OwnerDrawHandleKey;
	cgDC.feederCount = &CG_FeederCount;
	cgDC.feederItemImage = &CG_FeederItemImage;
	cgDC.feederItemText = &CG_FeederItemText;
	cgDC.feederSelection = &CG_FeederSelection;
	//cgDC.setBinding = &trap_Key_SetBinding;
	//cgDC.getBindingBuf = &trap_Key_GetBindingBuf;
	//cgDC.keynumToStringBuf = &trap_Key_KeynumToStringBuf;
	//cgDC.executeText = &trap_Cmd_ExecuteText;
	cgDC.Error = &Com_Error; 
	cgDC.Print = &Com_Printf; 
	cgDC.ownerDrawWidth = &CG_OwnerDrawWidth;
	//cgDC.Pause = &CG_Pause;
	cgDC.registerSound = &trap_S_RegisterSound;
	cgDC.startBackgroundTrack = &trap_S_StartBackgroundTrack;
	cgDC.stopBackgroundTrack = &trap_S_StopBackgroundTrack;
	cgDC.playCinematic = &CG_PlayCinematic;
	cgDC.stopCinematic = &CG_StopCinematic;
	cgDC.drawCinematic = &CG_DrawCinematic;
	cgDC.runCinematicFrame = &CG_RunCinematicFrame;
	
	Init_Display(&cgDC);

	Menu_Reset();

	hudSet = cg_hudFiles.string;
	if (hudSet[0] == '\0') 
	{
		hudSet = "ui/jahud.txt";
	}

	CG_LoadMenus(hudSet);

}

void CG_AssetCache() {
	//if (Assets.textFont == NULL) {
	//  trap_R_RegisterFont("fonts/arial.ttf", 72, &Assets.textFont);
	//}
	//Assets.background = trap_R_RegisterShaderNoMip( ASSET_BACKGROUND );
	//Com_Printf("Menu Size: %i bytes\n", sizeof(Menus));
	cgDC.Assets.gradientBar = trap_R_RegisterShaderNoMip( ASSET_GRADIENTBAR );
	cgDC.Assets.fxBasePic = trap_R_RegisterShaderNoMip( ART_FX_BASE );
	cgDC.Assets.fxPic[0] = trap_R_RegisterShaderNoMip( ART_FX_RED );
	cgDC.Assets.fxPic[1] = trap_R_RegisterShaderNoMip( ART_FX_YELLOW );
	cgDC.Assets.fxPic[2] = trap_R_RegisterShaderNoMip( ART_FX_GREEN );
	cgDC.Assets.fxPic[3] = trap_R_RegisterShaderNoMip( ART_FX_TEAL );
	cgDC.Assets.fxPic[4] = trap_R_RegisterShaderNoMip( ART_FX_BLUE );
	cgDC.Assets.fxPic[5] = trap_R_RegisterShaderNoMip( ART_FX_CYAN );
	cgDC.Assets.fxPic[6] = trap_R_RegisterShaderNoMip( ART_FX_WHITE );
	cgDC.Assets.scrollBar = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR );
	cgDC.Assets.scrollBarArrowDown = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWDOWN );
	cgDC.Assets.scrollBarArrowUp = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWUP );
	cgDC.Assets.scrollBarArrowLeft = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWLEFT );
	cgDC.Assets.scrollBarArrowRight = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWRIGHT );
	cgDC.Assets.scrollBarThumb = trap_R_RegisterShaderNoMip( ASSET_SCROLL_THUMB );
	cgDC.Assets.sliderBar = trap_R_RegisterShaderNoMip( ASSET_SLIDER_BAR );
	cgDC.Assets.sliderThumb = trap_R_RegisterShaderNoMip( ASSET_SLIDER_THUMB );
}

/*


/*
Ghoul2 Insert Start
*/

// initialise the cg_entities structure - take into account the ghoul2 stl stuff in the active snap shots
void CG_Init_CG(void)
{
	memset( &cg, 0, sizeof(cg));
}

// initialise the cg_entities structure - take into account the ghoul2 stl stuff
void CG_Init_CGents(void)
{
	memset(&cg_entities, 0, sizeof(cg_entities));
}


void CG_InitItems(void)
{
	memset( cg_items, 0, sizeof( cg_items ) );
}

void CG_TransitionPermanent(void)
{
	centity_t	*cent = cg_entities;
	int			i;

	cg_numpermanents = 0;
	for(i=0;i<MAX_GENTITIES;i++,cent++)
	{
		if (trap_GetDefaultState(i, &cent->currentState))
		{
			cent->nextState = cent->currentState;
			VectorCopy (cent->currentState.origin, cent->lerpOrigin);
			VectorCopy (cent->currentState.angles, cent->lerpAngles);
			cent->currentValid = qtrue;

			cg_permanents[cg_numpermanents++] = cent;
		}
	}
}

/*
Ghoul2 Insert End
*/

extern playerState_t *cgSendPS[MAX_GENTITIES]; //is not MAX_CLIENTS because NPCs exceed MAX_CLIENTS
void CG_PmoveClientPointerUpdate();

void WP_SaberLoadParms( void );
void BG_VehicleLoadParms( void );

void demoSeekPreRecord(const char* preRecordTimeString);

/*
=================
CG_Init

Called after every level change or subsystem restart
Will perform callbacks to make the loading info screen update.
=================
*/
extern void trap_MME_RequestFeatures( void );
void CG_Init( int serverMessageNum, int serverCommandSequence, int clientNum )
{
	static gitem_t *item;
	char buf[64];
	const char	*s;
	int i = 0;

	trap_MME_RequestFeatures();

	BG_InitAnimsets(); //clear it out

	trap_CG_RegisterSharedMemory(cg.sharedBuffer);

	//Load external vehicle data
	BG_VehicleLoadParms();

	// clear everything
/*
Ghoul2 Insert Start
*/

//	memset( cg_entities, 0, sizeof( cg_entities ) );
	CG_Init_CGents();
// this is a No-No now we have stl vector classes in here.
//	memset( &cg, 0, sizeof( cg ) );
	CG_Init_CG();
	CG_InitItems();

	//create the global jetpack instance
	CG_InitJetpackGhoul2();

	CG_PmoveClientPointerUpdate();

/*
Ghoul2 Insert End
*/

	//Load sabers.cfg data
	WP_SaberLoadParms();

	// this is kinda dumb as well, but I need to pre-load some fonts in order to have the text available
	//	to say I'm loading the assets.... which includes loading the fonts. So I'll set these up as reasonable
	//	defaults, then let the menu asset parser (which actually specifies the ingame fonts) load over them
	//	if desired during parse.  Dunno how legal it is to store in these cgDC things, but it causes no harm
	//	and even if/when they get overwritten they'll be legalised by the menu asset parser :-)
//	CG_LoadFonts();
	cgDC.Assets.qhSmallFont  = trap_R_RegisterFont("ocr_a");
	cgDC.Assets.qhMediumFont = trap_R_RegisterFont("ergoec");
	cgDC.Assets.qhBigFont = cgDC.Assets.qhMediumFont;

	memset( &cgs, 0, sizeof( cgs ) );
	memset( cg_weapons, 0, sizeof(cg_weapons) );

	cg.clientNum = clientNum;

	cgs.processedSnapshotNum = serverMessageNum;
	cgs.serverCommandSequence = serverCommandSequence;

	cg.loadLCARSStage		= 0;

	cg.itemSelect = -1;
	cg.forceSelect = -1;
	
	srand(trap_Milliseconds());
	cg.tip = rand();

	// load a few needed things before we do any screen updates
	cgs.media.charsetShader		= trap_R_RegisterShaderNoMip( "gfx/2d/charsgrid_med" );
	cgs.media.whiteShader		= trap_R_RegisterShader( "white" );

	cgs.media.loadBarLED		= trap_R_RegisterShaderNoMip( "gfx/hud/load_tick" );
	cgs.media.loadBarLEDCap		= trap_R_RegisterShaderNoMip( "gfx/hud/load_tick_cap" );
	cgs.media.loadBarLEDSurround= trap_R_RegisterShaderNoMip( "gfx/hud/mp_levelload" );

	// Force HUD set up
	cg.forceHUDActive = qtrue;
	cg.forceHUDTotalFlashTime = 0;
	cg.forceHUDNextFlashTime = 0;

	i = WP_NONE+1;
	while (i <= LAST_USEABLE_WEAPON)
	{
		item = BG_FindItemForWeapon(i);

		if (item && item->icon && item->icon[0])
		{
			cgs.media.weaponIcons[i] = trap_R_RegisterShaderNoMip(item->icon);
			cgs.media.weaponIcons_NA[i] = trap_R_RegisterShaderNoMip(va("%s_na", item->icon));
		}
		else
		{ //make sure it is zero'd (default shader)
			cgs.media.weaponIcons[i] = 0;
			cgs.media.weaponIcons_NA[i] = 0;
		}
		i++;
	}
	trap_Cvar_VariableStringBuffer("com_buildscript", buf, sizeof(buf));
	if (atoi(buf))
	{
		trap_R_RegisterShaderNoMip("gfx/hud/w_icon_saberstaff");
		trap_R_RegisterShaderNoMip("gfx/hud/w_icon_duallightsaber");
	}
	i = 0;

	// HUD artwork for cycling inventory,weapons and force powers 
	cgs.media.weaponIconBackground		= trap_R_RegisterShaderNoMip( "gfx/hud/background");
	cgs.media.forceIconBackground		= trap_R_RegisterShaderNoMip( "gfx/hud/background_f");
	cgs.media.inventoryIconBackground	= trap_R_RegisterShaderNoMip( "gfx/hud/background_i");

	//rww - precache holdable item icons here
	while (i < bg_numItems)
	{
		if (bg_itemlist[i].giType == IT_HOLDABLE)
		{
			if (bg_itemlist[i].icon)
			{
				cgs.media.invenIcons[bg_itemlist[i].giTag] = trap_R_RegisterShaderNoMip(bg_itemlist[i].icon);
			}
			else
			{
				cgs.media.invenIcons[bg_itemlist[i].giTag] = 0;
			}
		}

		i++;
	}

	//rww - precache force power icons here
	i = 0;

	while (i < NUM_FORCE_POWERS)
	{
		cgs.media.forcePowerIcons[i] = trap_R_RegisterShaderNoMip(HolocronIcons[i]);

		i++;
	}
	cgs.media.rageRecShader = trap_R_RegisterShaderNoMip("gfx/mp/f_icon_ragerec");


	//body decal shaders -rww
	cgs.media.bdecal_bodyburn1 = trap_R_RegisterShader("gfx/damage/bodyburnmark1");
	cgs.media.bdecal_saberglow = trap_R_RegisterShader("gfx/damage/saberglowmark");
	cgs.media.bdecal_burn1 = trap_R_RegisterShader("gfx/damage/bodybigburnmark1");
	cgs.media.mSaberDamageGlow = trap_R_RegisterShader("gfx/effects/saberDamageGlow");

	CG_InitConsoleCommands();

	//Raz: initialise third person setting
	cg.renderingThirdPerson = (cg_thirdPerson.integer % 2);

	cg.weaponSelect = WP_BRYAR_PISTOL;

	cgs.redflag = cgs.blueflag = -1; // For compatibily, default to unset for
	cgs.flagStatus = -1;
	// old servers

	// get the rendering configuration from the client system
	trap_GetGlconfig( &cgs.glconfig );
	cgs.screenXScale = cgs.glconfig.vidWidth / (float)SCREEN_WIDTH;
	cgs.screenYScale = cgs.glconfig.vidHeight / (float)SCREEN_HEIGHT;
	
	CG_RegisterCvars();

	// get the gamestate from the client system
	trap_GetGameState( &cgs.gameState );

	CG_TransitionPermanent(); //rwwRMG - added

	// check version
	s = CG_ConfigString( CS_GAME_VERSION );
	if ( strcmp( s, GAME_VERSION ) ) {
		//CG_Error( "Client/Server game mismatch: %s/%s", GAME_VERSION, s );
	}

	s = CG_ConfigString( CS_LEVEL_START_TIME );
	cgs.levelStartTime = atoi( s );

	CG_ParseServerinfo();

	CG_SetExtendedColours();

	// load the new map
//	CG_LoadingString( "collision map" );

	trap_CM_LoadMap( cgs.mapname, qfalse );

	String_Init();

	cg.loading = qtrue;		// force players to load instead of defer

	//make sure saber data is loaded before this! (so we can precache the appropriate hilts)
	CG_InitSiegeMode();

	//[TrueView]
	CG_TrueViewInit();
	//[/TrueView]

	CG_RegisterSounds();

//	CG_LoadingString( "graphics" );

	CG_RegisterGraphics();

//	CG_LoadingString( "clients" );

	CG_RegisterClients();		// if low on memory, some clients will be deferred

	CG_AssetCache();
	CG_LoadHudMenu();      // load new hud stuff

	cg.loading = qfalse;	// future players will be deferred

	CG_InitLocalEntities();

	CG_InitMarkPolys();

	// remove the last loading update
	cg.infoScreenText[0] = 0;

	// Make sure we have update values (scores)
	CG_SetConfigValues();

	CG_StartMusic(qfalse);

//	CG_LoadingString( "Clearing light styles" );
	CG_ClearLightStyles();

//	CG_LoadingString( "Creating automap data" );
	//init automap
	trap_R_InitWireframeAutomap();

	CG_LoadingString( "" );

	CG_ShaderStateChanged();

	trap_S_ClearLoopingSounds();

	trap_R_GetDistanceCull(&cg.distanceCull);

	CG_ParseEntitiesFromString();

	CG_Set2DRatio();

	CG_MultiSpecInit();

	CG_CheckNotification();

	//Raz: warn for poor settings
	trap_Cvar_VariableStringBuffer( "rate", buf, sizeof( buf ) );
	if ( atoi( buf ) == 4000 )
		CG_Printf( "^3WARNING: Default /rate value detected. Suggest typing /rate 25000 for a smoother connection!\n" );
}

//makes sure returned string is in localized format
const char *CG_GetLocationString(const char *loc)
{
	static char text[1024]={0};

	if (!loc || loc[0] != '@')
	{ //just a raw string
		return loc;
	}

	trap_SP_GetStringTextString(loc+1, text, sizeof(text));
	return text;
}

//clean up all the ghoul2 allocations, the nice and non-hackly way -rww
void CG_KillCEntityG2(int entNum);
void CG_DestroyAllGhoul2(void)
{
	int i = 0;
	int j;

//	Com_Printf("... CGameside GHOUL2 Cleanup\n");
	while (i < MAX_GENTITIES)
	{ //free all dynamically allocated npc client info structs and ghoul2 instances
		CG_KillCEntityG2(i);	
		i++;
	}
	
	//Clean the weapon instances
	CG_ShutDownG2Weapons();

	i = 0;
	while (i < MAX_ITEMS)
	{ //and now for items
		j = 0;
		while (j < MAX_ITEM_MODELS)
		{
			if (cg_items[i].g2Models[j] && trap_G2_HaveWeGhoul2Models(cg_items[i].g2Models[j]))
			{
				trap_G2API_CleanGhoul2Models(&cg_items[i].g2Models[j]);
				cg_items[i].g2Models[j] = NULL;
			}
			j++;
		}
		i++;
	}

	//Clean the global jetpack instance
	CG_CleanJetpackGhoul2();
}

/*
=================
CG_Shutdown

Called before every level change or subsystem restart
=================
*/
void CG_Shutdown( void ) 
{
	BG_ClearAnimsets(); //free all dynamic allocations made through the engine

    CG_DestroyAllGhoul2();

//	Com_Printf("... FX System Cleanup\n");
	trap_FX_FreeSystem();
	trap_ROFF_Clean();

	if (cgWeatherOverride)
	{
		trap_R_WeatherContentsOverride(0); //rwwRMG - reset it engine-side
	}

	//reset weather
	trap_R_WorldEffectCommand("die");

	UI_CleanupGhoul2();
	//If there was any ghoul2 stuff in our side of the shared ui code, then remove it now.

	// some mods may need to do cleanup work here,
	// like closing files or archiving session data

	CG_MultiSpecShutDown();
}

/*
===============
CG_NextForcePower_f
===============
*/
void CG_NextForcePower_f( void ) 
{
	int current;
	usercmd_t cmd;
	if ( !cg.snap )
	{
		return;
	}

	if (cg.predictedPlayerState.pm_type == PM_SPECTATOR)
	{
		return;
	}

	current = trap_GetCurrentCmdNumber();
	trap_GetUserCmd(current, &cmd);
	if ((cmd.buttons & BUTTON_USE) || CG_NoUseableForce())
	{
		CG_NextInventory_f();
		return;
	}

	if (cg.snap->ps.pm_flags & PMF_FOLLOW)
	{
		return;
	}

//	BG_CycleForce(&cg.snap->ps, 1);
	if (cg.forceSelect != -1)
	{
		cg.snap->ps.fd.forcePowerSelected = cg.forceSelect;
	}

	BG_CycleForce(&cg.snap->ps, 1);

	if (cg.snap->ps.fd.forcePowersKnown & (1 << cg.snap->ps.fd.forcePowerSelected))
	{
		cg.forceSelect = cg.snap->ps.fd.forcePowerSelected;
		cg.forceSelectTime = cg.time;
	}
}

/*
===============
CG_PrevForcePower_f
===============
*/
void CG_PrevForcePower_f( void ) 
{
	int current;
	usercmd_t cmd;
	if ( !cg.snap )
	{
		return;
	}

	if (cg.predictedPlayerState.pm_type == PM_SPECTATOR)
	{
		return;
	}

	current = trap_GetCurrentCmdNumber();
	trap_GetUserCmd(current, &cmd);
	if ((cmd.buttons & BUTTON_USE) || CG_NoUseableForce())
	{
		CG_PrevInventory_f();
		return;
	}

	if (cg.snap->ps.pm_flags & PMF_FOLLOW)
	{
		return;
	}

//	BG_CycleForce(&cg.snap->ps, -1);
	if (cg.forceSelect != -1)
	{
		cg.snap->ps.fd.forcePowerSelected = cg.forceSelect;
	}

	BG_CycleForce(&cg.snap->ps, -1);

	if (cg.snap->ps.fd.forcePowersKnown & (1 << cg.snap->ps.fd.forcePowerSelected))
	{
		cg.forceSelect = cg.snap->ps.fd.forcePowerSelected;
		cg.forceSelectTime = cg.time;
	}
}

void CG_NextInventory_f(void)
{
	if ( !cg.snap )
	{
		return;
	}

	if (cg.snap->ps.pm_flags & PMF_FOLLOW)
	{
		return;
	}

	if (cg.predictedPlayerState.pm_type == PM_SPECTATOR)
	{
		return;
	}

	if (cg.itemSelect != -1)
	{
		cg.snap->ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(cg.itemSelect, IT_HOLDABLE);
	}
	BG_CycleInven(&cg.snap->ps, 1);

	if (cg.snap->ps.stats[STAT_HOLDABLE_ITEM])
	{
		cg.itemSelect = bg_itemlist[cg.snap->ps.stats[STAT_HOLDABLE_ITEM]].giTag;
		cg.invenSelectTime = cg.time;
	}
}

void CG_PrevInventory_f(void)
{
	if ( !cg.snap )
	{
		return;
	}

	if (cg.snap->ps.pm_flags & PMF_FOLLOW)
	{
		return;
	}

	if (cg.predictedPlayerState.pm_type == PM_SPECTATOR)
	{
		return;
	}

	if (cg.itemSelect != -1)
	{
		cg.snap->ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(cg.itemSelect, IT_HOLDABLE);
	}
	BG_CycleInven(&cg.snap->ps, -1);

	if (cg.snap->ps.stats[STAT_HOLDABLE_ITEM])
	{
		cg.itemSelect = bg_itemlist[cg.snap->ps.stats[STAT_HOLDABLE_ITEM]].giTag;
		cg.invenSelectTime = cg.time;
	}
}
