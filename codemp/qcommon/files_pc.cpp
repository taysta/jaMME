/*****************************************************************************
 * name:		files_pc.cpp
 *
 * desc:		PC-specific file code
 *
 *****************************************************************************/

//Anything above this #include will be ignored by the compiler
#include "../qcommon/exe_headers.h"

#include "../client/client.h"
#include "files.h"

#include "platform.h"

// TTimo - https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=540
// wether we did a reorder on the current search path when joining the server
static qboolean fs_reordered;

/*
=================
FS_PakIsPure
=================
*/
qboolean FS_PakIsPure( pack_t *pack ) {
	int i;

	if ( fs_numServerPaks ) {
    // NOTE TTimo we are matching checksums without checking the pak names
    //   this means you can have the same pk3 as the server under a different name, you will still get through sv_pure validation
    //   (what happens when two pk3's have the same checkums? is it a likely situation?)
    //   also, if there's a wrong checksumed pk3 and autodownload is enabled, the checksum will be appended to the downloaded pk3 name
		for ( i = 0 ; i < fs_numServerPaks ; i++ ) {
			// FIXME: also use hashed file names
			if ( pack->checksum == fs_serverPaks[i] ) {
				return qtrue;		// on the aproved list
			}
		}
		return qfalse;	// not on the pure server pak list
	}
	return qtrue;
}

/*
================
return a hash value for the filename
================
*/
static long FS_HashFileName( const char *fname, int hashSize ) {
	int		i;
	long	hash;
	char	letter;

	hash = 0;
	i = 0;
	while (fname[i] != '\0') {
		letter = tolower(fname[i]);
		if (letter =='.') break;				// don't include extension
		if (letter =='\\') letter = '/';		// damn path names
		if (letter == PATH_SEP) letter = '/';		// damn path names
		hash+=(long)(letter)*(i+119);
		i++;
	}
	hash = (hash ^ (hash >> 10) ^ (hash >> 20));
	hash &= (hashSize-1);
	return hash;
}


static FILE	*FS_FileForHandle( fileHandle_t f ) {
	if ( f < 1 || f >= MAX_FILE_HANDLES ) {
		Com_Error( ERR_DROP, "FS_FileForHandle: out of reange" );
	}
	if (fsh[f].zipFile == qtrue) {
		Com_Error( ERR_DROP, "FS_FileForHandle: can't get FILE on zip file" );
	}
	if ( ! fsh[f].handleFiles.file.o ) {
		Com_Error( ERR_DROP, "FS_FileForHandle: NULL" );
	}
	
	return fsh[f].handleFiles.file.o;
}


void	FS_ForceFlush( fileHandle_t f ) {
	FILE *file;

	file = FS_FileForHandle(f);
	setvbuf( file, NULL, _IONBF, 0 );
}


/*
================
FS_filelength

If this is called on a non-unique FILE (from a pak file),
it will return the size of the pak file, not the expected
size of the file.
================
*/
int FS_filelength( fileHandle_t f ) {
	int		pos;
	int		end;
	FILE*	h;

	h = FS_FileForHandle(f);
	pos = ftell (h);
	fseek (h, 0, SEEK_END);
	end = ftell (h);
	fseek (h, pos, SEEK_SET);

	return end;
}

/*
============
FS_CreatePath

Creates any directories needed to store the given filename
============
*/
qboolean FS_CreatePath (char *OSPath) {
	char	*ofs;
	char	path[MAX_OSPATH];
	
	// make absolutely sure that it can't back up the path
	// FIXME: is c: allowed???
	if ( strstr( OSPath, ".." ) || strstr( OSPath, "::" ) ) {
		Com_Printf( "WARNING: refusing to create relative path \"%s\"\n", OSPath );
		return qtrue;
	}

	Q_strncpyz( path, OSPath, sizeof( path ) );
	FS_ReplaceSeparators( path );

	// Skip creation of the root directory as it will always be there
	ofs = strchr( path, PATH_SEP );
	ofs++;

	for (; ofs != NULL && *ofs ; ofs++) {
		if (*ofs == PATH_SEP) {
			// create the directory
			*ofs = 0;
			if (!Sys_Mkdir (path)) {
				Com_Error( ERR_FATAL, "FS_CreatePath: failed to create path \"%s\"",
					path );
			}
			*ofs = PATH_SEP;
		}
	}
	return qfalse;
}

/*
=================
FS_CopyFile

Copy a fully specified file from one place to another
=================
*/
qboolean FS_CopyFileAbsolute(char *fromOSPath, char *toOSPath) {
	FILE	*f;
	int		len;
	byte	*buf;

	Com_DPrintf( "copy %s to %s\n", fromOSPath, toOSPath );

	if (strstr(fromOSPath, "journal.dat") || strstr(fromOSPath, "journaldata.dat")) {
		Com_Printf( "Ignoring journal files\n");
		return qfalse;
	}

	f = fopen( fromOSPath, "rb" );
	if ( !f ) {
		return qfalse;
	}
	fseek (f, 0, SEEK_END);
	len = ftell (f);
	fseek (f, 0, SEEK_SET);

	// we are using direct malloc instead of Z_Malloc here, so it
	// probably won't work on a mac... Its only for developers anyway...
	buf = (unsigned char *)malloc( len );
	if (fread( buf, 1, len, f ) != len)
		Com_Error( ERR_FATAL, "Short read in FS_Copyfiles()\n" );
	fclose( f );

	if( FS_CreatePath( toOSPath ) ) {
		return qfalse;
	}

	f = fopen(FS_BuildOSPath( fs_homepath->string, fs_gamedir, toOSPath ), "wb");
	if (!f) {
		return qfalse;
	}
	if (fwrite( buf, 1, len, f ) != len)
		Com_Error( ERR_FATAL, "Short write in FS_Copyfiles()\n" );
	fclose( f );
	free( buf );
	return qtrue;
}
/*
=================
FS_CopyFile

Copy a fully specified file from one place to another
=================
*/
qboolean FS_CopyFile( char *fromOSPath, char *toOSPath, char *newOSPath, const int newSize ) {
	FILE	*f;
	int		len;
	byte	*buf;
	int		fileCount = 1;
	char	*lExt, nExt[MAX_QPATH];
	char	stripped[MAX_QPATH];

	Com_DPrintf( "copy %s to %s\n", fromOSPath, toOSPath );

	if (strstr(fromOSPath, "journal.dat") || strstr(fromOSPath, "journaldata.dat")) {
		Com_Printf( "Ignoring journal files\n");
		return qfalse;
	}

	f = fopen( fromOSPath, "rb" );
	if ( !f ) {
		return qfalse;
	}
	fseek (f, 0, SEEK_END);
	len = ftell (f);
	fseek (f, 0, SEEK_SET);

	// we are using direct malloc instead of Z_Malloc here, so it
	// probably won't work on a mac... Its only for developers anyway...
	buf = (unsigned char *)malloc( len );
	if (fread( buf, 1, len, f ) != len)
		Com_Error( ERR_FATAL, "Short read in FS_Copyfiles()\n" );
	fclose( f );

	if( FS_CreatePath( toOSPath ) ) {
		return qfalse;
	}

	// if the file exists then create a new one with (N)
	if (fopen(toOSPath, "rb") && newOSPath && newSize > 0) {
		lExt = strchr(toOSPath, '.');
		if (!lExt) {
			lExt = "";
		}
		while (strchr(lExt+1, '.')) {
			lExt = strchr(lExt+1, '.');
		}
		Q_strncpyz(nExt, lExt, sizeof(nExt));
		COM_StripExtension(toOSPath, stripped, sizeof(stripped));
		fileCount++;
		while (f = fopen(va("%s (%i)%s", stripped, fileCount, nExt), "rb")) {
			fileCount++;
			fclose(f);
		}
	}
	if (fileCount > 1 && newOSPath && newSize > 0) {
		Q_strncpyz(newOSPath, va("%s (%i)%s", stripped, fileCount, nExt), newSize);
		f = fopen(newOSPath, "wb");
	} else {
		if (newOSPath && newSize > 0) {
			Q_strncpyz(newOSPath, "", newSize);
		}
		f = fopen(toOSPath, "wb");
	}
	if ( !f ) {
		return qfalse;
	}
	if (fwrite( buf, 1, len, f ) != len)
		Com_Error( ERR_FATAL, "Short write in FS_Copyfiles()\n" );
	fclose( f );
	free( buf );
	return qtrue;
}

/*
===========
FS_Remove

===========
*/
void FS_Remove( const char *osPath ) {
	remove( osPath );
}

/*
===========
FS_HomeRemove

===========
*/
void FS_HomeRemove( const char *homePath ) {
	remove( FS_BuildOSPath( fs_homepath->string,
			fs_gamedir, homePath ) );
}

FILE * FS_DirectOpen( const char *name, const char *mode ) {
	char *testpath;

	testpath = FS_BuildOSPath( fs_homepath->string, fs_gamedir, name );

	return fopen( testpath, mode );
}


qboolean FS_FileExistsInPaks(const char *qpath) {
	fileHandle_t f;
	FS_FOpenFileRead(qpath, &f, qfalse);
	if (f) {
		FS_FCloseFile(f);
		return qtrue;
	}
	return qfalse;
}
/*
================
FS_FileExists

Tests if the file exists in the current gamedir, this DOES NOT
search the paths.  This is to determine if opening a file to write
(which always goes into the current gamedir) will cause any overwrites.
NOTE TTimo: this goes with FS_FOpenFileWrite for opening the file afterwards
================
*/
qboolean FS_FileExists( const char *file ) {
	FILE *f;
	char *testpath;

	testpath = FS_BuildOSPath( fs_homepath->string, fs_gamedir, file );

	f = fopen( testpath, "rb" );
	if (f) {
		fclose( f );
		return qtrue;
	}
	return qfalse;
}

qboolean FS_FileErase( const char *file )
{
	char *testpath;

	testpath = FS_BuildOSPath( fs_homepath->string, fs_gamedir, file );

#ifndef _WIN32
	return (qboolean)(unlink( testpath ) == 0);
#else
	return (qboolean)(_unlink( testpath ) == 0);
#endif
}

/*
================
FS_SV_FileExists

Tests if the file exists 
================
*/
qboolean FS_SV_FileExists( const char *file )
{
	FILE *f;
	char *testpath;

	testpath = FS_BuildOSPath( fs_homepath->string, file, "");
	testpath[strlen(testpath)-1] = '\0';

	f = fopen( testpath, "rb" );
	if (f) {
		fclose( f );
		return qtrue;
	}
	return qfalse;
}


