// Copyright (C) 2006 Sjoerd van der Berg
//
// cl_demos.c -- Enhanced demo player and server demo recorder

#include "client.h"
#include "../server/server.h"
#include "cl_demos.h"
#include "../qcommon/game_version.h"

demo_t demo;
static entityState_t	demoNullEntityState;
static playerState_t	demoNullPlayerState;

static const char *demoHeader = JK_VERSION " Demo";

static void demoFrameAddString( demoString_t *string, int num, const char *newString) {
	int			dataLeft, len;
	int			i;
	char		cache[sizeof(string->data)];

	if (!newString[0]) {
		string->offsets[num] = 0;
		return;
	}
	len = strlen( newString ) + 1;
	dataLeft = sizeof(string->data) - string->used;
	if (len <= dataLeft) {
		Com_Memcpy( string->data + string->used, newString, len );
		string->offsets[num] = string->used;
		string->used += len;
		return;
	}
	cache[0] = 0;
	string->used = 1;
	for ( i = 0 ; i < MAX_CONFIGSTRINGS ; i++ ) {
		const char * s;
		s = (i == num) ? newString : string->data + string->offsets[i];
		if (!s[0]) {
			string->offsets[i] = 0;
			continue;
		}
		dataLeft = sizeof(cache) - string->used;
		len = strlen( s ) + 1;
		if ( len >= dataLeft) {
			Com_Printf( "Adding configstring %i with %s overflowed buffer", i, s );
			return;
		}
		Com_Memcpy( cache + string->used, s, len );
		string->offsets[i] = string->used;
		string->used += len;
	}
	Com_Memcpy( string->data, cache, string->used );
}

static void demoFrameUnpack( msg_t *msg, demoFrame_t *oldFrame, demoFrame_t *newFrame, qboolean *firstPack ) {
	int last;
	qboolean isDelta = MSG_ReadBits( msg, 1 ) ? qfalse : qtrue;
	if (!isDelta)
		oldFrame = 0;

	newFrame->serverTime = MSG_ReadLong( msg );
	/* Read config strings */
	newFrame->string.data[0] = 0;
	newFrame->string.used = 1;
	last = 0;
	/* Extract config strings */
	while ( 1 ) {
		int i, num = MSG_ReadShort( msg );
		if (!isDelta ) {
			for (i = last;i<num;i++)
				newFrame->string.offsets[i] = 0;
		} else {
			for (i = last;i<num;i++)
				demoFrameAddString( &newFrame->string, i, oldFrame->string.data + oldFrame->string.offsets[i] );
		}
		if (num < MAX_CONFIGSTRINGS) {
			demoFrameAddString( &newFrame->string, num, MSG_ReadBigString( msg ) );
		} else {
			break;
		}
		last = num + 1;
	}
    /* Extract player states */
	Com_Memset( newFrame->clientData, 0, sizeof( newFrame->clientData ));
	last = MSG_ReadByte( msg );
	while (last < MAX_CLIENTS) {
		playerState_t *oldPlayer, *newPlayer;
		playerState_t *oldVeh, *newVeh;
		newFrame->clientData[last] = 1;
		oldPlayer = isDelta && oldFrame->clientData[last] ? &oldFrame->clients[last] : &demoNullPlayerState;
		newPlayer = &newFrame->clients[last];
		MSG_ReadDeltaPlayerstate( msg, oldPlayer, newPlayer );
		if (newPlayer->m_iVehicleNum) {
			oldVeh = isDelta && oldFrame->clientData[last] ? &oldFrame->vehs[last] : &demoNullPlayerState;
			newVeh = &newFrame->vehs[last];
			MSG_ReadDeltaPlayerstate( msg, oldVeh, newVeh, qtrue );
		}
		last = MSG_ReadByte( msg );
	}
	/* Extract entity states */
	last = 0;
	while ( 1 ) {
		int i, num = MSG_ReadBits( msg, GENTITYNUM_BITS );
		entityState_t *oldEntity, *newEntity;	
		if ( isDelta ) {
			for (i = last;i<num;i++)
				if (oldFrame->entities[i].number == i)
					newFrame->entities[i] = oldFrame->entities[i];
				else
					newFrame->entities[i].number = MAX_GENTITIES - 1;
		} else {
			for (i = last;i<num;i++)
				newFrame->entities[i].number = MAX_GENTITIES - 1;
		}
		if (num < MAX_GENTITIES - 1) {
			if (isDelta) {
				oldEntity = &oldFrame->entities[num];
				if (oldEntity->number != num)
					oldEntity = &demoNullEntityState;
			} else {
				oldEntity = &demoNullEntityState;
			}
			newEntity = &newFrame->entities[i];
			MSG_ReadDeltaEntity( msg, oldEntity, newEntity, num );
		} else
			break;
		last = num + 1;
	}
	/* Read the area mask */
	newFrame->areaUsed = MSG_ReadByte( msg );
	MSG_ReadData( msg, newFrame->areamask, newFrame->areaUsed );
	/* Read the command string data */
	newFrame->commandUsed = MSG_ReadLong( msg );
	MSG_ReadData( msg, newFrame->commandData, newFrame->commandUsed );
	/* Extract the entity baselines */
	while ( *firstPack ) {
		byte cmd = MSG_ReadByte( msg );
		if (cmd == svc_EOF)
			break;
		if (cmd == svc_baseline) {
			int num = MSG_ReadBits( msg, GENTITYNUM_BITS );
			if ( num < 0 || num >= MAX_GENTITIES ) {
				Com_Error( ERR_DROP, "Baseline number out of range: %i.\n", num );
			} else {
				entityState_t *newEntity = &newFrame->entityBaselines[num];
				MSG_ReadDeltaEntity( msg, &demoNullEntityState, newEntity, num );
			}
		} else {
			Com_Error( ERR_DROP, "Unknown block %d while unpacking demo frame.\n", cmd );
		}
	}
	*firstPack = qfalse;
}

static void demoFramePack( msg_t *msg, const demoFrame_t *newFrame, const demoFrame_t *oldFrame, qboolean *firstPack ) {
	int i;
	/* Full or delta frame marker */
	MSG_WriteBits( msg, oldFrame ? 0 : 1, 1 );
	MSG_WriteLong( msg, newFrame->serverTime );
	/* Add the config strings */
	for (i = 0;i<MAX_CONFIGSTRINGS;i++) {
		const char *oldString = !oldFrame ? "" : &oldFrame->string.data[oldFrame->string.offsets[i]];
		const char *newString = newFrame->string.data + newFrame->string.offsets[i];
		if (strcmp( oldString, newString)) {
			MSG_WriteShort( msg, i );
			MSG_WriteBigString( msg, newString );
		}
	}
	MSG_WriteShort( msg, MAX_CONFIGSTRINGS );
	/* Add the playerstates */
	for (i=0; i<MAX_CLIENTS; i++) {
		const playerState_t *oldPlayer, *newPlayer;
		const playerState_t *oldVeh, *newVeh;
		if (!newFrame->clientData[i])
			continue;
		oldPlayer = (!oldFrame || !oldFrame->clientData[i]) ? &demoNullPlayerState : &oldFrame->clients[i];
		newPlayer = &newFrame->clients[i];
		MSG_WriteByte( msg, i );
		MSG_WriteDeltaPlayerstate( msg, (playerState_t *)oldPlayer, (playerState_t *)newPlayer );
		if (newPlayer->m_iVehicleNum) {
			oldVeh = (!oldFrame || !oldFrame->clientData[i]) ? &demoNullPlayerState : &oldFrame->vehs[i];
			newVeh = &newFrame->vehs[i];
			MSG_WriteDeltaPlayerstate( msg, (playerState_t *)oldVeh, (playerState_t *)newVeh, qtrue );
		}
	}
	MSG_WriteByte( msg, MAX_CLIENTS );
	/* Add the entities */
	for (i=0; i<MAX_GENTITIES-1; i++) {
		const entityState_t *oldEntity, *newEntity;
		newEntity = &newFrame->entities[i];
		if (oldFrame) {
			oldEntity = &oldFrame->entities[i];
			if (oldEntity->number == (MAX_GENTITIES -1))
				oldEntity = 0;
		} else {
			oldEntity = 0;
		}
		if (newEntity->number != i || newEntity->number >= (MAX_GENTITIES -1)) {
			newEntity = 0;
		} else {
			if (!oldEntity) {
				oldEntity = &demoNullEntityState;
			}
		}
		MSG_WriteDeltaEntity( msg, (entityState_t *)oldEntity, (entityState_t *)newEntity, qtrue );
	}
	MSG_WriteBits( msg, (MAX_GENTITIES-1), GENTITYNUM_BITS );
	/* Add the area mask */
	MSG_WriteByte( msg, newFrame->areaUsed );
	MSG_WriteData( msg, newFrame->areamask, newFrame->areaUsed );
	/* Add the command string data */
	MSG_WriteLong( msg, newFrame->commandUsed );
	MSG_WriteData( msg, newFrame->commandData, newFrame->commandUsed );
	/* Add the entity baselines */
	if (*firstPack) {
		for (i=0; i<MAX_GENTITIES; i++) {
			const entityState_t *newEntity = &newFrame->entityBaselines[i];
			if ( !newEntity->number ) {
				continue;
			}
			MSG_WriteByte ( msg, svc_baseline );
			MSG_WriteDeltaEntity( msg, &demoNullEntityState, (entityState_t *)newEntity, qtrue );
		}
		MSG_WriteByte( msg, svc_EOF ); //using it for correct break in unpack
		*firstPack = qfalse;
	}
}

