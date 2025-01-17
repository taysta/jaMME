#include <SDL.h>
#include "../qcommon/qcommon.h"
#ifdef __ANDROID
#include "../rd-gles/tr_local.h"
#else
#include "../rd-vanilla/tr_local.h"
#endif
#include "sdl_qgl.h"
#include "../sys/sys_local.h"

static SDL_Window *window = NULL;

static float displayAspect;
cvar_t *r_allowSoftwareGL; // Don't abort out if a hardware visual can't be obtained
cvar_t *r_allowResize; // make window resizable
cvar_t *r_sdlDriver;
cvar_t *r_sdlHighDpi;

typedef enum
{
	RSERR_OK,

	RSERR_INVALID_FULLSCREEN,
	RSERR_INVALID_MODE,

	RSERR_UNKNOWN
} rserr_t;

static SDL_Window *screen = NULL;
static SDL_DisplayMode *videoInfo = NULL;

/* Just hack it for now. */
#ifdef MACOS_X
#include <OpenGL/OpenGL.h>
typedef CGLContextObj QGLContext;
#define GLimp_GetCurrentContext() CGLGetCurrentContext()
#define GLimp_SetCurrentContext(ctx) CGLSetCurrentContext(ctx)
#else
typedef void *QGLContext;
#define GLimp_GetCurrentContext() (NULL)
#define GLimp_SetCurrentContext(ctx)
#endif
static QGLContext opengl_context;

bool g_bTextureRectangleHack = false;

void (APIENTRYP qglActiveTextureARB) (GLenum texture);
void (APIENTRYP qglClientActiveTextureARB) (GLenum texture);
void (APIENTRYP qglMultiTexCoord2fARB) (GLenum target, GLfloat s, GLfloat t);

void (APIENTRYP qglCombinerParameterfvNV) (GLenum pname,const GLfloat *params);
void (APIENTRYP qglCombinerParameterivNV) (GLenum pname,const GLint *params);
void (APIENTRYP qglCombinerParameterfNV) (GLenum pname,GLfloat param);
void (APIENTRYP qglCombinerParameteriNV) (GLenum pname,GLint param);
void (APIENTRYP qglCombinerInputNV) (GLenum stage,GLenum portion,GLenum variable,GLenum input,GLenum mapping,
		GLenum componentUsage);
void (APIENTRYP qglCombinerOutputNV) (GLenum stage,GLenum portion,GLenum abOutput,GLenum cdOutput,GLenum sumOutput,
		GLenum scale, GLenum bias,GLboolean abDotProduct,GLboolean cdDotProduct,
		GLboolean muxSum);

void (APIENTRYP qglFinalCombinerInputNV) (GLenum variable,GLenum input,GLenum mapping,GLenum componentUsage);
void (APIENTRYP qglGetCombinerInputParameterfvNV) (GLenum stage,GLenum portion,GLenum variable,GLenum pname,GLfloat *params);
void (APIENTRYP qglGetCombinerInputParameterivNV) (GLenum stage,GLenum portion,GLenum variable,GLenum pname,GLint *params);
void (APIENTRYP qglGetCombinerOutputParameterfvNV) (GLenum stage,GLenum portion,GLenum pname,GLfloat *params);
void (APIENTRYP qglGetCombinerOutputParameterivNV) (GLenum stage,GLenum portion,GLenum pname,GLint *params);
void (APIENTRYP qglGetFinalCombinerInputParameterfvNV) (GLenum variable,GLenum pname,GLfloat *params);
void (APIENTRYP qglGetFinalCombinerInputParameterivNV) (GLenum variable,GLenum pname,GLint *params);

PFNGLPROGRAMSTRINGARBPROC qglProgramStringARB = NULL;
PFNGLBINDPROGRAMARBPROC qglBindProgramARB = NULL;
PFNGLDELETEPROGRAMSARBPROC qglDeleteProgramsARB = NULL;
PFNGLGENPROGRAMSARBPROC qglGenProgramsARB = NULL;
PFNGLPROGRAMENVPARAMETER4DARBPROC qglProgramEnvParameter4dARB = NULL;
PFNGLPROGRAMENVPARAMETER4DVARBPROC qglProgramEnvParameter4dvARB = NULL;
PFNGLPROGRAMENVPARAMETER4FARBPROC qglProgramEnvParameter4fARB = NULL;
PFNGLPROGRAMENVPARAMETER4FVARBPROC qglProgramEnvParameter4fvARB = NULL;
PFNGLPROGRAMLOCALPARAMETER4DARBPROC qglProgramLocalParameter4dARB = NULL;
PFNGLPROGRAMLOCALPARAMETER4DVARBPROC qglProgramLocalParameter4dvARB = NULL;
PFNGLPROGRAMLOCALPARAMETER4FARBPROC qglProgramLocalParameter4fARB = NULL;
PFNGLPROGRAMLOCALPARAMETER4FVARBPROC qglProgramLocalParameter4fvARB = NULL;
PFNGLGETPROGRAMENVPARAMETERDVARBPROC qglGetProgramEnvParameterdvARB = NULL;
PFNGLGETPROGRAMENVPARAMETERFVARBPROC qglGetProgramEnvParameterfvARB = NULL;
PFNGLGETPROGRAMLOCALPARAMETERDVARBPROC qglGetProgramLocalParameterdvARB = NULL;
PFNGLGETPROGRAMLOCALPARAMETERFVARBPROC qglGetProgramLocalParameterfvARB = NULL;
PFNGLGETPROGRAMIVARBPROC qglGetProgramivARB = NULL;
PFNGLGETPROGRAMSTRINGARBPROC qglGetProgramStringARB = NULL;
PFNGLISPROGRAMARBPROC qglIsProgramARB = NULL;
PFNGLGENBUFFERSARBPROC qglGenBuffersARB = NULL;
PFNGLBINDBUFFERARBPROC qglBindBufferARB = NULL;
PFNGLBUFFERDATAARBPROC qglBufferDataARB = NULL;
PFNGLMAPBUFFERARBPROC qglMapBufferARB = NULL;
PFNGLUNMAPBUFFERARBPROC qglUnmapBufferARB = NULL;

void ( APIENTRY * qglTexImage3DEXT) (GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *);
void ( APIENTRY * qglTexSubImage3DEXT) (GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);

void ( APIENTRY * qglPointParameterfEXT)( GLenum param, GLfloat value );
void ( APIENTRY * qglPointParameterfvEXT)( GLenum param, GLfloat *value );

void ( * qglLockArraysEXT)( int, int);
void ( * qglUnlockArraysEXT) ( void );

//added framebuffer extensions
void (APIENTRY * qglGenFramebuffers )(GLsizei, GLuint *);
void (APIENTRY * qglBindFramebuffer )(GLenum, GLuint);
void (APIENTRY * qglGenRenderbuffers )(GLsizei, GLuint *);
void (APIENTRY * qglBindRenderbuffer )(GLenum, GLuint);
void (APIENTRY * qglRenderbufferStorage )(GLenum, GLenum, GLsizei, GLsizei);
void (APIENTRY * qglFramebufferRenderbuffer )(GLenum, GLenum, GLenum, GLuint);
void (APIENTRY * qglFramebufferTexture2D )(GLenum, GLenum, GLenum, GLuint, GLint);
GLenum (APIENTRY * qglCheckFramebufferStatus )(GLenum);
void (APIENTRY * qglDeleteFramebuffers )(GLsizei, const GLuint *);
void (APIENTRY * qglDeleteRenderbuffers )(GLsizei, const GLuint *);

void (APIENTRY * qglRenderbufferStorageMultisampleEXT ) ( GLenum target, GLsizei samples, GLenum internalformat, GLsizei width , GLsizei height );
void (APIENTRY * qglBlitFramebufferEXT )( GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter );

//added fragment/vertex program extensions
void (APIENTRYP qglAttachShader) (GLuint, GLuint);
void (APIENTRYP qglBindAttribLocation) (GLuint, GLuint, const GLchar *);
void (APIENTRYP qglCompileShader) (GLuint);
GLuint (APIENTRYP qglCreateProgram) (void);
GLuint (APIENTRYP qglCreateShader) (GLenum);
void (APIENTRYP qglDeleteProgram) (GLuint);
void (APIENTRYP qglDeleteShader) (GLuint);
void (APIENTRYP qglShaderSource) (GLuint, GLsizei, const GLchar* *, const GLint *);
void (APIENTRYP qglLinkProgram) (GLuint);
void (APIENTRYP qglUseProgram) (GLuint);	
GLint (APIENTRYP qglGetUniformLocation) (GLuint, const GLchar *);
void (APIENTRYP qglUniform1f) (GLint, GLfloat);
void (APIENTRYP qglUniform2f) (GLint, GLfloat, GLfloat);
void (APIENTRYP qglUniform1i) (GLint, GLint);
void (APIENTRYP qglGetProgramiv) (GLuint, GLenum, GLint *);
void (APIENTRYP qglGetProgramInfoLog) (GLuint, GLsizei, GLsizei *, GLchar *);
void (APIENTRYP qglGetShaderiv) (GLuint, GLenum, GLint *);
void (APIENTRYP qglGetShaderInfoLog) (GLuint, GLsizei, GLsizei *, GLchar *);

