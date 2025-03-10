// win_main.c
//Anything above this #include will be ignored by the compiler
#include "../qcommon/exe_headers.h"

#include "../client/client.h"
#include "win_local.h"
#include "resource.h"
#include <errno.h>
#include <float.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <direct.h>
#include <io.h>
#include <conio.h>
#include "../qcommon/stringed_ingame.h"

#define MEM_THRESHOLD 128*1024*1024

UINT MSH_BROADCASTARGS;
UINT MSG_SHOWNOTIFICATION;

//static char		sys_cmdline[MAX_STRING_CHARS];

/* win_shared.cpp */
void Sys_SetBinaryPath(const char *path);
char *Sys_BinaryPath(void);

/*
==================
Sys_ShowNotification
==================
*/
//#include <CommCtrl.h>
void Sys_ShowNotification( const char *message, const int flags ) {
	if (flags & NOTIFICATION_FLASH) {
		static FLASHWINFO fi = {};
		if (!fi.cbSize) {
			fi.cbSize = sizeof(fi);
			fi.hwnd = g_wv.hWnd;
			fi.dwFlags = FLASHW_ALL | FLASHW_TIMERNOFG;
		}
		FlashWindowEx(&fi);
	}
    
	if (flags & NOTIFICATION_TEXT) {
		static NOTIFYICONDATA nid = {};
		if (!nid.cbSize) {
			nid.cbSize = sizeof(nid);
			nid.hWnd = g_wv.hWnd;
			nid.uFlags = NIF_ICON | NIF_INFO | NIF_MESSAGE;
			nid.uCallbackMessage = MSG_SHOWNOTIFICATION = RegisterWindowMessage("MSG_SHOWNOTIFICATION");
			nid.hIcon = LoadIcon(g_wv.hInstance, MAKEINTRESOURCE(IDI_ICON1));
			Q_strncpyz(nid.szInfo, message, ARRAYSIZE(nid.szInfo));
			Shell_NotifyIcon(NIM_ADD, &nid);
			nid.uVersion = NOTIFYICON_VERSION_4;
			Shell_NotifyIcon(NIM_SETVERSION, &nid);
		} else {
			Q_strncpyz(nid.szInfo, message, ARRAYSIZE(nid.szInfo));
			Shell_NotifyIcon(NIM_MODIFY, &nid);
		}
	}
}

/*
==================
Sys_LowPhysicalMemory()
==================
*/

qboolean Sys_LowPhysicalMemory(void) {
	static MEMORYSTATUSEX stat;
	static qboolean bAsked = qfalse;
	static cvar_t* sys_lowmem = Cvar_Get( "sys_lowmem", "0", 0 );

	if (!bAsked)	// just in case it takes a little time for GlobalMemoryStatusEx() to gather stats on
	{				//	stuff we don't care about such as virtual mem etc.
		bAsked = qtrue;
		GlobalMemoryStatusEx (&stat);
	}
	if (sys_lowmem->integer)
	{
		return qtrue;
	}
	return (stat.ullTotalPhys <= MEM_THRESHOLD) ? qtrue : qfalse;
}

/*
==================
Sys_BeginProfiling
==================
*/
void Sys_BeginProfiling( void ) {
	// this is just used on the mac build
}

/*
=============
Sys_Error

Show the early console as an error dialog
=============
*/
void QDECL Sys_Error( const char *error, ... ) {
	va_list		argptr;
	char		text[4096];
    MSG        msg;

	va_start (argptr, error);
	Q_vsnprintf(text, sizeof(text), error, argptr);
	va_end (argptr);

	Conbuf_AppendText( text );
	Conbuf_AppendText( "\n" );

	Sys_SetErrorText( text );
	Sys_ShowConsole( 1, qtrue );

	timeEndPeriod( 1 );

	IN_Shutdown();

	// wait for the user to quit
	while ( 1 ) {
		if (!GetMessage (&msg, NULL, 0, 0))
			Com_Quit_f ();
		TranslateMessage (&msg);
      	DispatchMessage (&msg);
	}

	Sys_DestroyConsole();
	Com_ShutdownZoneMemory();
 	Com_ShutdownHunkMemory();

	exit (1);
}

/*
==============
Sys_Quit
==============
*/
void Sys_Quit( void ) {
	timeEndPeriod( 1 );
	IN_Shutdown();
	Sys_DestroyConsole();
	Com_ShutdownZoneMemory();
 	Com_ShutdownHunkMemory();

	exit (0);
}

/*
==============
Sys_Print
==============
*/
void Sys_Print( const char *msg ) {
	// TTimo - prefix for text that shows up in console but not in notify
	// backported from RTCW
	if ( !Q_strncmp( msg, "[skipnotify]", 12 ) ) {
		msg += 12;
	}
	if ( msg[0] == '*' ) {
		msg += 1;
	}
	Conbuf_AppendText( msg );
}


/*
==============
Sys_Mkdir
==============
*/
qboolean Sys_Mkdir( const char *path ) {
	if( !CreateDirectory( path, NULL ) )
	{
		if( GetLastError( ) != ERROR_ALREADY_EXISTS )
			return qfalse;
	}
	return qtrue;
}

/*
==============
Sys_Cwd
==============
*/
char *Sys_Cwd( void ) {
	static char cwd[MAX_OSPATH];

	_getcwd( cwd, sizeof( cwd ) - 1 );
	cwd[MAX_OSPATH-1] = 0;

	return cwd;
}

/*
==============
Sys_DefaultCDPath
==============
*/
char *Sys_DefaultCDPath( void ) {
	return "";
}