static void demoFrameInterpolate( demoFrame_t frames[], int frameCount, int index ) {
	int i;
	demoFrame_t *workFrame;

	workFrame = &frames[index % frameCount];
//	return;

	for (i=0; i<MAX_CLIENTS; i++) {
		entityState_t *workEntity;
		workEntity = &workFrame->entities[i];
		if (workEntity->number != ENTITYNUM_NONE && !workFrame->entityData[i]) {
			int m;
			demoFrame_t *prevFrame, *nextFrame;
			prevFrame = nextFrame = 0;
			for (m = index - 1; (m > (index - frameCount)) && m >0 ; m--) {
				demoFrame_t *testFrame = &frames[ m % frameCount];
				if ( !testFrame->entityData[i])
					continue;
				if (!testFrame->serverTime || testFrame->serverTime >= workFrame->serverTime)
					break;
				if ( testFrame->entities[i].number != i)
					break;
				if ( (testFrame->entities[i].eFlags ^ workEntity->eFlags) & EF_TELEPORT_BIT )
					break;
				prevFrame = testFrame;
				break;
			}
			for (m = index + 1; (m < (index + frameCount)); m++) {
				demoFrame_t *testFrame = &frames[ m % frameCount];
				if ( !testFrame->entityData[i])
					continue;
				if (!testFrame->serverTime || testFrame->serverTime <= workFrame->serverTime)
					break;
				if ( testFrame->entities[i].number != i)
					break;
				if ( (testFrame->entities[i].eFlags ^ workEntity->eFlags) & EF_TELEPORT_BIT )
					break;
				nextFrame = testFrame;
				break;
			}
			if (prevFrame && nextFrame) {
				const entityState_t *prevEntity = &prevFrame->entities[i];
				const entityState_t *nextEntity = &nextFrame->entities[i];
				float lerp;
				int posDelta;
				float posLerp;
				int	 prevTime, nextTime;
                  
				prevTime = prevFrame->serverTime;
				nextTime = nextFrame->serverTime;
				lerp = (workFrame->serverTime - prevTime) / (float)( nextTime - prevTime );
				posDelta = nextEntity->pos.trTime - prevEntity->pos.trTime;
				if ( posDelta ) {
					workEntity->pos.trTime = prevEntity->pos.trTime + posDelta * lerp;
					posLerp = (workEntity->pos.trTime - prevEntity->pos.trTime) / (float) posDelta;
				} else {
					posLerp = lerp;
				}
				LerpOrigin( prevEntity->pos.trBase, nextEntity->pos.trBase, workEntity->pos.trBase, posLerp );
				LerpOrigin( prevEntity->pos.trDelta, nextEntity->pos.trDelta, workEntity->pos.trDelta, posLerp );

				LerpAngles( prevEntity->apos.trBase, nextEntity->apos.trBase, workEntity->apos.trBase, lerp );
			}
		}
	}
}

const char postEOFMetadataMarker[] = { "HIDDENMETA" };

const char* demoCutReadPossibleMetadata(msg_t* msg) {

	// Normal demo readers will quit here. For all intents and purposes this demo message is over. But we're gonna put the metadata here now. Since it comes after svc_EOF, nobody will ever be bothered by it 
	// but we can read it if we want to.
	const int metaMarkerLength = sizeof(postEOFMetadataMarker) - 1;
	// This is how the demo huffman operates. Worst case a byte can take almost 2 bytes to save, from what I understand. When reading past the end, we need to detect if we SHOULD read past the end.
	// For each byte we need to read, thus, the message length must be at least 2 bytes longer still. Hence at the end we will artificially set the message length to be minimum that long.
	// We will only read x amount of bytes (where x is the length of the meta marker) and see if the meta marker is present. If it is, we then proceeed to read a bigstring.
	// This same thing is technically not true for the custom compressed types (as their size is always the real size of the data) but we'll just leave it like this to be universal and simple.
	const int maxBytePerByteSaved = 2;
	const int metaMarkerPresenceMinimumByteLengthExtra = metaMarkerLength * maxBytePerByteSaved;

	const int requiredCursize = msg->readcount + metaMarkerPresenceMinimumByteLengthExtra; // We'll just set it to this value at the end if it ends up smaller.

	if (msg->cursize < requiredCursize) {
		return NULL;
	}

	for (int i = 0; i < metaMarkerLength; i++) {
		if (msg->cursize < msg->readcount + maxBytePerByteSaved)
		{
			return NULL;
		}
		if (MSG_ReadByte(msg) != postEOFMetadataMarker[i]) {
			return NULL;
		}
	}
	return MSG_ReadBigString(msg);
}

// extremely primitive way to get a value for a key in a json document.
// Will only work properly for getting strings/numbers pointed to by a key. Won't return objects or arrays or whatever.
const char* simpleGetJSONValueForKey(const char* json, const char* key, int depth =1) {
	char valueBufferReal[4][100];
	char* valueBuffer;
	static int	bufferIndex = 0;
	char* buf;
	valueBuffer = (char*)&valueBufferReal[bufferIndex++ & 3];

	int objectDepth = 0;
	bool escaped = false;
	bool wasEscaped = false;
	bool inQuote = false;
	bool thisIsValueString = false;
	int valueStringOutIndex = 0;

	const char* jsonHere = json;
	int keyLength = strlen(key);

	while (*jsonHere) {
		wasEscaped = escaped;
		escaped = false;
		if (*jsonHere == '{' && !wasEscaped) {
			objectDepth++;
		}
		else if (*jsonHere == '}' && !wasEscaped) {
			objectDepth--;
		}
		else if (*jsonHere == '\\' && !wasEscaped) {
			escaped = true;
		}
		else if (*jsonHere == '"' && !wasEscaped) {
			inQuote = !inQuote;
			if (inQuote && objectDepth == 1) {
				if (!Q_stricmpn(jsonHere+1, key, keyLength)) {
					// Maybe we found our key. Check.
					const char* jsonFoundMaybe = jsonHere + keyLength+1;
					if (*jsonFoundMaybe != '"') { // this isn't the end of what might be a key. keep searching.
						jsonHere++;
						continue;
					}
					jsonFoundMaybe++;
					while (*jsonFoundMaybe && (*jsonFoundMaybe == '\t' || *jsonFoundMaybe == '\n' || *jsonFoundMaybe == '\r' || *jsonFoundMaybe == ' ')) {
						jsonFoundMaybe++; // fast forward through whitespaces
					}
					if (*jsonFoundMaybe != ':') {
						jsonHere++; // It wasn't a key.
						continue;
					}
					jsonFoundMaybe++;
					while (*jsonFoundMaybe && (*jsonFoundMaybe == '\t' || *jsonFoundMaybe == '\n' || *jsonFoundMaybe == '\r' || *jsonFoundMaybe == ' ')) {
						jsonFoundMaybe++; // fast forward through whitespaces
					}
					if (*jsonFoundMaybe == '{' || *jsonFoundMaybe == '[') {
						jsonHere++; // Unsupported. Move on.
						continue;
					}
					if (*jsonFoundMaybe != '"') {
						// This is a number probably.
						// Parse until we find something that ends the number, like a , or } or whitespace
						int outIndex = 0;
						while (*jsonFoundMaybe && *jsonFoundMaybe != ' ' && *jsonFoundMaybe != '}' && *jsonFoundMaybe != ',' && outIndex<(sizeof(valueBufferReal[0])-1)) {
							valueBuffer[outIndex++] = *jsonFoundMaybe;
							*jsonFoundMaybe++;
						}
						valueBuffer[outIndex] = '\0';
						return valueBuffer;
					}
					else {
						// This is a string.
						*jsonFoundMaybe++;
						jsonHere = jsonFoundMaybe;
						thisIsValueString = true;
						valueStringOutIndex = 0;
						continue;
					}
				}
			}
			else if(!inQuote && thisIsValueString) {
				valueBuffer[valueStringOutIndex] = 0;
				return valueBuffer;
			}
		}
		else if (thisIsValueString && inQuote){
			if (valueStringOutIndex < (sizeof(valueBufferReal[0]) - 1)) {
				valueBuffer[valueStringOutIndex++] = *jsonHere;
			}
			else {
				// To long. Return it now in truncated form :(
				valueBuffer[valueStringOutIndex] = 0;
				return valueBuffer;
			}
		}
		jsonHere++;
	}

	return NULL;
}