void		GLimp_Minimize( void ) {
}

void		GLimp_EndFrame( void )
{
  SDL_GL_SwapWindow(screen);
}

/*
===============
GLimp_CompareModes
===============
*/
static int GLimp_CompareModes( const void *a, const void *b )
{
	const float ASPECT_EPSILON = 0.001f;
	SDL_Rect *modeA = (SDL_Rect *)&a;
	SDL_Rect *modeB = (SDL_Rect *)&b;
	float aspectA = (float)modeA->w / (float)modeA->h;
	float aspectB = (float)modeB->w / (float)modeB->h;
	int areaA = modeA->w * modeA->h;
	int areaB = modeB->w * modeB->h;
	float aspectDiffA = fabs( aspectA - displayAspect );
	float aspectDiffB = fabs( aspectB - displayAspect );
	float aspectDiffsDiff = aspectDiffA - aspectDiffB;

	if( aspectDiffsDiff > ASPECT_EPSILON )
		return 1;
	else if( aspectDiffsDiff < -ASPECT_EPSILON )
		return -1;
	else
		return areaA - areaB;
}

/*
===============
GLimp_DetectAvailableModes
===============
*/
static void GLimp_DetectAvailableModes(void)
{
	int i;
	char buf[ MAX_STRING_CHARS ] = { 0 };
	SDL_Rect modes[ 128 ];
	int numModes = 0;

	int display = SDL_GetWindowDisplayIndex( screen );
	SDL_DisplayMode windowMode;

	if( SDL_GetWindowDisplayMode( screen, &windowMode ) < 0 )
	{
		Com_Printf( "Couldn't get window display mode, no resolutions detected\n" );
		return;
	}

	for( i = 0; i < SDL_GetNumDisplayModes( display ); i++ )
	{
		SDL_DisplayMode mode;

		if( SDL_GetDisplayMode( display, i, &mode ) < 0 )
			continue;

		if( !mode.w || !mode.h )
		{
			Com_Printf( "Display supports any resolution\n" );
			return;
		}

		if( windowMode.format != mode.format )
			continue;

		modes[ numModes ].w = mode.w;
		modes[ numModes ].h = mode.h;
		numModes++;
	}

	if( numModes > 1 )
		qsort( modes, numModes, sizeof( SDL_Rect ), GLimp_CompareModes );

	for( i = 0; i < numModes; i++ )
	{
		const char *newModeString = va( "%ux%u ", modes[ i ].w, modes[ i ].h );

		if( strlen( newModeString ) < (int)sizeof( buf ) - strlen( buf ) )
			Q_strcat( buf, sizeof( buf ), newModeString );
		else
			Com_Printf( "Skipping mode %ux%x, buffer too small\n", modes[ i ].w, modes[ i ].h );
	}

	if( *buf )
	{
		buf[ strlen( buf ) - 1 ] = 0;
		Com_Printf( "Available modes: '%s'\n", buf );
		ri.Cvar_Set( "r_availableModes", buf );
	}
}

static float GLimp_GetDisplayScale(int display)
{
    float scale = 1.0f;

#if SDL_VERSION_ATLEAST(2, 0, 4)
    const char *driver = SDL_GetCurrentVideoDriver();

    if (!strcmp(driver, "windows")) {
        float ddpi;

        // on windows driver dpi is always 96 * desktop scaling
        if (!SDL_GetDisplayDPI(display, &ddpi, NULL, NULL)) {
            scale = ddpi / 96.0f;
        }
    } else if (!strcmp(driver, "x11")) {
        float ddpi;

        // this is a hack: some environments return real display DPI,
        // others synthetic, based on desktop scaling. Not sure if 96
        // is universal synthetic 1:1 neither. x11 has no fractional
        // scaling so round to integer.
        if (!SDL_GetDisplayDPI(display, &ddpi, NULL, NULL)) {
            scale = roundf(ddpi / 96.0f);
        }

    }
#endif

    return scale;
}

/*
===============
GLimp_SetMode
===============
*/
static rserr_t GLimp_SetMode(int mode, qboolean fullscreen, qboolean noborder)
{
	const char *glstring;
	int perChannelColorBits;
	int colorBits, depthBits, stencilBits;
	int samples;
	int i = 0;
	SDL_Surface *icon = NULL;
	Uint32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL;
	SDL_DisplayMode desktopMode;
	int display = 0;
	int x = 0, y = 0;
    int width, height;

	Com_Printf( "Initializing OpenGL display\n");

	if ( r_allowResize->integer && !fullscreen )
		flags |= SDL_WINDOW_RESIZABLE;
    
    if ( r_sdlHighDpi->integer )
        flags |= SDL_WINDOW_ALLOW_HIGHDPI;
    
	/*icon = SDL_CreateRGBSurfaceFrom(
			(void *)CLIENT_WINDOW_ICON.pixel_data,
			CLIENT_WINDOW_ICON.width,
			CLIENT_WINDOW_ICON.height,
			CLIENT_WINDOW_ICON.bytes_per_pixel * 8,
			CLIENT_WINDOW_ICON.bytes_per_pixel * CLIENT_WINDOW_ICON.width,
#ifdef Q3_LITTLE_ENDIAN
			0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000
#else
			0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF
#endif
			);*/

	// If a window exists, note its display index
	if( screen != NULL )
    {
        display = SDL_GetWindowDisplayIndex( screen );
        SDL_GetWindowPosition( screen, &x, &y );
        ri.Printf( PRINT_DEVELOPER, "Existing window at %dx%d before being destroyed\n", x, y );
        SDL_DestroyWindow( screen );
        screen = NULL;
    }

	if( SDL_GetDesktopDisplayMode( display, &desktopMode ) == 0 )
	{
		displayAspect = (float)desktopMode.w / (float)desktopMode.h;

		Com_Printf( "Display aspect: %.3f\n", displayAspect );
	}
	else
	{
		Com_Memset( &desktopMode, 0, sizeof( SDL_DisplayMode ) );

		Com_Printf( "Cannot determine display aspect, assuming 1.333\n" );
	}

	Com_Printf( "...setting mode %d:", mode );

	if (mode == -2)
	{
		// use desktop video resolution
		if( desktopMode.h > 0 )
		{
            width = desktopMode.w;
            height = desktopMode.h;
		}
		else
		{
            width = 640;
            height = 480;
			Com_Printf( "Cannot determine display resolution, assuming 640x480\n" );
		}

		//glConfig.windowAspect = (float)width / (float)height;
	}
	else if ( !R_GetModeInfo( &width, &height, /*&glConfig.windowAspect,*/ mode ) )
	{
		Com_Printf( " invalid mode\n" );
		return RSERR_INVALID_MODE;
	}
	Com_Printf( " %d %d\n", width, height);

	// Center window
	if( r_centerWindow->integer && !fullscreen )
	{
		x = ( desktopMode.w / 2 ) - ( width / 2 );
		y = ( desktopMode.h / 2 ) - ( height / 2 );
	}

	// Destroy existing state if it exists
	if( opengl_context != NULL )
	{
		SDL_GL_DeleteContext( opengl_context );
		opengl_context = NULL;
	}

	if( fullscreen )
	{
		flags |= SDL_WINDOW_FULLSCREEN;
		glConfig.isFullscreen = qtrue;
	}
	else
	{
		if( noborder )
			flags |= SDL_WINDOW_BORDERLESS;

		glConfig.isFullscreen = qfalse;
	}

	colorBits = r_colorbits->value;
	if ((!colorBits) || (colorBits >= 32))
		colorBits = 24;

	if (!r_depthbits->value)
		depthBits = 24;
	else
		depthBits = r_depthbits->value;

	stencilBits = r_stencilbits->value;
    samples = r_ext_multisample->value;

	for (i = 0; i < 16; i++)
	{
		int testColorBits, testDepthBits, testStencilBits;

		// 0 - default
		// 1 - minus colorBits
		// 2 - minus depthBits
		// 3 - minus stencil
		if ((i % 4) == 0 && i)
		{
			// one pass, reduce
			switch (i / 4)
			{
				case 2 :
					if (colorBits == 24)
						colorBits = 16;
					break;
				case 1 :
					if (depthBits == 24)
						depthBits = 16;
					else if (depthBits == 16)
						depthBits = 8;
				case 3 :
					if (stencilBits == 24)
						stencilBits = 16;
					else if (stencilBits == 16)
						stencilBits = 8;
			}
		}

		testColorBits = colorBits;
		testDepthBits = depthBits;
		testStencilBits = stencilBits;

		if ((i % 4) == 3)
		{ // reduce colorBits
			if (testColorBits == 24)
				testColorBits = 16;
		}

		if ((i % 4) == 2)
		{ // reduce depthBits
			if (testDepthBits == 24)
				testDepthBits = 16;
			else if (testDepthBits == 16)
				testDepthBits = 8;
		}

		if ((i % 4) == 1)
		{ // reduce stencilBits
			if (testStencilBits == 24)
				testStencilBits = 16;
			else if (testStencilBits == 16)
				testStencilBits = 8;
			else
				testStencilBits = 0;
		}

		if (testColorBits == 24)
			perChannelColorBits = 8;
		else
			perChannelColorBits = 4;

#ifdef __sgi /* Fix for SGIs grabbing too many bits of color */
		if (perChannelColorBits == 4)
			perChannelColorBits = 0; /* Use minimum size for 16-bit color */

		/* Need alpha or else SGIs choose 36+ bit RGB mode */
		SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 1);
#endif

		SDL_GL_SetAttribute( SDL_GL_RED_SIZE, perChannelColorBits );
		SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, perChannelColorBits );
		SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, perChannelColorBits );
		SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, testDepthBits );
		SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, testStencilBits );

		SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, samples ? 1 : 0 );
		SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, samples );

		if(r_stereo->integer)
		{
			glConfig.stereoEnabled = qtrue;
			SDL_GL_SetAttribute(SDL_GL_STEREO, 1);
		}
		else
		{
			glConfig.stereoEnabled = qfalse;
			SDL_GL_SetAttribute(SDL_GL_STEREO, 0);
		}

		SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

		// If not allowing software GL, demand accelerated
		if( !r_allowSoftwareGL->integer )
			SDL_GL_SetAttribute( SDL_GL_ACCELERATED_VISUAL, 1 );

		if( ( screen = SDL_CreateWindow( CLIENT_WINDOW_TITLE, x, y,
                                        width, height, flags ) ) == 0 )
		{
			ri.Printf( PRINT_DEVELOPER, "SDL_CreateWindow failed: %s\n", SDL_GetError( ) );
			continue;
		}

		if( fullscreen )
		{
			SDL_DisplayMode mode;

			switch( testColorBits )
			{
				case 16: mode.format = SDL_PIXELFORMAT_RGB565; break;
				case 24: mode.format = SDL_PIXELFORMAT_RGB24;  break;
				default: ri.Printf( PRINT_DEVELOPER, "testColorBits is %d, can't fullscreen\n", testColorBits ); continue;
			}

			mode.w = width;
			mode.h = height;
			mode.refresh_rate = glConfig.displayFrequency = ri.Cvar_VariableIntegerValue( "r_displayRefresh" );
			mode.driverdata = NULL;

			if( SDL_SetWindowDisplayMode( screen, &mode ) < 0 )
			{
				ri.Printf( PRINT_DEVELOPER, "SDL_SetWindowDisplayMode failed: %s\n", SDL_GetError( ) );
				continue;
			}
		}

		SDL_SetWindowTitle( screen, CLIENT_WINDOW_TITLE );
		SDL_SetWindowIcon( screen, icon );

        if( ( opengl_context = (QGLContext)SDL_GL_CreateContext( screen ) ) == NULL )
		{
			Com_Printf( "SDL_GL_CreateContext failed: %s\n", SDL_GetError( ) );
			continue;
		}

		if( SDL_GL_MakeCurrent( screen, opengl_context ) < 0 )
		{
			Com_Printf( "SDL_GL_MakeCurrent failed: %s\n", SDL_GetError( ) );
			continue;
		}

		SDL_GL_SetSwapInterval( r_swapInterval->integer );

		glConfig.colorBits = testColorBits;
		glConfig.depthBits = testDepthBits;
		glConfig.stencilBits = testStencilBits;

		Com_Printf( "Using %d color bits, %d depth, %d stencil display.\n",
				glConfig.colorBits, glConfig.depthBits, glConfig.stencilBits );
		break;
	}
    
    SDL_GL_GetDrawableSize( screen, &glConfig.vidWidth, &glConfig.vidHeight );
    Com_Printf( "...renderer size: %d %d\n", glConfig.vidWidth, glConfig.vidHeight );
    
    glConfig.displayScale = GLimp_GetDisplayScale(display);
    glConfig.displayScale *= glConfig.vidHeight / height;
    
	/*SDL_FreeSurface( icon );*/

	GLimp_DetectAvailableModes();

	glstring = (char *) qglGetString (GL_RENDERER);
	Com_Printf( "GL_RENDERER: %s\n", glstring );

	return RSERR_OK;