/* Resolves path names and determines if they are the same */
/* For use with full OS paths not quake paths */
/* Returns true if resulting paths are valid and the same, otherwise false */
bool Sys_PathCmp( const char *path1, const char *path2 ) {
	char *r1, *r2;

	r1 = _fullpath(NULL, path1, MAX_OSPATH);
	r2 = _fullpath(NULL, path2, MAX_OSPATH);

	if(r1 && r2 && !Q_stricmp(r1, r2))
	{
		free(r1);
		free(r2);
		return true;
	}

	free(r1);
	free(r2);
	return false;
}

/*
==============================================================

DIRECTORY SCANNING

==============================================================
*/

#define	MAX_FOUND_FILES	0x1000

void Sys_ListFilteredFiles( const char *basedir, char *subdirs, char *filter, char **psList, int *numfiles ) {
	char		search[MAX_OSPATH], newsubdirs[MAX_OSPATH];
	char		filename[MAX_OSPATH];
	intptr_t	findhandle;
	struct _finddata_t findinfo;

	if ( *numfiles >= MAX_FOUND_FILES - 1 ) {
		return;
	}

	if (strlen(subdirs)) {
		Com_sprintf( search, sizeof(search), "%s\\%s\\*", basedir, subdirs );
	}
	else {
		Com_sprintf( search, sizeof(search), "%s\\*", basedir );
	}

	findhandle = _findfirst (search, &findinfo);
	if (findhandle == -1) {
		return;
	}

	do {
		if (findinfo.attrib & _A_SUBDIR) {
			if (Q_stricmp(findinfo.name, ".") && Q_stricmp(findinfo.name, "..")) {
				if (strlen(subdirs)) {
					Com_sprintf( newsubdirs, sizeof(newsubdirs), "%s\\%s", subdirs, findinfo.name);
				}
				else {
					Com_sprintf( newsubdirs, sizeof(newsubdirs), "%s", findinfo.name);
				}
				Sys_ListFilteredFiles( basedir, newsubdirs, filter, psList, numfiles );
			}
		}
		if ( *numfiles >= MAX_FOUND_FILES - 1 ) {
			break;
		}
		Com_sprintf( filename, sizeof(filename), "%s\\%s", subdirs, findinfo.name );
		if (!Com_FilterPath( filter, filename, qfalse ))
			continue;
		psList[ *numfiles ] = CopyString( filename );
		(*numfiles)++;
	} while ( _findnext (findhandle, &findinfo) != -1 );

	_findclose (findhandle);
}

static qboolean strgtr(const char *s0, const char *s1) {
	int l0, l1, i;

	l0 = strlen(s0);
	l1 = strlen(s1);

	if (l1<l0) {
		l0 = l1;
	}

	for(i=0;i<l0;i++) {
		if (s1[i] > s0[i]) {
			return qtrue;
		}
		if (s1[i] < s0[i]) {
			return qfalse;
		}
	}
	return qfalse;
}

char **Sys_ListFiles( const char *directory, const char *extension, char *filter, int *numfiles, qboolean wantsubs ) {
	char		search[MAX_OSPATH];
	int			nfiles;
	char		**listCopy;
	char		*list[MAX_FOUND_FILES];
	struct _finddata_t findinfo;
	intptr_t	findhandle;
	int			flag;
	int			i;

	if (filter) {

		nfiles = 0;
		Sys_ListFilteredFiles( directory, "", filter, list, &nfiles );

		list[ nfiles ] = 0;
		*numfiles = nfiles;

		if (!nfiles)
			return NULL;

		listCopy = (char **)Z_Malloc( ( nfiles + 1 ) * sizeof( *listCopy ), TAG_FILESYS );
		for ( i = 0 ; i < nfiles ; i++ ) {
			listCopy[i] = list[i];
		}
		listCopy[i] = NULL;

		return listCopy;
	}

	if ( !extension) {
		extension = "";
	}

	// passing a slash as extension will find directories
	if ( extension[0] == '/' && extension[1] == 0 ) {
		extension = "";
		flag = 0;
	} else {
		flag = _A_SUBDIR;
	}

	Com_sprintf( search, sizeof(search), "%s\\*%s", directory, extension );

	// search
	nfiles = 0;

	findhandle = _findfirst (search, &findinfo);
	if (findhandle == -1) {
		*numfiles = 0;
		return NULL;
	}

	do {
		if ( (!wantsubs && flag ^ ( findinfo.attrib & _A_SUBDIR )) || (wantsubs && findinfo.attrib & _A_SUBDIR) ) {
			if ( nfiles == MAX_FOUND_FILES - 1 ) {
				break;
			}
			list[ nfiles ] = CopyString( findinfo.name );
			nfiles++;
		}
	} while ( _findnext (findhandle, &findinfo) != -1 );

	list[ nfiles ] = 0;

	_findclose (findhandle);

	// return a copy of the list
	*numfiles = nfiles;

	if ( !nfiles ) {
		return NULL;
	}

	listCopy = (char **)Z_Malloc( ( nfiles + 1 ) * sizeof( *listCopy ), TAG_FILESYS );
	for ( i = 0 ; i < nfiles ; i++ ) {
		listCopy[i] = list[i];
	}
	listCopy[i] = NULL;

	do {
		flag = 0;
		for(i=1; i<nfiles; i++) {
			if (strgtr(listCopy[i-1], listCopy[i])) {
				char *temp = listCopy[i];
				listCopy[i] = listCopy[i-1];
				listCopy[i-1] = temp;
				flag = 1;
			}
		}
	} while(flag);

	return listCopy;
}