void demoConvert( const char *oldName, const char *newBaseName, qboolean smoothen ) {
	fileHandle_t	oldHandle = 0;
	fileHandle_t	newHandle = 0;
	int				temp;
	int				oldSize;
	int				msgSequence = 0;
	msg_t			oldMsg;
	byte			oldData[ MAX_MSGLEN ];
	int				oldTime, nextTime, fullTime;
	int				clientNum;
	demoFrame_t		*workFrame;
	int				parseEntitiesNum = 0;
	demoConvert_t	*convert;
	char			bigConfigString[BIG_INFO_STRING];
	int				bigConfigNumber;
	const char		*s;
	clSnapshot_t	*oldSnap = 0;
	clSnapshot_t	*newSnap;
	int				levelCount = 0;
	char			newName[MAX_OSPATH];
	short			rmgDataSize = 0;
	short			flatDataSize = 0;
	short			automapCount = 0;
	int				i = 0;
	qboolean		wasFirstCommandByte = qfalse;
	qboolean		firstCommandByteRead = qfalse;

	oldSize = FS_FOpenFileRead( oldName, &oldHandle, qtrue );
	if (!oldHandle) {
		Com_Printf("Failed to open %s for conversion.\n", oldName);
		return;
	}
	demo.firstPack = qtrue;
	/* Alloc some memory */
	convert = (demoConvert_t *)Z_Malloc( sizeof( demoConvert_t), TAG_GENERAL ); //what tag do we need?
	memset( convert, 0, sizeof(demoConvert_t));
	Com_SetLoadingMsg("Converting the demo...");
	/* Initialize the first workframe's strings */
	while (oldSize > 0) {
		MSG_Init( &oldMsg, oldData, sizeof( oldData ) );
		/* Read the sequence number */
		if (FS_Read( &convert->messageNum, 4, oldHandle) != 4)
			goto conversionerror;
		convert->messageNum = LittleLong( convert->messageNum );
		oldSize -= 4;
		/* Read the message size */
		if (FS_Read( &oldMsg.cursize,4, oldHandle) != 4)
			goto conversionerror;
		oldSize -= 4;
		oldMsg.cursize = LittleLong( oldMsg.cursize );
		/* Negative size signals end of demo */
		if (oldMsg.cursize < 0)
			break;
		if ( oldMsg.cursize > oldMsg.maxsize ) 
			goto conversionerror;
		/* Read the actual message */
		if (FS_Read( oldMsg.data, oldMsg.cursize, oldHandle ) != oldMsg.cursize)
			goto conversionerror;
		oldSize -= oldMsg.cursize;
		// init the bitstream
		MSG_BeginReading( &oldMsg );
		// Skip the reliable sequence acknowledge number
		MSG_ReadLong( &oldMsg );
		//
		// parse the message
		//
		while ( 1 ) {
			byte cmd;
			if ( oldMsg.readcount > oldMsg.cursize ) {
				Com_Printf("Demo conversion, read past end of server message.\n");
				goto conversionerror;
			}
            cmd = MSG_ReadByte( &oldMsg );

			wasFirstCommandByte = (qboolean)!firstCommandByteRead;
			firstCommandByteRead = qtrue;

			if ( cmd == svc_EOF) {
				if (wasFirstCommandByte) {
					// check for hidden meta content
					const char* maybeMeta = demoCutReadPossibleMetadata(&oldMsg);
					if (maybeMeta) {

						Com_Printf("Demo metadata found: %s.\n", maybeMeta);
						const char* prsoValue = simpleGetJSONValueForKey(maybeMeta, "prso"); // Pre-recording start offset.
						if (prsoValue) {
							int prso = atoi(prsoValue);
							Com_Printf("Pre-recording start offset found: %d.\n", prso);
						}
					}
				}
                break;
			}
			workFrame = &convert->frames[ convert->frameIndex % DEMOCONVERTFRAMES ];
			// other commands
			switch ( cmd ) {
			default:
				Com_Printf(S_COLOR_RED"ERROR: CL_ParseServerMessage: Illegible server message\n");
				goto conversionerror;
			case svc_nop:
				break;
			case svc_serverCommand:
				temp = MSG_ReadLong( &oldMsg );
				s = MSG_ReadString( &oldMsg );
				if (temp<=msgSequence)
					break;
//				Com_Printf( " server command %s\n", s );
				msgSequence = temp;
				Cmd_TokenizeString( s );
	
				if ( !Q_stricmp( Cmd_Argv(0), "bcs0" ) ) {
					bigConfigNumber = atoi( Cmd_Argv(1) );
					Q_strncpyz( bigConfigString, Cmd_Argv(2), sizeof( bigConfigString ));
					break;
				}
				if ( !Q_stricmp( Cmd_Argv(0), "bcs1" ) ) {
					Q_strcat( bigConfigString, sizeof( bigConfigString ), Cmd_Argv(2));
					break;
				}
				if ( !Q_stricmp( Cmd_Argv(0), "bcs2" ) ) {
					Q_strcat( bigConfigString, sizeof( bigConfigString ), Cmd_Argv(2));
					demoFrameAddString( &workFrame->string, bigConfigNumber, bigConfigString );
					break;
				}
				if ( !Q_stricmp( Cmd_Argv(0), "cs" ) ) {
					int num = atoi( Cmd_Argv(1) );
					s = Cmd_ArgsFrom( 2 );
					demoFrameAddString( &workFrame->string, num, Cmd_ArgsFrom( 2 ) );	
					break;
				}
				if ( clientNum >= 0 && clientNum < MAX_CLIENTS ) {
					int len = strlen( s ) + 1;
					char *dst;
					if (workFrame->commandUsed + len + 1 > sizeof( workFrame->commandData)) {
						Com_Printf("Overflowed state command data.\n");
						goto conversionerror;
					}
					dst = workFrame->commandData + workFrame->commandUsed;
					*dst = clientNum;
					Com_Memcpy( dst+1, s, len );
					workFrame->commandUsed += len + 1;
				}
				break;
			case svc_gamestate:
				if (newHandle) {
					FS_FCloseFile( newHandle );
					newHandle = 0;
				}
				if (levelCount) {
					demo.firstPack = qtrue;
					Com_sprintf( newName, sizeof( newName ), "%s.%d.mme", newBaseName, levelCount );
				} else {
					Com_sprintf( newName, sizeof( newName ), "%s.mme", newBaseName );
				}
				fullTime = -1;
				clientNum = -1;
				oldTime = -1;
				Com_Memset( convert, 0, sizeof( *convert ));
				convert->frames[0].string.used = 1;
				levelCount++;
				newHandle = FS_FOpenFileWrite( newName );
				if (!newHandle) {
					Com_Printf("Failed to open %s for target conversion target.\n", newName);
					goto conversionerror;
				} else {
					FS_Write ( demoHeader, strlen( demoHeader ), newHandle );
				}
				Com_sprintf( newName, sizeof( newName ), "%s.txt", newBaseName );
				workFrame = &convert->frames[ convert->frameIndex % DEMOCONVERTFRAMES ];
				msgSequence = MSG_ReadLong( &oldMsg );
				while( 1 ) {
					cmd = MSG_ReadByte( &oldMsg );
					if (cmd == svc_EOF)
						break;
					if ( cmd == svc_configstring) {
						int		num;
						const char *s;
						num = MSG_ReadShort( &oldMsg );
						s = MSG_ReadBigString( &oldMsg );
						demoFrameAddString( &workFrame->string, num, s );
					} else if ( cmd == svc_baseline ) {
						int num = MSG_ReadBits( &oldMsg, GENTITYNUM_BITS );
						if ( num < 0 || num >= MAX_GENTITIES ) {
							Com_Printf( "Baseline number out of range: %i.\n", num );
							goto conversionerror;
						}
						MSG_ReadDeltaEntity( &oldMsg, &demoNullEntityState, &convert->entityBaselines[num], num );
						memcpy(&workFrame->entityBaselines[num], &convert->entityBaselines[num], sizeof(entityState_t));
					} else {
						Com_Printf( "Unknown block while converting demo gamestate.\n" );
						goto conversionerror;
					}
				}
				clientNum = MSG_ReadLong( &oldMsg );
				/* Skip the checksum feed */
				MSG_ReadLong( &oldMsg );

				// RMG stuff
				rmgDataSize = MSG_ReadShort(&oldMsg);
				if( rmgDataSize != 0) {
					unsigned char dataBuffer[MAX_HEIGHTMAP_SIZE];
					// height map
					MSG_ReadBits(&oldMsg, 1);
					MSG_ReadData(&oldMsg, dataBuffer, rmgDataSize);

					// Flatten map
					flatDataSize = MSG_ReadShort(&oldMsg);
					MSG_ReadBits(&oldMsg, 1);
					MSG_ReadData(&oldMsg, dataBuffer, flatDataSize);

					// Seed
					MSG_ReadLong(&oldMsg);

					// Automap symbols
					automapCount = MSG_ReadShort(&oldMsg);
					for(i = 0; i < automapCount; i++) {
						MSG_ReadByte(&oldMsg);
						MSG_ReadByte(&oldMsg);
						MSG_ReadLong(&oldMsg);
						MSG_ReadLong(&oldMsg);
					}
				}
				break;
			case svc_snapshot:
				nextTime = MSG_ReadLong( &oldMsg );
				/* Delta number, not needed */
				newSnap = &convert->snapshots[convert->messageNum & PACKET_MASK];
				Com_Memset (newSnap, 0, sizeof(*newSnap));
				newSnap->deltaNum = MSG_ReadByte( &oldMsg );
				newSnap->messageNum = convert->messageNum;
				if (!newSnap->deltaNum) {
					newSnap->deltaNum = -1;
					newSnap->valid = qtrue;		// uncompressed frame
					oldSnap  = NULL;
				} else {
					newSnap->deltaNum = newSnap->messageNum - newSnap->deltaNum;
					oldSnap = &convert->snapshots[newSnap->deltaNum & PACKET_MASK];
					if (!oldSnap->valid) {
						Com_Printf( "Delta snapshot without base.\n" );
//						goto conversionerror;
					} else if (oldSnap->messageNum != newSnap->deltaNum) {
						// The frame that the server did the delta from
						// is too old, so we can't reconstruct it properly.
						Com_Printf ("Delta frame too old.\n");
					} else if ( parseEntitiesNum - oldSnap->parseEntitiesNum > MAX_PARSE_ENTITIES-128 ) {
						Com_Printf ("Delta parseEntitiesNum too old.\n");
					} else {
						newSnap->valid = qtrue;	// valid delta parse
					}
				}

				/* Snapflags, not needed */
				newSnap->snapFlags = MSG_ReadByte( &oldMsg );
				// read areamask
				workFrame->areaUsed = MSG_ReadByte( &oldMsg );
				MSG_ReadData( &oldMsg, workFrame->areamask, workFrame->areaUsed );
				if (clientNum <0 || clientNum >= MAX_CLIENTS) {
					Com_Printf("Got snapshot with invalid client.\n");
					goto conversionerror;
				}
				MSG_ReadDeltaPlayerstate( &oldMsg, oldSnap ? &oldSnap->ps : &demoNullPlayerState, &newSnap->ps );
				if( newSnap->ps.m_iVehicleNum)
					MSG_ReadDeltaPlayerstate( &oldMsg, oldSnap ? &oldSnap->vps : &demoNullPlayerState, &newSnap->vps, qtrue );
				/* Read the individual entities */
				newSnap->parseEntitiesNum = parseEntitiesNum;
				newSnap->numEntities = 0;
				Com_Memset( workFrame->entityData, 0, sizeof( workFrame->entityData ));

				/* The beast that is entity parsing */
				{
				int			newnum;
				entityState_t	*oldstate, *newstate;
				int			oldindex = 0;
				int			oldnum;
				newnum = MSG_ReadBits( &oldMsg, GENTITYNUM_BITS );
				while ( 1 ) {
					// read the entity index number
					if (oldSnap && oldindex < oldSnap->numEntities) {
						oldstate = &convert->parseEntities[(oldSnap->parseEntitiesNum + oldindex) & (MAX_PARSE_ENTITIES-1)];
						oldnum = oldstate->number;
					} else {
						oldstate = 0;
						oldnum = 99999;
					}
					newstate = &convert->parseEntities[parseEntitiesNum];
					if ( !oldstate && (newnum == (MAX_GENTITIES-1))) {
						break;
					} else if ( oldnum < newnum ) {
						*newstate = *oldstate;
						oldindex++;
					} else if (oldnum == newnum) {
						oldindex++;
						MSG_ReadDeltaEntity( &oldMsg, oldstate, newstate, newnum );
						if ( newstate->number != MAX_GENTITIES-1)
							workFrame->entityData[ newstate->number ] = 1;
						newnum = MSG_ReadBits( &oldMsg, GENTITYNUM_BITS );
					} else if (oldnum > newnum) {
						MSG_ReadDeltaEntity( &oldMsg, &convert->entityBaselines[newnum], newstate , newnum );
						memcpy(&workFrame->entityBaselines[newnum], &convert->entityBaselines[newnum], sizeof(entityState_t));
						if ( newstate->number != MAX_GENTITIES-1)
							workFrame->entityData[ newstate->number ] = 1;
						newnum = MSG_ReadBits( &oldMsg, GENTITYNUM_BITS );
					}
					if (newstate->number == MAX_GENTITIES-1)
						continue;
					parseEntitiesNum++;
					parseEntitiesNum &= (MAX_PARSE_ENTITIES-1);
					newSnap->numEntities++;
				}}
				/* Stop processing this further since it's an invalid snap due to lack of delta data */
				if (!newSnap->valid)
					break;

				/* Skipped snapshots will be set invalid in the circular buffer */
				if ( newSnap->messageNum - convert->lastMessageNum >= PACKET_BACKUP ) {
					convert->lastMessageNum = newSnap->messageNum - ( PACKET_BACKUP - 1 );
				}
				for ( ; convert->lastMessageNum < newSnap->messageNum ; convert->lastMessageNum++ ) {
					convert->snapshots[convert->lastMessageNum & PACKET_MASK].valid = qfalse;
				}
				convert->lastMessageNum = newSnap->messageNum + 1;

				/* compress the frame into the new format */
				if (nextTime > oldTime) {
					demoFrame_t *cleanFrame;
					int writeIndex;
					for (temp = 0;temp<newSnap->numEntities;temp++) {
						int p = (newSnap->parseEntitiesNum+temp) & (MAX_PARSE_ENTITIES-1);
						entityState_t *newState = &convert->parseEntities[p];
						workFrame->entities[newState->number] = *newState;
					}
					workFrame->clientData[clientNum] = 1;
					workFrame->clients[clientNum] = newSnap->ps;
					workFrame->vehs[clientNum] = newSnap->vps;
					workFrame->serverTime = nextTime;

					/* Which frame from the cache to save */
					writeIndex = convert->frameIndex - (DEMOCONVERTFRAMES/2);
					if (writeIndex >= 0) {
						const demoFrame_t *newFrame;
						msg_t writeMsg;
						// init the message
						MSG_Init( &writeMsg, demo.buffer, sizeof (demo.buffer));
						MSG_Clear( &writeMsg );
						MSG_Bitstream( &writeMsg );
						newFrame = &convert->frames[ writeIndex  % DEMOCONVERTFRAMES];
						if ( smoothen )
							demoFrameInterpolate( convert->frames, DEMOCONVERTFRAMES, writeIndex );
						if ( nextTime > fullTime || writeIndex <= 0 ) {
							/* Plan the next time for a full write */
							fullTime = nextTime + 2000;
							demoFramePack( &writeMsg, newFrame, 0, &demo.firstPack );
						} else {
							const demoFrame_t *oldFrame = &convert->frames[ ( writeIndex -1 ) % DEMOCONVERTFRAMES];
							demoFramePack( &writeMsg, newFrame, oldFrame, &demo.firstPack );
						}
						/* Write away the new data in the msg queue */
						temp = LittleLong( writeMsg.cursize );
						FS_Write (&temp, 4, newHandle );
						FS_Write ( writeMsg.data , writeMsg.cursize, newHandle );
					}

					/* Clean up the upcoming frame for all new changes */
					convert->frameIndex++;
					cleanFrame = &convert->frames[ convert->frameIndex % DEMOCONVERTFRAMES];
					cleanFrame->serverTime = 0;
					for (temp = 0;temp<MAX_GENTITIES;temp++)
						cleanFrame->entities[temp].number = MAX_GENTITIES-1;
					Com_Memset( cleanFrame->clientData, 0, sizeof ( cleanFrame->clientData ));
					Com_Memcpy( cleanFrame->string.data, workFrame->string.data, workFrame->string.used );
					Com_Memcpy( cleanFrame->string.offsets, workFrame->string.offsets, sizeof( workFrame->string.offsets ));
					cleanFrame->string.used = workFrame->string.used;
					cleanFrame->commandUsed = 0;
					/* keep track of this last frame's time */
					oldTime = nextTime;
				}
				break;
			case svc_download:
				// read block number
				temp = MSG_ReadShort ( &oldMsg );
				if (!temp)	//0 block, read file size
					MSG_ReadLong( &oldMsg );
				// read block size
				temp = MSG_ReadShort ( &oldMsg );
				// read the data block
				for ( ;temp>0;temp--)
					MSG_ReadByte( &oldMsg );
				break;
			case svc_setgame:
				i = 0;
				while (i < MAX_QPATH) {
					if (!MSG_ReadByte(&oldMsg))
						break;
					i++;
				}
				break;
			case svc_mapchange:
				// nothing to parse.
				break;
			}
		}
	}
conversionerror:
	FS_FCloseFile( oldHandle );
	FS_FCloseFile( newHandle );
	Z_Free( convert );
	return;
}