#if 0
	const char*   glstring;
	int sdlcolorbits;
	int colorbits, depthbits, stencilbits;
	int tcolorbits, tdepthbits, tstencilbits;
	int samples;
	int i = 0;
	Uint32 flags = SDL_WINDOW_OPENGL;

	Com_Printf( "Initializing OpenGL display\n");

	if ( r_allowResize->integer )
		flags |= SDL_WINDOW_RESIZABLE;

	if( videoInfo == NULL )
	{
		static SDL_DisplayMode sVideoInfo;
		static SDL_PixelFormat sPixelFormat;

		if (SDL_GetCurrentDisplayMode( 0, &sVideoInfo ) < 0)
		  Com_Error(ERR_FATAL, "SDL_GetCurrentDisplayMode failed : %s\n", SDL_GetError());

		// Take a copy of the videoInfo
		sPixelFormat.format = sVideoInfo.format;
		sPixelFormat.palette = NULL; // Should already be the case
		//Com_Memcpy( &sVideoInfo, videoInfo, sizeof( SDL_DisplayMode ) );
		sVideoInfo.format = sPixelFormat.format;
		videoInfo = &sVideoInfo;

		if( videoInfo->h > 0 )
		{
			glConfig.displayWidth = videoInfo->w;
			glConfig.displayHeight = videoInfo->h;

			// Guess the display aspect ratio through the desktop resolution
			// by assuming (relatively safely) that it is set at or close to
			// the display's native aspect ratio
			glConfig.displayAspect = (float)videoInfo->w / (float)videoInfo->h;

			Com_Printf( "Estimated display aspect: %.3f\n", glConfig.displayAspect );
		}
		else
		{
			glConfig.displayWidth = 480;
			glConfig.displayHeight = 640;
			glConfig.displayAspect = 1.333f;

			Com_Printf(
					"Cannot estimate display resolution/aspect, assuming 640x480/1.333\n" );
		}
	}

	Com_Printf( "...setting mode %d:", mode );

	if (mode == -2)
	{
		// use desktop video resolution
		if( videoInfo->h > 0 )
		{
			glConfig.vidWidth = videoInfo->w;
			glConfig.vidHeight = videoInfo->h;
		}
		else
		{
			glConfig.vidWidth = 640;
			glConfig.vidHeight = 480;
			Com_Printf(
					"Cannot determine display resolution, assuming 640x480\n" );
		}

		glConfig.displayAspect = (float)glConfig.vidWidth / (float)glConfig.vidHeight;
	}
	else if ( !R_GetModeInfo( &glConfig.vidWidth, &glConfig.vidHeight, /*&glConfig.displayAspect,*/ mode ) )
	{
		Com_Printf( " invalid mode\n" );
		return RSERR_INVALID_MODE;
	}
	Com_Printf( " %d %d\n", glConfig.vidWidth, glConfig.vidHeight);

	if (fullscreen)
	{
		flags |= SDL_WINDOW_FULLSCREEN;
		glConfig.isFullscreen = qtrue;
	}
	else
	{
		if (noborder)
			flags |= SDL_WINDOW_BORDERLESS;

		glConfig.isFullscreen = qfalse;
	}

	colorbits = r_colorbits->value;
	if ((!colorbits) || (colorbits >= 32))
		colorbits = 24;

	if (!r_depthbits->value)
		depthbits = 24;
	else
		depthbits = r_depthbits->value;
	stencilbits = r_stencilbits->value;
    samples = r_ext_multisample->value;

	for (i = 0; i < 16; i++)
	{
		// 0 - default
		// 1 - minus colorbits
		// 2 - minus depthbits
		// 3 - minus stencil
		if ((i % 4) == 0 && i)
		{
			// one pass, reduce
			switch (i / 4)
			{
				case 2 :
					if (colorbits == 24)
						colorbits = 16;
					break;
				case 1 :
					if (depthbits == 24)
						depthbits = 16;
					else if (depthbits == 16)
						depthbits = 8;
				case 3 :
					if (stencilbits == 24)
						stencilbits = 16;
					else if (stencilbits == 16)
						stencilbits = 8;
			}
		}

		tcolorbits = colorbits;
		tdepthbits = depthbits;
		tstencilbits = stencilbits;

		if ((i % 4) == 3)
		{ // reduce colorbits
			if (tcolorbits == 24)
				tcolorbits = 16;
		}

		if ((i % 4) == 2)
		{ // reduce depthbits
			if (tdepthbits == 24)
				tdepthbits = 16;
			else if (tdepthbits == 16)
				tdepthbits = 8;
		}

		if ((i % 4) == 1)
		{ // reduce stencilbits
			if (tstencilbits == 24)
				tstencilbits = 16;
			else if (tstencilbits == 16)
				tstencilbits = 8;
			else
				tstencilbits = 0;
		}

		sdlcolorbits = 4;
		if (tcolorbits == 24)
			sdlcolorbits = 8;

#ifdef __sgi /* Fix for SGIs grabbing too many bits of color */
		if (sdlcolorbits == 4)
			sdlcolorbits = 0; /* Use minimum size for 16-bit color */

		/* Need alpha or else SGIs choose 36+ bit RGB mode */
		SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 1);