void	Sys_FreeFileList( char **psList ) {
	int		i;

	if ( !psList ) {
		return;
	}

	for ( i = 0 ; psList[i] ; i++ ) {
		Z_Free( psList[i] );
	}

	Z_Free( psList );
}

//========================================================

/*
================
Sys_GetClipboardData

================
*/
char *Sys_GetClipboardData( void ) {
	char *data = NULL;
	char *cliptext;

	if ( OpenClipboard( NULL ) != 0 ) {
		HANDLE hClipboardData;

		if ( ( hClipboardData = GetClipboardData( CF_TEXT ) ) != 0 ) {
			if ( ( cliptext = (char *)GlobalLock( hClipboardData ) ) != 0 ) {
				data = (char *)Z_Malloc( GlobalSize( hClipboardData ) + 1, TAG_CLIPBOARD);
				Q_strncpyz( data, cliptext, GlobalSize( hClipboardData )+1 );
				GlobalUnlock( hClipboardData );
				
				strtok( data, "\n\r\b" );
			}
		}
		CloseClipboard();
	}
	return data;
}


/*
========================================================================

LOAD/UNLOAD DLL

========================================================================
*/

/*
=================
Sys_UnloadDll

=================
*/
void Sys_UnloadDll( void *dllHandle ) {
	if ( !dllHandle ) {
		return;
	}
	if ( !FreeLibrary( (struct HINSTANCE__ *)dllHandle ) ) {
		Com_Error (ERR_FATAL, "Sys_UnloadDll FreeLibrary failed");
	}
}

//make sure the dll can be opened by the file system, then write the
//file back out again so it can be loaded is a library. If the read
//fails then the dll is probably not in the pk3 and we are running
//a pure server -rww
bool Sys_UnpackDLL(const char *name)
{
	void *data;
	fileHandle_t f;
	int len = FS_ReadFile(name, &data);
	int ck;

	if (len < 1)
	{ //failed to read the file (out of the pk3 if pure)
		return false;
	}

	if (FS_FileIsInPAK(name, &ck) == -1)
	{ //alright, it isn't in a pk3 anyway, so we don't need to write it.
		//this is allowable when running non-pure.
		FS_FreeFile(data);
		return true;
	}

	f = FS_FOpenFileWrite( name );
	if ( !f )
	{ //can't open for writing? Might be in use.
		//This is possibly a malicious user attempt to circumvent dll
		//replacement so we won't allow it.
		FS_FreeFile(data);
		return false;
	}

	if (FS_Write( data, len, f ) < len)
	{ //Failed to write the full length. Full disk maybe?
		FS_FreeFile(data);
		return false;
	}

	FS_FCloseFile( f );
	FS_FreeFile(data);

	return true;
}

/*
=================
Sys_LoadDll

First try to load library name from system library path,
from executable path, then fs_basepath.
=================
*/

void *Sys_LoadDll(const char *name, qboolean useSystemLib)
{
	void *dllhandle = NULL;

	if(useSystemLib)
		Com_Printf("Trying to load \"%s\"...\n", name);
	
	if(!useSystemLib || !(dllhandle = Sys_LoadLibrary(name)))
	{
		const char *topDir;
		char libPath[MAX_OSPATH];
        
		topDir = Sys_BinaryPath();
        
		if(!*topDir)
			topDir = ".";
        
		Com_Printf("Trying to load \"%s\" from \"%s\"...\n", name, topDir);
		Com_sprintf(libPath, sizeof(libPath), "%s%c%s", topDir, PATH_SEP, name);
        
		if(!(dllhandle = Sys_LoadLibrary(libPath)))
		{
			const char *basePath = Cvar_VariableString("fs_basepath");
			
			if(!basePath || !*basePath)
				basePath = ".";
			
			if(FS_FilenameCompare(topDir, basePath))
			{
				Com_Printf("Trying to load \"%s\" from \"%s\"...\n", name, basePath);
				Com_sprintf(libPath, sizeof(libPath), "%s%c%s", basePath, PATH_SEP, name);
				dllhandle = Sys_LoadLibrary(libPath);
			}
			
			if(!dllhandle)
			{
				const char *cdPath = Cvar_VariableString("fs_cdpath");

				if(!basePath || !*basePath)
					basePath = ".";

				if(FS_FilenameCompare(topDir, cdPath))
				{
					Com_Printf("Trying to load \"%s\" from \"%s\"...\n", name, cdPath);
					Com_sprintf(libPath, sizeof(libPath), "%s%c%s", cdPath, PATH_SEP, name);
					dllhandle = Sys_LoadLibrary(libPath);
				}

				if(!dllhandle)
				{
					Com_Printf("Loading \"%s\" failed\n", name);
				}
			}
		}
	}
	
	return dllhandle;
}

/*
=================
Sys_LoadGameDll

Used to load a development dll instead of a virtual machine
=================
*/

extern char		*FS_BuildOSPath( const char *base, const char *game, const char *qpath );