void demoCommandSmoothingEnable(qboolean enable) {
	demo.commandSmoothing = enable;
}

qboolean demoCommandSmoothingState(void) {
	return demo.commandSmoothing;
}

static void demoPlayAddCommand( demoPlay_t *play, const char *cmd ) {
	int len = strlen ( cmd ) + 1;
	int index = (++play->commandCount) % DEMO_PLAY_CMDS;
	int freeWrite;
	/* First clear previous command */
	int nextStart = play->commandStart[ index ];
	if (play->commandFree > nextStart) {
		int freeNext = sizeof( play->commandData ) - play->commandFree;
		if ( len <= freeNext ) {
			freeWrite = play->commandFree;
			play->commandFree += len;
		} else {
			if ( len > nextStart )
					Com_Error( ERR_DROP, "Ran out of server command space");
			freeWrite = 0;
			play->commandFree = len;
		}
	} else {
		int left = nextStart - play->commandFree;
		if ( len > left )
			Com_Error( ERR_DROP, "Ran out of server command space");
		freeWrite = play->commandFree;
		play->commandFree = (play->commandFree + len) % sizeof( play->commandData );
	}
	play->commandStart[ index ] = freeWrite;
	Com_Memcpy( play->commandData + freeWrite, cmd, len );
}

static void demoPlaySynch( demoPlay_t *play, demoFrame_t *frame) {
	int i;
	int startCount = play->commandCount;
	int totalLen = 0;
	for (i = 0;i<MAX_CONFIGSTRINGS;i++) {
		char *oldString = cl.gameState.stringData + cl.gameState.stringOffsets[i];
		char *newString = frame->string.data + frame->string.offsets[i];
		if (!strcmp( oldString, newString ))
			continue;
		totalLen += strlen( newString );
		demoPlayAddCommand( play, va("cs %d \"%s\"", i, newString) );
	}
	/* Copy the new server commands */
	for (i=0;i<frame->commandUsed;) {
		char *line = frame->commandData + i;
		int len, cmdClient;

		cmdClient = *line++;
		len = strlen( line ) + 1;
		i += len + 1;
		if ( cmdClient != play->clientNum ) 
			continue;
		demoPlayAddCommand( play, line );
		totalLen += strlen( line );
	}
	if (play->commandCount - startCount > DEMO_PLAY_CMDS) {
		Com_Error( ERR_DROP, "frame added more than %d commands", DEMO_PLAY_CMDS);
	}
//	Com_Printf("Added %d commands, length %d\n", play->commandCount - startCount, totalLen );
}