#endif

		SDL_GL_SetAttribute( SDL_GL_RED_SIZE, sdlcolorbits );
		SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, sdlcolorbits );
		SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, sdlcolorbits );
		SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, tdepthbits );
		SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, tstencilbits );

		SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, samples ? 1 : 0 );
		SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, samples );

		/*if(r_stereoEnabled->integer)
		{
			glConfig.stereoEnabled = qtrue;
			SDL_GL_SetAttribute(SDL_GL_STEREO, 1);
		}
		else
		{*/
			glConfig.stereoEnabled = qfalse;
			SDL_GL_SetAttribute(SDL_GL_STEREO, 0);
		//}

		SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

#if 0 // See http://bugzilla.icculus.org/show_bug.cgi?id=3526
		// If not allowing software GL, demand accelerated
		if( !r_allowSoftwareGL->integer )
		{
			if( SDL_GL_SetAttribute( SDL_GL_ACCELERATED_VISUAL, 1 ) < 0 )
			{
				Com_Printf( "Unable to guarantee accelerated "
						"visual with libSDL < 1.2.10\n" );
			}
		}
#endif

		if( SDL_GL_SetSwapInterval(r_swapInterval->integer ) < 0 )
			Com_Printf( "SDL_GL_SetSwapInterval not supported\n" );

#ifdef USE_ICON
		{
			SDL_Surface *icon = SDL_CreateRGBSurfaceFrom(
					(void *)CLIENT_WINDOW_ICON.pixel_data,
					CLIENT_WINDOW_ICON.width,
					CLIENT_WINDOW_ICON.height,
					CLIENT_WINDOW_ICON.bytes_per_pixel * 8,
					CLIENT_WINDOW_ICON.bytes_per_pixel * CLIENT_WINDOW_ICON.width,
#ifdef Q3_LITTLE_ENDIAN
					0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000
#else
					0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF
#endif
					);

			SDL_WM_SetIcon( icon, NULL );
			SDL_FreeSurface( icon );
		}
#endif

		SDL_ShowCursor(0);

		if (!(window = SDL_CreateWindow(CLIENT_WINDOW_TITLE, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, glConfig.vidWidth, glConfig.vidHeight, flags)))
		{
			Com_Printf( "SDL_CreateWindow failed: %s\n", SDL_GetError( ) );
			continue;
		}

		opengl_context = SDL_GL_CreateContext(window);

		Com_Printf( "Using %d/%d/%d Color bits, %d depth, %d stencil display.\n",
				sdlcolorbits, sdlcolorbits, sdlcolorbits, tdepthbits, tstencilbits);

		glConfig.colorBits = tcolorbits;
		glConfig.depthBits = tdepthbits;
		glConfig.stencilBits = tstencilbits;
		break;
	}

	GLimp_DetectAvailableModes();

	if (!window)
	{
		Com_Printf( "Couldn't get a visual\n" );
		return RSERR_INVALID_MODE;
	}

	screen = window;

	// FIXME: Defines needed here
	glstring = (char *) qglGetString (GL_RENDERER);
	Com_Printf( "GL_RENDERER: %s\n", glstring );

	return RSERR_OK;
#endif
}

/*
===============
GLimp_StartDriverAndSetMode
===============
*/
static qboolean GLimp_StartDriverAndSetMode(int mode, qboolean fullscreen, qboolean noborder)
{
	int i;
	rserr_t err;

	if (!SDL_WasInit(SDL_INIT_VIDEO))
	{
		char *driverName;

		if (SDL_Init(SDL_INIT_VIDEO) == -1)
		{
			Com_Printf( "SDL_Init( SDL_INIT_VIDEO ) FAILED (%s)\n",
					SDL_GetError());
			return qfalse;
		}

		if (!SDL_GetNumVideoDrivers())
		{
			Com_Printf( "SDL_GetNumVideoDrivers( ) FAILED (%s)\n",
					SDL_GetError());
			return qfalse;
		}

		//
		// TODO: Prompt the user to choose a specific video driver.
		driverName = (char *)SDL_GetVideoDriver( 0 );
		Com_Printf( "SDL using driver \"%s\"\n", driverName );
		ri.Cvar_Set( "r_sdlDriver", driverName );
	}

	if (fullscreen && ri.Cvar_VariableIntegerValue( "in_nograb" ) )
	{
		Com_Printf( "Fullscreen not allowed with in_nograb 1\n");
		ri.Cvar_Set( "r_fullscreen", "0" );
		r_fullscreen->modified = qfalse;
		fullscreen = qfalse;
	}

	err = GLimp_SetMode(mode, fullscreen, noborder);

	switch ( err )
	{
		case RSERR_INVALID_FULLSCREEN:
			Com_Printf( "...WARNING: fullscreen unavailable in this mode\n" );
			return qfalse;
		case RSERR_INVALID_MODE:
			Com_Printf( "...WARNING: could not set the given mode (%d)\n", mode );
			return qfalse;
		default:
			break;
	}

	return qtrue;
}

static int
SDL_SetGamma(SDL_Window *win, float red, float green, float blue)
{
    Uint16 red_ramp[256];
    Uint16 green_ramp[256];
    Uint16 blue_ramp[256];

    SDL_CalculateGammaRamp(red, red_ramp);
    if (green == red) {
        Com_Memcpy(&green_ramp, &red_ramp, sizeof(red_ramp));
    } else {
        SDL_CalculateGammaRamp(green, green_ramp);
    }
    if (blue == red) {
    	Com_Memcpy(blue_ramp, red_ramp, sizeof(red_ramp));
    } else {
        SDL_CalculateGammaRamp(blue, blue_ramp);
    }
    return SDL_SetWindowGammaRamp(win, red_ramp, green_ramp, blue_ramp);
}

static qboolean GLimp_HaveExtension(const char *ext)
{
	const char *ptr = Q_stristr( glConfig.extensions_string, ext );
	if (ptr == NULL)
		return qfalse;
	ptr += strlen(ext);
	return (qboolean)(((*ptr == ' ') || (*ptr == '\0')));  // verify it's complete string.
}

static void GLW_InitTextureCompression( void )
{
	qboolean newer_tc, old_tc;

	// Check for available tc methods.
	newer_tc = ( strstr( glConfig.extensions_string, "ARB_texture_compression" )
		&& strstr( glConfig.extensions_string, "EXT_texture_compression_s3tc" )) ? qtrue : qfalse;
	old_tc = ( strstr( glConfig.extensions_string, "GL_S3_s3tc" )) ? qtrue : qfalse;

	if ( old_tc )
	{
		Com_Printf ("...GL_S3_s3tc available\n" );
	}

	if ( newer_tc )
	{
		Com_Printf ("...GL_EXT_texture_compression_s3tc available\n" );
	}

	if ( !r_ext_compressed_textures->value )
	{
		// Compressed textures are off
		glConfig.textureCompression = TC_NONE;
		Com_Printf ("...ignoring texture compression\n" );
	}
	else if ( !old_tc && !newer_tc )
	{
		// Requesting texture compression, but no method found
		glConfig.textureCompression = TC_NONE;
		Com_Printf ("...no supported texture compression method found\n" );
		Com_Printf (".....ignoring texture compression\n" );
	}
	else
	{
		// some form of supported texture compression is avaiable, so see if the user has a preference
		if ( r_ext_preferred_tc_method->integer == TC_NONE )
		{
			// No preference, so pick the best
			if ( newer_tc )
			{
				Com_Printf ("...no tc preference specified\n" );
				Com_Printf (".....using GL_EXT_texture_compression_s3tc\n" );
				glConfig.textureCompression = TC_S3TC_DXT;
			}
			else
			{
				Com_Printf ("...no tc preference specified\n" );
				Com_Printf (".....using GL_S3_s3tc\n" );
				glConfig.textureCompression = TC_S3TC;
			}
		}
		else
		{
			// User has specified a preference, now see if this request can be honored
			if ( old_tc && newer_tc )
			{
				// both are avaiable, so we can use the desired tc method
				if ( r_ext_preferred_tc_method->integer == TC_S3TC )
				{
					Com_Printf ("...using preferred tc method, GL_S3_s3tc\n" );
					glConfig.textureCompression = TC_S3TC;
				}
				else
				{
					Com_Printf ("...using preferred tc method, GL_EXT_texture_compression_s3tc\n" );
					glConfig.textureCompression = TC_S3TC_DXT;
				}
			}
			else
			{
				// Both methods are not available, so this gets trickier
				if ( r_ext_preferred_tc_method->integer == TC_S3TC )
				{
					// Preferring to user older compression
					if ( old_tc )
					{
						Com_Printf ("...using GL_S3_s3tc\n" );
						glConfig.textureCompression = TC_S3TC;
					}
					else
					{
						// Drat, preference can't be honored
						Com_Printf ("...preferred tc method, GL_S3_s3tc not available\n" );
						Com_Printf (".....falling back to GL_EXT_texture_compression_s3tc\n" );
						glConfig.textureCompression = TC_S3TC_DXT;
					}
				}
				else
				{
					// Preferring to user newer compression
					if ( newer_tc )
					{
						Com_Printf ("...using GL_EXT_texture_compression_s3tc\n" );
						glConfig.textureCompression = TC_S3TC_DXT;
					}
					else
					{
						// Drat, preference can't be honored
						Com_Printf ("...preferred tc method, GL_EXT_texture_compression_s3tc not available\n" );
						Com_Printf (".....falling back to GL_S3_s3tc\n" );
						glConfig.textureCompression = TC_S3TC;
					}
				}
			}
		}
	}
}