/*void * QDECL Sys_LoadGameDll( const char *name, intptr_t (QDECL **entryPoint)(int, ...), intptr_t (QDECL *systemcalls)(intptr_t, ...) ) {*/
void * QDECL Sys_LoadLegacyGameDll( const char *name, entryPoint_t *vmMain, intptr_t (QDECL *systemcalls)(intptr_t, ...) ) {
	HINSTANCE	libHandle;
	void	(QDECL *dllEntry)( intptr_t (QDECL *syscallptr)(intptr_t, ...) );
	char	*basepath;
	char	*homepath;
	char	*cdpath;
	char	*gamedir;
	char	*fn;
	char	filename[MAX_QPATH];
	cvar_t	*fs_game = Cvar_FindVar("fs_game");
	
	Com_sprintf( filename, sizeof( filename ), "%sx86.dll", name );

	//let folder with dlls have a higher priority over pk3 with dlls
	if (!(fs_game && !Q_stricmpn(fs_game->string, "mme", 3)) && !Sys_UnpackDLL(filename)) {
		return NULL;
	}

	libHandle = LoadLibrary( filename );
	if ( !libHandle ) {
		basepath = Cvar_VariableString( "fs_basepath" );
		homepath = Cvar_VariableString( "fs_homepath" );
		cdpath = Cvar_VariableString( "fs_cdpath" );
		gamedir = Cvar_VariableString( "fs_game" );

		fn = FS_BuildOSPath( basepath, gamedir, filename );
		libHandle = LoadLibrary( fn );

		if ( !libHandle ) {
			if( homepath[0] ) {
				fn = FS_BuildOSPath( homepath, gamedir, filename );
				libHandle = LoadLibrary( fn );
			}
			if ( !libHandle ) {
				if( cdpath[0] ) {
					fn = FS_BuildOSPath( cdpath, gamedir, filename );
					libHandle = LoadLibrary( fn );
				}
				if ( !libHandle ) {
					return NULL;
				}
			}
		}
	}

	dllEntry = ( void (QDECL *)( intptr_t (QDECL *)( intptr_t, ... ) ) )GetProcAddress( libHandle, "dllEntry" ); 
	*vmMain = (entryPoint_t)GetProcAddress( libHandle, "vmMain" );
	if ( !*vmMain || !dllEntry ) {
		FreeLibrary( libHandle );
		return NULL;
	}
	dllEntry( systemcalls );

	return libHandle;
}


/*
========================================================================

BACKGROUND FILE STREAMING

========================================================================
*/

#if 1

void Sys_InitStreamThread( void ) {
}

void Sys_ShutdownStreamThread( void ) {
}

void Sys_BeginStreamedFile( fileHandle_t f, int readAhead ) {
}

void Sys_EndStreamedFile( fileHandle_t f ) {
}

int Sys_StreamedRead( void *buffer, int size, int count, fileHandle_t f ) {
   return FS_Read( buffer, size * count, f );
}

void Sys_StreamSeek( fileHandle_t f, int offset, int origin ) {
   FS_Seek( f, offset, origin );
}


#else

typedef struct {
	fileHandle_t	file;
	byte	*buffer;
	qboolean	eof;
	qboolean	active;
	int		bufferSize;
	int		streamPosition;	// next byte to be returned by Sys_StreamRead
	int		threadPosition;	// next byte to be read from file
} streamsIO_t;

typedef struct {
	HANDLE				threadHandle;
	int					threadId;
	CRITICAL_SECTION	crit;
	streamsIO_t			sIO[MAX_FILE_HANDLES];
} streamState_t;

streamState_t	stream;

/*
===============
Sys_StreamThread

A thread will be sitting in this loop forever
================
*/
void Sys_StreamThread( void ) {
	int		buffer;
	int		count;
	int		readCount;
	int		bufferPoint;
	int		r, i;

	while (1) {
		Sleep( 10 );
//		EnterCriticalSection (&stream.crit);

		for (i=1;i<MAX_FILE_HANDLES;i++) {
			// if there is any space left in the buffer, fill it up
			if ( stream.sIO[i].active  && !stream.sIO[i].eof ) {
				count = stream.sIO[i].bufferSize - (stream.sIO[i].threadPosition - stream.sIO[i].streamPosition);
				if ( !count ) {
					continue;
				}

				bufferPoint = stream.sIO[i].threadPosition % stream.sIO[i].bufferSize;
				buffer = stream.sIO[i].bufferSize - bufferPoint;
				readCount = buffer < count ? buffer : count;

				r = FS_Read( stream.sIO[i].buffer + bufferPoint, readCount, stream.sIO[i].file );
				stream.sIO[i].threadPosition += r;

				if ( r != readCount ) {
					stream.sIO[i].eof = qtrue;
				}
			}
		}
//		LeaveCriticalSection (&stream.crit);
	}
}

/*
===============
Sys_InitStreamThread

================
*/
void Sys_InitStreamThread( void ) {
	int i;

	InitializeCriticalSection ( &stream.crit );

	// don't leave the critical section until there is a
	// valid file to stream, which will cause the StreamThread
	// to sleep without any overhead
//	EnterCriticalSection( &stream.crit );

	stream.threadHandle = CreateThread(
	   NULL,	// LPSECURITY_ATTRIBUTES lpsa,
	   0,		// DWORD cbStack,
	   (LPTHREAD_START_ROUTINE)Sys_StreamThread,	// LPTHREAD_START_ROUTINE lpStartAddr,
	   0,			// LPVOID lpvThreadParm,
	   0,			//   DWORD fdwCreate,
	   &stream.threadId);
	for(i=0;i<MAX_FILE_HANDLES;i++) {
		stream.sIO[i].active = qfalse;
	}
}

/*
===============
Sys_ShutdownStreamThread

================
*/
void Sys_ShutdownStreamThread( void ) {
}