static void demoPlayForwardFrame( demoPlay_t *play ) {
	int			blockSize;
	msg_t		msg;

	if (play->filePos + 4 > play->fileSize) {
		if (mme_demoAutoNext->integer && demo.nextNum && !demo.precaching) {
			CL_Disconnect_f();
		}
		if (mme_demoAutoQuit->integer && !demo.precaching) {
			if (mme_demoAutoQuit->integer == 2)
				Cbuf_ExecuteText( EXEC_APPEND, "quit" );
			CL_Disconnect_f();
		}
		play->lastFrame = qtrue;
		return;
	}
	play->lastFrame = qfalse;
	play->filePos += 4;
	FS_Read( &blockSize, 4, play->fileHandle );
	blockSize = LittleLong( blockSize );
	FS_Read( demo.buffer, blockSize, play->fileHandle );
	play->filePos += blockSize;
	MSG_Init( &msg, demo.buffer, sizeof(demo.buffer) );
	MSG_BeginReading( &msg );
	msg.cursize = blockSize;
	if (demo.commandSmoothing) {
		play->frame = &play->storageFrame[(play->frameNumber + 1 + FRAME_BUF_SIZE) % FRAME_BUF_SIZE];
		play->nextFrame = &play->storageFrame[(play->frameNumber + 2 + FRAME_BUF_SIZE) % FRAME_BUF_SIZE];
	} else {
		demoFrame_t *copyFrame = play->frame;
		play->frame = play->nextFrame;
		play->nextFrame = copyFrame;
	}
	demoFrameUnpack( &msg, play->frame, play->nextFrame, &demo.firstPack );
	play->frameNumber++;
}