/*
===========
FS_SV_FOpenFileWrite

===========
*/
fileHandle_t FS_SV_FOpenFileWrite( const char *filename ) {
	char *ospath;
	fileHandle_t	f;

	if ( !fs_searchpaths ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	ospath = FS_BuildOSPath( fs_homepath->string, filename, "" );
	ospath[strlen(ospath)-1] = '\0';

	f = FS_HandleForFile();
	fsh[f].zipFile = qfalse;

	if ( fs_debug->integer ) {
		Com_Printf( "FS_SV_FOpenFileWrite: %s\n", ospath );
	}

	if( FS_CreatePath( ospath ) ) {
		return 0;
	}

	Com_DPrintf( "writing to: %s\n", ospath );
	fsh[f].handleFiles.file.o = fopen( ospath, "wb" );

	Q_strncpyz( fsh[f].name, filename, sizeof( fsh[f].name ) );

	fsh[f].handleSync = qfalse;
#ifdef USE_AIO
	fsh[f].handleAsync = qfalse;
#endif
	if (!fsh[f].handleFiles.file.o) {
		f = 0;
	}
	return f;
}

/*
===========
FS_SV_FOpenFileRead
search for a file somewhere below the home path, base path or cd path
we search in that order, matching FS_SV_FOpenFileRead order
===========
*/
int FS_SV_FOpenFileRead( const char *filename, fileHandle_t *fp ) {
	char *ospath;
	fileHandle_t	f = 0; // bk001129 - from cvs1.17

	if ( !fs_searchpaths ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	f = FS_HandleForFile();
	fsh[f].zipFile = qfalse;

	Q_strncpyz( fsh[f].name, filename, sizeof( fsh[f].name ) );

	// don't let sound stutter
	S_ClearSoundBuffer();

  // search homepath
	ospath = FS_BuildOSPath( fs_homepath->string, filename, "" );
	// remove trailing slash
	ospath[strlen(ospath)-1] = '\0';

	if ( fs_debug->integer ) {
		Com_Printf( "FS_SV_FOpenFileRead (fs_homepath): %s\n", ospath );
	}

	fsh[f].handleFiles.file.o = fopen( ospath, "rb" );
	fsh[f].handleSync = qfalse;
#ifdef USE_AIO
	fsh[f].handleAsync = qfalse;
#endif
	if (!fsh[f].handleFiles.file.o)
	{
		// NOTE TTimo on non *nix systems, fs_homepath == fs_basepath, might want to avoid
		if (Q_stricmp(fs_homepath->string,fs_basepath->string))
		{
			// search basepath
			ospath = FS_BuildOSPath( fs_basepath->string, filename, "" );
			ospath[strlen(ospath)-1] = '\0';

			if ( fs_debug->integer )
			{
				Com_Printf( "FS_SV_FOpenFileRead (fs_basepath): %s\n", ospath );
			}

			fsh[f].handleFiles.file.o = fopen( ospath, "rb" );
			fsh[f].handleSync = qfalse;
#ifdef USE_AIO
			fsh[f].handleAsync = qfalse;
#endif

			if ( !fsh[f].handleFiles.file.o )
			{
				f = 0;
			}
		}
	}

	if (!fsh[f].handleFiles.file.o) {
		// search cd path
		ospath = FS_BuildOSPath( fs_cdpath->string, filename, "" );
		ospath[strlen(ospath)-1] = '\0';

		if (fs_debug->integer)
		{
			Com_Printf( "FS_SV_FOpenFileRead (fs_cdpath) : %s\n", ospath );
		}

		fsh[f].handleFiles.file.o = fopen( ospath, "rb" );
		fsh[f].handleSync = qfalse;
#ifdef USE_AIO
		fsh[f].handleAsync = qfalse;
#endif

		if( !fsh[f].handleFiles.file.o ) {
			f = 0;
		}
	}
  
	*fp = f;
	if (f) {
		return FS_filelength(f);
	}
	return 0;
}

/*
===========
FS_SV_Rename

===========
*/
void FS_SV_Rename( const char *from, const char *to ) {
	char			*from_ospath, *to_ospath;

	if ( !fs_searchpaths ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	// don't let sound stutter
	S_ClearSoundBuffer();

	from_ospath = FS_BuildOSPath( fs_homepath->string, from, "" );
	to_ospath = FS_BuildOSPath( fs_homepath->string, to, "" );
	from_ospath[strlen(from_ospath)-1] = '\0';
	to_ospath[strlen(to_ospath)-1] = '\0';

	if ( fs_debug->integer ) {
		Com_Printf( "FS_SV_Rename: %s --> %s\n", from_ospath, to_ospath );
	}

	if (rename( from_ospath, to_ospath )) {
		// Failed, try copying it and deleting the original
		FS_CopyFile ( from_ospath, to_ospath, NULL, 0 );
		FS_Remove ( from_ospath );
	}
}

/*
===========
FS_Rename

===========
*/
void FS_Rename( const char *from, const char *to ) {
	char			*from_ospath, *to_ospath;

	if ( !fs_searchpaths ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	// don't let sound stutter
	S_ClearSoundBuffer();

	from_ospath = FS_BuildOSPath( fs_homepath->string, fs_gamedir, from );
	to_ospath = FS_BuildOSPath( fs_homepath->string, fs_gamedir, to );

	if ( fs_debug->integer ) {
		Com_Printf( "FS_Rename: %s --> %s\n", from_ospath, to_ospath );
	}

	if (rename( from_ospath, to_ospath )) {
		// Failed, try copying it and deleting the original
		FS_CopyFile ( from_ospath, to_ospath, NULL, 0 );
		FS_Remove ( from_ospath );
	}
}

#ifdef USE_AIO
/*
Callback for final write which will free all memory and close the file
(at this point no blocking should be needed).
*/
extern void Com_PushEvent( sysEvent_t *event );
static void aio_completion_handler( sigval_t sigval ) {
	fileHandle_t f = sigval.sival_int;
	sysEvent_t event;
	Com_Memset( &event, 0, sizeof( event ) );
	event.evType = SE_AIO_FCLOSE;
	event.evValue = f;
	Com_PushEvent( &event );
}

void FS_FCloseAio( int handle ) {
	fileHandle_t f = (fileHandle_t) handle;
	if ( f < 1 || f >= MAX_FILE_HANDLES ) {
		Com_Error( ERR_FATAL, "FCloseAio called with invalid handle %d\n", f );
	}
	fileBufferNode_t *node = fsh[f].pending;
	int numPendingWrites = 0;
	if ( !fsh[f].closing ) {
		Com_Printf( "Warning: closing file not set to closing: %d\n", f );
	} else {
		Com_Printf( "Closing file %d (%s)\n", f, fsh[f].name );
	}
	while ( node != NULL ) {
		fileBufferNode_t *nextNode = node->next;
		// busy-wait for completion of the write
		if ( aio_error( &node->cb ) == EINPROGRESS ) {
			numPendingWrites++;
		}
		while ( aio_error( &node->cb ) == EINPROGRESS );
		Z_Free( node->buffer );
		Z_Free( node );
		node = nextNode;
	}
	if ( numPendingWrites > 0 ) {
		Com_Printf( "Waited for %d pending writes to complete before closing\n", numPendingWrites );
	}
	fclose (fsh[f].handleFiles.file.o);
	Z_Free( fsh[f].pendingBuffer.buffer );
	Com_Memset( &fsh[f], 0, sizeof( fsh[f] ) );
}
#endif

/*
==============
FS_FCloseFile

If the FILE pointer is an open pak file, leave it open.

For some reason, other dll's can't just cal fclose()
on files returned by FS_FOpenFile...
==============
*/
#ifdef USE_AIO
static void FS_WriteAio( const void *buffer, int len, fileHandle_t h, void (*notify_function) (sigval_t) );
#endif
void FS_FCloseFile( fileHandle_t f ) {
	if ( !fs_searchpaths ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	if (fsh[f].streamed) {
		Sys_EndStreamedFile(f);
	}
	if (fsh[f].zipFile == qtrue) {
		unzCloseCurrentFile( fsh[f].handleFiles.file.z );
		if ( fsh[f].handleFiles.unique ) {
			unzClose( fsh[f].handleFiles.file.z );
		}
		Com_Memset( &fsh[f], 0, sizeof( fsh[f] ) );
		return;
	}

	// we didn't find it as a pak, so close it as a unique file
	if (fsh[f].handleFiles.file.o) {
#ifdef USE_AIO
		if ( fsh[f].handleAsync ) {
			if ( fsh[f].closing ) {
				// file is already closing
				Com_Printf( "Ignoring duplicate FCloseFile call for handle %d\n", f );
				return;
			}
			fileBufferNode_t *node = fsh[f].pending;
			int numPendingWrites = 0;
			while ( node != NULL ) {
				if ( aio_error( &node->cb ) == EINPROGRESS ) {
					numPendingWrites++;
				}
				node = node->next;
			}
			numPendingWrites++;
			{
				pendingBuffer_t *pb = &fsh[f].pendingBuffer;
				Com_Printf( "Waiting for %d pending writes to complete before closing\n", numPendingWrites );
				fsh[f].closing = qtrue;
				FS_WriteAio( pb->buffer, pb->bufferOffset, f, aio_completion_handler );
				// need to keep around the data
				return;
			}
		} else
#endif
		{
			fclose (fsh[f].handleFiles.file.o);
		}
	}
	Com_Memset( &fsh[f], 0, sizeof( fsh[f] ) );
}

#ifdef USE_AIO
static size_t BUFFER_SIZE = 64 * 1024;
fileHandle_t FS_FOpenFileWriteAsync( const char *filename ) {
	fileHandle_t f;
	FILE *fp;
	f = FS_FOpenFileWrite( filename );
	if ( f ) {
		fsh[f].handleAsync = qtrue;
		fsh[f].pending = NULL;
		fsh[f].closing = qfalse;
		fsh[f].pendingBuffer.buffer = ( byte * ) Z_Malloc( BUFFER_SIZE, TAG_FILESYS, qtrue );
		fsh[f].pendingBuffer.bufferLen = BUFFER_SIZE;
		fsh[f].pendingBuffer.bufferOffset = 0;
		fp = FS_FileForHandle( f );
		// need to set O_APPEND in order to not have every aio_write overwrite the others
		fcntl( fileno( fp ), F_SETFL, O_APPEND );
	}
	return f;
}
#endif
/*
===========
FS_FOpenFileWrite

===========
*/
fileHandle_t FS_FOpenFileWriteAbsolute( const char *qpath ) {
	fileHandle_t	f;

	f = FS_HandleForFile();
	fsh[f].zipFile = qfalse;

	// enabling the following line causes a recursive function call loop
	// when running with +set logfile 1 +set developer 1
	//Com_DPrintf( "writing to: %s\n", ospath );
	fsh[f].handleFiles.file.o = fopen( qpath, "wb" );

	Q_strncpyz( fsh[f].name, qpath, sizeof( fsh[f].name ) );

	fsh[f].handleSync = qfalse;
#ifdef USE_AIO
	fsh[f].handleAsync = qfalse;
#endif
	if (!fsh[f].handleFiles.file.o) {
		f = 0;
	}
	return f;
}
/*
===========
FS_FOpenFileWrite

===========
*/
fileHandle_t FS_FOpenFileWrite( const char *qpath ) {
	char			*ospath;
	fileHandle_t	f;

	if ( !fs_searchpaths ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	f = FS_HandleForFile();
	fsh[f].zipFile = qfalse;

	ospath = FS_BuildOSPath( fs_homepath->string, fs_gamedir, qpath );

	if ( fs_debug->integer ) {
		Com_Printf( "FS_FOpenFileWrite: %s\n", ospath );
	}

	if( FS_CreatePath( ospath ) ) {
		return 0;
	}

	// enabling the following line causes a recursive function call loop
	// when running with +set logfile 1 +set developer 1
	//Com_DPrintf( "writing to: %s\n", ospath );
	fsh[f].handleFiles.file.o = fopen( ospath, "wb" );

	Q_strncpyz( fsh[f].name, qpath, sizeof( fsh[f].name ) );

	fsh[f].handleSync = qfalse;
#ifdef USE_AIO
	fsh[f].handleAsync = qfalse;
#endif
	if (!fsh[f].handleFiles.file.o) {
		f = 0;
	}
	return f;
}

fileHandle_t FS_FDirectOpenFileWrite( const char *filename, const char *mode ) {
	char			*ospath;
	fileHandle_t	f;

	if ( !fs_searchpaths ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	f = FS_HandleForFile();
	fsh[f].zipFile = qfalse;

	ospath = FS_BuildOSPath( fs_homepath->string, fs_gamedir, filename );

	if ( fs_debug->integer ) {
		Com_Printf( "FS_FDirectOpenFileWrite: %s\n", ospath );
	}

	if( FS_CreatePath( ospath ) ) {
		return 0;
	}

	// enabling the following line causes a recursive function call loop
	// when running with +set logfile 1 +set developer 1
	//Com_DPrintf( "writing to: %s\n", ospath );
	fsh[f].handleFiles.file.o = fopen( ospath, mode );

	Q_strncpyz( fsh[f].name, filename, sizeof( fsh[f].name ) );

	fsh[f].handleSync = qfalse;
	if (!fsh[f].handleFiles.file.o) {
		f = 0;
	}
	return f;
}


/*
===========
FS_FOpenFileWrite

===========
*/
fileHandle_t FS_FOpenFileReadWrite( const char *filename ) {
	char			*ospath;
	fileHandle_t	f;

	if ( !fs_searchpaths ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	f = FS_HandleForFile();
	fsh[f].zipFile = qfalse;

	ospath = FS_BuildOSPath( fs_homepath->string, fs_gamedir, filename );

	if ( fs_debug->integer ) {
		Com_Printf( "FS_FOpenFileReadWrite: %s\n", ospath );
	}

	if( FS_CreatePath( ospath ) ) {
		return 0;
	}

	// enabling the following line causes a recursive function call loop
	// when running with +set logfile 1 +set developer 1
	//Com_DPrintf( "writing to: %s\n", ospath );
	fsh[f].handleFiles.file.o = fopen( ospath, "r+b" );

	Q_strncpyz( fsh[f].name, filename, sizeof( fsh[f].name ) );

	fsh[f].handleSync = qfalse;
#ifdef USE_AIO
	fsh[f].handleAsync = qfalse;
#endif
	if (!fsh[f].handleFiles.file.o) {
		f = 0;
	}
	return f;
}

/*
===========
FS_FOpenFileAppend

===========
*/
fileHandle_t FS_FOpenFileAppend( const char *filename ) {
	char			*ospath;
	fileHandle_t	f;

	if ( !fs_searchpaths ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	f = FS_HandleForFile();
	fsh[f].zipFile = qfalse;

	Q_strncpyz( fsh[f].name, filename, sizeof( fsh[f].name ) );

	// don't let sound stutter
	S_ClearSoundBuffer();

	ospath = FS_BuildOSPath( fs_homepath->string, fs_gamedir, filename );

	if ( fs_debug->integer ) {
		Com_Printf( "FS_FOpenFileAppend: %s\n", ospath );
	}

	if( FS_CreatePath( ospath ) ) {
		return 0;
	}

	fsh[f].handleFiles.file.o = fopen( ospath, "ab" );
	fsh[f].handleSync = qfalse;
#ifdef USE_AIO
	fsh[f].handleAsync = qfalse;
#endif
	if (!fsh[f].handleFiles.file.o) {
		f = 0;
	}
	return f;
}

#ifdef _WIN32

bool Sys_GetFileTime(LPCSTR psFileName, FILETIME &ft)
{
	bool bSuccess = false;
	HANDLE hFile = INVALID_HANDLE_VALUE;	

	hFile = CreateFile(	psFileName,	// LPCTSTR lpFileName,          // pointer to name of the file
						GENERIC_READ,			// DWORD dwDesiredAccess,       // access (read-write) mode
						FILE_SHARE_READ,		// DWORD dwShareMode,           // share mode
						NULL,					// LPSECURITY_ATTRIBUTES lpSecurityAttributes,	// pointer to security attributes
						OPEN_EXISTING,			// DWORD dwCreationDisposition,  // how to create
						FILE_FLAG_NO_BUFFERING,// DWORD dwFlagsAndAttributes,   // file attributes
						NULL					// HANDLE hTemplateFile          // handle to file with attributes to 
						);

	if (hFile != INVALID_HANDLE_VALUE)
	{			
		if (GetFileTime(hFile,	// handle to file
						NULL,	// LPFILETIME lpCreationTime
						NULL,	// LPFILETIME lpLastAccessTime
						&ft		// LPFILETIME lpLastWriteTime
						)
			)
		{
			bSuccess = true;
		}

		CloseHandle(hFile);
	}

	return bSuccess;
}

bool Sys_FileOutOfDate( LPCSTR psFinalFileName /* dest */, LPCSTR psDataFileName /* src */ )
{
	FILETIME ftFinalFile, ftDataFile;

	if (Sys_GetFileTime(psFinalFileName, ftFinalFile) && Sys_GetFileTime(psDataFileName, ftDataFile))
	{
		// timer res only accurate to within 2 seconds on FAT, so can't do exact compare...
		//
		//LONG l = CompareFileTime( &ftFinalFile, &ftDataFile );
		if (  (abs((double)(ftFinalFile.dwLowDateTime - ftDataFile.dwLowDateTime)) <= 20000000 ) &&
				  ftFinalFile.dwHighDateTime == ftDataFile.dwHighDateTime				
			)
		{
			return false;	// file not out of date, ie use it.
		}
		return true;	// flag return code to copy over a replacement version of this file
	}


	// extra error check, report as suspicious if you find a file locally but not out on the net.,.
	//
	if (com_developer->integer)
	{
		if (!Sys_GetFileTime(psDataFileName, ftDataFile))
		{
			Com_Printf( "Sys_FileOutOfDate: reading %s but it's not on the net!\n", psFinalFileName);
		}
	}

	return false;
}

#endif // _WIN32

bool FS_FileCacheable(const char* const filename) 
{
	extern	cvar_t	*com_buildScript;
	if (com_buildScript && com_buildScript->integer)
	{ 
		return true;
	}
	return( strchr(filename, '/') != 0 );
}

/*
===========
FS_ShiftedStrStr
===========
*/
const char *FS_ShiftedStrStr(const char *string, const char *substring, int shift) {
	char buf[MAX_STRING_TOKENS];
	int i;

	for (i = 0; substring[i]; i++) {
		buf[i] = substring[i] + shift;
	}
	buf[i] = '\0';
	return strstr(string, buf);
}

/*
===========
FS_FOpenFileRead

Finds the file in the search path.
Returns filesize and an open FILE pointer.
Used for streaming data out of either a
separate file or a ZIP file.
===========
*/
extern qboolean		com_fullyInitialized;

long FS_FOpenFileRead( const char *filename, fileHandle_t *file, qboolean uniqueFILE ) {
	searchpath_t	*search;
	char			*netpath;
	pack_t			*pak;
	fileInPack_t	*pakFile;
	directory_t		*dir;
	long			hash;
	unz_s			*zfi;
	void     		*temp;
	int				l;
	char demoExt[16];

	hash = 0;

	if ( !fs_searchpaths ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	if ( file == NULL ) {
		Com_Error( ERR_FATAL, "FS_FOpenFileRead: NULL 'file' parameter passed\n" );
	}

	if ( !filename ) {
		Com_Error( ERR_FATAL, "FS_FOpenFileRead: NULL 'filename' parameter passed\n" );
	}

	Com_sprintf (demoExt, sizeof(demoExt), ".dm_%d", PROTOCOL_VERSION );
	// qpaths are not supposed to have a leading slash
	if ( filename[0] == '/' || filename[0] == '\\' ) {
		filename++;
	}

	// make absolutely sure that it can't back up the path.
	// The searchpaths do guarantee that something will always
	// be prepended, so we don't need to worry about "c:" or "//limbo" 
	if ( strstr( filename, ".." ) || strstr( filename, "::" ) ) {
		*file = 0;
		return -1;
	}

	// make sure the q3key file is only readable by the quake3.exe at initialization
	// any other time the key should only be accessed in memory using the provided functions
	if( com_fullyInitialized && strstr( filename, "q3key" ) ) {
		*file = 0;
		return -1;
	}

	//
	// search through the path, one element at a time
	//

	*file = FS_HandleForFile();
	fsh[*file].handleFiles.unique = uniqueFILE;

	// this new bool is in for an optimisation, if you (eg) opened a BSP file under fs_copyfiles==2,
	//	then it triggered a copy operation to update your local HD version, then this will re-open the
	//	file handle on your local version, not the net build. This uses a bit more CPU to re-do the loop
	//	logic, but should read faster than accessing the net version a second time.
	//
	qboolean bFasterToReOpenUsingNewLocalFile = qfalse;

	do
	{
		bFasterToReOpenUsingNewLocalFile = qfalse;

		for ( search = fs_searchpaths ; search ; search = search->next ) {
			//
			if ( search->pack ) {
				hash = FS_HashFileName(filename, search->pack->hashSize);
			}
			// is the element a pak file?
			if ( search->pack && search->pack->hashTable[hash] ) {
				// disregard if it doesn't match one of the allowed pure pak files
				if ( !FS_PakIsPure(search->pack) ) {
					continue;
				}

				// look through all the pak file elements
				pak = search->pack;
				pakFile = pak->hashTable[hash];
				do {
					// case and separator insensitive comparisons
					if ( !FS_FilenameCompare( pakFile->name, filename ) ) {
						// found it!

						// mark the pak as having been referenced and mark specifics on cgame and ui
						// shaders, txt, arena files  by themselves do not count as a reference as 
						// these are loaded from all pk3s 
						// from every pk3 file.. 
						l = strlen( filename );
						if ( !(pak->referenced & FS_GENERAL_REF)) {
							if ( Q_stricmp(filename + l - 7, ".shader") != 0 &&
								Q_stricmp(filename + l - 4, ".txt") != 0 &&
								Q_stricmp(filename + l - 4, ".str") != 0 &&
								Q_stricmp(filename + l - 4, ".cfg") != 0 &&
								Q_stricmp(filename + l - 4, ".fcf") != 0 &&
								Q_stricmp(filename + l - 7, ".config") != 0 &&
								strstr(filename, "levelshots") == NULL &&
								Q_stricmp(filename + l - 4, ".bot") != 0 &&
								Q_stricmp(filename + l - 6, ".arena") != 0 &&
								Q_stricmp(filename + l - 5, ".menu") != 0) {
								pak->referenced |= FS_GENERAL_REF;
							}
						}

						/*
						FS_ShiftedStrStr(filename, "jampgamex86.dll", -13);
												  //]^&`cZT`Xk+)!W__
						FS_ShiftedStrStr(filename, "cgamex86.dll", -7);
												  //\`Zf^q1/']ee
						FS_ShiftedStrStr(filename, "uix86.dll", -5);
												  //pds31)_gg
						*/

						// jampgame.qvm	- 13
						// ]^&`cZT`X!di`
						if (!(pak->referenced & FS_QAGAME_REF))
						{
							if (FS_ShiftedStrStr(filename, "]T`cZT`X!di`", 13) ||
								FS_ShiftedStrStr(filename, "]T`cZT`Xk+)!W__", 13))
							{
								pak->referenced |= FS_QAGAME_REF;
							}
						}
						// cgame.qvm	- 7
						// \`Zf^'jof
						if (!(pak->referenced & FS_CGAME_REF))
						{
							if (FS_ShiftedStrStr(filename , "\\`Zf^'jof", 7) ||
								FS_ShiftedStrStr(filename , "\\`Zf^q1/']ee", 7))
							{
								pak->referenced |= FS_CGAME_REF;
							}
						}
						// ui.qvm		- 5
						// pd)lqh
						if (!(pak->referenced & FS_UI_REF))
						{
							if (FS_ShiftedStrStr(filename , "pd)lqh", 5) ||
								FS_ShiftedStrStr(filename , "pds31)_gg", 5))
							{
								pak->referenced |= FS_UI_REF;
							}
						}

						if ( uniqueFILE ) {
							// open a new file on the pakfile
							fsh[*file].handleFiles.file.z = unzOpen (pak->pakFilename);
							if (fsh[*file].handleFiles.file.z == NULL) {
								Com_Error (ERR_FATAL, "Couldn't reopen %s", pak->pakFilename);
							}
						} else {
							fsh[*file].handleFiles.file.z = pak->handle;
						}
						Q_strncpyz( fsh[*file].name, filename, sizeof( fsh[*file].name ) );
						fsh[*file].zipFile = qtrue;
						zfi = (unz_s *)fsh[*file].handleFiles.file.z;
						// in case the file was new
						temp = zfi->filestream;
						// set the file position in the zip file (also sets the current file info)
						unzSetOffset(pak->handle, pakFile->pos);
						// copy the file info into the unzip structure
						Com_Memcpy( zfi, pak->handle, sizeof(unz_s) );
						// we copy this back into the structure
						zfi->filestream = temp;
						// open the file in the zip
						unzOpenCurrentFile( fsh[*file].handleFiles.file.z );
						fsh[*file].zipFilePos = pakFile->pos;

						if ( fs_debug->integer ) {
							Com_Printf( "FS_FOpenFileRead: %s (found in '%s')\n", 
								filename, pak->pakFilename );
						}
	#ifndef DEDICATED
	#ifndef FINAL_BUILD
						// Check for unprecached files when in game but not in the menus
						if((cls.state == CA_ACTIVE) && !(Key_GetCatcher( ) & KEYCATCH_UI))
						{
							Com_Printf(S_COLOR_YELLOW "WARNING: File %s not precached\n", filename);
						}
	#endif
	#endif // DEDICATED
						return zfi->cur_file_info.uncompressed_size;
					}
					pakFile = pakFile->next;
				} while(pakFile != NULL);
			} else if ( search->dir ) {
				// check a file in the directory tree

				// if we are running restricted, the only files we
				// will allow to come from the directory are .cfg files
				l = strlen( filename );
		  // FIXME TTimo I'm not sure about the fs_numServerPaks test
		  // if you are using FS_ReadFile to find out if a file exists,
		  //   this test can make the search fail although the file is in the directory
		  // I had the problem on https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=8
		  // turned out I used FS_FileExists instead
				if ( fs_numServerPaks ) {

					if ( Q_stricmp( filename + l - 4, ".cfg" )		// for config files
						&& Q_stricmp( filename + l - 4, ".fcf" )	// force configuration files
						&& Q_stricmp( filename + l - 5, ".menu" )	// menu files
						&& Q_stricmp( filename + l - 5, ".game" )	// menu files
                        && Q_stricmp( filename + l - strlen(demoExt), demoExt )	// menu files
                        && Q_stricmp( filename + l - 4, ".wav" )	// wav audio files
                        && Q_stricmp( filename + l - 4, ".mp3" )	// mp3 audio files
                        && Q_stricmp( filename + l - 4, ".ogg" )	// ogg audio files
                        && Q_stricmp( filename + l - 5, ".flac" )	// flac audio files
                        && Q_stricmp( filename + l - 4, ".dat" ) ) {	// for journal files
						continue;
					}
				}

				dir = search->dir;
				
				netpath = FS_BuildOSPath( dir->path, dir->gamedir, filename );
				fsh[*file].handleFiles.file.o = fopen (netpath, "rb");
				if ( !fsh[*file].handleFiles.file.o ) {
					continue;
				}

				if ( Q_stricmp( filename + l - 4, ".cfg" )		// for config files
					&& Q_stricmp( filename + l - 4, ".fcf" )	// force configuration files
					&& Q_stricmp( filename + l - 5, ".menu" )	// menu files
					&& Q_stricmp( filename + l - 5, ".game" )	// menu files
                    && Q_stricmp( filename + l - strlen(demoExt), demoExt )	// menu files
                    && Q_stricmp( filename + l - 4, ".wav" )	// wav audio files
                    && Q_stricmp( filename + l - 4, ".mp3" )	// mp3 audio files
                    && Q_stricmp( filename + l - 4, ".ogg" )	// ogg audio files
                    && Q_stricmp( filename + l - 5, ".flac" )	// flac audio files
					&& Q_stricmp( filename + l - 4, ".dat" ) ) {	// for journal files
					fs_fakeChkSum = random();
				}
#ifdef _WIN32				
				// if running with fs_copyfiles 2, and search path == local, then we need to fail to open
				//	if the time/date stamp != the network version (so it'll loop round again and use the network path,
				//	which comes later in the search order)
				//
				if ( fs_copyfiles->integer == 2 && fs_cdpath->string[0] && !Q_stricmp( dir->path, fs_basepath->string ) 
					&& FS_FileCacheable(filename) )
				{
					if ( Sys_FileOutOfDate( netpath, FS_BuildOSPath( fs_cdpath->string, dir->gamedir, filename ) ))
					{
						fclose(fsh[*file].handleFiles.file.o);
						fsh[*file].handleFiles.file.o = 0;
						continue;	//carry on to find the cdpath version.
					}
				}      
#endif 
				Q_strncpyz( fsh[*file].name, filename, sizeof( fsh[*file].name ) );
				fsh[*file].zipFile = qfalse;
				if ( fs_debug->integer ) {
					Com_Printf( "FS_FOpenFileRead: %s (found in '%s%c%s')\n", filename,
						dir->path, PATH_SEP, dir->gamedir );
				}

#ifdef _WIN32
				// if we are getting it from the cdpath, optionally copy it
				//  to the basepath
				if ( fs_copyfiles->integer && !Q_stricmp( dir->path, fs_cdpath->string ) ) {
					char	*copypath;

					copypath = FS_BuildOSPath( fs_basepath->string, dir->gamedir, filename );
					switch ( fs_copyfiles->integer )
					{
						default:
						case 1:
						{
							FS_CopyFile( netpath, copypath, NULL, 0 );
						}
						break;

						case 2:
						{
					
							if (FS_FileCacheable(filename) )
							{
								// maybe change this to Com_DPrintf?   On the other hand...
								//
								Com_Printf( "fs_copyfiles(2), Copying: %s to %s\n", netpath, copypath );
								
								FS_CreatePath( copypath );

								bool bOk = true;
								if (!CopyFile( netpath, copypath, FALSE ))
								{
									DWORD dwAttrs = GetFileAttributes(copypath);
									SetFileAttributes(copypath, dwAttrs & ~FILE_ATTRIBUTE_READONLY);
									bOk = !!CopyFile( netpath, copypath, FALSE );
								}

								if (bOk)
								{
									// clear this handle and setup for re-opening of the new local copy...
									//
									bFasterToReOpenUsingNewLocalFile = qtrue;
									fclose(fsh[*file].handleFiles.file.o);
									fsh[*file].handleFiles.file.o = NULL;
								}
							}
						}
						break;
					}
				}
#endif
				if (bFasterToReOpenUsingNewLocalFile)
				{
					break;	// and re-read the local copy, not the net version
				}

	#ifndef DEDICATED
	#ifndef FINAL_BUILD
				// Check for unprecached files when in game but not in the menus
				if((cls.state == CA_ACTIVE) && !(Key_GetCatcher( ) & KEYCATCH_UI))
				{
					Com_Printf(S_COLOR_YELLOW "WARNING: File %s not precached\n", filename);
				}
	#endif
	#endif // dedicated
				return FS_filelength (*file);
			}		
		}
	}
	while ( bFasterToReOpenUsingNewLocalFile );
	
	Com_DPrintf ("Can't find %s\n", filename);
#ifdef FS_MISSING
	if (missingFiles) {
		fprintf(missingFiles, "%s\n", filename);
	}
#endif
	*file = 0;
	return -1;
}

// Ensiform: This is a bit of a hack but it is used for other 
// OS'/arch to still be acceptable with pure servers.
qboolean FS_FindPureDLL(const char *name)
{
	char dllName[MAX_OSPATH];
	fileHandle_t h;

	if(!fs_searchpaths)
		Com_Error(ERR_FATAL, "Filesystem call made without initialization");

	Com_sprintf(dllName, sizeof(dllName), "%sx86.dll", name);

	if(FS_FOpenFileRead(dllName, &h, qtrue) > 0)
	{
		FS_FCloseFile( h );
		return qtrue;
	}
	return qfalse;
}

/*
=================
FS_Read

Properly handles partial reads
=================
*/
int FS_Read2( void *buffer, int len, fileHandle_t f ) {
	if ( !fs_searchpaths ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	if ( !f ) {
		return 0;
	}
	if (fsh[f].streamed) {
		int r;
		fsh[f].streamed = qfalse;
		r = Sys_StreamedRead( buffer, len, 1, f);
		fsh[f].streamed = qtrue;
		return r;
	} else {
		return FS_Read( buffer, len, f);
	}
}

int FS_Read( void *buffer, int len, fileHandle_t f ) {
	int		block, remaining;
	int		read;
	byte	*buf;
	int		tries;

	if ( !fs_searchpaths ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	if ( !f ) {
		return 0;
	}

	buf = (byte *)buffer;
	fs_readCount += len;

	if (fsh[f].zipFile == qfalse) {
		remaining = len;
		tries = 0;
		while (remaining) {
			block = remaining;
			read = fread (buf, 1, block, fsh[f].handleFiles.file.o);
			if (read == 0) {
				// we might have been trying to read from a CD, which
				// sometimes returns a 0 read on windows
				if (!tries) {
					tries = 1;
				} else {
					return len-remaining;	//Com_Error (ERR_FATAL, "FS_Read: 0 bytes read");
				}
			}

			if (read == -1) {
				Com_Error (ERR_FATAL, "FS_Read: -1 bytes read");
			}

			remaining -= read;
			buf += read;
		}
		return len;
	} else {
		return unzReadCurrentFile(fsh[f].handleFiles.file.z, buffer, len);
	}
}

#ifdef USE_AIO
static void FS_WriteAio( const void *buffer, int len, fileHandle_t h, void (*notify_function) (sigval_t) ) {
	FILE *f = FS_FileForHandle(h);
	fileBufferNode_t *node = ( fileBufferNode_t * ) Z_Malloc( sizeof( fileBufferNode_t ), TAG_FILESYS, qtrue );
	node->buffer = Z_Malloc( len, TAG_FILESYS, qfalse );
	memcpy( node->buffer, buffer, len );
	node->next = NULL;
	node->cb.aio_fildes = fileno( f );
	node->cb.aio_buf = node->buffer;
	node->cb.aio_nbytes = len;
	if ( notify_function == NULL ) {
		node->cb.aio_sigevent.sigev_notify = SIGEV_NONE;
	} else {
		node->cb.aio_sigevent.sigev_notify = SIGEV_THREAD;
		node->cb.aio_sigevent.sigev_notify_function = notify_function;
		node->cb.aio_sigevent.sigev_notify_attributes = NULL;
		node->cb.aio_sigevent.sigev_value.sival_int = (int) h;
	}
	if ( aio_write( &node->cb ) ) {
		Com_Error( ERR_FATAL, "Failed to write to file: error %d\n", aio_error( &node->cb ) );
	}
	fileBufferNode_t *lastNode = fsh[h].pending;
	fileBufferNode_t *prevNode = NULL;
	while ( lastNode != NULL ) {
		fileBufferNode_t *nextNode = lastNode->next;
		if ( aio_error( &lastNode->cb ) != EINPROGRESS ) {
			// this write completed, so remove it from the list
			if ( prevNode == NULL ) {
				fsh[h].pending = lastNode->next;
			} else {
				prevNode->next = lastNode->next;
			}
			Z_Free( lastNode->buffer );
			Z_Free( lastNode );
		} else {
			prevNode = lastNode;
		}
		lastNode = nextNode;
	}
	if ( prevNode == NULL ) {
		fsh[h].pending = node;
	} else {
		prevNode->next = node;
	}
}
#endif

/*
=================
FS_Write

Properly handles partial writes
=================
*/
int FS_Write( const void *buffer, int len, fileHandle_t h ) {
	int		block, remaining;
	int		written;
	byte	*buf;
	int		tries;
	FILE	*f;

	if ( !fs_searchpaths ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	if ( !h ) {
		return 0;
	}

	f = FS_FileForHandle(h);
	buf = (byte *)buffer;

#if defined(USE_AIO)
	if ( fsh[h].handleAsync ) {
		if ( fsh[h].closing ) {
			Com_Printf( "Denying attempt to write to async file %d after closing it\n", h );
			return 0;
		}
		// first buffer as much as we can
		pendingBuffer_t *pb = &fsh[h].pendingBuffer;
		int remainingLen = len;
		while ( remainingLen > 0 ) {
			int copyLen = Q_min(remainingLen, (int) (pb->bufferLen - pb->bufferOffset ) );
			memcpy( &pb->buffer[pb->bufferOffset], buf, copyLen );
			pb->bufferOffset += copyLen;
			buf += copyLen;
			remainingLen -= copyLen;
			if ( pb->bufferOffset >= pb->bufferLen ) {
				// buffer is full, dump it
				FS_WriteAio( pb->buffer, pb->bufferLen, h, NULL );
				pb->bufferOffset = 0;
			}
		}
		return len;
	} else
#endif
	{
		remaining = len;
		tries = 0;
		while (remaining) {
			block = remaining;
			written = fwrite (buf, 1, block, f);
			if (written == 0) {
				if (!tries) {
					tries = 1;
				} else {
					Com_Printf( "FS_Write: 0 bytes written\n" );
					return 0;
				}
			}

			if (written == -1) {
				Com_Printf( "FS_Write: -1 bytes written\n" );
				return 0;
			}

			remaining -= written;
			buf += written;
		}
	}
	if ( fsh[h].handleSync ) {
		fflush( f );
	}
	return len;
}

#define PK3_SEEK_BUFFER_SIZE 65536
/*
=================
FS_Seek

=================
*/
int FS_Seek( fileHandle_t f, long offset, int origin ) {
	int		_origin;

	if ( !fs_searchpaths ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
		return -1;
	}

	if (fsh[f].streamed) {
		fsh[f].streamed = qfalse;
		Sys_StreamSeek( f, offset, origin );
		fsh[f].streamed = qtrue;
	}

	if (fsh[f].zipFile == qtrue) {
		//FIXME: this is incomplete and really, really
		//crappy (but better than what was here before)
		byte	buffer[PK3_SEEK_BUFFER_SIZE];
		int		remainder = offset;

		if( offset < 0 || origin == FS_SEEK_END ) {
			Com_Error( ERR_FATAL, "Negative offsets and FS_SEEK_END not implemented "
					"for FS_Seek on pk3 file contents" );
			return -1;
		}

		switch( origin ) {
			case FS_SEEK_SET:
				unzSetOffset(fsh[f].handleFiles.file.z, fsh[f].zipFilePos);
				unzOpenCurrentFile(fsh[f].handleFiles.file.z);
				//fallthrough

			case FS_SEEK_CUR:
				while( remainder > PK3_SEEK_BUFFER_SIZE ) {
					FS_Read( buffer, PK3_SEEK_BUFFER_SIZE, f );
					remainder -= PK3_SEEK_BUFFER_SIZE;
				}
				FS_Read( buffer, remainder, f );
				return offset;
				break;

			default:
				Com_Error( ERR_FATAL, "Bad origin in FS_Seek" );
				return -1;
				break;
		}
	} else {
		FILE *file;
		file = FS_FileForHandle(f);
		switch( origin ) {
		case FS_SEEK_CUR:
			_origin = SEEK_CUR;
			break;
		case FS_SEEK_END:
			_origin = SEEK_END;
			break;
		case FS_SEEK_SET:
			_origin = SEEK_SET;
			break;
		default:
			_origin = SEEK_CUR;
			Com_Error( ERR_FATAL, "Bad origin in FS_Seek\n" );
			break;
		}

		return fseek( file, offset, _origin );
	}
}

/*
======================================================================================

CONVENIENCE FUNCTIONS FOR ENTIRE FILES

======================================================================================
*/

int	FS_FileIsInPAK(const char *filename, int *pChecksum ) {
	searchpath_t	*search;
	pack_t			*pak;
	fileInPack_t	*pakFile;
	long			hash = 0;

	if ( !fs_searchpaths ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	if ( !filename ) {
		Com_Error( ERR_FATAL, "FS_FOpenFileRead: NULL 'filename' parameter passed\n" );
	}

	// qpaths are not supposed to have a leading slash
	if ( filename[0] == '/' || filename[0] == '\\' ) {
		filename++;
	}

	// make absolutely sure that it can't back up the path.
	// The searchpaths do guarantee that something will always
	// be prepended, so we don't need to worry about "c:" or "//limbo" 
	if ( strstr( filename, ".." ) || strstr( filename, "::" ) ) {
		return -1;
	}

	//
	// search through the path, one element at a time
	//

	for ( search = fs_searchpaths ; search ; search = search->next ) {
		//
		if (search->pack) {
			hash = FS_HashFileName(filename, search->pack->hashSize);
		}
		// is the element a pak file?
		if ( search->pack && search->pack->hashTable[hash] ) {
			// disregard if it doesn't match one of the allowed pure pak files
			if ( !FS_PakIsPure(search->pack) ) {
				continue;
			}

			// look through all the pak file elements
			pak = search->pack;
			pakFile = pak->hashTable[hash];
			do {
				// case and separator insensitive comparisons
				if ( !FS_FilenameCompare( pakFile->name, filename ) ) {
					if (pChecksum) {
						*pChecksum = pak->pure_checksum;
					}
					return 1;
				}
				pakFile = pakFile->next;
			} while(pakFile != NULL);
		}
	}
	return -1;
}

/*
============
FS_ReadFile

Filename are relative to the quake search path
a null buffer will just return the file length without loading
============
*/
long FS_ReadFile( const char *qpath, void **buffer ) {
	fileHandle_t	h;
	byte*			buf;
	qboolean		isConfig;
	int				len;

	if ( !fs_searchpaths ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	if ( !qpath || !qpath[0] ) {
		Com_Error( ERR_FATAL, "FS_ReadFile with empty name\n" );
	}

	buf = NULL;	// quiet compiler warning

	// if this is a .cfg file and we are playing back a journal, read
	// it from the journal file
	if ( strstr( qpath, ".cfg" ) ) {
		isConfig = qtrue;
		if ( com_journal && com_journal->integer == 2 ) {
			int		r;

			Com_DPrintf( "Loading %s from journal file.\n", qpath );
			r = FS_Read( &len, sizeof( len ), com_journalDataFile );
			if ( r != sizeof( len ) ) {
				if (buffer != NULL) *buffer = NULL;
				return -1;
			}
			// if the file didn't exist when the journal was created
			if (!len) {
				if (buffer == NULL) {
					return 1;			// hack for old journal files
				}
				*buffer = NULL;
				return -1;
			}
			if (buffer == NULL) {
				return len;
			}

			buf = (unsigned char *)Hunk_AllocateTempMemory(len+1);
			*buffer = buf;

			r = FS_Read( buf, len, com_journalDataFile );
			if ( r != len ) {
				Com_Error( ERR_FATAL, "Read from journalDataFile failed" );
			}

			fs_loadCount++;
			fs_loadStack++;

			// guarantee that it will have a trailing 0 for string operations
			buf[len] = 0;

			return len;
		}
	} else {
		isConfig = qfalse;
	}

	// look for it in the filesystem or pack files
	len = FS_FOpenFileRead( qpath, &h, qfalse );
	if ( h == 0 ) {
		if ( buffer ) {
			*buffer = NULL;
		}
		// if we are journalling and it is a config file, write a zero to the journal file
		if ( isConfig && com_journal && com_journal->integer == 1 ) {
			Com_DPrintf( "Writing zero for %s to journal file.\n", qpath );
			len = 0;
			FS_Write( &len, sizeof( len ), com_journalDataFile );
			FS_Flush( com_journalDataFile );
		}
		return -1;
	}
	
	if ( !buffer ) {
		if ( isConfig && com_journal && com_journal->integer == 1 ) {
			Com_DPrintf( "Writing len for %s to journal file.\n", qpath );
			FS_Write( &len, sizeof( len ), com_journalDataFile );
			FS_Flush( com_journalDataFile );
		}
		FS_FCloseFile( h);
		return len;
	}

	fs_loadCount++;
/*	fs_loadStack++;

	buf = (unsigned char *)Hunk_AllocateTempMemory(len+1);
	*buffer = buf;*/

	buf = (byte*)Z_Malloc( len+1, TAG_FILESYS, qfalse);
	buf[len]='\0';	// because we're not calling Z_Malloc with optional trailing 'bZeroIt' bool
	*buffer = buf;	

//	Z_Label(buf, qpath);

	FS_Read (buf, len, h);

	// guarantee that it will have a trailing 0 for string operations
	buf[len] = 0;
	FS_FCloseFile( h );

	// if we are journalling and it is a config file, write it to the journal file
	if ( isConfig && com_journal && com_journal->integer == 1 ) {
		Com_DPrintf( "Writing %s to journal file.\n", qpath );
		FS_Write( &len, sizeof( len ), com_journalDataFile );
		FS_Write( buf, len, com_journalDataFile );
		FS_Flush( com_journalDataFile );
	}
	return len;
}

/*
=============
FS_FreeFile
=============
*/
void FS_FreeFile( void *buffer ) {
	/*	
	if ( !fs_searchpaths ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}
	if ( !buffer ) {
		Com_Error( ERR_FATAL, "FS_FreeFile( NULL )" );
	}
	fs_loadStack--;

	Hunk_FreeTempMemory( buffer );

	// if all of our temp files are free, clear all of our space
	if ( fs_loadStack == 0 ) {
		Hunk_ClearTempMemory();
	}
	*/

	if ( !fs_searchpaths ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}
	if ( !buffer ) {
		Com_Error( ERR_FATAL, "FS_FreeFile( NULL )" );
	}

	Z_Free( buffer );
}

/*
==========================================================================

ZIP FILE LOADING

==========================================================================
*/

/*
=================
FS_LoadZipFile

Creates a new pak_t in the search chain for the contents
of a zip file.
=================
*/
static pack_t *FS_LoadZipFile( const char *zipfile, const char *basename )
{
	fileInPack_t	*buildBuffer;
	pack_t			*pack;
	unzFile			uf;
	int				err;
	unz_global_info gi;
	char			filename_inzip[MAX_ZPATH];
	unz_file_info	file_info;
	int				i, len;
	long			hash;
	int				fs_numHeaderLongs;
	int				*fs_headerLongs;
	char			*namePtr;

	fs_numHeaderLongs = 0;

	uf = unzOpen(zipfile);
	err = unzGetGlobalInfo (uf,&gi);

	if (err != UNZ_OK)
		return NULL;

	len = 0;
	unzGoToFirstFile(uf);
	for (i = 0; i < gi.number_entry; i++)
	{
		err = unzGetCurrentFileInfo(uf, &file_info, filename_inzip, sizeof(filename_inzip), NULL, 0, NULL, 0);
		if (err != UNZ_OK) {
			break;
		}
		len += strlen(filename_inzip) + 1;
		unzGoToNextFile(uf);
	}

	buildBuffer = (struct fileInPack_s *)Z_Malloc( (gi.number_entry * sizeof( fileInPack_t )) + len, TAG_FILESYS, qtrue );
	namePtr = ((char *) buildBuffer) + gi.number_entry * sizeof( fileInPack_t );
	fs_headerLongs = (int *)Z_Malloc( ( gi.number_entry + 1 ) * sizeof(int), TAG_FILESYS, qtrue );
	fs_headerLongs[ fs_numHeaderLongs++ ] = LittleLong( fs_checksumFeed );

	// get the hash table size from the number of files in the zip
	// because lots of custom pk3 files have less than 32 or 64 files
	for (i = 1; i <= MAX_FILEHASH_SIZE; i <<= 1) {
		if (i > gi.number_entry) {
			break;
		}
	}

	pack = (pack_t *)Z_Malloc( sizeof( pack_t ) + i * sizeof(fileInPack_t *), TAG_FILESYS, qtrue );
	pack->hashSize = i;
	pack->hashTable = (fileInPack_t **) (((char *) pack) + sizeof( pack_t ));
	for(i = 0; i < pack->hashSize; i++) {
		pack->hashTable[i] = NULL;
	}

	Q_strncpyz( pack->pakFilename, zipfile, sizeof( pack->pakFilename ) );
	Q_strncpyz( pack->pakBasename, basename, sizeof( pack->pakBasename ) );

	// strip .pk3 if needed
	if ( strlen( pack->pakBasename ) > 4 && !Q_stricmp( pack->pakBasename + strlen( pack->pakBasename ) - 4, ".pk3" ) ) {
		pack->pakBasename[strlen( pack->pakBasename ) - 4] = 0;
	}

	pack->handle = uf;
	pack->numfiles = gi.number_entry;
	unzGoToFirstFile(uf);

	for (i = 0; i < gi.number_entry; i++)
	{
		err = unzGetCurrentFileInfo(uf, &file_info, filename_inzip, sizeof(filename_inzip), NULL, 0, NULL, 0);
		if (err != UNZ_OK) {
			break;
		}
		if (file_info.uncompressed_size > 0) {
			fs_headerLongs[fs_numHeaderLongs++] = LittleLong(file_info.crc);
		}
		Q_strlwr( filename_inzip );
		hash = FS_HashFileName(filename_inzip, pack->hashSize);
		buildBuffer[i].name = namePtr;
		strcpy( buildBuffer[i].name, filename_inzip );
		namePtr += strlen(filename_inzip) + 1;
		// store the file position in the zip
		buildBuffer[i].pos = unzGetOffset(uf);
		//
		buildBuffer[i].next = pack->hashTable[hash];
		pack->hashTable[hash] = &buildBuffer[i];
		unzGoToNextFile(uf);
	}

	pack->checksum = Com_BlockChecksum( &fs_headerLongs[ 1 ], sizeof(*fs_headerLongs) * ( fs_numHeaderLongs - 1 ) );
	pack->pure_checksum = Com_BlockChecksum( fs_headerLongs, sizeof(*fs_headerLongs) * fs_numHeaderLongs );
	pack->checksum = LittleLong( pack->checksum );
	pack->pure_checksum = LittleLong( pack->pure_checksum );

	Z_Free(fs_headerLongs);

	pack->buildBuffer = buildBuffer;
	return pack;
}

/*
=================
FS_FreePak

Frees a pak structure and releases all associated resources
=================
*/

void FS_FreePak(pack_t *thepak)
{
	unzClose(thepak->handle);
	Z_Free(thepak->buildBuffer);
	Z_Free(thepak);
}

/*
=================
FS_GetZipChecksum

Compares whether the given pak file matches a referenced checksum
=================
*/
qboolean FS_CompareZipChecksum(const char *zipfile)
{
	pack_t *thepak;
	int index, checksum;

	thepak = FS_LoadZipFile(zipfile, "");

	if(!thepak)
		return qfalse;

	checksum = thepak->checksum;
	FS_FreePak(thepak);

	for(index = 0; index < fs_numServerReferencedPaks; index++)
	{
		if(checksum == fs_serverReferencedPaks[index])
			return qtrue;
	}

	return qfalse;
}

/*
=================================================================================

DIRECTORY SCANNING FUNCTIONS

=================================================================================
*/

#define	MAX_FOUND_FILES	0x1000

static int FS_ReturnPath( const char *zname, char *zpath, int *depth ) {
	int len, at, newdep;

	newdep = 0;
	zpath[0] = 0;
	len = 0;
	at = 0;

	while(zname[at] != 0)
	{
		if (zname[at]=='/' || zname[at]=='\\') {
			len = at;
			newdep++;
		}
		at++;
	}
	strcpy(zpath, zname);
	zpath[len] = 0;
	*depth = newdep;

	return len;
}

/*
==================
FS_AddFileToList
==================
*/
static int FS_AddFileToList( char *name, char *list[MAX_FOUND_FILES], int nfiles ) {
	int		i;

	if ( nfiles == MAX_FOUND_FILES - 1 ) {
		return nfiles;
	}
	for ( i = 0 ; i < nfiles ; i++ ) {
		if ( !Q_stricmp( name, list[i] ) ) {
			return nfiles;		// allready in list
		}
	}
	list[nfiles] = CopyString( name );
	nfiles++;

	return nfiles;
}

/*
===============
FS_ListFilteredFiles

Returns a uniqued list of files that match the given criteria
from all search paths
===============
*/
char **FS_ListFilteredFiles( const char *path, const char *extension, char *filter, int *numfiles ) {
	int				nfiles;
	char			**listCopy;
	char			*list[MAX_FOUND_FILES];
	searchpath_t	*search;
	int				i;
	int				pathLength;
	int				extensionLength;
	int				length, pathDepth, temp;
	pack_t			*pak;
	fileInPack_t	*buildBuffer;
	char			zpath[MAX_ZPATH];

	if ( !fs_searchpaths ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	if ( !path ) {
		*numfiles = 0;
		return NULL;
	}
	if ( !extension ) {
		extension = "";
	}

	pathLength = strlen( path );
	if ( path[pathLength-1] == '\\' || path[pathLength-1] == '/' ) {
		pathLength--;
	}
	extensionLength = strlen( extension );
	nfiles = 0;
	FS_ReturnPath(path, zpath, &pathDepth);

	//
	// search through the path, one element at a time, adding to list
	//
	for (search = fs_searchpaths ; search ; search = search->next) {
		// is the element a pak file?
		if (search->pack) {

			//ZOID:  If we are pure, don't search for files on paks that
			// aren't on the pure list
			if ( !FS_PakIsPure(search->pack) ) {
				continue;
			}

			// look through all the pak file elements
			pak = search->pack;
			buildBuffer = pak->buildBuffer;
			for (i = 0; i < pak->numfiles; i++) {
				char	*name;
				int		zpathLen, depth;

				// check for directory match
				name = buildBuffer[i].name;
				//
				if (filter) {
					// case insensitive
					if (!Com_FilterPath( filter, name, qfalse ))
						continue;
					// unique the match
					nfiles = FS_AddFileToList( name, list, nfiles );
				}
				else {

					zpathLen = FS_ReturnPath(name, zpath, &depth);

					if ( (depth-pathDepth)>2 || pathLength > zpathLen || Q_stricmpn( name, path, pathLength ) ) {
						continue;
					}

					// check for extension match
					length = strlen( name );
					if ( length < extensionLength ) {
						continue;
					}

					if ( Q_stricmp( name + length - extensionLength, extension ) ) {
						continue;
					}
					// unique the match

					temp = pathLength;
					if (pathLength) {
						temp++;		// include the '/'
					}
					nfiles = FS_AddFileToList( name + temp, list, nfiles );
				}
			}
		} else if (search->dir) { // scan for files in the filesystem
			char	*netpath;
			int		numSysFiles;
			char	**sysFiles;
			char	*name;

			// don't scan directories for files if we are pure or restricted
			if ( fs_numServerPaks &&
				 (!extension || Q_stricmp(extension, "fcf")) )
			{
				//rww - allow scanning for fcf files outside of pak even if pure
			    continue;
		    }
			else
			{
				netpath = FS_BuildOSPath( search->dir->path, search->dir->gamedir, path );
				sysFiles = Sys_ListFiles( netpath, extension, filter, &numSysFiles, qfalse );
				for ( i = 0 ; i < numSysFiles ; i++ ) {
					// unique the match
					name = sysFiles[i];
					nfiles = FS_AddFileToList( name, list, nfiles );
				}
				Sys_FreeFileList( sysFiles );
			}
		}		
	}

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

	return listCopy;
}

/*
=================
FS_ListFiles
=================
*/
char **FS_ListFiles( const char *path, const char *extension, int *numfiles ) {
	return FS_ListFilteredFiles( path, extension, NULL, numfiles );
}

/*
=================
FS_FreeFileList
=================
*/
void FS_FreeFileList( char **fileList ) {
	//rwwRMG - changed to fileList to not conflict with list type
	int		i;

	if ( !fs_searchpaths ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	if ( !fileList ) {
		return;
	}

	for ( i = 0 ; fileList[i] ; i++ ) {
		Z_Free( fileList[i] );
	}

	Z_Free( fileList );
}


/*
================
FS_GetFileList
================
*/
int	FS_GetFileList(  const char *path, const char *extension, char *listbuf, int bufsize ) {
	int		nFiles, i, nTotal, nLen;
	char **pFiles = NULL;

	*listbuf = 0;
	nFiles = 0;
	nTotal = 0;

	if (Q_stricmp(path, "$modlist") == 0) {
		return FS_GetModList(listbuf, bufsize);
	}

	pFiles = FS_ListFiles(path, extension, &nFiles);

	for (i =0; i < nFiles; i++) {
		nLen = strlen(pFiles[i]) + 1;
		if (nTotal + nLen + 1 < bufsize) {
			strcpy(listbuf, pFiles[i]);
			listbuf += nLen;
			nTotal += nLen;
		}
		else {
			nFiles = i;
			break;
		}
	}

	FS_FreeFileList(pFiles);

	return nFiles;
}

// NOTE: could prolly turn out useful for the win32 version too, but it's used only by linux and Mac OS X
//#if defined(__linux__) || defined(MACOS_X)
/*
=======================
Sys_ConcatenateFileLists

mkv: Naive implementation. Concatenates three lists into a
     new list, and frees the old lists from the heap.
bk001129 - from cvs1.17 (mkv)

FIXME TTimo those two should move to common.c next to Sys_ListFiles
=======================
 */
static unsigned int Sys_CountFileList(char **fileList)
{
	int i = 0;

	if (fileList)
	{
		while (*fileList)
		{
			fileList++;
			i++;
		}
	}
	return i;
}

static char** Sys_ConcatenateFileLists( char **list0, char **list1, char **list2 )
{
	int totalLength = 0;
	char** cat = NULL, **dst, **src;

	totalLength += Sys_CountFileList(list0);
	totalLength += Sys_CountFileList(list1);
	totalLength += Sys_CountFileList(list2);

	/* Create new list. */
	dst = cat = (char **)Z_Malloc( ( totalLength + 1 ) * sizeof( char* ), TAG_FILESYS, qtrue );

	/* Copy over lists. */
	if (list0) {
		for (src = list0; *src; src++, dst++)
			*dst = *src;
	}
	if (list1) {
		for (src = list1; *src; src++, dst++)
			*dst = *src;
	}
	if (list2) {
		for (src = list2; *src; src++, dst++)
			*dst = *src;
	}

	// Terminate the list
	*dst = NULL;

	// Free our old lists.
	// NOTE: not freeing their content, it's been merged in dst and still being used
	if (list0) Z_Free( list0 );
	if (list1) Z_Free( list1 );
	if (list2) Z_Free( list2 );

	return cat;
}
//#endif

/*
================
FS_GetModList

Returns a list of mod directory names
A mod directory is a peer to base with a pk3 in it
The directories are searched in base path, cd path and home path
================
*/
int	FS_GetModList( char *listbuf, int bufsize ) {
	int		nMods, i, j, nTotal, nLen, nPaks, nPotential, nDescLen;
	char **pFiles = NULL;
	char **pPaks = NULL;
	char *name, *path;
	char descPath[MAX_OSPATH];
	fileHandle_t descHandle;

	int dummy;
	char **pFiles0 = NULL;
	char **pFiles1 = NULL;
	char **pFiles2 = NULL;
	qboolean bDrop = qfalse;

	*listbuf = 0;
	nMods = nPotential = nTotal = 0;

	pFiles0 = Sys_ListFiles( fs_homepath->string, NULL, NULL, &dummy, qtrue );
	pFiles1 = Sys_ListFiles( fs_basepath->string, NULL, NULL, &dummy, qtrue );
	pFiles2 = Sys_ListFiles( fs_cdpath->string, NULL, NULL, &dummy, qtrue );
	// we searched for mods in the three paths
	// it is likely that we have duplicate names now, which we will cleanup below
	pFiles = Sys_ConcatenateFileLists( pFiles0, pFiles1, pFiles2 );
	nPotential = Sys_CountFileList(pFiles);

	for ( i = 0 ; i < nPotential ; i++ ) {
		name = pFiles[i];
		// NOTE: cleaner would involve more changes
		// ignore duplicate mod directories
		if (i!=0) {
			bDrop = qfalse;
			for(j=0; j<i; j++)
			{
				if (Q_stricmp(pFiles[j],name)==0) {
					// this one can be dropped
					bDrop = qtrue;
					break;
				}
			}
		}
		if (bDrop) {
			continue;
		}
		// we drop "base" "." and ".."
		if (Q_stricmp(name, BASEGAME) && Q_stricmpn(name, ".", 1)) {
			// now we need to find some .pk3 files to validate the mod
			// NOTE TTimo: (actually I'm not sure why .. what if it's a mod under developement with no .pk3?)
			// we didn't keep the information when we merged the directory names, as to what OS Path it was found under
			//   so it could be in base path, cd path or home path
			//   we will try each three of them here (yes, it's a bit messy)
			path = FS_BuildOSPath( fs_basepath->string, name, "" );
			nPaks = 0;
			pPaks = Sys_ListFiles(path, ".pk3", NULL, &nPaks, qfalse); 
			Sys_FreeFileList( pPaks ); // we only use Sys_ListFiles to check wether .pk3 files are present

			/* Try on cd path */
			if( nPaks <= 0 ) {
				path = FS_BuildOSPath( fs_cdpath->string, name, "" );
				nPaks = 0;
				pPaks = Sys_ListFiles( path, ".pk3", NULL, &nPaks, qfalse );
				Sys_FreeFileList( pPaks );
			}

			/* try on home path */
			if ( nPaks <= 0 )
			{
				path = FS_BuildOSPath( fs_homepath->string, name, "" );
				nPaks = 0;
				pPaks = Sys_ListFiles( path, ".pk3", NULL, &nPaks, qfalse );
				Sys_FreeFileList( pPaks );
			}

			if (nPaks > 0) {
				nLen = strlen(name) + 1;
				// nLen is the length of the mod path
				// we need to see if there is a description available
				descPath[0] = '\0';
				strcpy(descPath, name);
				strcat(descPath, "/description.txt");
				nDescLen = FS_SV_FOpenFileRead( descPath, &descHandle );
				if ( nDescLen > 0 && descHandle) {
					FILE *file;
					file = FS_FileForHandle(descHandle);
					Com_Memset( descPath, 0, sizeof( descPath ) );
					nDescLen = fread(descPath, 1, 48, file);
					if (nDescLen >= 0) {
						descPath[nDescLen] = '\0';
					}
					FS_FCloseFile(descHandle);
				} else {
					strcpy(descPath, name);
				}
				nDescLen = strlen(descPath) + 1;

				if (nTotal + nLen + 1 + nDescLen + 1 < bufsize) {
					strcpy(listbuf, name);
					listbuf += nLen;
					strcpy(listbuf, descPath);
					listbuf += nDescLen;
					nTotal += nLen + nDescLen;
					nMods++;
				}
				else {
					break;
				}
			}
		}
	}
	Sys_FreeFileList( pFiles );

	return nMods;
}




//============================================================================

/*
================
FS_Dir_f
================
*/
void FS_Dir_f( void ) {
	char	*path;
	char	*extension;
	char	**dirnames;
	int		ndirs;
	int		i;

	if ( Cmd_Argc() < 2 || Cmd_Argc() > 3 ) {
		Com_Printf( "usage: dir <directory> [extension]\n" );
		return;
	}

	if ( Cmd_Argc() == 2 ) {
		path = Cmd_Argv( 1 );
		extension = "";
	} else {
		path = Cmd_Argv( 1 );
		extension = Cmd_Argv( 2 );
	}

	Com_Printf( "Directory of %s %s\n", path, extension );
	Com_Printf( "---------------\n" );

	dirnames = FS_ListFiles( path, extension, &ndirs );

	for ( i = 0; i < ndirs; i++ ) {
		Com_Printf( "%s\n", dirnames[i] );
	}
	FS_FreeFileList( dirnames );
}

/*
===========
FS_ConvertPath
===========
*/
void FS_ConvertPath( char *s ) {
	while (*s) {
		if ( *s == '\\' || *s == ':' ) {
			*s = '/';
		}
		s++;
	}
}

/*
===========
FS_PathCmp

Ignore case and seprator char distinctions
===========
*/
int FS_PathCmp( const char *s1, const char *s2 ) {
	int		c1, c2;
	
	do {
		c1 = *s1++;
		c2 = *s2++;

		if (c1 >= 'a' && c1 <= 'z') {
			c1 -= ('a' - 'A');
		}
		if (c2 >= 'a' && c2 <= 'z') {
			c2 -= ('a' - 'A');
		}

		if ( c1 == '\\' || c1 == ':' ) {
			c1 = '/';
		}
		if ( c2 == '\\' || c2 == ':' ) {
			c2 = '/';
		}
		
		if (c1 < c2) {
			return -1;		// strings not equal
		}
		if (c1 > c2) {
			return 1;
		}
	} while (c1);
	
	return 0;		// strings are equal
}

/*
================
FS_SortFileList
================
*/
void FS_SortFileList(char **filelist, int numfiles) {
	int i, j, k, numsortedfiles;
	char **sortedlist;

	sortedlist = (char **)Z_Malloc( ( numfiles + 1 ) * sizeof( *sortedlist ), TAG_FILESYS, qtrue );
	sortedlist[0] = NULL;
	numsortedfiles = 0;
	for (i = 0; i < numfiles; i++) {
		for (j = 0; j < numsortedfiles; j++) {
			if (FS_PathCmp(filelist[i], sortedlist[j]) < 0) {
				break;
			}
		}
		for (k = numsortedfiles; k > j; k--) {
			sortedlist[k] = sortedlist[k-1];
		}
		sortedlist[j] = filelist[i];
		numsortedfiles++;
	}
	Com_Memcpy(filelist, sortedlist, numfiles * sizeof( *filelist ) );
	Z_Free(sortedlist);
}

/*
================
FS_NewDir_f
================
*/
void FS_NewDir_f( void ) {
	char	*filter;
	char	**dirnames;
	int		ndirs;
	int		i;

	if ( Cmd_Argc() < 2 ) {
		Com_Printf( "usage: fdir <filter>\n" );
		Com_Printf( "example: fdir *ffa*.bsp\n");
		return;
	}

	filter = Cmd_Argv( 1 );

	Com_Printf( "---------------\n" );

	dirnames = FS_ListFilteredFiles( "", "", filter, &ndirs );

	FS_SortFileList(dirnames, ndirs);

	for ( i = 0; i < ndirs; i++ ) {
		FS_ConvertPath(dirnames[i]);
		Com_Printf( "%s\n", dirnames[i] );
	}
	Com_Printf( "%d files listed\n", ndirs );
	FS_FreeFileList( dirnames );
}

/*
============
FS_Path_f

============
*/
void FS_Path_f( void ) {
	searchpath_t	*s;
	int				i;

	Com_Printf ("Current search path:\n");
	for (s = fs_searchpaths; s; s = s->next) {
		if (s->pack) {
			Com_Printf ("%s (%i files)\n", s->pack->pakFilename, s->pack->numfiles);
			if ( fs_numServerPaks ) {
				if ( !FS_PakIsPure(s->pack) ) {
					Com_Printf( "    not on the pure list\n" );
				} else {
					Com_Printf( "    on the pure list\n" );
				}
			}
		} else {
			Com_Printf ("%s%c%s\n", s->dir->path, PATH_SEP, s->dir->gamedir );
		}
	}

	Com_Printf( "\n" );
	for ( i = 1 ; i < MAX_FILE_HANDLES ; i++ ) {
		if ( fsh[i].handleFiles.file.o ) {
			Com_Printf( "handle %i: %s\n", i, fsh[i].name );
		}
	}
}

/*
============
FS_TouchFile_f

The only purpose of this function is to allow game script files to copy
arbitrary files furing an "fs_copyfiles 1" run.
============
*/
void FS_TouchFile_f( void ) {
	fileHandle_t	f;

	if ( Cmd_Argc() != 2 ) {
		Com_Printf( "Usage: touchFile <file>\n" );
		return;
	}

	FS_FOpenFileRead( Cmd_Argv( 1 ), &f, qfalse );
	if ( f ) {
		FS_FCloseFile( f );
	}
}

//===========================================================================


static int QDECL paksort( const void *a, const void *b ) {
	char	*aa, *bb;

	aa = *(char **)a;
	bb = *(char **)b;

	return FS_PathCmp( aa, bb );
}

/*
================
FS_AddGameDirectory

Sets fs_gamedir, adds the directory to the head of the path,
then loads the zip headers
================
*/
#define	MAX_PAKFILES	1024
static void FS_AddGameDirectory( const char *path, const char *dir ) {
	searchpath_t	*sp;
	int				i;
	searchpath_t	*search;
	searchpath_t	*thedir;
	pack_t			*pak;
	char			curpath[MAX_OSPATH + 1], *pakfile;
	int				numfiles;
	char			**pakfiles;
	char			*sorted[MAX_PAKFILES];

	// this fixes the case where fs_basepath is the same as fs_cdpath
	// which happens on full installs
	for ( sp = fs_searchpaths ; sp ; sp = sp->next ) {
		if ( sp->dir && Sys_PathCmp(sp->dir->path, path) && !Q_stricmp(sp->dir->gamedir, dir)) {
			return;			// we've already got this one
		}
	}
	
	Q_strncpyz( fs_gamedir, dir, sizeof( fs_gamedir ) );

	// find all pak files in this directory
	Q_strncpyz(curpath, FS_BuildOSPath(path, dir, ""), sizeof(curpath));
	curpath[strlen(curpath) - 1] = '\0';	// strip the trailing slash

	//
	// add the directory to the search path
	//
	search = (struct searchpath_s *)Z_Malloc (sizeof(searchpath_t), TAG_FILESYS, qtrue);
	search->dir = (directory_t *)Z_Malloc( sizeof( *search->dir ), TAG_FILESYS, qtrue );

	Q_strncpyz( search->dir->path, path, sizeof( search->dir->path ) );
	Q_strncpyz( search->dir->fullpath, curpath, sizeof( search->dir->fullpath ) );
	Q_strncpyz( search->dir->gamedir, dir, sizeof( search->dir->gamedir ) );
	search->next = fs_searchpaths;
	fs_searchpaths = search;

	thedir = search;

	pakfiles = Sys_ListFiles( curpath, ".pk3", NULL, &numfiles, qfalse );

	// sort them so that later alphabetic matches override
	// earlier ones.  This makes pak1.pk3 override pak0.pk3
	if ( numfiles > MAX_PAKFILES ) {
		numfiles = MAX_PAKFILES;
	}
	for ( i = 0 ; i < numfiles ; i++ ) {
		sorted[i] = pakfiles[i];
	}

	qsort( sorted, numfiles, sizeof(char*), paksort );

	for ( i = 0 ; i < numfiles ; i++ ) {
		pakfile = FS_BuildOSPath( path, dir, sorted[i] );
		if ( ( pak = FS_LoadZipFile( pakfile, sorted[i] ) ) == 0 )
			continue;
		Q_strncpyz(pak->pakPathname, curpath, sizeof(pak->pakPathname));
		// store the game name for downloading
		Q_strncpyz(pak->pakGamename, dir, sizeof(pak->pakGamename));

		fs_packFiles += pak->numfiles;

		search = (searchpath_s *)Z_Malloc (sizeof(searchpath_t), TAG_FILESYS, qtrue);
		search->pack = pak;

		if (fs_dirbeforepak && fs_dirbeforepak->integer && thedir)
		{
			searchpath_t *oldnext = thedir->next;
			thedir->next = search;

			while (oldnext)
			{
				search->next = oldnext;
				search = search->next;
				oldnext = oldnext->next;
			}
		}
		else
		{
			search->next = fs_searchpaths;
			fs_searchpaths = search;
		}
	}

	// done
	Sys_FreeFileList( pakfiles );
}

/*
================
FS_idPak
================
*/
qboolean FS_idPak( char *pak, char *base ) {
	int i;

	for (i = 0; i < NUM_ID_PAKS; i++) {
		if ( !FS_FilenameCompare(pak, va("%s/assets%d", base, i)) ) {
			break;
		}
	}
	if (i < NUM_ID_PAKS) {
		return qtrue;
	}
	return qfalse;
}

/*
================
FS_CheckDirTraversal

Check whether the string contains stuff like "../" to prevent directory traversal bugs
and return qtrue if it does.
================
*/

qboolean FS_CheckDirTraversal(const char *checkdir)
{
	if(strstr(checkdir, "../") || strstr(checkdir, "..\\"))
		return qtrue;
	
	return qfalse;
}

/*
================
FS_ComparePaks

if dlstring == qtrue

Returns a list of pak files that we should download from the server. They all get stored
in the current gamedir and an FS_Restart will be fired up after we download them all.

The string is the format:

@remotename@localname [repeat]

static int		fs_numServerReferencedPaks;
static int		fs_serverReferencedPaks[MAX_SEARCH_PATHS];
static char		*fs_serverReferencedPakNames[MAX_SEARCH_PATHS];

----------------
dlstring == qfalse

we are not interested in a download string format, we want something human-readable
(this is used for diagnostics while connecting to a pure server)
================
*/
qboolean FS_ComparePaks( char *neededpaks, int len, qboolean dlstring ) {
	searchpath_t	*sp;
	qboolean havepak;
	char *origpos = neededpaks;
	int i;

	if ( !fs_numServerReferencedPaks ) {
		return qfalse; // Server didn't send any pack information along
	}

	*neededpaks = 0;

	for ( i = 0 ; i < fs_numServerReferencedPaks ; i++ ) {
		// Ok, see if we have this pak file
		havepak = qfalse;

		// never autodownload any of the id paks
		if ( FS_idPak(fs_serverReferencedPakNames[i], "base") || FS_idPak(fs_serverReferencedPakNames[i], "missionpack") ) {
			continue;
		}

		// Make sure the server cannot make us write to non-quake3 directories.
		if(FS_CheckDirTraversal(fs_serverReferencedPakNames[i]))
		{
			Com_Printf("WARNING: Invalid download name %s\n", fs_serverReferencedPakNames[i]);
			continue;
		}

		for ( sp = fs_searchpaths ; sp ; sp = sp->next ) {
			if ( sp->pack && sp->pack->checksum == fs_serverReferencedPaks[i] ) {
				havepak = qtrue; // This is it!
				break;
			}
		}

		if ( !havepak && fs_serverReferencedPakNames[i] && *fs_serverReferencedPakNames[i] ) { 
			// Don't got it
			
			if (dlstring)
			{
				// We need this to make sure we won't hit the end of the buffer or the server could
				// overwrite non-pk3 files on clients by writing so much crap into neededpaks that
				// Q_strcat cuts off the .pk3 extension.

				origpos += strlen(origpos);

				// Remote name
				Q_strcat( neededpaks, len, "@");
				Q_strcat( neededpaks, len, fs_serverReferencedPakNames[i] );
				Q_strcat( neededpaks, len, ".pk3" );
				
				// Local name
				Q_strcat( neededpaks, len, "@");
				// Do we have one with the same name?
				if ( FS_SV_FileExists( va( "%s.pk3", fs_serverReferencedPakNames[i] ) ) ) {
					char st[MAX_ZPATH];
					// We already have one called this, we need to download it to another name
					// Make something up with the checksum in it
					Com_sprintf( st, sizeof( st ), "%s.%08x.pk3", fs_serverReferencedPakNames[i], fs_serverReferencedPaks[i] );
					Q_strcat( neededpaks, len, st );
				} else {
					Q_strcat( neededpaks, len, fs_serverReferencedPakNames[i] );
					Q_strcat( neededpaks, len, ".pk3" );
				}

				// Find out whether it might have overflowed the buffer and don't add this file to the
				// list if that is the case.
				if(strlen(origpos) + (origpos - neededpaks) >= len - 1)
				{
					*origpos = '\0';
					break;
				}
			}
			else
			{
				Q_strcat( neededpaks, len, fs_serverReferencedPakNames[i] );
				Q_strcat( neededpaks, len, ".pk3" );
				// Do we have one with the same name?
				if ( FS_SV_FileExists( va( "%s.pk3", fs_serverReferencedPakNames[i] ) ) )
				{
					Q_strcat( neededpaks, len, " (local file exists with wrong checksum)");
				}
				Q_strcat( neededpaks, len, "\n");
			}
		}
	}
	if ( *neededpaks ) {
		return qtrue;
	}

	return qfalse; // We have them all
}

//rww - add search paths in for received svc_setgame
//Ensiform - this is so wrong rww
void FS_UpdateGamedir(void)
{
	if ( fs_gamedirvar->string[0] && Q_stricmp( fs_gamedirvar->string, BASEGAME ) )
	{
		if (fs_cdpath->string[0])
		{
			FS_AddGameDirectory(fs_cdpath->string, fs_gamedirvar->string);
		}
		if (fs_basepath->string[0])
		{
			FS_AddGameDirectory(fs_basepath->string, fs_gamedirvar->string);
		}
		if (fs_homepath->string[0] && Q_stricmp(fs_homepath->string,fs_basepath->string))
		{
			FS_AddGameDirectory(fs_homepath->string, fs_gamedirvar->string);
		}
	}
}

/*
================
FS_ReorderPurePaks
NOTE TTimo: the reordering that happens here is not reflected in the cvars (\cvarlist *pak*)
  this can lead to misleading situations, see https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=540
================
*/
static void FS_ReorderPurePaks()
{
	searchpath_t *s;
	int i;
	searchpath_t **p_insert_index, // for linked list reordering
		**p_previous; // when doing the scan
	
	// only relevant when connected to pure server
	if ( !fs_numServerPaks )
		return;
	
	fs_reordered = qfalse;
	
	p_insert_index = &fs_searchpaths; // we insert in order at the beginning of the list 
	for ( i = 0 ; i < fs_numServerPaks ; i++ ) {
		p_previous = p_insert_index; // track the pointer-to-current-item
		for (s = *p_insert_index; s; s = s->next) {
			// the part of the list before p_insert_index has been sorted already
			if (s->pack && fs_serverPaks[i] == s->pack->checksum) {
				fs_reordered = qtrue;
				// move this element to the insert list
				*p_previous = s->next;
				s->next = *p_insert_index;
				*p_insert_index = s;
				// increment insert list
				p_insert_index = &s->next;
				break; // iterate to next server pack
			}
			p_previous = &s->next; 
		}
	}
}

/*
================
FS_Startup
================
*/
void FS_Startup( const char *gameName ) {
	const char *homePath;
	char fs_gamedirLast[MAX_OSPATH];

	Com_Printf( "----- FS_Startup -----\n" );

	fs_packFiles = 0;

	fs_debug = Cvar_Get( "fs_debug", "0", 0 );
	fs_copyfiles = Cvar_Get( "fs_copyfiles", "0", CVAR_INIT );
	fs_cdpath = Cvar_Get ("fs_cdpath", "", CVAR_INIT|CVAR_PROTECTED );
	fs_basepath = Cvar_Get ("fs_basepath", Sys_DefaultInstallPath(), CVAR_INIT|CVAR_PROTECTED );
	fs_basegame = Cvar_Get ("fs_basegame", "", CVAR_INIT );
	homePath = Sys_DefaultHomePath();
	if (!homePath || !homePath[0]) {
		homePath = fs_basepath->string;
	}
	fs_homepath = Cvar_Get ("fs_homepath", homePath, CVAR_INIT|CVAR_PROTECTED );
	fs_gamedirvar = Cvar_Get ("fs_game", "", CVAR_INIT|CVAR_SYSTEMINFO );
	fs_extragamedirs = Cvar_Get( "fs_extragames", "", CVAR_INIT );

	fs_dirbeforepak = Cvar_Get("fs_dirbeforepak", "0", CVAR_INIT|CVAR_PROTECTED);

	// add search path elements in reverse priority order
	if (fs_cdpath->string[0]) {
		FS_AddGameDirectory( fs_cdpath->string, gameName );
	}
	if (fs_basepath->string[0]) {
		FS_AddGameDirectory( fs_basepath->string, gameName );
	}
#ifdef MACOS_X
	fs_apppath = Cvar_Get ("fs_apppath", Sys_DefaultAppPath(), CVAR_INIT|CVAR_PROTECTED );
	// Make MacOSX also include the base path included with the .app bundle
	if (fs_apppath->string[0]) {
		FS_AddGameDirectory( fs_apppath->string, gameName );
	}
#endif
#ifdef __ANDROID__
	if (fs_homepath->string)
		LOGI("fs_homepath->string == %s",fs_homepath->string);
	if (fs_basepath->string)
		LOGI("fs_basepath->string == %s",fs_basepath->string);
#endif
	// fs_homepath is somewhat particular to *nix systems, only add if relevant
	// NOTE: same filtering below for mods and basegame
	if (fs_homepath->string[0] && !Sys_PathCmp(fs_homepath->string, fs_basepath->string)) {
#ifdef __ANDROID__
		FS_CreatePath ( fs_homepath->string );
#endif
		FS_AddGameDirectory ( fs_homepath->string, gameName );
	}
        
	// check for additional base game so mods can be based upon other mods
	if ( fs_basegame->string[0] && !Q_stricmp( gameName, BASEGAME ) && Q_stricmp( fs_basegame->string, gameName ) ) {
		if (fs_cdpath->string[0]) {
			FS_AddGameDirectory(fs_cdpath->string, fs_basegame->string);
		}
		if (fs_basepath->string[0]) {
			FS_AddGameDirectory(fs_basepath->string, fs_basegame->string);
		}
		if (fs_homepath->string[0] && !Sys_PathCmp(fs_homepath->string, fs_basepath->string)) {
			FS_AddGameDirectory(fs_homepath->string, fs_basegame->string);
		}
	}

	//if we don't have the mod folder (fs_game), but have the extra mods folders (fs_extraGames),
	//then the last one from the extra mods folders will be treated as the mod folder,
	//so we have to store the last game directory and read it later
	Q_strncpyz(fs_gamedirLast, fs_gamedir, sizeof(fs_gamedirLast));
	/* Read the fs_extragames after the base */
	if ( fs_extragamedirs->string[0] ) {
		int i;
		Cmd_TokenizeString( fs_extragamedirs->string );
		for( i=0; i<Cmd_Argc();i++) {
			const char *newPath = Cmd_Argv( i );
			if (fs_cdpath->string[0]) {
				FS_AddGameDirectory(fs_cdpath->string, newPath );
			}
			if (fs_basepath->string[0]) {
				FS_AddGameDirectory(fs_basepath->string, newPath );
			}
			if (fs_homepath->string[0] && Q_stricmp(fs_homepath->string,fs_basepath->string)) {
				FS_AddGameDirectory(fs_homepath->string, newPath );
			}
		}
	}

	// check for additional game folder for mods
	if ( fs_gamedirvar->string[0] && !Q_stricmp( gameName, BASEGAME ) && Q_stricmp( fs_gamedirvar->string, gameName ) ) {
		if (fs_cdpath->string[0]) {
			FS_AddGameDirectory(fs_cdpath->string, fs_gamedirvar->string);
		}
		if (fs_basepath->string[0]) {
			FS_AddGameDirectory(fs_basepath->string, fs_gamedirvar->string);
		}
		if (fs_homepath->string[0] && !Sys_PathCmp(fs_homepath->string, fs_basepath->string)) {
			FS_AddGameDirectory(fs_homepath->string, fs_gamedirvar->string);
		}
	//read the last game directory
	} else {
		Q_strncpyz(fs_gamedir, fs_gamedirLast, sizeof(fs_gamedir));
	}

	// add our commands
	Cmd_AddCommand ("path", FS_Path_f);
	Cmd_AddCommand ("dir", FS_Dir_f );
	Cmd_AddCommand ("fdir", FS_NewDir_f );
	Cmd_AddCommand ("touchFile", FS_TouchFile_f );

	// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=506
	// reorder the pure pk3 files according to server order
	FS_ReorderPurePaks();

	// print the current search paths
	FS_Path_f();

	fs_gamedirvar->modified = qfalse; // We just loaded, it's not modified

	Com_Printf( "----------------------\n" );

#ifdef FS_MISSING
	if (missingFiles == NULL) {
		missingFiles = fopen( "\\missing.txt", "ab" );
	}
#endif
	Com_Printf( "%d files in pk3 files\n", fs_packFiles );
}

/*
=====================
FS_LoadedPakChecksums

Returns a space separated string containing the checksums of all loaded pk3 files.
Servers with sv_pure set will get this string and pass it to clients.
=====================
*/
const char *FS_LoadedPakChecksums( void ) {
	static char	info[BIG_INFO_STRING];
	searchpath_t	*search;

	info[0] = 0;

	for ( search = fs_searchpaths ; search ; search = search->next ) {
		// is the element a pak file? 
		if ( !search->pack ) {
			continue;
		}

		Q_strcat( info, sizeof( info ), va("%i ", search->pack->checksum ) );
	}

	return info;
}

/*
=====================
FS_LoadedPakNames

Returns a space separated string containing the names of all loaded pk3 files.
Servers with sv_pure set will get this string and pass it to clients.
=====================
*/
const char *FS_LoadedPakNames( void ) {
	static char	info[BIG_INFO_STRING];
	searchpath_t	*search;

	info[0] = 0;

	for ( search = fs_searchpaths ; search ; search = search->next ) {
		// is the element a pak file?
		if ( !search->pack ) {
			continue;
		}

		if (*info) {
			Q_strcat(info, sizeof( info ), " " );
		}
		Q_strcat( info, sizeof( info ), search->pack->pakBasename );
	}

	return info;
}

/*
=====================
FS_LoadedPakPureChecksums

Returns a space separated string containing the pure checksums of all loaded pk3 files.
Servers with sv_pure use these checksums to compare with the checksums the clients send
back to the server.
=====================
*/
const char *FS_LoadedPakPureChecksums( void ) {
	static char	info[BIG_INFO_STRING];
	searchpath_t	*search;

	info[0] = 0;

	for ( search = fs_searchpaths ; search ; search = search->next ) {
		// is the element a pak file? 
		if ( !search->pack ) {
			continue;
		}

		Q_strcat( info, sizeof( info ), va("%i ", search->pack->pure_checksum ) );
	}

	return info;
}

/*
=====================
FS_ReferencedPakChecksums

Returns a space separated string containing the checksums of all referenced pk3 files.
The server will send this to the clients so they can check which files should be auto-downloaded. 
=====================
*/
const char *FS_ReferencedPakChecksums( void ) {
	static char	info[BIG_INFO_STRING];
	searchpath_t *search;

	info[0] = 0;


	for ( search = fs_searchpaths ; search ; search = search->next ) {
		// is the element a pak file?
		if ( search->pack ) {
			if (search->pack->referenced || Q_stricmpn(search->pack->pakGamename, BASEGAME, strlen(BASEGAME))) {
				Q_strcat( info, sizeof( info ), va("%i ", search->pack->checksum ) );
			}
		}
	}

	return info;
}

/*
=====================
FS_ReferencedPakPureChecksums

Returns a space separated string containing the pure checksums of all referenced pk3 files.
Servers with sv_pure set will get this string back from clients for pure validation 

The string has a specific order, "cgame ui @ ref1 ref2 ref3 ..."
=====================
*/
const char *FS_ReferencedPakPureChecksums( void ) {
	static char	info[BIG_INFO_STRING];
	searchpath_t	*search;
	int nFlags, numPaks, checksum;

	info[0] = 0;

	checksum = fs_checksumFeed;
	numPaks = 0;
	for (nFlags = FS_CGAME_REF; nFlags; nFlags = nFlags >> 1) {
		if (nFlags & FS_GENERAL_REF) {
			// add a delimter between must haves and general refs
			//Q_strcat(info, sizeof(info), "@ ");
			info[strlen(info)+1] = '\0';
			info[strlen(info)+2] = '\0';
			info[strlen(info)] = '@';
			info[strlen(info)] = ' ';
		}
		for ( search = fs_searchpaths ; search ; search = search->next ) {
			// is the element a pak file and has it been referenced based on flag?
			if ( search->pack && (search->pack->referenced & nFlags)) {
				Q_strcat( info, sizeof( info ), va("%i ", search->pack->pure_checksum ) );
				if (nFlags & (FS_CGAME_REF | FS_UI_REF)) {
					break;
				}
				checksum ^= search->pack->pure_checksum;
				numPaks++;
			}
		}
		if (fs_fakeChkSum != 0) {
			// only added if a non-pure file is referenced
			Q_strcat( info, sizeof( info ), va("%i ", fs_fakeChkSum ) );
		}
	}
	// last checksum is the encoded number of referenced pk3s
	checksum ^= numPaks;
	Q_strcat( info, sizeof( info ), va("%i ", checksum ) );

	return info;
}

/*
=====================
FS_ReferencedPakNames

Returns a space separated string containing the names of all referenced pk3 files.
The server will send this to the clients so they can check which files should be auto-downloaded. 
=====================
*/
const char *FS_ReferencedPakNames( void ) {
	static char	info[BIG_INFO_STRING];
	searchpath_t	*search;

	info[0] = 0;

	// we want to return ALL pk3's from the fs_game path
	// and referenced one's from base
	for ( search = fs_searchpaths ; search ; search = search->next ) {
		// is the element a pak file?
		if ( search->pack ) {
			if (search->pack->referenced || Q_stricmpn(search->pack->pakGamename, BASEGAME, strlen(BASEGAME))) {
				if (*info) {
					Q_strcat(info, sizeof( info ), " " );
				}
				Q_strcat( info, sizeof( info ), search->pack->pakGamename );
				Q_strcat( info, sizeof( info ), "/" );
				Q_strcat( info, sizeof( info ), search->pack->pakBasename );
			}
		}
	}

	return info;
}

/*
=====================
FS_ClearPakReferences
=====================
*/
void FS_ClearPakReferences( int flags ) {
	searchpath_t *search;

	if ( !flags ) {
		flags = -1;
	}
	for ( search = fs_searchpaths; search; search = search->next ) {
		// is the element a pak file and has it been referenced?
		if ( search->pack ) {
			search->pack->referenced &= ~flags;
		}
	}
}


/*
=====================
FS_PureServerSetLoadedPaks

If the string is empty, all data sources will be allowed.
If not empty, only pk3 files that match one of the space
separated checksums will be checked for files, with the
exception of .cfg and .dat files.
=====================
*/
void FS_PureServerSetLoadedPaks( const char *pakSums, const char *pakNames ) {
	int		i, c, d;

	Cmd_TokenizeString( pakSums );

	c = Cmd_Argc();
	if ( c > MAX_SEARCH_PATHS ) {
		c = MAX_SEARCH_PATHS;
	}

	fs_numServerPaks = c;

	for ( i = 0 ; i < c ; i++ ) {
		fs_serverPaks[i] = atoi( Cmd_Argv( i ) );
	}

	if (fs_numServerPaks) {
		Com_DPrintf( "Connected to a pure server.\n" );
	}
	else
	{
		if (fs_reordered)
		{
			// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=540
			// force a restart to make sure the search order will be correct
			Com_DPrintf( "FS search reorder is required\n" );
			FS_Restart(fs_checksumFeed);
			return;
		}
	}

	for ( i = 0 ; i < c ; i++ ) {
		if (fs_serverPakNames[i]) {
			Z_Free(fs_serverPakNames[i]);
		}
		fs_serverPakNames[i] = NULL;
	}
	if ( pakNames && *pakNames ) {
		Cmd_TokenizeString( pakNames );

		d = Cmd_Argc();
		if ( d > MAX_SEARCH_PATHS ) {
			d = MAX_SEARCH_PATHS;
		}

		for ( i = 0 ; i < d ; i++ ) {
			fs_serverPakNames[i] = CopyString( Cmd_Argv( i ) );
		}
	}
}

/*
=====================
FS_PureServerSetReferencedPaks

The checksums and names of the pk3 files referenced at the server
are sent to the client and stored here. The client will use these
checksums to see if any pk3 files need to be auto-downloaded. 
=====================
*/
void FS_PureServerSetReferencedPaks( const char *pakSums, const char *pakNames ) {
	int		i, c, d = 0;

	Cmd_TokenizeString( pakSums );

	c = Cmd_Argc();
	if ( c > MAX_SEARCH_PATHS ) {
		c = MAX_SEARCH_PATHS;
	}

	for ( i = 0 ; i < c ; i++ ) {
		fs_serverReferencedPaks[i] = atoi( Cmd_Argv( i ) );
	}

	for (i = 0 ; i < ARRAY_LEN(fs_serverReferencedPakNames); i++)
	{
		if(fs_serverReferencedPakNames[i])
			Z_Free(fs_serverReferencedPakNames[i]);

		fs_serverReferencedPakNames[i] = NULL;
	}

	if ( pakNames && *pakNames ) {
		Cmd_TokenizeString( pakNames );

		d = Cmd_Argc();
		if ( d > c ) {
			d = c;
		}

		for ( i = 0 ; i < d ; i++ ) {
			fs_serverReferencedPakNames[i] = CopyString( Cmd_Argv( i ) );
		}
	}

	// ensure that there are as many checksums as there are pak names.
	if(d < c)
		c = d;

	fs_numServerReferencedPaks = c;
}

/*
================
FS_Restart
================
*/
void FS_Restart( int checksumFeed ) {

	// free anything we currently have loaded
	FS_Shutdown(qfalse);

	// set the checksum feed
	fs_checksumFeed = checksumFeed;

	// clear pak references
	FS_ClearPakReferences(0);

	// try to start up normally
	FS_Startup( BASEGAME );

	// if we can't find default.cfg, assume that the paths are
	// busted and error out now, rather than getting an unreadable
	// graphics screen when the font fails to load
	if ( FS_ReadFile( "mpdefault.cfg", NULL ) <= 0 ) {
		// this might happen when connecting to a pure server not using BASEGAME/pak0.pk3
		// (for instance a TA demo server)
		if (lastValidBase[0]) {
			FS_PureServerSetLoadedPaks("", "");
			Cvar_Set("fs_basepath", lastValidBase);
			Cvar_Set("fs_game", lastValidGame);
			lastValidBase[0] = '\0';
			lastValidGame[0] = '\0';
			FS_Restart(checksumFeed);
			Com_Error( ERR_DROP, "Invalid game folder\n" );
			return;
		}
		Com_Error( ERR_FATAL, "Couldn't load mpdefault.cfg" );
	}

	// bk010116 - new check before safeMode
	if ( Q_stricmp(fs_gamedirvar->string, lastValidGame) ) {
		// skip the jampconfig.cfg if "safe" is on the command line
		if ( !Com_SafeMode() ) {
			Cbuf_AddText ("exec " Q3CONFIG_CFG "\n");
		}
	}

	Q_strncpyz(lastValidBase, fs_basepath->string, sizeof(lastValidBase));
	Q_strncpyz(lastValidGame, fs_gamedirvar->string, sizeof(lastValidGame));

}

/*
=================
FS_ConditionalRestart
restart if necessary
=================
*/
qboolean FS_ConditionalRestart( int checksumFeed ) {
	if( fs_gamedirvar->modified || checksumFeed != fs_checksumFeed ) {
		FS_Restart( checksumFeed );
		return qtrue;
	}
#if 0
	if(fs_gamedirvar->modified)
	{
		if(FS_FilenameCompare(lastValidGame, fs_gamedirvar->string) &&
				(*lastValidGame || FS_FilenameCompare(fs_gamedirvar->string, BASEGAME)) &&
				(*fs_gamedirvar->string || FS_FilenameCompare(lastValidGame, BASEGAME)))
		{
			FS_Restart(checksumFeed);
			//Cvar_Restart(qtrue);
			return qtrue;
		}
		else
			fs_gamedirvar->modified = qfalse;
	}

	if(checksumFeed != fs_checksumFeed)
		FS_Restart(checksumFeed);
	else if(fs_numServerPaks && !fs_reordered)
		FS_ReorderPurePaks();
#endif
	return qfalse;
}

/*
========================================================================================

Handle based file calls for virtual machines

========================================================================================
*/

int		FS_FOpenFileByMode( const char *qpath, fileHandle_t *f, fsMode_t mode ) {
	int		r;
	qboolean	sync;

	sync = qfalse;

	switch( mode ) {
	case FS_READ:
		r = FS_FOpenFileRead( qpath, f, qtrue );
		break;
	case FS_WRITE:
		*f = FS_FOpenFileWrite( qpath );
		r = 0;
		if (*f == 0) {
			r = -1;
		}
		break;
	case FS_APPEND_SYNC:
		sync = qtrue;
	case FS_APPEND:
		*f = FS_FOpenFileAppend( qpath );
		r = 0;
		if (*f == 0) {
			r = -1;
		}
		break;
	default:
		Com_Error( ERR_FATAL, "FSH_FOpenFile: bad mode" );
		return -1;
	}

	if (!f) {
		return r;
	}

	if ( *f ) {
		if (fsh[*f].zipFile == qtrue) {
			fsh[*f].baseOffset = unztell(fsh[*f].handleFiles.file.z);
		} else {
			fsh[*f].baseOffset = ftell(fsh[*f].handleFiles.file.o);
		}
		fsh[*f].fileSize = r;
		fsh[*f].streamed = qfalse;

		if (mode == FS_READ) {
			Sys_BeginStreamedFile( *f, 0x4000 );
			fsh[*f].streamed = qtrue;
		}
	}
	fsh[*f].handleSync = sync;
#ifdef USE_AIO
	fsh[*f].handleAsync = qfalse;
#endif

	return r;
}

qboolean FS_FEof( fileHandle_t f ) {
	qboolean eof;
	if (fsh[f].zipFile == qtrue) {
		eof = (qboolean)unzeof(fsh[f].handleFiles.file.z);
	} else {
		eof = (qboolean)feof(fsh[f].handleFiles.file.o);
	}
	return eof;
}

int		FS_FTell( fileHandle_t f ) {
	int pos;
	if (fsh[f].zipFile == qtrue) {
		pos = unztell(fsh[f].handleFiles.file.z);
	} else {
		pos = ftell(fsh[f].handleFiles.file.o);
	}
	return pos;
}

void	FS_Flush( fileHandle_t f ) {
	fflush(fsh[f].handleFiles.file.o);
}

#ifdef MACOS_X
bool FS_LoadMachOBundle( const char *name )
{
	int     len;
	void    *data;
	fileHandle_t    f;
	char    *fn;
	unzFile dll;
	byte* buf;
	char    dllName[MAX_QPATH];
	char    *tempName;
	unz_s   *zfi;
	
	//read zipped bundle from pk3
	len = FS_ReadFile(name, &data);
	
	if (len < 1) {
		return false;
	}
	
	//write temporary file of zipped bundle to e.g. uixxxxxx
	//unique filename to avoid any clashes
	Com_sprintf( dllName, sizeof(dllName), "%sXXXXXX", name );
	
	tempName = mktemp( dllName );
	
	f = FS_FOpenFileWrite( dllName );
	
	if ( !f )
	{
		FS_FreeFile(data);
		return false;
	}
	
	if (FS_Write( data, len, f ) < len)
	{
		FS_FreeFile(data);
		return false;
	}
	
	FS_FCloseFile( f );
	FS_FreeFile(data);
	
	//unzOpen zipped bundle, find the dylib, and try to write it
	fn = FS_BuildOSPath( fs_homepath->string, fs_gamedir, dllName );
	
	dll = unzOpen( fn );
	
	Com_sprintf (dllName, sizeof(dllName), "%s.bundle/Contents/MacOS/%s", name, name);
	
	if (unzLocateFile(dll, dllName, 0) != UNZ_OK)
	{
		unzClose(dll);
		remove( fn );
		return false;
	}
	
	unzOpenCurrentFile( dll );
	
	Com_sprintf( dllName, sizeof(dllName), "%s_pk3" DLL_EXT, name );
	
	f = FS_FOpenFileWrite( dllName );
	
	if ( !f )
	{
		unzCloseCurrentFile( dll );
		unzClose( dll );
		remove( fn );
		return false;
	}
	
	zfi = (unz_s *)dll;
	
	len = zfi->cur_file_info.uncompressed_size;
	
	buf = (byte*)Z_Malloc( len+1, TAG_FILESYS, qfalse);
	
	if (unzReadCurrentFile( dll, buf, len ) < len)
	{
		FS_FCloseFile( f );
		unzCloseCurrentFile( dll );
		unzClose( dll );
		return false;
	}
	
	if (FS_Write(buf, len, f) < len)
	{
		FS_FCloseFile( f );
		unzCloseCurrentFile( dll );
		unzClose( dll );
		return false;
	}
	
	FS_FCloseFile( f );
	unzCloseCurrentFile( dll );
	unzClose( dll );
	Z_Free( buf );
	
	//remove temporary zipped bundle
	remove( fn );
	
	return true;
}
#endif

//pipes!!
fileHandle_t FS_PipeOpen(const char *qcmd, const char *qpath, const char *mode) {
    char			*ospath;
    fileHandle_t	f;
    char            cmd[2048];
    
    if (!fs_searchpaths) {
        Com_Error(ERR_FATAL, "Filesystem call made without initialization\n");
    }
    
    f = FS_HandleForFile();
    fsh[f].zipFile = qfalse;
    
    ospath = FS_BuildOSPath(fs_homepath->string, fs_gamedir, qpath);
    
    if (fs_debug->integer) {
        Com_Printf("FS_PipeOpen ospath: %s\n", ospath);
    }
    
    if(FS_CreatePath(ospath)) {
        return 0;
    }
    
	Com_sprintf(cmd, sizeof(cmd), "%s", qcmd);
	FS_ReplaceSeparators(cmd);

    if (fs_debug->integer) {
        Com_Printf("FS_PipeOpen cmd: %s\n", cmd);
    }
    
#ifdef _WIN32
    fsh[f].handleFiles.file.o = _popen(cmd, mode);
#else
    fsh[f].handleFiles.file.o = popen(cmd, mode);
#endif
    
    Q_strncpyz(fsh[f].name, qpath, sizeof(fsh[f].name));
    
    fsh[f].handleSync = qfalse;
#ifdef USE_AIO
    fsh[f].handleAsync = qfalse;
#endif
    if (!fsh[f].handleFiles.file.o) {
        f = 0;
    }
    return f;
}

int FS_PipeWrite(const void *buffer, int len, fileHandle_t h) {
    return FS_Write(buffer, len, h);
}

void FS_PipeClose(fileHandle_t f) {
    if (!fs_searchpaths) {
        Com_Error(ERR_FATAL, "Filesystem call made without initialization\n");
    }
    
    if (fsh[f].streamed) {
        Sys_EndStreamedFile(f);
    }
    // we didn't find it as a pak, so close it as a unique file
    if (fsh[f].handleFiles.file.o) {
#ifdef _WIN32
        _pclose(fsh[f].handleFiles.file.o);
#else
        pclose(fsh[f].handleFiles.file.o);
#endif
    }
    Com_Memset(&fsh[f], 0, sizeof(fsh[f]));
}