/*
===============
GLimp_InitExtensions
===============
*/
extern bool g_bDynamicGlowSupported;
static void GLimp_InitExtensions( void )
{
	if ( !r_allowExtensions->integer )
	{
		Com_Printf ("*** IGNORING OPENGL EXTENSIONS ***\n" );
		g_bDynamicGlowSupported = false;
		ri.Cvar_Set( "r_DynamicGlow","0" );
		return;
	}

	Com_Printf ("Initializing OpenGL extensions\n" );

	// Select our tc scheme
	GLW_InitTextureCompression();

	// GL_EXT_texture_env_add
	glConfig.textureEnvAddAvailable = qfalse;
	if ( strstr( glConfig.extensions_string, "EXT_texture_env_add" ) )
	{
		if ( r_ext_texture_env_add->integer )
		{
			glConfig.textureEnvAddAvailable = qtrue;
			Com_Printf ("...using GL_EXT_texture_env_add\n" );
		}
		else
		{
			glConfig.textureEnvAddAvailable = qfalse;
			Com_Printf ("...ignoring GL_EXT_texture_env_add\n" );
		}
	}
	else
	{
		Com_Printf ("...GL_EXT_texture_env_add not found\n" );
	}

	// GL_EXT_texture_filter_anisotropic
	glConfig.maxTextureFilterAnisotropy = 0;
	if ( strstr( glConfig.extensions_string, "EXT_texture_filter_anisotropic" ) )
	{
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF	//can't include glext.h here ... sigh
		qglGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &glConfig.maxTextureFilterAnisotropy );
		Com_Printf ("...GL_EXT_texture_filter_anisotropic available\n" );

		if ( r_ext_texture_filter_anisotropic->integer>1 )
		{
			Com_Printf ("...using GL_EXT_texture_filter_anisotropic\n" );
		}
		else
		{
			Com_Printf ("...ignoring GL_EXT_texture_filter_anisotropic\n" );
		}
		ri.Cvar_Set( "r_ext_texture_filter_anisotropic_avail", va("%f",glConfig.maxTextureFilterAnisotropy) );
		if ( r_ext_texture_filter_anisotropic->value > glConfig.maxTextureFilterAnisotropy )
		{
			ri.Cvar_Set( "r_ext_texture_filter_anisotropic", va("%f",glConfig.maxTextureFilterAnisotropy) );
		}
	}
	else
	{
		Com_Printf ("...GL_EXT_texture_filter_anisotropic not found\n" );
		ri.Cvar_Set( "r_ext_texture_filter_anisotropic_avail", "0" );
	}

	// GL_EXT_clamp_to_edge
	glConfig.clampToEdgeAvailable = qfalse;
	if ( strstr( glConfig.extensions_string, "GL_EXT_texture_edge_clamp" ) )
	{
		glConfig.clampToEdgeAvailable = qtrue;
		Com_Printf ("...Using GL_EXT_texture_edge_clamp\n" );
	}

	// GL_ARB_multitexture
	qglMultiTexCoord2fARB = NULL;
	qglActiveTextureARB = NULL;
	qglClientActiveTextureARB = NULL;
	if ( strstr( glConfig.extensions_string, "GL_ARB_multitexture" )  )
	{
		if ( r_ext_multitexture->integer )
		{
			qglMultiTexCoord2fARB = ( PFNGLMULTITEXCOORD2FARBPROC ) SDL_GL_GetProcAddress( "glMultiTexCoord2fARB" );
			qglActiveTextureARB = ( PFNGLACTIVETEXTUREARBPROC ) SDL_GL_GetProcAddress( "glActiveTextureARB" );
			qglClientActiveTextureARB = ( PFNGLCLIENTACTIVETEXTUREARBPROC ) SDL_GL_GetProcAddress( "glClientActiveTextureARB" );

			if ( qglActiveTextureARB )
			{
				qglGetIntegerv( GL_MAX_ACTIVE_TEXTURES_ARB, &glConfig.maxActiveTextures );

				if ( glConfig.maxActiveTextures > 1 )
				{
					Com_Printf ("...using GL_ARB_multitexture\n" );
				}
				else
				{
					qglMultiTexCoord2fARB = NULL;
					qglActiveTextureARB = NULL;
					qglClientActiveTextureARB = NULL;
					Com_Printf ("...not using GL_ARB_multitexture, < 2 texture units\n" );
				}
			}
		}
		else
		{
			Com_Printf ("...ignoring GL_ARB_multitexture\n" );
		}
	}
	else
	{
		Com_Printf ("...GL_ARB_multitexture not found\n" );
	}

	// GL_EXT_compiled_vertex_array
	qglLockArraysEXT = NULL;
	qglUnlockArraysEXT = NULL;
	if ( strstr( glConfig.extensions_string, "GL_EXT_compiled_vertex_array" ) )
	{
		if ( r_ext_compiled_vertex_array->integer )
		{
			Com_Printf ("...using GL_EXT_compiled_vertex_array\n" );
			qglLockArraysEXT = ( void ( APIENTRY * )( int, int ) ) SDL_GL_GetProcAddress( "glLockArraysEXT" );
			qglUnlockArraysEXT = ( void ( APIENTRY * )( void ) ) SDL_GL_GetProcAddress( "glUnlockArraysEXT" );
			if (!qglLockArraysEXT || !qglUnlockArraysEXT) {
				Com_Error (ERR_FATAL, "bad getprocaddress");
			}
		}
		else
		{
			Com_Printf ("...ignoring GL_EXT_compiled_vertex_array\n" );
		}
	}
	else
	{
		Com_Printf ("...GL_EXT_compiled_vertex_array not found\n" );
	}

	qglPointParameterfEXT = NULL;
	qglPointParameterfvEXT = NULL;

	//3d textures -rww
	qglTexImage3DEXT = NULL;
	qglTexSubImage3DEXT = NULL;

	if ( strstr( glConfig.extensions_string, "GL_EXT_point_parameters" ) )
	{
		if ( r_ext_compiled_vertex_array->integer || 1)
		{
			Com_Printf ("...using GL_EXT_point_parameters\n" );
			qglPointParameterfEXT = ( void ( APIENTRY * )( GLenum, GLfloat) ) SDL_GL_GetProcAddress( "glPointParameterfEXT" );
			qglPointParameterfvEXT = ( void ( APIENTRY * )( GLenum, GLfloat *) ) SDL_GL_GetProcAddress( "glPointParameterfvEXT" );

			//3d textures -rww
			qglTexImage3DEXT = (void ( APIENTRY * ) (GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *) ) SDL_GL_GetProcAddress( "glTexImage3DEXT" );
			qglTexSubImage3DEXT = (void ( APIENTRY * ) (GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *) ) SDL_GL_GetProcAddress( "glTexSubImage3DEXT" );

			if (!qglPointParameterfEXT || !qglPointParameterfvEXT)
			{
				Com_Error (ERR_FATAL, "bad getprocaddress");
			}
		}
		else
		{
			Com_Printf ("...ignoring GL_EXT_point_parameters\n" );
		}
	}
	else
	{
		Com_Printf ("...GL_EXT_point_parameters not found\n" );
	}

	bool bNVRegisterCombiners = false;
	// Register Combiners.
	if ( strstr( glConfig.extensions_string, "GL_NV_register_combiners" ) )
	{
		// NOTE: This extension requires multitexture support (over 2 units).
		if ( glConfig.maxActiveTextures >= 2 )
		{
			bNVRegisterCombiners = true;
			// Register Combiners function pointer address load.	- AReis
			// NOTE: VV guys will _definetly_ not be able to use regcoms. Pixel Shaders are just as good though :-)
			// NOTE: Also, this is an nVidia specific extension (of course), so fragment shaders would serve the same purpose
			// if we needed some kind of fragment/pixel manipulation support.
			qglCombinerParameterfvNV = (PFNGLCOMBINERPARAMETERFVNVPROC)SDL_GL_GetProcAddress( "glCombinerParameterfvNV" );
			qglCombinerParameterivNV = (PFNGLCOMBINERPARAMETERIVNVPROC)SDL_GL_GetProcAddress( "glCombinerParameterivNV" );
			qglCombinerParameterfNV = (PFNGLCOMBINERPARAMETERFNVPROC)SDL_GL_GetProcAddress( "glCombinerParameterfNV" );
			qglCombinerParameteriNV = (PFNGLCOMBINERPARAMETERINVPROC)SDL_GL_GetProcAddress( "glCombinerParameteriNV" );
			qglCombinerInputNV = (PFNGLCOMBINERINPUTNVPROC)SDL_GL_GetProcAddress( "glCombinerInputNV" );
			qglCombinerOutputNV = (PFNGLCOMBINEROUTPUTNVPROC)SDL_GL_GetProcAddress( "glCombinerOutputNV" );
			qglFinalCombinerInputNV = (PFNGLFINALCOMBINERINPUTNVPROC)SDL_GL_GetProcAddress( "glFinalCombinerInputNV" );
			qglGetCombinerInputParameterfvNV	= (PFNGLGETCOMBINERINPUTPARAMETERFVNVPROC)SDL_GL_GetProcAddress( "glGetCombinerInputParameterfvNV" );
			qglGetCombinerInputParameterivNV	= (PFNGLGETCOMBINERINPUTPARAMETERIVNVPROC)SDL_GL_GetProcAddress( "glGetCombinerInputParameterivNV" );
			qglGetCombinerOutputParameterfvNV = (PFNGLGETCOMBINEROUTPUTPARAMETERFVNVPROC)SDL_GL_GetProcAddress( "glGetCombinerOutputParameterfvNV" );
			qglGetCombinerOutputParameterivNV = (PFNGLGETCOMBINEROUTPUTPARAMETERIVNVPROC)SDL_GL_GetProcAddress( "glGetCombinerOutputParameterivNV" );
			qglGetFinalCombinerInputParameterfvNV = (PFNGLGETFINALCOMBINERINPUTPARAMETERFVNVPROC)SDL_GL_GetProcAddress( "glGetFinalCombinerInputParameterfvNV" );
			qglGetFinalCombinerInputParameterivNV = (PFNGLGETFINALCOMBINERINPUTPARAMETERIVNVPROC)SDL_GL_GetProcAddress( "glGetFinalCombinerInputParameterivNV" );

			// Validate the functions we need.
			if ( !qglCombinerParameterfvNV || !qglCombinerParameterivNV || !qglCombinerParameterfNV || !qglCombinerParameteriNV || !qglCombinerInputNV ||
				 !qglCombinerOutputNV || !qglFinalCombinerInputNV || !qglGetCombinerInputParameterfvNV || !qglGetCombinerInputParameterivNV ||
				 !qglGetCombinerOutputParameterfvNV || !qglGetCombinerOutputParameterivNV || !qglGetFinalCombinerInputParameterfvNV || !qglGetFinalCombinerInputParameterivNV )
			{
				bNVRegisterCombiners = false;
				qglCombinerParameterfvNV = NULL;
				qglCombinerParameteriNV = NULL;
				Com_Printf ("...GL_NV_register_combiners failed\n" );
			}
		}
		else
		{
			bNVRegisterCombiners = false;
			Com_Printf ("...ignoring GL_NV_register_combiners\n" );
		}
	}
	else
	{
		bNVRegisterCombiners = false;
		Com_Printf ("...GL_NV_register_combiners not found\n" );
	}

	// NOTE: Vertex and Fragment Programs are very dependant on each other - this is actually a
	// good thing! So, just check to see which we support (one or the other) and load the shared
	// function pointers. ARB rocks!

	// Vertex Programs.
	bool bARBVertexProgram = false;
	if ( strstr( glConfig.extensions_string, "GL_ARB_vertex_program" ) )
	{
		bARBVertexProgram = true;
	}
	else
	{
		bARBVertexProgram = false;
		Com_Printf ("...GL_ARB_vertex_program not found\n" );
	}

	// Fragment Programs.
	bool bARBFragmentProgram = false;
	if ( strstr( glConfig.extensions_string, "GL_ARB_fragment_program" ) )
	{
		bARBFragmentProgram = true;
	}
	else
	{
		bARBFragmentProgram = false;
		Com_Printf ("...GL_ARB_fragment_program not found\n" );
	}

	// If we support one or the other, load the shared function pointers.
	if ( bARBVertexProgram || bARBFragmentProgram )
	{
		qglProgramStringARB					= (PFNGLPROGRAMSTRINGARBPROC)  SDL_GL_GetProcAddress("glProgramStringARB");
		qglBindProgramARB					= (PFNGLBINDPROGRAMARBPROC)    SDL_GL_GetProcAddress("glBindProgramARB");
		qglDeleteProgramsARB				= (PFNGLDELETEPROGRAMSARBPROC) SDL_GL_GetProcAddress("glDeleteProgramsARB");
		qglGenProgramsARB					= (PFNGLGENPROGRAMSARBPROC)    SDL_GL_GetProcAddress("glGenProgramsARB");
		qglProgramEnvParameter4dARB			= (PFNGLPROGRAMENVPARAMETER4DARBPROC)    SDL_GL_GetProcAddress("glProgramEnvParameter4dARB");
		qglProgramEnvParameter4dvARB		= (PFNGLPROGRAMENVPARAMETER4DVARBPROC)   SDL_GL_GetProcAddress("glProgramEnvParameter4dvARB");
		qglProgramEnvParameter4fARB			= (PFNGLPROGRAMENVPARAMETER4FARBPROC)    SDL_GL_GetProcAddress("glProgramEnvParameter4fARB");
		qglProgramEnvParameter4fvARB		= (PFNGLPROGRAMENVPARAMETER4FVARBPROC)   SDL_GL_GetProcAddress("glProgramEnvParameter4fvARB");
		qglProgramLocalParameter4dARB		= (PFNGLPROGRAMLOCALPARAMETER4DARBPROC)  SDL_GL_GetProcAddress("glProgramLocalParameter4dARB");
		qglProgramLocalParameter4dvARB		= (PFNGLPROGRAMLOCALPARAMETER4DVARBPROC) SDL_GL_GetProcAddress("glProgramLocalParameter4dvARB");
		qglProgramLocalParameter4fARB		= (PFNGLPROGRAMLOCALPARAMETER4FARBPROC)  SDL_GL_GetProcAddress("glProgramLocalParameter4fARB");
		qglProgramLocalParameter4fvARB		= (PFNGLPROGRAMLOCALPARAMETER4FVARBPROC) SDL_GL_GetProcAddress("glProgramLocalParameter4fvARB");
		qglGetProgramEnvParameterdvARB		= (PFNGLGETPROGRAMENVPARAMETERDVARBPROC) SDL_GL_GetProcAddress("glGetProgramEnvParameterdvARB");
		qglGetProgramEnvParameterfvARB		= (PFNGLGETPROGRAMENVPARAMETERFVARBPROC) SDL_GL_GetProcAddress("glGetProgramEnvParameterfvARB");
		qglGetProgramLocalParameterdvARB	= (PFNGLGETPROGRAMLOCALPARAMETERDVARBPROC) SDL_GL_GetProcAddress("glGetProgramLocalParameterdvARB");
		qglGetProgramLocalParameterfvARB	= (PFNGLGETPROGRAMLOCALPARAMETERFVARBPROC) SDL_GL_GetProcAddress("glGetProgramLocalParameterfvARB");
		qglGetProgramivARB					= (PFNGLGETPROGRAMIVARBPROC)     SDL_GL_GetProcAddress("glGetProgramivARB");
		qglGetProgramStringARB				= (PFNGLGETPROGRAMSTRINGARBPROC) SDL_GL_GetProcAddress("glGetProgramStringARB");
		qglIsProgramARB						= (PFNGLISPROGRAMARBPROC)        SDL_GL_GetProcAddress("glIsProgramARB");
		qglGenBuffersARB					= (PFNGLGENBUFFERSARBPROC)       SDL_GL_GetProcAddress("glGenBuffersARB");
		qglBindBufferARB					= (PFNGLBINDBUFFERARBPROC)       SDL_GL_GetProcAddress("glBindBufferARB");
		qglBufferDataARB					= (PFNGLBUFFERDATAARBPROC)       SDL_GL_GetProcAddress("glBufferDataARB");
		qglMapBufferARB						= (PFNGLMAPBUFFERARBPROC)		 SDL_GL_GetProcAddress("glMapBufferARB");
		qglUnmapBufferARB					= (PFNGLUNMAPBUFFERARBPROC)		 SDL_GL_GetProcAddress("glUnmapBufferARB");

		// Validate the functions we need.
		if ( !qglProgramStringARB || !qglBindProgramARB || !qglDeleteProgramsARB || !qglGenProgramsARB ||
			 !qglProgramEnvParameter4dARB || !qglProgramEnvParameter4dvARB || !qglProgramEnvParameter4fARB ||
             !qglProgramEnvParameter4fvARB || !qglProgramLocalParameter4dARB || !qglProgramLocalParameter4dvARB ||
             !qglProgramLocalParameter4fARB || !qglProgramLocalParameter4fvARB || !qglGetProgramEnvParameterdvARB ||
             !qglGetProgramEnvParameterfvARB || !qglGetProgramLocalParameterdvARB || !qglGetProgramLocalParameterfvARB ||
             !qglGetProgramivARB || !qglGetProgramStringARB || !qglIsProgramARB )
		{
			bARBVertexProgram = false;
			bARBFragmentProgram = false;
			qglGenProgramsARB = NULL;	//clear ptrs that get checked
			qglProgramEnvParameter4fARB = NULL;
			Com_Printf ("...ignoring GL_ARB_vertex_program\n" );
			Com_Printf ("...ignoring GL_ARB_fragment_program\n" );
		}
	}

	// Figure out which texture rectangle extension to use.
	bool bTexRectSupported = false;
	if ( strnicmp( glConfig.vendor_string, "ATI Technologies",16 )==0
		&& strnicmp( glConfig.version_string, "1.3.3",5 )==0
		&& glConfig.version_string[5] < '9' ) //1.3.34 and 1.3.37 and 1.3.38 are broken for sure, 1.3.39 is not
	{
		g_bTextureRectangleHack = true;
	}

	if ( strstr( glConfig.extensions_string, "GL_NV_texture_rectangle" )
		   || strstr( glConfig.extensions_string, "GL_EXT_texture_rectangle" ) )
	{
		bTexRectSupported = true;
	}

	// Find out how many general combiners they have.
	#define GL_MAX_GENERAL_COMBINERS_NV       0x854D
	GLint iNumGeneralCombiners = 0;
	qglGetIntegerv( GL_MAX_GENERAL_COMBINERS_NV, &iNumGeneralCombiners );

	// Only allow dynamic glows/flares if they have the hardware
	if ( bTexRectSupported && bARBVertexProgram && qglActiveTextureARB && glConfig.maxActiveTextures >= 4 &&
		( ( bNVRegisterCombiners && iNumGeneralCombiners >= 2 ) || bARBFragmentProgram ) )
	{
		g_bDynamicGlowSupported = true;
		// this would overwrite any achived setting gwg
		// Cvar_Set( "r_DynamicGlow", "1" );
	}
	else
	{
		g_bDynamicGlowSupported = false;
		ri.Cvar_Set( "r_DynamicGlow","0" );
	}

	//mme
	if ( strstr( glConfig.extensions_string, "GL_EXT_texture_filter_anisotropic" ) )
	{

		qglGetIntegerv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &glMMEConfig.maxAnisotropy );
		if ( glMMEConfig.maxAnisotropy <= 0 ) {
			ri.Printf( PRINT_ALL, "...GL_EXT_texture_filter_anisotropic not properly supported!\n" );
		} else {
			ri.Printf( PRINT_ALL, "...using GL_EXT_texture_filter_anisotropic (max: %d)\n", glMMEConfig.maxAnisotropy );
		}
	}
	else
	{
		glMMEConfig.maxAnisotropy = 0;
		ri.Printf( PRINT_ALL, "...GL_EXT_texture_filter_anisotropic not found\n" );
	}

	glMMEConfig.framebufferObject = qfalse;
	glMMEConfig.shaderSupport = qfalse;
	if ( strstr( glConfig.extensions_string, "GL_EXT_framebuffer_object" ) &&
			(1 || strstr( glConfig.extensions_string, "GL_ARB_texture_non_power_of_two" )) ) {
		ri.Printf( PRINT_ALL, "...using GL_EXT_framebuffer_object\n" );
		glMMEConfig.framebufferObject = qtrue;
		qglGenFramebuffers = ( void (APIENTRY *  )(GLsizei, GLuint *) ) SDL_GL_GetProcAddress( "glGenFramebuffersEXT");
		qglBindFramebuffer = ( void (APIENTRY *  )(GLenum, GLuint) ) SDL_GL_GetProcAddress( "glBindFramebufferEXT");
		qglGenRenderbuffers = ( void (APIENTRY *  )(GLsizei, GLuint *) ) SDL_GL_GetProcAddress( "glGenRenderbuffersEXT");
		qglBindRenderbuffer = ( void (APIENTRY *  )(GLenum, GLuint) ) SDL_GL_GetProcAddress( "glBindRenderbufferEXT");
		qglRenderbufferStorage = ( void (APIENTRY *  )(GLenum, GLenum, GLsizei, GLsizei) ) SDL_GL_GetProcAddress( "glRenderbufferStorageEXT");
		qglFramebufferRenderbuffer = ( void (APIENTRY *  )(GLenum, GLenum, GLenum, GLuint) ) SDL_GL_GetProcAddress( "glFramebufferRenderbufferEXT");
		qglFramebufferTexture2D = ( void (APIENTRY *  )(GLenum, GLenum, GLenum, GLuint, GLint) ) SDL_GL_GetProcAddress( "glFramebufferTexture2DEXT");
		qglCheckFramebufferStatus = ( GLenum (APIENTRY *)(GLenum) ) SDL_GL_GetProcAddress( "glCheckFramebufferStatusEXT");
		qglDeleteFramebuffers = ( void (APIENTRY * )(GLsizei, const GLuint *) ) SDL_GL_GetProcAddress( "glDeleteFramebuffersEXT");
		qglDeleteRenderbuffers = ( void (APIENTRY * )(GLsizei, const GLuint *) ) SDL_GL_GetProcAddress( "glDeleteRenderbuffersEXT");
	
		if ( !strstr( glConfig.extensions_string, "GL_ARB_depth_texture" ) )  {
			ri.Printf( PRINT_WARNING, "WARNING: GL_ARB_depth_texture is missing\n");
		}
		if (	!strstr( glConfig.extensions_string, "GL_EXT_packed_depth_stencil" )  || 
				!strstr( glConfig.extensions_string, "GL_NV_packed_depth_stencil" ) ) {
			ri.Printf( PRINT_WARNING, "WARNING: packed_depth_stencil is missing\n");
		}
	}
	glMMEConfig.framebufferMultiSample = qfalse;
	if ( strstr( glConfig.extensions_string, "GL_EXT_framebuffer_multisample") && strstr( glConfig.extensions_string, "GL_EXT_framebuffer_blit")) {
		qglRenderbufferStorageMultisampleEXT = (void (APIENTRYP) ( GLenum, GLsizei, GLenum, GLsizei, GLsizei ) )SDL_GL_GetProcAddress( "glRenderbufferStorageMultisampleEXT" );
		qglBlitFramebufferEXT = (void (APIENTRYP) ( GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLbitfield, GLenum ) )  SDL_GL_GetProcAddress( "glBlitFramebufferEXT" );
		glMMEConfig.framebufferMultiSample = qfalse;
	}
		 		

	//added fragment/vertex program extensions
	if ( strstr( glConfig.extensions_string, "GL_ARB_fragment_shader" ) && 
		 strstr( glConfig.extensions_string, "GL_ARB_vertex_program" ) &&
		 strstr( glConfig.extensions_string, "GL_ARB_vertex_shader" ) &&
		 strstr( glConfig.extensions_string, "GL_ARB_fragment_program" ) &&
		 strstr( glConfig.extensions_string, "GL_ARB_shading_language_100" )) 
	{
		ri.Printf( PRINT_ALL, "...using GL_ARB_fragment_program\n" );
		ri.Printf( PRINT_ALL, "...using GL_ARB_vertex_program\n" );
		ri.Printf( PRINT_ALL, "...using GL_ARB_shading_language_100\n" );
		glMMEConfig.shaderSupport = qtrue;
		qglAttachShader = ( void (APIENTRY * ) (GLuint, GLuint) ) SDL_GL_GetProcAddress( "glAttachShader");
		qglBindAttribLocation = ( void (APIENTRY * ) (GLuint, GLuint, const GLchar *) ) SDL_GL_GetProcAddress( "glBindAttribLocation");
		qglCompileShader = ( void (APIENTRY * ) (GLuint) ) SDL_GL_GetProcAddress( "glCompileShader");
		qglCreateProgram = ( GLuint (APIENTRY * ) (void) ) SDL_GL_GetProcAddress( "glCreateProgram");
		qglCreateShader = ( GLuint (APIENTRY * ) (GLenum) ) SDL_GL_GetProcAddress( "glCreateShader");
		qglDeleteProgram = ( void (APIENTRY * ) (GLuint) ) SDL_GL_GetProcAddress( "glDeleteProgram");
		qglDeleteShader = ( void (APIENTRY * ) (GLuint) ) SDL_GL_GetProcAddress( "glDeleteShader");
		qglShaderSource = ( void (APIENTRY * ) (GLuint, GLsizei, const GLchar* *, const GLint *) ) SDL_GL_GetProcAddress( "glShaderSource");
		qglLinkProgram = ( void (APIENTRY * ) (GLuint) ) SDL_GL_GetProcAddress( "glLinkProgram");
		qglUseProgram = ( void (APIENTRY * ) (GLuint) ) SDL_GL_GetProcAddress( "glUseProgram");
		qglGetUniformLocation = ( GLint (APIENTRY * ) (GLuint, const GLchar *) ) SDL_GL_GetProcAddress( "glGetUniformLocation");
		qglUniform1f = ( void (APIENTRY * ) (GLint, GLfloat) ) SDL_GL_GetProcAddress( "glUniform1f");
		qglUniform2f = ( void (APIENTRY * ) (GLint, GLfloat, GLfloat) ) SDL_GL_GetProcAddress( "glUniform2f");
		qglUniform1i = ( void (APIENTRY * ) (GLint, GLint) ) SDL_GL_GetProcAddress( "glUniform1i");
		qglGetProgramiv = ( void (APIENTRY * ) (GLuint, GLenum, GLint *) ) SDL_GL_GetProcAddress( "glGetProgramiv");
		qglGetProgramInfoLog = ( void (APIENTRY * ) (GLuint, GLsizei, GLsizei *, GLchar *) ) SDL_GL_GetProcAddress( "glGetProgramInfoLog");
		qglGetShaderiv = ( void (APIENTRY * ) (GLuint, GLenum, GLint *) ) SDL_GL_GetProcAddress( "glGetShaderiv");
		qglGetShaderInfoLog = ( void (APIENTRY * ) (GLuint, GLsizei, GLsizei *, GLchar *) ) SDL_GL_GetProcAddress( "glGetShaderInfoLog");
	}
}