static void demoPlaySetIndex( demoPlay_t *play, int index ) {
	int wantPos;
	if (index <0 || index >= play->fileIndexCount) {
		Com_Printf("demoFile index Out of range search\n");
		index = 0;
	}
	wantPos = play->fileIndex[index].pos;
#if 0
	if ( !play->fileHandle || play->filePos > wantPos) {
		trap_FS_FCloseFile( play->fileHandle );
		trap_FS_FOpenFile( play->fileName, &play->fileHandle, FS_READ );
		play->filePos = 0;
	}
	if (!play->fileHandle)
		Com_Error(0, "Failed to open demo file \n");
	while (play->filePos < wantPos) {
		int toRead = wantPos - play->filePos;
		if (toRead > sizeof(demoBuffer))
			toRead = sizeof(demoBuffer);
		play->filePos += toRead;
		trap_FS_Read( demoBuffer, toRead, play->fileHandle );
	}
#else
	FS_Seek( play->fileHandle, wantPos, FS_SEEK_SET );
	play->filePos = wantPos;
#endif

	if (demo.commandSmoothing)
		play->frameNumber = play->fileIndex[index].frame - 2;

	demoPlayForwardFrame( play );
	demoPlayForwardFrame( play );

	if (!demo.commandSmoothing)
		play->frameNumber = play->fileIndex[index].frame;
}
static int demoPlaySeek(demoPlay_t *play, int seekTime) {
	int i;
	play->seekTime = seekTime;
	seekTime += play->startTime;
	if (seekTime < 0)
		seekTime = 0;
	if (seekTime < play->frame->serverTime || (seekTime > play->frame->serverTime + 1000)) {
		for (i=0;i<(play->fileIndexCount-1);i++) {
			if (play->fileIndex[i].time <= seekTime && 
				play->fileIndex[i+1].time > seekTime)
				goto foundit;
		}
		i = play->fileIndexCount-1;
foundit:
		demoPlaySetIndex(play, i);
	}
	while (!play->lastFrame && (demo.commandSmoothing ? (seekTime >= play->frame->serverTime) : (seekTime > play->nextFrame->serverTime))) {
		demoPlayForwardFrame(play);
	}
	return seekTime;
}
int demoLength(void) {
	demoPlay_t *play = demo.play.handle;
	if (!play)
		return 0;
	return play->endTime - play->startTime;
}
int demoTime(void) {
	demoPlay_t *play = demo.play.handle;
	if (!play)
		return 0;
	if (play->seekTime < 0)
		play->seekTime = 0;
	return play->seekTime;
}
float demoProgress(void) {
	int pos = demoTime();
	int length = demoLength();
	length = length ? length : 1;
	return (float)pos/length;
}
static int demoFindNext(const char *fileName) {
	int i;
	int len = strlen(fileName);
	char name[MAX_QPATH], seekName[MAX_QPATH];
	qboolean tryAgain = qtrue;
	if (len > MAX_QPATH) {
		Com_Printf(S_COLOR_YELLOW"WARNING: Demo name length too big: %d > %d\n", len, MAX_QPATH);
		len = MAX_QPATH;
	}
	Com_sprintf(seekName, MAX_QPATH, fileName);
	if (isdigit(fileName[len-1]) && ((fileName[len-2] == '.'))) {
		seekName[len-2] = 0;
		demo.currentNum = fileName[len-1] - '0';
	} else if (isdigit(fileName[len-1]) && (isdigit(fileName[len-2]) && (fileName[len-3] == '.'))) {
		seekName[len-3] = 0;
		demo.currentNum = (fileName[len-2] - '0')*10 + (fileName[len-1] - '0');
	} else {
		demo.currentNum = demo.nextNum;
		tryAgain = qfalse;
	}
	do {
		for (i = demo.currentNum + 1; i < 100; i++) {
			Com_sprintf(name, MAX_QPATH, "mmedemos/%s.%d.mme", seekName, i);
			if (FS_FileExists(name)) {
				Com_Printf("Next demo file: %s\n", name);
				return i;
			}
		}
		if (tryAgain) {
			Com_sprintf(seekName, MAX_QPATH, fileName);
			tryAgain = qfalse;
		} else {
			break;
		}
	} while (1);
	return 0;
}

static demoPlay_t *demoPlayOpen( const char* fileName ) {
	demoPlay_t	*play;
	fileHandle_t fileHandle;
	int	fileSize, filePos;
	int i;

	msg_t msg;
	fileSize = FS_FOpenFileRead( fileName, &fileHandle, qtrue );
	if (fileHandle<=0) {
		Com_Printf("Failed to open demo file %s \n", fileName );
		return 0;
	}
	filePos = strlen( demoHeader );
	i = FS_Read( &demo.buffer, filePos, fileHandle );
	if ( i != filePos || Q_strncmp( (char *)demo.buffer, demoHeader, filePos )) {
		Com_Printf("demo file %s is wrong version\n", fileName );
		FS_FCloseFile( fileHandle );
		return 0;
	}
	demo.nextNum = demoFindNext(mme_demoFileName->string);
	if ( filePos == fileSize) {
		Com_Printf("demo file %s is empty\n", fileName );
		FS_FCloseFile( fileHandle );
		return 0;
	}
	Com_SetLoadingMsg("Opening the demo...");
	play = (demoPlay_t *)Z_Malloc( sizeof( demoPlay_t ), TAG_GENERAL);//what tag do we need?
	memset( play, 0, sizeof(demoPlay_t) ); // In Q3MME the Z_Malloc doees a memset 0, it doesn't here though. So we gotta do it.
	Q_strncpyz( play->fileName, fileName, sizeof( play->fileName ));
	play->fileSize = fileSize;
	play->frame = &play->storageFrame[0];
	play->nextFrame = &play->storageFrame[1];
	for (i=0;i<DEMO_PLAY_CMDS;i++)
		play->commandStart[i] = i;
	play->commandFree = DEMO_PLAY_CMDS;

	for ( ; filePos<fileSize; ) {
		int blockSize, isFull, serverTime;
		FS_Read( &blockSize, 4, fileHandle );
		blockSize = LittleLong( blockSize );
		if (blockSize > sizeof(demo.buffer)) {
			Com_Printf( "Block too large to be read in.\n");
			goto errorreturn;
		}
		if ( blockSize + filePos > fileSize) {
			Com_Printf( "Block would read past the end of the file.\n");
			goto errorreturn;
		}
		FS_Read( demo.buffer, blockSize, fileHandle );
		MSG_Init( &msg, demo.buffer, sizeof(demo.buffer) );
		MSG_BeginReading( &msg );
		msg.cursize = blockSize;	
		isFull = MSG_ReadBits( &msg, 1 );
		serverTime = MSG_ReadLong( &msg );
		if (!play->startTime)
			play->startTime = serverTime;
		if (isFull) {
			if (play->fileIndexCount < DEMO_MAX_INDEX) {
				play->fileIndex[play->fileIndexCount].pos = filePos;
				play->fileIndex[play->fileIndexCount].frame = play->totalFrames;
				play->fileIndex[play->fileIndexCount].time = serverTime;
				play->fileIndexCount++;
			}
		}
		play->endTime = serverTime;
		filePos += 4 + blockSize;
		play->totalFrames++;
	}
	play->fileHandle = fileHandle;
	demo.firstPack = qtrue;
	demoPlaySetIndex( play, 0 );
	play->clientNum = -1;
	for( i=0;i<MAX_CLIENTS;i++)
		if (play->frame->clientData[i]) {
			play->clientNum = i;
			break;
		}
	return play;
errorreturn:
	Z_Free( play );
	FS_FCloseFile( fileHandle );
	return 0;
}

static void demoPlayStop(demoPlay_t *play) {
	FS_FCloseFile( play->fileHandle );
	if ((mme_demoRemove->integer || demo.del) && FS_FileExists(play->fileName)) {
		FS_FileErase(play->fileName);
	}
	if (demo.del) {
		char demoPath[MAX_QPATH];
		char *ext = Cvar_FindVar("mme_demoExt")->string;
		if (!*ext)
			ext = ".dm_26";
		Com_sprintf(demoPath, sizeof(demoPath), "demos/%s%s", mme_demoFileName->string, ext);
		if (FS_FileExists(demoPath)) {
			FS_FileErase(demoPath);
		}
	}
	Z_Free(play);
	demo.del = qfalse;
}