/*
===============
Sys_BeginStreamedFile

================
*/
void Sys_BeginStreamedFile( fileHandle_t f, int readAhead ) {
	if ( stream.sIO[f].file ) {
		Sys_EndStreamedFile( stream.sIO[f].file );
	}

	stream.sIO[f].file = f;
	stream.sIO[f].buffer = Z_Malloc( readAhead );
	stream.sIO[f].bufferSize = readAhead;
	stream.sIO[f].streamPosition = 0;
	stream.sIO[f].threadPosition = 0;
	stream.sIO[f].eof = qfalse;
	stream.sIO[f].active = qtrue;

	// let the thread start running
//	LeaveCriticalSection( &stream.crit );
}

/*
===============
Sys_EndStreamedFile

================
*/
void Sys_EndStreamedFile( fileHandle_t f ) {
	if ( f != stream.sIO[f].file ) {
		Com_Error( ERR_FATAL, "Sys_EndStreamedFile: wrong file");
	}
	// don't leave critical section until another stream is started
	EnterCriticalSection( &stream.crit );

	stream.sIO[f].file = 0;
	stream.sIO[f].active = qfalse;

	Z_Free( stream.sIO[f].buffer );

	LeaveCriticalSection( &stream.crit );
}


/*
===============
Sys_StreamedRead

================
*/
int Sys_StreamedRead( void *buffer, int size, int count, fileHandle_t f ) {
	int		available;
	int		remaining;
	int		sleepCount;
	int		copy;
	int		bufferCount;
	int		bufferPoint;
	byte	*dest;

	if (stream.sIO[f].active == qfalse) {
		Com_Error( ERR_FATAL, "Streamed read with non-streaming file" );
	}

	dest = (byte *)buffer;
	remaining = size * count;

	if ( remaining <= 0 ) {
		Com_Error( ERR_FATAL, "Streamed read with non-positive size" );
	}

	sleepCount = 0;
	while ( remaining > 0 ) {
		available = stream.sIO[f].threadPosition - stream.sIO[f].streamPosition;
		if ( !available ) {
			if ( stream.sIO[f].eof ) {
				break;
			}
			if ( sleepCount == 1 ) {
				Com_DPrintf( "Sys_StreamedRead: waiting\n" );
			}
			if ( ++sleepCount > 100 ) {
				Com_Error( ERR_FATAL, "Sys_StreamedRead: thread has died");
			}
			Sleep( 10 );
			continue;
		}

		EnterCriticalSection( &stream.crit );

		bufferPoint = stream.sIO[f].streamPosition % stream.sIO[f].bufferSize;
		bufferCount = stream.sIO[f].bufferSize - bufferPoint;

		copy = available < bufferCount ? available : bufferCount;
		if ( copy > remaining ) {
			copy = remaining;
		}
		memcpy( dest, stream.sIO[f].buffer + bufferPoint, copy );
		stream.sIO[f].streamPosition += copy;
		dest += copy;
		remaining -= copy;

		LeaveCriticalSection( &stream.crit );
	}

	return (count * size - remaining) / size;
}

/*
===============
Sys_StreamSeek

================
*/
void Sys_StreamSeek( fileHandle_t f, int offset, int origin ) {

	// halt the thread
	EnterCriticalSection( &stream.crit );

	// clear to that point
	FS_Seek( f, offset, origin );
	stream.sIO[f].streamPosition = 0;
	stream.sIO[f].threadPosition = 0;
	stream.sIO[f].eof = qfalse;

	// let the thread start running at the new position
	LeaveCriticalSection( &stream.crit );
}

#endif

/*
========================================================================

EVENT LOOP

========================================================================
*/

#define	MAX_QUED_EVENTS		256
#define	MASK_QUED_EVENTS	( MAX_QUED_EVENTS - 1 )

sysEvent_t	eventQue[MAX_QUED_EVENTS];
int			eventHead, eventTail;
byte		sys_packetReceived[MAX_MSGLEN];

/*
================
Sys_QueEvent

A time of 0 will get the current time
Ptr should either be null, or point to a block of data that can
be freed by the game later.
================
*/
void Sys_QueEvent( int time, sysEventType_t type, int value, int value2, int ptrLength, void *ptr ) {
	sysEvent_t	*ev;

	ev = &eventQue[ eventHead & MASK_QUED_EVENTS ];
	if ( eventHead - eventTail >= MAX_QUED_EVENTS ) {
		Com_Printf("Sys_QueEvent: overflow\n");
		// we are discarding an event, but don't leak memory
		if ( ev->evPtr ) {
			Z_Free( ev->evPtr );
		}
		eventTail++;
	}

	eventHead++;

	if ( time == 0 ) {
		time = Sys_Milliseconds();
	}

	ev->evTime = time;
	ev->evType = type;
	ev->evValue = value;
	ev->evValue2 = value2;
	ev->evPtrLength = ptrLength;
	ev->evPtr = ptr;
}