void 		GLimp_Init( void )
{
	ri.Cvar_Get( "r_restartOnResize", "1", CVAR_ARCHIVE );
	ri.Cvar_Get( "r_resizeDelay", "1000", CVAR_ARCHIVE );
	r_allowSoftwareGL = ri.Cvar_Get( "r_allowSoftwareGL", "0", CVAR_LATCH );
	r_sdlDriver = ri.Cvar_Get( "r_sdlDriver", "", CVAR_ROM | CVAR_LATCH );
    r_sdlHighDpi = ri.Cvar_Get( "r_sdlHighDpi", "0", CVAR_ARCHIVE );
	r_allowResize = ri.Cvar_Get( "r_allowResize", "0", CVAR_ARCHIVE );

	/*	if( Cvar_VariableIntegerValue( "com_abnormalExit" ) )
	{
		Cvar_Set( "r_mode", va( "%d", R_MODE_FALLBACK ) );
		Cvar_Set( "r_picmap", "1" );
		Cvar_Set( "r_texturebits", "0" );
		Cvar_Set( "r_textureMode", "GL_LINEAR_MIPMAP_NEAREST" );
		Cvar_Set( "r_fullscreen", "0" );
		Cvar_Set( "r_centerWindow", "0" );
		Cvar_Set( "com_abnormalExit", "0" );
		}*/

	Sys_SetEnv( "SDL_VIDEO_CENTERED", r_centerWindow->integer ? "1" : "" );

	// Create the window and set up the context
	if(GLimp_StartDriverAndSetMode(r_mode->integer, (qboolean)r_fullscreen->integer, (qboolean)r_noborder->integer))
		goto success;

	// Try again, this time in a platform specific "safe mode"
	/* Sys_GLimpSafeInit( );

	if(GLimp_StartDriverAndSetMode(r_mode->integer, r_fullscreen->integer, qfalse))
		goto success; */

	/*	// Finally, try the default screen resolution
	if( r_mode->integer != R_MODE_FALLBACK )
	{
		Com_Printf( "Setting r_mode %d failed, falling back on r_mode %d\n",
				r_mode->integer, R_MODE_FALLBACK );

		if(GLimp_StartDriverAndSetMode(R_MODE_FALLBACK, qfalse, qfalse))
			goto success;
			}*/

	// Nothing worked, give up
	Com_Error( ERR_FATAL, "GLimp_Init() - could not load OpenGL subsystem" );

success:
	// This values force the UI to disable driver selection
	//	glConfig.driverType = GLDRV_ICD;
	//	glConfig.hardwareType = GLHW_GENERIC;
	glConfig.deviceSupportsGamma = (qboolean)(SDL_SetGamma( screen, 1.0f, 1.0f, 1.0f ) >= 0);

	// Mysteriously, if you use an NVidia graphics card and multiple monitors,
	// SDL_SetGamma will incorrectly return false... the first time; ask
	// again and you get the correct answer. This is a suspected driver bug, see
	// http://bugzilla.icculus.org/show_bug.cgi?id=4316
	glConfig.deviceSupportsGamma = (qboolean)(SDL_SetGamma( screen, 1.0f, 1.0f, 1.0f ) >= 0);

	if ( -1 == r_ignorehwgamma->integer)
		glConfig.deviceSupportsGamma = qtrue;

	if ( 1 == r_ignorehwgamma->integer)
		glConfig.deviceSupportsGamma = qfalse;

	// get our config strings
/*	Q_strncpyz( glConfig.vendor_string, (char *) qglGetString (GL_VENDOR), sizeof( glConfig.vendor_string ) );
	Q_strncpyz( glConfig.renderer_string, (char *) qglGetString (GL_RENDERER), sizeof( glConfig.renderer_string ) );
	if (*glConfig.renderer_string && glConfig.renderer_string[strlen(glConfig.renderer_string) - 1] == '\n')
		glConfig.renderer_string[strlen(glConfig.renderer_string) - 1] = 0;
	Q_strncpyz( glConfig.version_string, (char *) qglGetString (GL_VERSION), sizeof( glConfig.version_string ) );
	Q_strncpyz( glConfig.extensions_string, (char *) qglGetString (GL_EXTENSIONS), sizeof( glConfig.extensions_string ) );*/

    glConfig.vendor_string = (const char *) qglGetString (GL_VENDOR);
	glConfig.renderer_string = (const char *) qglGetString (GL_RENDERER);
	glConfig.version_string = (const char *) qglGetString (GL_VERSION);
	glConfig.extensions_string = (const char *) qglGetString (GL_EXTENSIONS);

	// OpenGL driver constants
	qglGetIntegerv( GL_MAX_TEXTURE_SIZE, &glConfig.maxTextureSize );
	// stubbed or broken drivers may have reported 0...
	if ( glConfig.maxTextureSize <= 0 )
	{
		glConfig.maxTextureSize = 0;
	}

	// initialize extensions
	GLimp_InitExtensions( );

	ri.Cvar_Get( "r_availableModes", "", CVAR_ROM );

	// This depends on SDL_INIT_VIDEO, hence having it here
	ri.IN_Init( screen );
}