qboolean demoGetDefaultState(int index, entityState_t *state) {
	demoPlay_t *play = demo.play.handle;
	if (index < 0 || index >= MAX_GENTITIES)
		return qfalse;
	if (!(play->frame->entityBaselines[index].eFlags & EF_PERMANENT))
		return qfalse;
	*state = play->frame->entityBaselines[index];
	return qtrue;
}

extern void CL_ConfigstringModified( void );
qboolean demoGetServerCommand( int cmdNumber ) {
	demoPlay_t *play = demo.play.handle;
	int index = cmdNumber % DEMO_PLAY_CMDS;
	const char *cmd;

	if (cmdNumber < play->commandCount - DEMO_PLAY_CMDS || cmdNumber > play->commandCount )
		return qfalse;
	
	Cmd_TokenizeString( play->commandData + play->commandStart[index] );

	cmd = Cmd_Argv( 0 );
	if ( !strcmp( cmd, "cs" ) ) {
		CL_ConfigstringModified();
	}
	if (!strcmp(cmd, "lchat") && Cmd_Argc() > 3) {
		cvar_t *cg_teamChatsOnly = Cvar_FindVar("cg_teamChatsOnly");
		if (cg_teamChatsOnly && !cg_teamChatsOnly->integer)
			Con_SetFilter(CON_FILTER_CHAT);
	} else if (!strcmp(cmd, "ltchat") && Cmd_Argc() > 3) {
		Con_SetFilter(CON_FILTER_TEAMCHAT);
	} else if (!strcmp(cmd, "chat")) {
		cvar_t *cg_teamChatsOnly = Cvar_FindVar("cg_teamChatsOnly");
		if (cg_teamChatsOnly && !cg_teamChatsOnly->integer)
			Con_SetFilter(CON_FILTER_CHAT);
	} else if (!strcmp(cmd, "tchat")) {
		Con_SetFilter(CON_FILTER_TEAMCHAT);
	} else if (!strcmp(cmd, "print")) {
		Con_SetFilter(CON_FILTER_SERVER);
	}
	return qtrue;
}

int demoSeek( int seekTime ) {
	return demoPlaySeek( demo.play.handle, seekTime );
}

void demoRenderFrame( stereoFrame_t stereo ) {
	VM_Call( cgvm, CG_DRAW_ACTIVE_FRAME, cls.realtime, stereo, 2 );	
}

void demoGetSnapshotNumber( int *snapNumber, int *serverTime ) {
	demoPlay_t *play = demo.play.handle;

	*snapNumber = play->frameNumber + (play->lastFrame ? 0 : 1);
	*serverTime = play->lastFrame ?  play->frame->serverTime : play->nextFrame->serverTime;
}

qboolean demoGetSnapshot( int snapNumber, snapshot_t *snap ) {
	demoPlay_t *play = demo.play.handle;
	demoFrame_t *frame;
	int i;

	if (demo.commandSmoothing && snapNumber < play->frameNumber - FRAME_BUF_SIZE + 2)
		return qfalse;
	if (!demo.commandSmoothing && snapNumber < play->frameNumber)
		return qfalse;
	if (snapNumber > play->frameNumber + 1)
		return qfalse;
	if (snapNumber == play->frameNumber + 1 && play->lastFrame)
		return qfalse;

	if (demo.commandSmoothing)
		frame = &play->storageFrame[snapNumber % FRAME_BUF_SIZE];
	else
		frame = snapNumber == play->frameNumber ? play->frame : play->nextFrame;

	demoPlaySynch( play, frame );
	snap->serverCommandSequence = play->commandCount;
	snap->serverTime = frame->serverTime;
	if (play->clientNum >=0 && play->clientNum < MAX_CLIENTS) {
		snap->ps = frame->clients[ play->clientNum ];
		snap->vps = frame->vehs[ play->clientNum ];
	} else {
		Com_Memset( &snap->ps, 0, sizeof(snap->ps) );
		Com_Memset( &snap->vps, 0, sizeof(snap->vps) );
	}

	snap->numEntities = 0;
	for (i=0;i<MAX_GENTITIES-1 && snap->numEntities < MAX_ENTITIES_IN_SNAPSHOT;i++) {
		if (frame->entities[i].number != i)
			continue;
		/* Skip your own entity if there ever comes server side recording */
		if (frame->entities[i].number == snap->ps.clientNum)
			continue;
		memcpy(&snap->entities[snap->numEntities++], &frame->entities[i], sizeof(entityState_t));
	}
	snap->snapFlags = 0;
	snap->ping = 0;
	snap->numServerCommands = 0;
	Com_Memcpy( snap->areamask, frame->areamask, frame->areaUsed );
	return qtrue;
}

void CL_DemoSetCGameTime( void ) {
	/* We never timeout */
	clc.lastPacketTime = cls.realtime;
}

void CL_DemoShutDown( void ) {
	if ( demo.play.handle ) {
		FS_FCloseFile( demo.play.handle->fileHandle );
		Z_Free( demo.play.handle );
		demo.play.handle = 0;
	}
}

void demoStop( void ) {
	if (demo.play.handle) {
		demoPlayStop( demo.play.handle );
	}
	Com_Memset( &demo.play, 0, sizeof( demo.play ));
}

static void demoPrecacheModel(char *str) {
	char skinName[MAX_QPATH], modelName[MAX_QPATH];
	char *p;
	qhandle_t skinID = 0;
	int i = 0;
	
	strcpy(modelName, str);
	if (strstr(modelName, ".glm")) {
		//Check to see if it has a custom skin attached.
		//see if it has a skin name
		p = Q_strrchr(modelName, '*');
		if (p) { //found a *, we should have a model name before it and a skin name after it.
			*p = 0; //terminate the modelName string at this point, then go ahead and parse to the next 0 for the skin.
			p++;
			while (p && *p) {
				skinName[i] = *p;
				i++;
				p++;
			}
			skinName[i] = 0;

			if (skinName[0]) { //got it, register the skin under the model path.
				char baseFolder[MAX_QPATH];

				strcpy(baseFolder, modelName);
				p = Q_strrchr(baseFolder, '/'); //go back to the first /, should be the path point

				if (p) { //got it.. terminate at the slash and register.
					char *useSkinName;
					*p = 0;
					if (strchr(skinName, '|')) {//three part skin
						useSkinName = va("%s/|%s", baseFolder, skinName);
					} else {
						useSkinName = va("%s/model_%s.skin", baseFolder, skinName);
					}
					skinID = re.RegisterSkin(useSkinName);
				}
			}
		}
	}
	re.RegisterModel( str );
}

static void demoPrecacheClient(char *str) {
	const char *v;
	int     team;
	char	modelName[MAX_QPATH];
	char	skinName[MAX_QPATH];
	char		*slash;
	qhandle_t torsoSkin;
	void	  *ghoul2;
	memset(&ghoul2, 0, sizeof(ghoul2));

	v = Info_ValueForKey(str, "t");
	team = atoi(v);
	v = Info_ValueForKey(str, "model");
	Q_strncpyz( modelName, v, sizeof( modelName ) );

	slash = strchr( modelName, '/' );
	if ( !slash ) {
		// modelName did not include a skin name
		Q_strncpyz( skinName, "default", sizeof( skinName ) );
	} else {
		Q_strncpyz( skinName, slash + 1, sizeof( skinName ) );
		// truncate modelName
		*slash = 0;
	}

	// fix for transparent custom skin parts
	if (strchr(skinName, '|')
		&& strstr(skinName,"head")
		&& strstr(skinName,"torso")
		&& strstr(skinName,"lower"))
	{//three part skin
		torsoSkin = re.RegisterSkin(va("models/players/%s/|%s", modelName, skinName));
	} else {
		if (team == TEAM_RED) {
			Q_strncpyz(skinName, "red", sizeof(skinName));
		} else if (team == TEAM_BLUE) {
			Q_strncpyz(skinName, "blue", sizeof(skinName));
		}
		torsoSkin = re.RegisterSkin(va("models/players/%s/model_%s.skin", modelName, skinName));
	}
	re.G2API_InitGhoul2Model((CGhoul2Info_v **)&ghoul2, va("models/players/%s/model.glm", modelName), 0, torsoSkin, 0, 0, 0);
}