/*
================
Sys_GetEvent

================
*/
sysEvent_t Sys_GetEvent( void ) {
    MSG			msg;
	sysEvent_t	ev;
	char		*s;
	msg_t		netmsg;
	netadr_t	adr;

	// return if we have data
	if ( eventHead > eventTail ) {
		eventTail++;
		return eventQue[ ( eventTail - 1 ) & MASK_QUED_EVENTS ];
	}

	// pump the message loop
	while (PeekMessage (&msg, NULL, 0, 0, PM_NOREMOVE)) {
		if ( !GetMessage (&msg, NULL, 0, 0) ) {
			Com_Quit_f();
		}

		// save the msg time, because wndprocs don't have access to the timestamp
		g_wv.sysMsgTime = msg.time;

		TranslateMessage (&msg);
      	DispatchMessage (&msg);
	}

	// check for console commands
	s = Sys_ConsoleInput();
	if ( s ) {
		char	*b;
		int		len;

		len = strlen( s ) + 1;
		b = (char *)Z_Malloc( len, TAG_EVENT );
		Q_strncpyz( b, s, len );
		Sys_QueEvent( 0, SE_CONSOLE, 0, 0, len, b );
	}

	// check for network packets
	MSG_Init( &netmsg, sys_packetReceived, sizeof( sys_packetReceived ) );
	if ( Sys_GetPacket ( &adr, &netmsg ) ) {
		netadr_t		*buf;
		int				len;

		// copy out to a seperate buffer for qeueing
		// the readcount stepahead is for SOCKS support
		len = sizeof( netadr_t ) + netmsg.cursize - netmsg.readcount;
		buf = (netadr_t *)Z_Malloc( len, TAG_EVENT, qtrue );
		*buf = adr;
		memcpy( buf+1, &netmsg.data[netmsg.readcount], netmsg.cursize - netmsg.readcount );
		Sys_QueEvent( 0, SE_PACKET, 0, 0, len, buf );
	}

	// return if we have data
	if ( eventHead > eventTail ) {
		eventTail++;
		return eventQue[ ( eventTail - 1 ) & MASK_QUED_EVENTS ];
	}

	// create an empty event to return

	memset( &ev, 0, sizeof( ev ) );
	ev.evTime = timeGetTime();

	return ev;
}

//================================================================

/*
=================
Sys_In_Restart_f

Restart the input subsystem
=================
*/
void Sys_In_Restart_f( void ) {
	IN_Shutdown();
	IN_Init();
}


/*
=================
Sys_Net_Restart_f

Restart the network subsystem
=================
*/
void Sys_Net_Restart_f( void ) {
	NET_Restart();
}

/*
================
Sys_Init

Called after the common systems (cvars, files, etc)
are initialized
================
*/
#define OSR2_BUILD_NUMBER 1111
#define WIN98_BUILD_NUMBER 1998

void Sys_Init( void ) {
	// make sure the timer is high precision, otherwise
	// NT gets 18ms resolution
	timeBeginPeriod( 1 );

	Cmd_AddCommand ("in_restart", Sys_In_Restart_f);
	Cmd_AddCommand ("net_restart", Sys_Net_Restart_f);

	g_wv.osversion.dwOSVersionInfoSize = sizeof( g_wv.osversion );

	if (!GetVersionEx (&g_wv.osversion))
		Sys_Error ("Couldn't get OS info");

	if (g_wv.osversion.dwMajorVersion < 4)
		Sys_Error ("This game requires Windows version 4 or greater");
	if (g_wv.osversion.dwPlatformId == VER_PLATFORM_WIN32s)
		Sys_Error ("This game doesn't run on Win32s");

	if ( g_wv.osversion.dwPlatformId == VER_PLATFORM_WIN32_NT )
	{
		Cvar_Set( "arch", "winnt" );
	}
	else if ( g_wv.osversion.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS )
	{
		if ( LOWORD( g_wv.osversion.dwBuildNumber ) >= WIN98_BUILD_NUMBER )
		{
			Cvar_Set( "arch", "win98" );
		}
		else if ( LOWORD( g_wv.osversion.dwBuildNumber ) >= OSR2_BUILD_NUMBER )
		{
			Cvar_Set( "arch", "win95 osr2.x" );
		}
		else
		{
			Cvar_Set( "arch", "win95" );
		}
	}
	else
	{
		Cvar_Set( "arch", "unknown Windows variant" );
	}

	// save out a couple things in rom cvars for the renderer to access
	Cvar_Get( "win_hinstance", va("%i", (int)g_wv.hInstance), CVAR_ROM );
	Cvar_Get( "win_wndproc", va("%i", (int)MainWndProc), CVAR_ROM );

	Cvar_Set( "username", Sys_GetCurrentUser() );

	IN_Init();		// FIXME: not in dedicated?
}

// do a quick mem test to check for any potential future mem problems...
//
void QuickMemTest(void)
{
//	if (!Sys_LowPhysicalMemory())
	{
		const int iMemTestMegs = 128;	// useful search label
		// special test, 
		void *pvData = malloc(iMemTestMegs * 1024 * 1024);
		if (pvData)
		{
			free(pvData);
		}
		else
		{
			// err...
			//
			LPCSTR psContinue = re.Language_IsAsian() ? 
								"Your machine failed to allocate %dMB in a memory test, which may mean you'll have problems running this game all the way through.\n\nContinue anyway?"
								: 
								SE_GetString("CON_TEXT_FAILED_MEMTEST");
								// ( since it's too much hassle doing MBCS code pages and decodings etc for MessageBox command )

			#define GetYesNo(psQuery)	(!!(MessageBox(NULL,psQuery,"Query",MB_YESNO|MB_ICONWARNING|MB_TASKMODAL)==IDYES))
			if (!GetYesNo(va(psContinue,iMemTestMegs)))
			{
				LPCSTR psNoMem = re.Language_IsAsian() ?
								"Insufficient memory to run this game!\n"
								:
								SE_GetString("CON_TEXT_INSUFFICIENT_MEMORY");
								// ( since it's too much hassle doing MBCS code pages and decodings etc for MessageBox command )

				Com_Error( ERR_FATAL, psNoMem );
			}
		}
	}
}

/* Begin Sam Lantinga Public Domain 4/13/98 */