/*
===============
GLimp_Shutdown
===============
*/
void 		GLimp_Shutdown( void )
{
	ri.IN_Shutdown();

	SDL_QuitSubSystem( SDL_INIT_VIDEO );

	Com_Memset( &glConfig, 0, sizeof( glConfig ) );
	Com_Memset( &glState, 0, sizeof( glState ) );
}

void		GLimp_EnableLogging( qboolean enable )
{
}

void 		GLimp_LogComment( char *comment )
{
}

void GLimp_SetGamma( unsigned char red[256], unsigned char green[256], unsigned char blue[256] )
{
	Uint16 table[3][256];
	int i, j;
	
	if( !glConfig.deviceSupportsGamma || r_ignorehwgamma->integer > 0 )
		return;
	
	for (i = 0; i < 256; i++)
	{
		table[0][i] = ( ( ( Uint16 ) red[i] ) << 8 ) | red[i];
		table[1][i] = ( ( ( Uint16 ) green[i] ) << 8 ) | green[i];
		table[2][i] = ( ( ( Uint16 ) blue[i] ) << 8 ) | blue[i];
	}
	
#ifdef _WIN32
#include <windows.h>
	
	// Win2K and newer put this odd restriction on gamma ramps...
	{
		OSVERSIONINFO	vinfo;
		
		vinfo.dwOSVersionInfoSize = sizeof( vinfo );
		GetVersionEx( &vinfo );
		if( vinfo.dwMajorVersion >= 5 && vinfo.dwPlatformId == VER_PLATFORM_WIN32_NT )
		{
			ri.Printf( PRINT_DEVELOPER, "performing gamma clamp.\n" );
			for( j = 0 ; j < 3 ; j++ )
			{
				for( i = 0 ; i < 128 ; i++ )
				{
					if( table[ j ] [ i] > ( ( 128 + i ) << 8 ) )
						table[ j ][ i ] = ( 128 + i ) << 8;
				}
				
				if( table[ j ] [127 ] > 254 << 8 )
					table[ j ][ 127 ] = 254 << 8;
			}
		}
	}
#endif
	
	// enforce constantly increasing
	for (j = 0; j < 3; j++)
	{
		for (i = 1; i < 256; i++)
		{
			if (table[j][i] < table[j][i-1])
				table[j][i] = table[j][i-1];
		}
	}
	
	SDL_SetWindowGammaRamp(screen, table[0], table[1], table[2]);
}