static void demoPrecache( void ) {
	demoPlay_t *play = demo.play.handle;
	int latestSequence = 0, time = play->startTime;
	demoPlaySetIndex(play, 0);
	Com_SetLoadingMsg("Precaching the demo...");
	while (!play->lastFrame) {
		demoPlaySynch( play, play->frame );
		while (latestSequence < play->commandCount) {
			if (demoGetServerCommand(++latestSequence)) {
				if ( !Q_stricmp( Cmd_Argv(0), "cs" ) ) {
					int num = atoi( Cmd_Argv(1) );
					char *str = cl.gameState.stringData + cl.gameState.stringOffsets[num];
					if ( num >= CS_MODELS && num < CS_MODELS+MAX_MODELS ) {
						demoPrecacheModel(str);
					} else if ( num >= CS_SOUNDS && num < CS_SOUNDS+MAX_SOUNDS && (str[0] || str[1] == '$') ) {
						if ( str[0] != '*' ) S_StartSound(vec3_origin, 0, CHAN_AUTO, -1, S_RegisterSound( str ));
					} else if ( num >= CS_PLAYERS && num < CS_PLAYERS+MAX_CLIENTS ) {
						demoPrecacheClient(str);
					}	
				}
			}
		}
		time += 50;
		while (!play->lastFrame && time > play->nextFrame->serverTime)
			demoPlayForwardFrame(play);
	}
	S_Update();
	S_StopAllSounds();
	demoPlaySetIndex(play, 0);
}

qboolean demoPlay( const char *fileName, qboolean del ) {
	demo.play.handle = demoPlayOpen( fileName );
	if (demo.play.handle) {
		demoPlay_t *play = demo.play.handle;
		clc.demoplaying = qtrue;
		clc.newDemoPlayer = qtrue;
		clc.serverMessageSequence = 0;
		clc.lastExecutedServerCommand = 0;
		Com_Printf("Opened %s, which has %d seconds and %d frames\n", fileName, (play->endTime - play->startTime) / 1000, play->totalFrames );
		Con_Close();
		
		// wipe local client state
		CL_ClearState();
		cls.state = CA_LOADING;
		// Pump the loop, this may change gamestate!
		Com_EventLoop();
		// starting to load a map so we get out of full screen ui mode
		Cvar_Set("r_uiFullScreen", "0");
		// flush client memory and start loading stuff
		// this will also (re)load the UI
		// if this is a local client then only the client part of the hunk
		// will be cleared, note that this is done after the hunk mark has been set
		CL_FlushMemory();
		// initialize the CGame
		cls.cgameStarted = qtrue;
		// Create the gamestate
		Com_Memcpy( cl.gameState.stringOffsets, play->frame->string.offsets, sizeof( play->frame->string.offsets ));
		Com_Memcpy( cl.gameState.stringData, play->frame->string.data, play->frame->string.used );
		cl.gameState.dataCount = play->frame->string.used;
		if (mme_demoPrecache->integer) {
			demo.precaching = qtrue;
			demoPrecache();
		}
		demo.precaching = qfalse;
		CL_InitCGame();
		cls.state = CA_ACTIVE;
		demo.del = del;
		return qtrue;
	} else if (demo.nextNum) {
		return qtrue;
	} else {
		return qfalse;
	}
}

void CL_MMEDemo_f( void ) {
	const char *cmd = Cmd_Argv( 1 );

	if (Cmd_Argc() == 2) {
		char mmeName[MAX_OSPATH];
		Com_sprintf (mmeName, MAX_OSPATH, "mmedemos/%s.mme", cmd);
		if (FS_FileExists( mmeName )) {
			Cvar_Set( "mme_demoFileName", cmd );
			demoPlay( mmeName );
		} else {
			Com_Printf("%s not found.\n", mmeName );
		}
		return;
	}
	if (!Q_stricmp( cmd, "convert")) {
		demoConvert( Cmd_Argv( 2 ), Cmd_Argv( 3 ), (qboolean)mme_demoSmoothen->integer );
	} else if (!Q_stricmp( cmd, "play")) {
		demoPlay( Cmd_Argv( 2 ) );
	} else {
		Com_Printf("That does not compute...%s\n", cmd );
	}
}

void CL_DemoList_f(void) {
	int len, i;
	char *buf;
	char word[MAX_OSPATH];
	int	index;
	qboolean readName;
	qboolean haveQuote;

	demo.list.count = 0;
	demo.list.index = 0;
	haveQuote = qfalse;

	if (Cmd_Argc() < 2) {
		Com_Printf( "Usage demoList filename.\n");
		Com_Printf( "That file should have lines with demoname projectname.\n" );
		Com_Printf( "These will be played after each other.\n" );
	}
	if (!FS_FileExists( Cmd_Argv(1))) {
		Com_Printf( "Listfile %s doesn't exist\n", Cmd_Argv(1));
		return;
	}
	len = FS_ReadFile( Cmd_Argv(1), (void **)&buf);
	if (!buf) {
		Com_Printf("file %s couldn't be opened\n", Cmd_Argv(1));
		return;
	}
	i = 0;
	index = 0;
	readName = qtrue;
	while( i < len) {
		switch (buf[i]) {
		case '\r':
			break;
		case '"':
			if (!haveQuote) {
				haveQuote = qtrue;
				break;
			}
		case '\n':
		case ' ':
		case '\t':
			if (haveQuote && buf[i] != '"') {
				if (index < (sizeof(word)-1)) {
	              word[index++] = buf[i];  
				}
				break;
			}
			if (!index)
				break;
			haveQuote = qfalse;
			word[index++] = 0;
			if (readName) {
				Com_Memcpy( demo.list.entry[demo.list.count].demoName, word, index );
				readName = qfalse;
			} else {
				if (demo.list.count < DEMOLISTSIZE) {
					Com_Memcpy( demo.list.entry[demo.list.count].projectName, word, index );
					demo.list.count++;
				}
				readName = qtrue;
			}
			index = 0;
			break;
		default:
			if (index < (sizeof(word)-1)) {
              word[index++] = buf[i];  
			}
			break;
		}
		i++;
	}
	/* Handle a final line if any */
	if (!readName && index && demo.list.count < DEMOLISTSIZE) {
		word[index++] = 0;
		Com_Memcpy( demo.list.entry[demo.list.count].projectName, word, index );
		demo.list.count++;
	}

	FS_FreeFile ( buf );
	demo.list.index = 0;
}

void CL_DemoListNext_f(void) {
	if ( demo.list.index < demo.list.count ) {
		const demoListEntry_t *entry = &demo.list.entry[demo.list.index++];
		Cvar_Set( "mme_demoStartProject", entry->projectName );
		Com_Printf( "Starting demo %s with project %s\n",
			entry->demoName, entry->projectName );
		Cmd_ExecuteString( va( "demo \"%s\"\n", entry->demoName ));
	} else if (demo.list.count) {
		Com_Printf( "DemoList:Finished playing %d demos\n", demo.list.count );
		demo.list.count = 0;
		demo.list.index = 0;
		if ( mme_demoListQuit->integer )
			Cbuf_ExecuteText( EXEC_APPEND, "quit" );
	}
	if (mme_demoAutoNext->integer && demo.nextNum) {
		char demoName[MAX_OSPATH];
		if (demo.nextNum == 1) {
			Com_sprintf(demoName, MAX_OSPATH, "%s.1", mme_demoFileName->string);
		} else if (demo.nextNum < 10) {
			Com_sprintf(demoName, strlen(mme_demoFileName->string)-1, mme_demoFileName->string);
			strcat(demoName, va(".%d", demo.nextNum));
		} else if (demo.nextNum < 100) {
			Com_sprintf(demoName, strlen(mme_demoFileName->string)-2, mme_demoFileName->string);
			strcat(demoName, va(".%d", demo.nextNum));
		}
		Cmd_ExecuteString(va("mmeDemo \"%s\"\n", demoName));
	}
}