static void UnEscapeQuotes(char *arg)
{
	char *last = NULL;

	while (*arg) {
		if (*arg == '"' && (last != NULL && *last == '\\')) {
			char *c_curr = arg;
			char *c_last = last;

			while (*c_curr) {
				*c_last = *c_curr;
				c_last = c_curr;
				c_curr++;
			}
			*c_last = '\0';
		}
		last = arg;
		arg++;
	}
}

/* Parse a command line buffer into arguments */
static int ParseCommandLine(char *cmdline, char **argv)
{
	char *bufp;
	char *lastp = NULL;
	int argc, last_argc;

	argc = last_argc = 0;
	for (bufp = cmdline; *bufp;) {
		/* Skip leading whitespace */
		while (isspace(*bufp)) {
			++bufp;
		}
		/* Skip over argument */
		if (*bufp == '"') {
			++bufp;
			if (*bufp) {
				if (argv) {
					argv[argc] = bufp;
				}
				++argc;
			}
			/* Skip over word */
			lastp = bufp;
			while (*bufp && (*bufp != '"' || *lastp == '\\')) {
				lastp = bufp;
				++bufp;
			}
		} else {
			if (*bufp) {
				if (argv) {
					argv[argc] = bufp;
				}
				++argc;
			}
			/* Skip over word */
			while (*bufp && !isspace(*bufp)) {
				++bufp;
			}
		}
		if (*bufp) {
			if (argv) {
				*bufp = '\0';
			}
			++bufp;
		}

		/* Strip out \ from \" sequences */
		if (argv && last_argc != argc) {
			UnEscapeQuotes(argv[last_argc]);
		}
		last_argc = argc;
	}
	if (argv) {
		argv[argc] = NULL;
	}
	return (argc);
}

/* End Sam Lantinga Public Domain 4/13/98 */

//=======================================================================
//int	totalMsec, countMsec;

#ifndef DEFAULT_BASEDIR
#	ifdef MACOS_X
#		define DEFAULT_BASEDIR Sys_StripAppBundle(Sys_BinaryPath())
#	else
#		define DEFAULT_BASEDIR Sys_BinaryPath()
#	endif
#endif

#include <Shlobj.h>

typedef struct {
	string extension;
	string desc;
} extensionsTable_t;

static extensionsTable_t et[] = {
	//should we reg it? q3mme and other mme mods use it too
//	{".mme", "MovieMaker's Edition Demo"},
	{".dm_26", "Jedi Academy Demo (v1.01)"},
	{".dm_25", "Jedi Academy Demo (v1.00)"},
};

char *GetStringRegKey(HKEY hkey, const char *valueName) {
	if (!hkey) {
		return NULL;
	}
    static CHAR szBuffer[512];
    DWORD dwBufferSize = sizeof(szBuffer);
    LSTATUS nError = RegQueryValueEx(hkey, valueName, 0, NULL, (LPBYTE)szBuffer, &dwBufferSize);
    if (ERROR_SUCCESS == nError) {
        return szBuffer;
    }
    return NULL;
}

bool AddRegistry(const HKEY key, const char *subkey, const char *value, const char *valueName = NULL) {
	Com_DPrintf(S_COLOR_YELLOW"AddRegistry(%u,%s,%s,%s)\n", key, subkey, value, valueName ? valueName : "NULL");
	HKEY hkey;
	LSTATUS nError = RegOpenKeyEx(key, subkey, 0, KEY_READ, &hkey);
	if (nError != ERROR_SUCCESS && nError != ERROR_FILE_NOT_FOUND) {
		Com_DPrintf(S_COLOR_RED"RegOpenKeyEx(%u,%s,%s) error: %d\n", key, subkey, value, nError);
		return false;
	}
	const char *setValue = GetStringRegKey(hkey, valueName);
	RegCloseKey(hkey);
	//ignore the same value
	if (!setValue || Q_stricmp(setValue, value)) {
		nError = RegCreateKeyEx(key, subkey, 0, 0, 0, KEY_ALL_ACCESS, 0, &hkey, 0);
		if (nError == ERROR_SUCCESS) {
			RegSetValueEx(hkey, valueName, 0, REG_SZ, (BYTE*)value, strlen(value)+1);
			RegCloseKey(hkey);
			return true;
		} else {
			Com_DPrintf(S_COLOR_RED"RegCreateKeyEx(%u,%s,%s) error: %d\n", key, subkey, value, nError);
		}
	}
	return false;
}

void RegisterProtocol(char *program) {
	string action="open";
	string app = program + string(" %1");
	string protocol = "Software\\Classes\\ja";
	string icon = protocol + string("\\DefaultIcon");

	string path=protocol+
				"\\shell\\"+
				action+
				"\\command\\";
	
	bool refresh = false;
	//using | to be able to call all 3 functions even if true got returned
	//register the protocol
	refresh |= AddRegistry(HKEY_CURRENT_USER, protocol.c_str(), "")
	//register application association
	| AddRegistry(HKEY_CURRENT_USER, protocol.c_str(), "", "URL Protocol")
	//register application association
	| AddRegistry(HKEY_CURRENT_USER, path.c_str(), app.c_str())
	//register icon for the protocol
	| AddRegistry(HKEY_CURRENT_USER, icon.c_str(), program);
	if (refresh) {
		SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
	}
}

void RegisterFileTypes(char *program) {
	string action="jaMME";
	bool refresh = false; //once true - forever true
	for (int i = 0; i < ARRAY_LEN(et); i++) {
		string app = program + string(" +set fs_game \"mme\" +set fs_extraGames \"japlus japp\" +demo \"%1\" del");
		string extension = "Software\\Classes\\" + et[i].extension;
		string desc = et[i].desc;
		string icon = extension + string("\\DefaultIcon");

		string path=extension+
					"\\shell\\"+
					action+
					"\\command\\";
	
		//using | to be able to call all 3 functions even if true got returned
		//register the filetype extension
		refresh |= AddRegistry(HKEY_CURRENT_USER, extension.c_str(), desc.c_str())
		//register application association
		| AddRegistry(HKEY_CURRENT_USER, path.c_str(), app.c_str())
		//register icon for the filetype
		| AddRegistry(HKEY_CURRENT_USER, icon.c_str(), program);
	}
	if (refresh) {
		SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
	}
}

static HANDLE mutex;
bool isOtherInstanceRunning(void) {
	mutex = CreateMutex(NULL, TRUE, PROGRAM_MUTEX);
	//prevent multiple instances if we are opening a demo
	if (ERROR_ALREADY_EXISTS == GetLastError()) {
		return true;
	}
	//in the other case just open another sintance
	return false;
}

int main( int argc, char **argv )
{
	int		i;
	char	commandLine[ MAX_STRING_CHARS ] = { 0 };

	Sys_CreateConsole();

	// no abort/retry/fail errors
	SetErrorMode( SEM_FAILCRITICALERRORS );

	// get the initial time base
	Sys_Milliseconds();

	Sys_InitStreamThread();

	Sys_SetBinaryPath( Sys_Dirname( argv[ 0 ] ) );
	Sys_SetDefaultInstallPath( DEFAULT_BASEDIR );

	// Concatenate the command line for passing to Com_Init
	for( i = 1; i < argc; i++ )
	{
		const bool containsSpaces = (strchr(argv[i], ' ') != NULL);
		if (containsSpaces)
			Q_strcat( commandLine, sizeof( commandLine ), "\"" );

		Q_strcat( commandLine, sizeof( commandLine ), argv[ i ] );

		if (containsSpaces)
			Q_strcat( commandLine, sizeof( commandLine ), "\"" );

		Q_strcat( commandLine, sizeof( commandLine ), " " );
	}

	Com_Init( commandLine );
	
	CHAR path[512];
	if (GetModuleFileName(NULL, path, sizeof(path))) {
		RegisterFileTypes(path);
		RegisterProtocol(path);
	}
	Com_Printf("MSH_BROADCASTARGS: %u\n", MSH_BROADCASTARGS);

#if !defined(DEDICATED)
	QuickMemTest();
#endif

	NET_Init();

	// hide the early console since we've reached the point where we
	// have a working graphics subsystems
	if ( !com_dedicated->integer && !com_viewlog->integer ) {
		Sys_ShowConsole( 0, qfalse );
	}
	
	DragAcceptFiles(GetActiveWindow(), TRUE);

    // main game loop
	while( 1 ) {
		// if not running as a game client, sleep a bit
		if ( g_wv.isMinimized || ( com_dedicated && com_dedicated->integer ) ) {
			Sleep( 5 );
		}

#ifdef _DEBUG
		if (!g_wv.activeApp)
		{
			Sleep(50);
		}
#endif // _DEBUG
		// run the game
		Com_Frame();
	}
}

/*
==================
WinMain

==================
*/
int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // should never get a previous instance in Win32
    if ( hPrevInstance ) {
        return 0;
	}
	
//	__asm int 3;
	/* Begin Sam Lantinga Public Domain 4/13/98 */
	TCHAR *text = GetCommandLine();
	char *cmdline = _strdup(text);
	if ( cmdline == NULL ) {
		MessageBox(NULL, "Out of memory - aborting", "Fatal Error", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}
	MSH_BROADCASTARGS = RegisterWindowMessage("MSH_BROADCASTARGS");
	if (isOtherInstanceRunning()) {
		char *bcArgs = strstr(cmdline, "+demo");
		if (!bcArgs)
			bcArgs = strstr(cmdline, "ja:");
		if (bcArgs) {
			Sys_CopySharedData(bcArgs, strlen(bcArgs)+1);
			PostMessage(HWND_BROADCAST, MSH_BROADCASTARGS, 0, 0);
			Sleep(1337);
			free(cmdline);
			ReleaseMutex(mutex);
			CloseHandle(mutex);
			return 0;
		}
	}
	//it's a unique instance so reset current directory to the program path
	//if it was called to open external demo
	TCHAR path[512] = { 0 }, *sep = NULL, *lastSep = NULL;
	DWORD gotPath = GetModuleFileName(NULL, path, sizeof(path));
	if (gotPath) {
		sep = strchr(path, '\\');
		do {
			lastSep = sep;
			sep = strchr(sep+1, '\\');
		} while (sep);
		//cut off the executable name
		if (lastSep && lastSep[0] && (lastSep+1) && lastSep[1]) {
			*(lastSep+1) = '\0';
		}
		SetCurrentDirectory(path);
	}

	int    argc = ParseCommandLine(cmdline, NULL);
	char **argv = (char **)alloca(sizeof(char *) * argc + 1);
	if ( argv == NULL ) {
		MessageBox(NULL, "Out of memory - aborting", "Fatal Error", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}
	ParseCommandLine(cmdline, argv);
	
	/* End Sam Lantinga Public Domain 4/13/98 */

	g_wv.hInstance = hInstance;

	/* Begin Sam Lantinga Public Domain 4/13/98 */

	main(argc, argv);

	free(cmdline);

	ReleaseMutex(mutex);
	CloseHandle(mutex);

	/* End Sam Lantinga Public Domain 4/13/98 */

	// never gets here
	return 0;
}
