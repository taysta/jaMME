//Anything above this #include will be ignored by the compiler
#include "../qcommon/exe_headers.h"

// tr_init.c -- functions that are not called every frame

#include "tr_local.h"
#include "tr_WorldEffects.h"
#include "tr_font.h"
#include "../qcommon/MiniHeap.h"
#include "../qcommon/qcommon.h"
#include "G2_local.h"
#include "../libpng/png.h"
//#include "zlib/zlib.h" // used for old mme stuff

//#ifdef __USEA3D
//// Defined in snd_a3dg_refcommon.c
//void RE_A3D_RenderGeometry (void *pVoidA3D, void *pVoidGeom, void *pVoidMat, void *pVoidGeomStatus);
//#endif

glconfig_t	glConfig;
glstate_t	glState;

glMMEConfig_t	glMMEConfig;

static void GfxInfo_f( void );

// global now, we share data directly on it.
refexport_t re = {0};

cvar_t	*r_verbose;
cvar_t	*r_ignore;

cvar_t	*r_displayRefresh;

cvar_t	*r_detailTextures;

cvar_t	*r_znear;				// near Z clip plane
cvar_t	*r_zproj;				// z distance of projection plane
cvar_t	*r_stereoSeparation;	// separation of cameras for stereo capture

cvar_t	*r_skipBackEnd;

cvar_t	*r_ignorehwgamma;
cvar_t	*r_measureOverdraw;

cvar_t	*r_inGameVideo;
cvar_t	*r_fastsky;
cvar_t	*r_drawSun;
cvar_t	*r_dynamiclight;
// rjr - removed for hacking cvar_t	*r_dlightBacks;

cvar_t	*r_lodbias;
cvar_t	*r_lodscale;
cvar_t	*r_autolodscalevalue;

cvar_t	*r_norefresh;
cvar_t	*r_drawentities;
cvar_t	*r_drawworld;
cvar_t	*r_drawfog;
cvar_t	*r_speeds;
cvar_t	*r_fullbright;
cvar_t	*r_novis;
cvar_t	*r_nocull;
cvar_t	*r_facePlaneCull;
cvar_t	*r_cullRoofFaces; //attempted smart method of culling out upwards facing surfaces on roofs for automap shots -rww
cvar_t	*r_roofCullCeilDist; //ceiling distance cull tolerance -rww
cvar_t	*r_roofCullFloorDist; //floor distance cull tolerance -rww
cvar_t	*r_showcluster;
cvar_t	*r_nocurves;

cvar_t	*r_autoMap; //automap renderside toggle for debugging -rww
cvar_t	*r_autoMapBackAlpha; //alpha of automap bg -rww
cvar_t	*r_autoMapDisable; //don't calc it (since it's slow in debug) -rww

cvar_t	*r_dlightStyle;
cvar_t	*r_surfaceSprites;
cvar_t	*r_surfaceWeather;

cvar_t	*r_windSpeed;
cvar_t	*r_windAngle;
cvar_t	*r_windGust;
cvar_t	*r_windDampFactor;
cvar_t	*r_windPointForce;
cvar_t	*r_windPointX;
cvar_t	*r_windPointY;

cvar_t	*r_allowExtensions;

cvar_t	*r_ext_compressed_textures;
cvar_t	*r_ext_compressed_lightmaps;
cvar_t	*r_ext_preferred_tc_method;
cvar_t	*r_ext_gamma_control;
cvar_t	*r_ext_multitexture;
cvar_t	*r_ext_compiled_vertex_array;
cvar_t	*r_ext_texture_env_add;
cvar_t	*r_ext_texture_filter_anisotropic;
cvar_t	*r_ext_multisample;


cvar_t	*r_environmentMapping;

cvar_t	*r_DynamicGlow;
cvar_t	*r_DynamicGlowPasses;
cvar_t	*r_DynamicGlowDelta;
cvar_t	*r_DynamicGlowIntensity;
cvar_t	*r_DynamicGlowSoft;
cvar_t	*r_DynamicGlowWidth;
cvar_t	*r_DynamicGlowHeight;

cvar_t	*r_smartpicmip;

cvar_t	*r_ignoreGLErrors;
cvar_t	*r_logFile;

cvar_t	*r_stencilbits;
cvar_t	*r_depthbits;
cvar_t	*r_colorbits;
cvar_t	*r_stereo;
cvar_t	*r_primitives;
cvar_t	*r_texturebits;
cvar_t	*r_texturebitslm;
cvar_t	*r_multiSample;
cvar_t	*r_multiSampleNvidia;
cvar_t	*r_anisotropy;

cvar_t	*r_lightmap;
cvar_t	*r_vertexLight;
cvar_t	*r_uiFullScreen;
cvar_t	*r_shadows;
cvar_t	*r_shadowRange;
cvar_t	*r_flares;
cvar_t	*r_mode;
cvar_t	*r_nobind;
cvar_t	*r_singleShader;
cvar_t	*r_colorMipLevels;
cvar_t	*r_picmip;
cvar_t	*r_showtris;
cvar_t	*r_showsky;
cvar_t	*r_shownormals;
cvar_t	*r_finish;
cvar_t	*r_clear;
cvar_t	*r_swapInterval;
cvar_t	*r_markcount;
cvar_t	*r_textureMode;
cvar_t	*r_offsetFactor;
cvar_t	*r_offsetUnits;
cvar_t	*r_gamma;
cvar_t	*r_intensity;
cvar_t	*r_lockpvs;
cvar_t	*r_noportals;
cvar_t	*r_portalOnly;

cvar_t	*r_subdivisions;
cvar_t	*r_lodCurveError;

cvar_t	*r_fullscreen = 0;
cvar_t	*r_noborder;
cvar_t	*r_centerWindow;

cvar_t	*r_customwidth;
cvar_t	*r_customheight;

cvar_t	*r_overBrightBits;
cvar_t	*r_mapOverBrightBits;

cvar_t	*r_debugSurface;
cvar_t	*r_simpleMipMaps;

cvar_t	*r_showImages;

cvar_t	*r_ambientScale;
cvar_t	*r_directedScale;
cvar_t	*r_debugLight;
cvar_t	*r_debugSort;

cvar_t	*r_maxpolys;
int		max_polys;
cvar_t	*r_maxpolyverts;
int		max_polyverts;

cvar_t	*r_modelpoolmegs;

cvar_t	*r_backEndMegs;

cvar_t	*r_drawAllAreas;
/*
Ghoul2 Insert Start
*/
#ifdef _DEBUG
cvar_t	*r_noPrecacheGLA;
#endif

cvar_t	*r_noServerGhoul2;
cvar_t	*r_Ghoul2AnimSmooth=0;
cvar_t	*r_Ghoul2UnSqashAfterSmooth=0;
//cvar_t	*r_Ghoul2UnSqash;
//cvar_t	*r_Ghoul2TimeBase=0; from single player
//cvar_t	*r_Ghoul2NoLerp;
//cvar_t	*r_Ghoul2NoBlend;
//cvar_t	*r_Ghoul2BlendMultiplier=0;

cvar_t	*broadsword=0;
cvar_t	*broadsword_kickbones=0;
cvar_t	*broadsword_kickorigin=0;
cvar_t	*broadsword_playflop=0;
cvar_t	*broadsword_dontstopanim=0;
cvar_t	*broadsword_waitforshot=0;
cvar_t	*broadsword_smallbbox=0;
cvar_t	*broadsword_extra1=0;
cvar_t	*broadsword_extra2=0;

cvar_t	*broadsword_effcorr=0;
cvar_t	*broadsword_ragtobase=0;
cvar_t	*broadsword_dircap=0;

/*
Ghoul2 Insert End
*/

//RAZFIXME: renderer has a copy of this now because stringed is common code and
// we use it for fonts
cvar_t *se_language;

#ifdef _WIN32
void ( APIENTRY * qglMultiTexCoord2fARB )( GLenum texture, GLfloat s, GLfloat t );
void ( APIENTRY * qglActiveTextureARB )( GLenum texture );
void ( APIENTRY * qglClientActiveTextureARB )( GLenum texture );

void ( APIENTRY * qglLockArraysEXT)( GLint, GLint);
void ( APIENTRY * qglUnlockArraysEXT) ( void );

void ( APIENTRY * qglPointParameterfEXT)( GLenum, GLfloat);
void ( APIENTRY * qglPointParameterfvEXT)( GLenum, GLfloat *);

//3d textures -rww
void ( APIENTRY * qglTexImage3DEXT) (GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *);
void ( APIENTRY * qglTexSubImage3DEXT) (GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);


// Declare Register Combiners function pointers.
PFNGLCOMBINERPARAMETERFVNV				qglCombinerParameterfvNV = NULL;
PFNGLCOMBINERPARAMETERIVNV				qglCombinerParameterivNV = NULL;
PFNGLCOMBINERPARAMETERFNV				qglCombinerParameterfNV = NULL;
PFNGLCOMBINERPARAMETERINV				qglCombinerParameteriNV = NULL;
PFNGLCOMBINERINPUTNV					qglCombinerInputNV = NULL;
PFNGLCOMBINEROUTPUTNV					qglCombinerOutputNV = NULL;
PFNGLFINALCOMBINERINPUTNV				qglFinalCombinerInputNV = NULL;
PFNGLGETCOMBINERINPUTPARAMETERFVNV		qglGetCombinerInputParameterfvNV = NULL;
PFNGLGETCOMBINERINPUTPARAMETERIVNV		qglGetCombinerInputParameterivNV = NULL;
PFNGLGETCOMBINEROUTPUTPARAMETERFVNV		qglGetCombinerOutputParameterfvNV = NULL;
PFNGLGETCOMBINEROUTPUTPARAMETERIVNV		qglGetCombinerOutputParameterivNV = NULL;
PFNGLGETFINALCOMBINERINPUTPARAMETERFVNV	qglGetFinalCombinerInputParameterfvNV = NULL;
PFNGLGETFINALCOMBINERINPUTPARAMETERIVNV	qglGetFinalCombinerInputParameterivNV = NULL;

// Declare Pixel Format function pointers.
PFNWGLGETPIXELFORMATATTRIBIVARBPROC		qwglGetPixelFormatAttribivARB = NULL;
PFNWGLGETPIXELFORMATATTRIBFVARBPROC		qwglGetPixelFormatAttribfvARB = NULL;
PFNWGLCHOOSEPIXELFORMATARBPROC			qwglChoosePixelFormatARB = NULL;

// Declare Pixel Buffer function pointers.
PFNWGLCREATEPBUFFERARBPROC				qwglCreatePbufferARB = NULL;
PFNWGLGETPBUFFERDCARBPROC				qwglGetPbufferDCARB = NULL;
PFNWGLRELEASEPBUFFERDCARBPROC			qwglReleasePbufferDCARB = NULL;
PFNWGLDESTROYPBUFFERARBPROC				qwglDestroyPbufferARB = NULL;
PFNWGLQUERYPBUFFERARBPROC				qwglQueryPbufferARB = NULL;

// Declare Render-Texture function pointers.
PFNWGLBINDTEXIMAGEARBPROC				qwglBindTexImageARB = NULL;
PFNWGLRELEASETEXIMAGEARBPROC			qwglReleaseTexImageARB = NULL;
PFNWGLSETPBUFFERATTRIBARBPROC			qwglSetPbufferAttribARB = NULL;

// Declare Vertex and Fragment Program function pointers.
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
//teh's PBO
PFNGLGENBUFFERSARBPROC qglGenBuffersARB = NULL;
PFNGLBINDBUFFERARBPROC qglBindBufferARB = NULL;
PFNGLBUFFERDATAARBPROC qglBufferDataARB = NULL;
PFNGLMAPBUFFERARBPROC qglMapBufferARB = NULL;
PFNGLUNMAPBUFFERARBPROC qglUnmapBufferARB = NULL;
#endif

void RE_SetLightStyle(int style, int color);

void RE_GetBModelVerts( int bmodelIndex, vec3_t *verts, vec3_t normal );

void R_Splash()
{
	image_t *pImage;
/*	const char* s = ri.Cvar_VariableString("se_language");
	if (stricmp(s,"english"))
	{
		pImage = R_FindImageFile( "menu/splash_eur", qfalse, qfalse, qfalse, GL_CLAMP);
	}
	else
	{
		pImage = R_FindImageFile( "menu/splash", qfalse, qfalse, qfalse, GL_CLAMP);
	}
*/
	pImage = R_FindImageFile( "menu/splash", qfalse, qfalse, qfalse, GL_CLAMP);
	extern void	RB_SetGL2D (void);
	RB_SetGL2D();	
	if (pImage )
	{//invalid paths?
		GL_Bind( pImage );
	}
	GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO);

	const int width = 640;
	const int height = 480;
	const float x1 = 320 - width / 2;
	const float x2 = 320 + width / 2;
	const float y1 = 240 - height / 2;
	const float y2 = 240 + height / 2;

#ifdef HAVE_GLES
	GLimp_EndFrame();
	GLfloat tex[] = {
		0,0 ,
		1,0,
		0,1,
		1,1
	};
	GLfloat vtx[] = {
		x1, y1,
		x2, y1,
		x1, y2,
		x2, y2
	};
	GLboolean text = qglIsEnabled(GL_TEXTURE_COORD_ARRAY);
	GLboolean glcol = qglIsEnabled(GL_COLOR_ARRAY);
	if (glcol)
		qglDisableClientState(GL_COLOR_ARRAY);
	if (!text)
		qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
	qglEnableClientState(GL_VERTEX_ARRAY);
	qglTexCoordPointer(2, GL_FLOAT, 0, tex);
	qglVertexPointer(2, GL_FLOAT, 0, vtx);
	qglDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	if (glcol)
		qglDisableClientState(GL_COLOR_ARRAY);
	if (!text)
		qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
#else
	qglBegin (GL_TRIANGLE_STRIP);
		qglTexCoord2f( 0,  0 );
		qglVertex2f(x1, y1);
		qglTexCoord2f( 1 ,  0 );
		qglVertex2f(x2, y1);
		qglTexCoord2f( 0, 1 );
		qglVertex2f(x1, y2);
		qglTexCoord2f( 1, 1 );
		qglVertex2f(x2, y2);
	qglEnd();
#endif

	GLimp_EndFrame();
}

/*
** InitOpenGL
**
** This function is responsible for initializing a valid OpenGL subsystem.  This
** is done by calling GLimp_Init (which gives us a working OGL subsystem) then
** setting variables, checking GL constants, and reporting the gfx system config
** to the user.
*/
static void InitOpenGL( void )
{
	//
	// initialize OS specific portions of the renderer
	//
	// GLimp_Init directly or indirectly references the following cvars:
	//		- r_fullscreen
	//		- r_mode
	//		- r_(color|depth|stencil)bits
	//		- r_ignorehwgamma
	//		- r_gamma
	//

	if ( glConfig.vidWidth == 0 )
	{
		glConfig.displayScale = 1.0f;
		GLimp_Init();
		// print info the first time only
		GL_SetDefaultState();
		R_Splash();	//get something on screen asap
		GfxInfo_f();
		glMMEConfig.glWidth = glConfig.vidWidth;
		glMMEConfig.glHeight = glConfig.vidHeight;

		R_FrameBuffer_Init();
	}
	else
	{
		// set default state
		GL_SetDefaultState();
	}
	// init command buffers and SMP
	R_InitCommandBuffers();
}

/*
==================
GL_CheckErrors
==================
*/
void GL_CheckErrors( void ) {
    int		err;
    char	s[64];

    err = qglGetError();
    if ( err == GL_NO_ERROR ) {
        return;
    }
    if ( r_ignoreGLErrors->integer ) {
        return;
    }
    switch( err ) {
        case GL_INVALID_ENUM:
            strcpy( s, "GL_INVALID_ENUM" );
            break;
        case GL_INVALID_VALUE:
            strcpy( s, "GL_INVALID_VALUE" );
            break;
        case GL_INVALID_OPERATION:
            strcpy( s, "GL_INVALID_OPERATION" );
            break;
        case GL_STACK_OVERFLOW:
            strcpy( s, "GL_STACK_OVERFLOW" );
            break;
        case GL_STACK_UNDERFLOW:
            strcpy( s, "GL_STACK_UNDERFLOW" );
            break;
        case GL_OUT_OF_MEMORY:
            strcpy( s, "GL_OUT_OF_MEMORY" );
            break;
        default:
            Com_sprintf( s, sizeof(s), "%i", err);
            break;
    }

    Com_Error( ERR_FATAL, "GL_CheckErrors: %s", s );
}

/*
** R_GetModeInfo
*/
typedef struct vidmode_s
{
    const char *description;
    int         width, height;
} vidmode_t;

const vidmode_t r_vidModes[] = {
    { "Mode  0: 320x240",		320,	240 },
    { "Mode  1: 400x300",		400,	300 },
    { "Mode  2: 512x384",		512,	384 },
    { "Mode  3: 640x480",		640,	480 },
    { "Mode  4: 800x600",		800,	600 },
    { "Mode  5: 960x720",		960,	720 },
    { "Mode  6: 1024x768",		1024,	768 },
    { "Mode  7: 1152x864",		1152,	864 },
    { "Mode  8: 1280x1024",		1280,	1024 },
    { "Mode  9: 1600x1200",		1600,	1200 },
    { "Mode 10: 2048x1536",		2048,	1536 },
    { "Mode 11: 856x480 (wide)", 856,	 480 },
    { "Mode 12: 2400x600(surround)",2400,600 }
};
static const int	s_numVidModes = ( sizeof( r_vidModes ) / sizeof( r_vidModes[0] ) );

qboolean R_GetModeInfo( int *width, int *height, int mode ) {
	const vidmode_t	*vm;

    if ( mode < -1 ) {
        return qfalse;
	}
	if ( mode >= s_numVidModes ) {
		return qfalse;
	}

	if ( mode == -1 ) {
		*width = r_customwidth->integer;
		*height = r_customheight->integer;
		return qtrue;
	}

	vm = &r_vidModes[mode];

    *width  = vm->width;
    *height = vm->height;

    return qtrue;
}

/*
** R_ModeList_f
*/
static void R_ModeList_f( void )
{
	int i;

	Com_Printf ("\n" );
	Com_Printf ("Mode -2: Use desktop resolution\n" );
	Com_Printf ("Mode -1: Use r_customWidth and r_customHeight variables\n" );
	for ( i = 0; i < s_numVidModes; i++ )
	{
		Com_Printf ("%s\n", r_vidModes[i].description );
	}
	Com_Printf ("\n" );
}

/*
==============================================================================

						SCREEN SHOTS

==============================================================================
*/

/*
==================
RB_TakeScreenshotCmd
==================
*/
const void *RB_ScreenShotCmd( const void *data ) {
	const screenShotCommand_t *cmd = (const screenShotCommand_t *)data;
	byte *inBuf, *outBuf;
	int w, h, outSize, i, j, k, res;

	w = glConfig.vidWidth;
	h = glConfig.vidHeight;
	res = w * h;
	outSize = res * 4;
	inBuf = (byte *)ri.Hunk_AllocateTempMemory( outSize * 2 );
	outBuf = inBuf + outSize;

#ifdef HAVE_GLES
	qglReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, inBuf);
	for (i = 0, j = 0, k = 0; k < res; i += 4, j += 3, k++) {
		inBuf[j + 0] = inBuf[i + 0];
		inBuf[j + 1] = inBuf[i + 1];
		inBuf[j + 2] = inBuf[i + 2];
	}
#else
	qglReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, inBuf);
#endif
	if ( ( mme_screenShotGamma->integer || (tr.overbrightBits > 0) ) && (glConfig.deviceSupportsGamma ) ) {
		R_GammaCorrect( inBuf, outSize );
	}
	switch ( cmd->format ) {
	case mmeShotFormatJPG:
		outSize = SaveJPG( mme_jpegQuality->integer, w, h, mmeShotTypeRGB, inBuf, outBuf, outSize );
		break;
	case mmeShotFormatTGA:
		outSize = SaveTGA( mme_tgaCompression->integer, w, h, mmeShotTypeRGB, inBuf, outBuf, outSize );
		break;
	case mmeShotFormatPNG:
		outSize = SavePNG( mme_pngCompression->integer, w, h, mmeShotTypeRGB, inBuf, outBuf, outSize );
		break;
	default:
		outSize = 0;
	}
	if (outSize)
		ri.FS_WriteFile( cmd->name, outBuf, outSize );
	ri.Hunk_FreeTempMemory( inBuf );
	return (const void *)(cmd + 1);	
}


/*
====================
R_LevelShot

levelshots are specialized 256*256 thumbnails for
the menu system, sampled down from full screen distorted images
====================
*/
#define LEVELSHOTSIZE 256
static void R_LevelShot( void ) {
	char		checkname[MAX_OSPATH];
	byte		*buffer;
	byte		*source;
	byte		*src, *dst;
	int			x, y;
	int			r, g, b;
	float		xScale, yScale;
	int			xx, yy;
	int			i, j, k, res;
	res = glConfig.vidWidth * glConfig.vidHeight;

	Com_sprintf( checkname, sizeof(checkname), "levelshots/%s.tga", tr.world->baseName );
	
	source = (byte *)ri.Hunk_AllocateTempMemory( glConfig.vidWidth * glConfig.vidHeight * 3 );

	buffer = (byte *)ri.Hunk_AllocateTempMemory(LEVELSHOTSIZE * LEVELSHOTSIZE*3 + 18);
	Com_Memset (buffer, 0, 18);
	buffer[2] = 2;		// uncompressed type
	buffer[12] = LEVELSHOTSIZE & 255;
	buffer[13] = LEVELSHOTSIZE >> 8;
	buffer[14] = LEVELSHOTSIZE & 255;
	buffer[15] = LEVELSHOTSIZE >> 8;
	buffer[16] = 24;	// pixel size
	
#ifdef HAVE_GLES
	qglReadPixels(0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_RGBA, GL_UNSIGNED_BYTE, source);
	for (i = 0, j = 0, k = 0; k < res; i += 4, j += 3, k++) {
		source[j + 0] = source[i + 0];
		source[j + 1] = source[i + 1];
		source[j + 2] = source[i + 2];
	}
#else
	qglReadPixels(0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_RGB, GL_UNSIGNED_BYTE, source);
#endif

	// resample from source
	xScale = glConfig.vidWidth / (4.0*LEVELSHOTSIZE);
	yScale = glConfig.vidHeight / (3.0*LEVELSHOTSIZE);
	for ( y = 0 ; y < LEVELSHOTSIZE ; y++ ) {
		for ( x = 0 ; x < LEVELSHOTSIZE ; x++ ) {
			r = g = b = 0;
			for ( yy = 0 ; yy < 3 ; yy++ ) {
				for ( xx = 0 ; xx < 4 ; xx++ ) {
					src = source + 3 * ( glConfig.vidWidth * (int)( (y*3+yy)*yScale ) + (int)( (x*4+xx)*xScale ) );
					r += src[0];
					g += src[1];
					b += src[2];
				}
			}
			dst = buffer + 18 + 3 * ( y * LEVELSHOTSIZE + x );
			dst[0] = b / 12;
			dst[1] = g / 12;
			dst[2] = r / 12;
		}
	}

	// gamma correct
	if ( (mme_screenShotGamma->integer || ( tr.overbrightBits > 0 )) && glConfig.deviceSupportsGamma ) {
		R_GammaCorrect( buffer + 18, LEVELSHOTSIZE * LEVELSHOTSIZE * 3 );
	}

	ri.FS_WriteFile( checkname, buffer, LEVELSHOTSIZE * LEVELSHOTSIZE*3 + 18 );

	ri.Hunk_FreeTempMemory( buffer );
	ri.Hunk_FreeTempMemory( source );

	Com_Printf ("Wrote %s\n", checkname );
}
void R_ScreenShot(const char *shotName, mmeShotFormat_t shotFormat) {
	screenShotCommand_t *cmd;
	if (!tr.registered) {
		return;
	}
	cmd = (screenShotCommand_t *)R_GetCommandBuffer(sizeof(*cmd));
	if (!cmd) {
		return;
	}
	cmd->commandId = RC_SCREENSHOT;
	Q_strncpyz(cmd->name, shotName, sizeof(cmd->name));
	cmd->format = shotFormat;
}
extern cvar_t *mme_dofFrames;
void R_ScreenShotDOF(const char *shotName, float focus, float radius) {
	captureCommand_t *cmd;
	
	if ( !tr.registered ) {
		return;
	}
	cmd = (captureCommand_t *)R_GetCommandBuffer( sizeof( *cmd ) );
	if ( !cmd ) {
		return;
	}
	if (mme_dofFrames->integer > 0)
		tr.capturingMultiPass = qtrue;
	cmd->commandId = RC_CAPTURE;
	cmd->fps = -1;
	cmd->focus = focus;
	cmd->radius = radius;
	Q_strncpyz( cmd->name, shotName, sizeof( cmd->name ));
}
/* 
================== 
R_ScreenShot_f
screenshot
screenshot [silent]
screenshot [levelshot]
screenshot [filename]
Doesn't print the pacifier message if there is a second arg
================== 
*/
static char *R_ScreenShotName(const char *start, const char *ext, char *fileName) {
	int i;
	for (i=0;i<1000;i++) {
		Com_sprintf(fileName, MAX_OSPATH, "screenshots/%s.%04d.%s", 
			start, i, ext);
		if (!ri.FS_FileExists(fileName))
			return va("screenshots/%s.%04d", start, i);
	}
	Com_Printf("Screenshot limit reached\n");
	return va("screenshots/%s.%04d", start, i); // or should ret NULL?
}
static void R_ScreenShot_f(const char *ext, mmeShotFormat_t shotFormat) {
	char fileName[MAX_OSPATH];
	const char *cmd = ri.Cmd_Argv(1);
	qboolean silent = qfalse;
	if (!strcmp(ri.Cmd_Argv(1), "levelshot")) {
		R_LevelShot();
		return;
	}
	if (!strcmp(cmd, "silent"))
		silent = qtrue;
	if (!cmd[0] || silent)
		cmd = "shot";	
	if (R_ScreenShotName(cmd, ext, fileName)) {
		if (!silent)
			ri.Printf(PRINT_ALL, "Saving shot %s\n", fileName);
		R_ScreenShot(fileName, shotFormat);
	}
} 
static void R_ScreenShotTGA_f(void) {
	R_ScreenShot_f("tga", mmeShotFormatTGA);
} 
static void R_ScreenShotJPEG_f(void) {
	R_ScreenShot_f("jpg", mmeShotFormatJPG);
} 
static void R_ScreenShotPNG_f(void) {
	R_ScreenShot_f("png", mmeShotFormatPNG);
} 
static void R_ScreenShotDOF_f(void) {
	char fileName[MAX_OSPATH], *name;
	const char *cmd = ri.Cmd_Argv(1);
	char *ext = mme_screenShotFormat->string;
	if (mme_dofFrames->integer <= 0) {
		ri.Printf(PRINT_ALL, "Failed to take a DOF screenshot: mme_dofFrames <= 0 (%d)\n", mme_dofFrames->integer);
		return;
	}
	if (!cmd[0])
		cmd = "shot";	
	if ((Q_stricmp(ext, "png")
        && Q_stricmp(ext, "tga")
        && Q_stricmp(ext, "jpg"))
        || !ext[0]) {
		ext = "png";
	}
	if (name = R_ScreenShotName(cmd, ext, fileName)) {
		float focus = atof(ri.Cmd_Argv(2));
		float radius = atof(ri.Cmd_Argv(3));
		if (!focus)
			focus = 133.7f;
		if (!radius)
			radius = 1.337f;
		R_ScreenShotDOF(name, focus, radius);
	}
} 
//============================================================================
/*
** GL_SetDefaultState
*/
void GL_SetDefaultState( void ) {
	qglClearDepth( 1.0f );

	qglCullFace(GL_FRONT);

	qglColor4f (1,1,1,1);

	// initialize downstream texture unit if we're running
	// in a multitexture environment
	if ( qglActiveTextureARB ) {
		GL_SelectTexture( 1 );
		GL_TextureMode( r_textureMode->string );
		GL_TexEnv( GL_MODULATE );
		qglDisable( GL_TEXTURE_2D );
		GL_SelectTexture( 0 );
	}

	qglEnable(GL_TEXTURE_2D);
	GL_TextureMode( r_textureMode->string );
	GL_TexEnv( GL_MODULATE );
	GL_Anisotropy( r_anisotropy->integer );

	qglShadeModel( GL_SMOOTH );
	qglDepthFunc( GL_LEQUAL );

	// the vertex array is always enabled, but the color and texture
	// arrays are enabled and disabled around the compiled vertex array call
	qglEnableClientState (GL_VERTEX_ARRAY);

	//
	// make sure our GL state vector is set correctly
	//
	glState.glStateBits = GLS_DEPTHTEST_DISABLE | GLS_DEPTHMASK_TRUE;

#ifndef HAVE_GLES
	qglPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif
	qglDepthMask( GL_TRUE );
	qglDisable( GL_DEPTH_TEST );
	qglEnable( GL_SCISSOR_TEST );
	qglDisable( GL_CULL_FACE );
	qglDisable( GL_BLEND );

#ifdef HAVE_GLES
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
#endif
}


/*
================
GfxInfo_f
================
*/
extern bool g_bTextureRectangleHack;

/*
================
R_PrintLongString

Workaround for ri.Printf's 1024 characters buffer limit.
================
*/
void R_PrintLongString(const char *string) {
	char buffer[1024];
	const char *p;
	int size = strlen(string);

	p = string;
	while(size > 0)
	{
		Q_strncpyz(buffer, p, sizeof (buffer) );
		Com_Printf( "%s", buffer );
		p += 1023;
		size -= 1023;
	}
}

void GfxInfo_f( void ) 
{
	const char *enablestrings[] =
	{
		"disabled",
		"enabled"
	};
	const char *fsstrings[] =
	{
		"windowed",
		"fullscreen"
	};
	const char *noborderstrings[] =
	{
		"",
		"noborder "
	};

	const char *tc_table[] = 
	{
		"None",
		"GL_S3_s3tc",
		"GL_EXT_texture_compression_s3tc",
	};

	Com_Printf ("\nGL_VENDOR: %s\n", glConfig.vendor_string );
	Com_Printf ("GL_RENDERER: %s\n", glConfig.renderer_string );
	Com_Printf ("GL_VERSION: %s\n", glConfig.version_string );
	R_PrintLongString( glConfig.extensions_string );
	Com_Printf ("\n");
	Com_Printf ("GL_MAX_TEXTURE_SIZE: %d\n", glConfig.maxTextureSize );
	Com_Printf ("GL_MAX_ACTIVE_TEXTURES_ARB: %d\n", glConfig.maxActiveTextures );
	Com_Printf ("\nPIXELFORMAT: color(%d-bits) Z(%d-bit) stencil(%d-bits)\n", glConfig.colorBits, glConfig.depthBits, glConfig.stencilBits );
	Com_Printf ("MODE: %d, %d x %d %s%s hz:", r_mode->integer, glConfig.vidWidth, glConfig.vidHeight, r_fullscreen->integer == 0 ? noborderstrings[r_noborder->integer == 1] : noborderstrings[0] ,fsstrings[r_fullscreen->integer == 1] );
	if ( glConfig.displayFrequency )
	{
		Com_Printf ("%d\n", glConfig.displayFrequency );
	}
	else
	{
		Com_Printf ("N/A\n" );
	}
	if ( glConfig.deviceSupportsGamma )
	{
		Com_Printf ("GAMMA: hardware w/ %d overbright bits\n", tr.overbrightBits );
	}
	else
	{
		Com_Printf ("GAMMA: software w/ %d overbright bits\n", tr.overbrightBits );
	}

	// rendering primitives
	{
		int		primitives;

		// default is to use triangles if compiled vertex arrays are present
		Com_Printf ("rendering primitives: " );
		primitives = r_primitives->integer;
		if ( primitives == 0 ) {
			if ( qglLockArraysEXT ) {
				primitives = 2;
			} else {
				primitives = 1;
			}
		}
		if ( primitives == -1 ) {
			Com_Printf ("none\n" );
		} else if ( primitives == 2 ) {
			Com_Printf ("single glDrawElements\n" );
		} else if ( primitives == 1 ) {
			Com_Printf ("multiple glArrayElement\n" );
		} else if ( primitives == 3 ) {
			Com_Printf ("multiple glColor4ubv + glTexCoord2fv + glVertex3fv\n" );
		}
	}

	Com_Printf ("texturemode: %s\n", r_textureMode->string );
	Com_Printf ("picmip: %d\n", r_picmip->integer );
	Com_Printf ("texture bits: %d\n", r_texturebits->integer );
	Com_Printf ("lightmap texture bits: %d\n", r_texturebitslm->integer );
	Com_Printf ("multitexture: %s\n", enablestrings[qglActiveTextureARB != 0] );
	Com_Printf ("compiled vertex arrays: %s\n", enablestrings[qglLockArraysEXT != 0 ] );
	Com_Printf ("texenv add: %s\n", enablestrings[glConfig.textureEnvAddAvailable != 0] );
	Com_Printf ("compressed textures: %s\n", enablestrings[glConfig.textureCompression != TC_NONE] );
	Com_Printf ("compressed lightmaps: %s\n", enablestrings[(r_ext_compressed_lightmaps->integer != 0 && glConfig.textureCompression != TC_NONE)] );
	Com_Printf ("texture compression method: %s\n", tc_table[glConfig.textureCompression] );
	Com_Printf ("anisotropic filtering: %s  ", enablestrings[(r_ext_texture_filter_anisotropic->integer != 0) && glConfig.maxTextureFilterAnisotropy] );
		Com_Printf ("(%f of %f)\n", r_ext_texture_filter_anisotropic->value, glConfig.maxTextureFilterAnisotropy );
	Com_Printf ("Dynamic Glow: %s\n", enablestrings[r_DynamicGlow->integer != 0] );
	if (g_bTextureRectangleHack) Com_Printf ("Dynamic Glow ATI BAD DRIVER HACK %s\n", enablestrings[g_bTextureRectangleHack] );

	if ( r_finish->integer ) {
		Com_Printf ("Forcing glFinish\n" );
	}
	if ( r_displayRefresh ->integer ) {
		Com_Printf ("Display refresh set to %d\n", r_displayRefresh->integer );
	}
	if (tr.world)
	{
		Com_Printf ("Light Grid size set to (%.2f %.2f %.2f)\n", tr.world->lightGridSize[0], tr.world->lightGridSize[1], tr.world->lightGridSize[2] );
	}
}

void R_AtiHackToggle_f(void)
{
	g_bTextureRectangleHack = !g_bTextureRectangleHack;
}

/*
===============
R_Register
===============
*/
void R_Register( void ) 
{

	//RAZFIXME: lol badness
	se_language = ri.Cvar_Get("se_language", "english", CVAR_ARCHIVE | CVAR_NORESTART);
	//
	// latched and archived variables
	//
	r_allowExtensions					= ri.Cvar_Get( "r_allowExtensions",					"1",						CVAR_ARCHIVE|CVAR_LATCH );
	r_ext_compressed_textures			= ri.Cvar_Get( "r_ext_compress_textures",			"1",						CVAR_ARCHIVE|CVAR_LATCH );
	r_ext_compressed_lightmaps			= ri.Cvar_Get( "r_ext_compress_lightmaps",			"0",						CVAR_ARCHIVE|CVAR_LATCH );
	r_ext_preferred_tc_method			= ri.Cvar_Get( "r_ext_preferred_tc_method",			"0",						CVAR_ARCHIVE|CVAR_LATCH );
	r_ext_gamma_control					= ri.Cvar_Get( "r_ext_gamma_control",				"1",						CVAR_ARCHIVE|CVAR_LATCH );
	r_ext_multitexture					= ri.Cvar_Get( "r_ext_multitexture",				"1",						CVAR_ARCHIVE|CVAR_LATCH );
	r_ext_compiled_vertex_array			= ri.Cvar_Get( "r_ext_compiled_vertex_array",		"1",						CVAR_ARCHIVE|CVAR_LATCH );
#ifdef __linux__ // broken on linux
	r_ext_texture_env_add				= ri.Cvar_Get( "r_ext_texture_env_add",				"0",						CVAR_ARCHIVE|CVAR_LATCH );
#else
	r_ext_texture_env_add				= ri.Cvar_Get( "r_ext_texture_env_add",				"1",						CVAR_ARCHIVE|CVAR_LATCH );
#endif
    r_ext_texture_filter_anisotropic	= ri.Cvar_Get( "r_ext_texture_filter_anisotropic",	"16",						CVAR_ARCHIVE );
    r_ext_multisample                   = ri.Cvar_Get( "r_ext_multisample",                 "0",						CVAR_ARCHIVE|CVAR_LATCH );
	r_environmentMapping				= ri.Cvar_Get( "r_environmentMapping",				"1",						CVAR_ARCHIVE );
	r_DynamicGlow						= ri.Cvar_Get( "r_DynamicGlow",						"0",						CVAR_ARCHIVE );
	r_DynamicGlowPasses					= ri.Cvar_Get( "r_DynamicGlowPasses",				"5",						CVAR_ARCHIVE );
	r_DynamicGlowDelta					= ri.Cvar_Get( "r_DynamicGlowDelta",				"0.8f",						CVAR_ARCHIVE );
	r_DynamicGlowIntensity				= ri.Cvar_Get( "r_DynamicGlowIntensity",			"1.13f",					CVAR_ARCHIVE );
	r_DynamicGlowSoft					= ri.Cvar_Get( "r_DynamicGlowSoft",					"1",						CVAR_ARCHIVE );
	r_DynamicGlowWidth					= ri.Cvar_Get( "r_DynamicGlowWidth",				"320",						CVAR_ARCHIVE|CVAR_LATCH );
	r_DynamicGlowHeight					= ri.Cvar_Get( "r_DynamicGlowHeight",				"240",						CVAR_ARCHIVE|CVAR_LATCH );
	r_picmip							= ri.Cvar_Get( "r_picmip",							"0",						CVAR_ARCHIVE|CVAR_LATCH );
	r_smartpicmip						= ri.Cvar_Get( "r_smartpicmip",						"0",						CVAR_ARCHIVE | CVAR_LATCH);
	ri.Cvar_CheckRange( r_picmip, 0, 16, qtrue );
	r_colorMipLevels					= ri.Cvar_Get( "r_colorMipLevels",					"0",						CVAR_LATCH );
	r_detailTextures					= ri.Cvar_Get( "r_detailtextures",					"1",						CVAR_ARCHIVE|CVAR_LATCH );
	r_texturebits						= ri.Cvar_Get( "r_texturebits",						"0",						CVAR_ARCHIVE|CVAR_LATCH );
	r_texturebitslm						= ri.Cvar_Get( "r_texturebitslm",					"0",						CVAR_ARCHIVE|CVAR_LATCH );
	r_colorbits							= ri.Cvar_Get( "r_colorbits",						"0",						CVAR_ARCHIVE|CVAR_LATCH );
	r_stereo							= ri.Cvar_Get( "r_stereo",							"0",						CVAR_ARCHIVE|CVAR_LATCH );
#ifdef __linux__
	r_stencilbits						= ri.Cvar_Get( "r_stencilbits",						"0",						CVAR_ARCHIVE|CVAR_LATCH );
#else
	r_stencilbits						= ri.Cvar_Get( "r_stencilbits",						"8",						CVAR_ARCHIVE|CVAR_LATCH );
#endif
	r_depthbits							= ri.Cvar_Get( "r_depthbits",						"0",						CVAR_ARCHIVE|CVAR_LATCH );
	
	r_multiSample						= ri.Cvar_Get( "r_multiSample",						"0",						CVAR_ARCHIVE | CVAR_LATCH );
	r_multiSampleNvidia					= ri.Cvar_Get( "r_multiSampleNvidia",				"0",						CVAR_ARCHIVE | CVAR_LATCH );
	r_anisotropy						= ri.Cvar_Get( "r_anisotropy",						"0",						CVAR_ARCHIVE );
	r_backEndMegs						= ri.Cvar_Get( "r_backEndMegs",						"2",						CVAR_ARCHIVE | CVAR_LATCH );

	r_overBrightBits					= ri.Cvar_Get( "r_overBrightBits",					"0",						CVAR_ARCHIVE|CVAR_LATCH );
	r_mapOverBrightBits					= ri.Cvar_Get( "r_mapOverBrightBits",				"0",						CVAR_ARCHIVE|CVAR_LATCH );
	r_ignorehwgamma						= ri.Cvar_Get( "r_ignorehwgamma",					"0",						CVAR_ARCHIVE|CVAR_LATCH );
	r_mode								= ri.Cvar_Get( "r_mode",							"4",						CVAR_ARCHIVE|CVAR_LATCH );
	r_fullscreen						= ri.Cvar_Get( "r_fullscreen",						"1",						CVAR_ARCHIVE|CVAR_LATCH );
	r_noborder							= ri.Cvar_Get( "r_noborder",						"0",						CVAR_ARCHIVE|CVAR_LATCH );
	r_centerWindow						= ri.Cvar_Get( "r_centerWindow",					"0",						CVAR_ARCHIVE|CVAR_LATCH );
	r_customwidth						= ri.Cvar_Get( "r_customwidth",						"1600",						CVAR_ARCHIVE|CVAR_LATCH );
	r_customheight						= ri.Cvar_Get( "r_customheight",					"1024",						CVAR_ARCHIVE|CVAR_LATCH );
	r_simpleMipMaps						= ri.Cvar_Get( "r_simpleMipMaps",					"1",						CVAR_ARCHIVE|CVAR_LATCH );
	r_vertexLight						= ri.Cvar_Get( "r_vertexLight",						"0",						CVAR_ARCHIVE|CVAR_LATCH );
	r_uiFullScreen						= ri.Cvar_Get( "r_uifullscreen",					"0",						CVAR_NONE );
	r_subdivisions						= ri.Cvar_Get( "r_subdivisions",					"4",						CVAR_ARCHIVE|CVAR_LATCH );
	r_displayRefresh					= ri.Cvar_Get( "r_displayRefresh",					"0",						CVAR_LATCH );
	ri.Cvar_CheckRange( r_displayRefresh, 0, 200, qtrue );
	r_fullbright						= ri.Cvar_Get( "r_fullbright",						"0",						CVAR_CHEAT );
	r_intensity							= ri.Cvar_Get( "r_intensity",						"1",						CVAR_LATCH );
	r_singleShader						= ri.Cvar_Get( "r_singleShader",					"0",						CVAR_CHEAT|CVAR_LATCH );
	r_lodCurveError						= ri.Cvar_Get( "r_lodCurveError",					"250",						CVAR_ARCHIVE );
	r_lodbias							= ri.Cvar_Get( "r_lodbias",							"0",						CVAR_ARCHIVE );
	r_autolodscalevalue					= ri.Cvar_Get( "r_autolodscalevalue",				"0",						CVAR_ROM );
	r_flares							= ri.Cvar_Get( "r_flares",							"1",						CVAR_ARCHIVE );
	r_znear								= ri.Cvar_Get( "r_znear",							"1",						CVAR_ARCHIVE );
	ri.Cvar_CheckRange( r_znear, 0.001f, 200, qfalse );
	r_zproj								= ri.Cvar_Get( "r_zproj",							"107",						CVAR_ARCHIVE );
	r_stereoSeparation					= ri.Cvar_Get( "r_stereoSeparation",				"0",						CVAR_ARCHIVE );
	r_ignoreGLErrors					= ri.Cvar_Get( "r_ignoreGLErrors",					"1",						CVAR_ARCHIVE );
	r_fastsky							= ri.Cvar_Get( "r_fastsky",							"0",						CVAR_ARCHIVE );
	r_inGameVideo						= ri.Cvar_Get( "r_inGameVideo",						"1",						CVAR_ARCHIVE );
	r_drawSun							= ri.Cvar_Get( "r_drawSun",							"0",						CVAR_ARCHIVE );
	r_dynamiclight						= ri.Cvar_Get( "r_dynamiclight",					"1",						CVAR_ARCHIVE );
	// rjr - removed for hacking
//	r_dlightBacks						= ri.Cvar_Get( "r_dlightBacks",						"1",						CVAR_CHEAT );
	r_finish							= ri.Cvar_Get( "r_finish",							"0",						CVAR_ARCHIVE );
	r_textureMode						= ri.Cvar_Get( "r_textureMode",						"GL_LINEAR_MIPMAP_NEAREST",	CVAR_ARCHIVE );
	r_swapInterval						= ri.Cvar_Get( "r_swapInterval",					"0",						CVAR_ARCHIVE );
	r_markcount							= ri.Cvar_Get( "r_markcount",						"100",						CVAR_ARCHIVE );
#ifdef __MACOS__
	r_gamma								= ri.Cvar_Get( "r_gamma",							"1.2",						CVAR_ARCHIVE );
#else
	r_gamma								= ri.Cvar_Get( "r_gamma",							"1",						CVAR_ARCHIVE );
#endif
	r_facePlaneCull						= ri.Cvar_Get( "r_facePlaneCull",					"1",						CVAR_ARCHIVE );
	r_cullRoofFaces						= ri.Cvar_Get( "r_cullRoofFaces",					"0",						CVAR_CHEAT ); //attempted smart method of culling out upwards facing surfaces on roofs for automap shots -rww
	r_roofCullCeilDist					= ri.Cvar_Get( "r_roofCullCeilDist",				"256",						CVAR_CHEAT ); //attempted smart method of culling out upwards facing surfaces on roofs for automap shots -rww
	r_roofCullFloorDist					= ri.Cvar_Get( "r_roofCeilFloorDist",				"128",						CVAR_CHEAT ); //attempted smart method of culling out upwards facing surfaces on roofs for automap shots -rww
	r_primitives						= ri.Cvar_Get( "r_primitives",						"0",						CVAR_ARCHIVE );
	ri.Cvar_CheckRange( r_primitives, -1, 3, qtrue );
	r_ambientScale						= ri.Cvar_Get( "r_ambientScale",					"0.6",						CVAR_CHEAT );
	r_directedScale						= ri.Cvar_Get( "r_directedScale",					"1",						CVAR_CHEAT );
	r_autoMap							= ri.Cvar_Get( "r_autoMap",							"0",						CVAR_ARCHIVE ); //automap renderside toggle for debugging -rww
	r_autoMapBackAlpha					= ri.Cvar_Get( "r_autoMapBackAlpha",				"0",						CVAR_NONE ); //alpha of automap bg -rww
	r_autoMapDisable					= ri.Cvar_Get( "r_autoMapDisable",					"1",						CVAR_NONE );
	r_showImages						= ri.Cvar_Get( "r_showImages",						"0",						CVAR_CHEAT );
	r_debugLight						= ri.Cvar_Get( "r_debuglight",						"0",						CVAR_TEMP );
	r_debugSort							= ri.Cvar_Get( "r_debugSort",						"0",						CVAR_CHEAT );
	r_dlightStyle						= ri.Cvar_Get( "r_dlightStyle",						"1",						CVAR_ARCHIVE );
	r_surfaceSprites					= ri.Cvar_Get( "r_surfaceSprites",					"1",						CVAR_TEMP );
	r_surfaceWeather					= ri.Cvar_Get( "r_surfaceWeather",					"0",						CVAR_TEMP );
	r_windSpeed							= ri.Cvar_Get( "r_windSpeed",						"0",						CVAR_NONE );
	r_windAngle							= ri.Cvar_Get( "r_windAngle",						"0",						CVAR_NONE );
	r_windGust							= ri.Cvar_Get( "r_windGust",						"0",						CVAR_NONE );
	r_windDampFactor					= ri.Cvar_Get( "r_windDampFactor",					"0.1",						CVAR_NONE );
	r_windPointForce					= ri.Cvar_Get( "r_windPointForce",					"0",						CVAR_NONE );
	r_windPointX						= ri.Cvar_Get( "r_windPointX",						"0",						CVAR_NONE );
	r_windPointY						= ri.Cvar_Get( "r_windPointY",						"0",						CVAR_NONE );
	r_nocurves							= ri.Cvar_Get( "r_nocurves",						"0",						CVAR_CHEAT );
	r_drawworld							= ri.Cvar_Get( "r_drawworld",						"1",						CVAR_CHEAT );
	r_drawfog							= ri.Cvar_Get( "r_drawfog",							"2",						CVAR_CHEAT );
	r_lightmap							= ri.Cvar_Get( "r_lightmap",						"0",						CVAR_CHEAT );
	r_portalOnly						= ri.Cvar_Get( "r_portalOnly",						"0",						CVAR_CHEAT );
	r_skipBackEnd						= ri.Cvar_Get( "r_skipBackEnd",						"0",						CVAR_CHEAT );
	r_measureOverdraw					= ri.Cvar_Get( "r_measureOverdraw",					"0",						CVAR_CHEAT );
	r_lodscale							= ri.Cvar_Get( "r_lodscale",						"5",						CVAR_NONE );
	r_norefresh							= ri.Cvar_Get( "r_norefresh",						"0",						CVAR_CHEAT );
	r_drawentities						= ri.Cvar_Get( "r_drawentities",					"1",						CVAR_CHEAT );
	r_ignore							= ri.Cvar_Get( "r_ignore",							"1",						CVAR_CHEAT );
	r_nocull							= ri.Cvar_Get( "r_nocull",							"0",						CVAR_CHEAT );
	r_novis								= ri.Cvar_Get( "r_novis",							"0",						CVAR_CHEAT );
	r_showcluster						= ri.Cvar_Get( "r_showcluster",						"0",						CVAR_CHEAT );
	r_speeds							= ri.Cvar_Get( "r_speeds",							"0",						CVAR_CHEAT );
	r_verbose							= ri.Cvar_Get( "r_verbose",							"0",						CVAR_CHEAT );
	r_logFile							= ri.Cvar_Get( "r_logFile",							"0",						CVAR_CHEAT );
	r_debugSurface						= ri.Cvar_Get( "r_debugSurface",					"0",						CVAR_CHEAT );
	r_nobind							= ri.Cvar_Get( "r_nobind",							"0",						CVAR_CHEAT );
	r_showtris							= ri.Cvar_Get( "r_showtris",						"0",						CVAR_CHEAT );
	r_showsky							= ri.Cvar_Get( "r_showsky",							"0",						CVAR_CHEAT );
	r_shownormals						= ri.Cvar_Get( "r_shownormals",						"0",						CVAR_CHEAT );
	r_clear								= ri.Cvar_Get( "r_clear",							"0",						CVAR_CHEAT );
	r_offsetFactor						= ri.Cvar_Get( "r_offsetfactor",					"-1",						CVAR_CHEAT );
	r_offsetUnits						= ri.Cvar_Get( "r_offsetunits",						"-2",						CVAR_CHEAT );
	r_lockpvs							= ri.Cvar_Get( "r_lockpvs",							"0",						CVAR_CHEAT );
	r_noportals							= ri.Cvar_Get( "r_noportals",						"0",						CVAR_CHEAT );
	r_shadows							= ri.Cvar_Get( "cg_shadows",						"1",						CVAR_NONE );
	r_shadowRange						= ri.Cvar_Get( "r_shadowRange",						"1000",						CVAR_NONE );
	r_maxpolys							= ri.Cvar_Get( "r_maxpolys",						XSTRING( MAX_POLYS ),		CVAR_NONE );
	r_maxpolyverts						= ri.Cvar_Get( "r_maxpolyverts",					XSTRING( MAX_POLYVERTS ),	CVAR_NONE );
/*
Ghoul2 Insert Start
*/
#ifdef _DEBUG
	r_noPrecacheGLA						= ri.Cvar_Get( "r_noPrecacheGLA",					"0",						CVAR_CHEAT );
#endif
	r_noServerGhoul2					= ri.Cvar_Get( "r_noserverghoul2",					"0",						CVAR_CHEAT );
	r_Ghoul2AnimSmooth					= ri.Cvar_Get( "r_ghoul2animsmooth",				"0.3",						CVAR_NONE );
	r_Ghoul2UnSqashAfterSmooth			= ri.Cvar_Get( "r_ghoul2unsqashaftersmooth",		"1",						CVAR_NONE );
	broadsword							= ri.Cvar_Get( "broadsword",						"0",						CVAR_NONE );
	broadsword_kickbones				= ri.Cvar_Get( "broadsword_kickbones",				"1",						CVAR_NONE );
	broadsword_kickorigin				= ri.Cvar_Get( "broadsword_kickorigin",				"1",						CVAR_NONE );
	broadsword_dontstopanim				= ri.Cvar_Get( "broadsword_dontstopanim",			"0",						CVAR_NONE );
	broadsword_waitforshot				= ri.Cvar_Get( "broadsword_waitforshot",			"0",						CVAR_NONE );
	broadsword_playflop					= ri.Cvar_Get( "broadsword_playflop",				"1",						CVAR_NONE );
	broadsword_smallbbox				= ri.Cvar_Get( "broadsword_smallbbox",				"0",						CVAR_NONE );
	broadsword_extra1					= ri.Cvar_Get( "broadsword_extra1",					"0",						CVAR_NONE );
	broadsword_extra2					= ri.Cvar_Get( "broadsword_extra2",					"0",						CVAR_NONE );
	broadsword_effcorr					= ri.Cvar_Get( "broadsword_effcorr",				"1",						CVAR_NONE );
	broadsword_ragtobase				= ri.Cvar_Get( "broadsword_ragtobase",				"2",						CVAR_NONE );
	broadsword_dircap					= ri.Cvar_Get( "broadsword_dircap",					"64",						CVAR_NONE );
/*
Ghoul2 Insert End
*/
	r_modelpoolmegs = ri.Cvar_Get("r_modelpoolmegs", "20", CVAR_ARCHIVE);
	if (ri.Sys_LowPhysicalMemory() )
		ri.Cvar_Set("r_modelpoolmegs", "0");

	r_drawAllAreas = ri.Cvar_Get("r_drawAllAreas", "0", CVAR_TEMP | CVAR_CHEAT);

	// make sure all the commands added here are also
	// removed in R_Shutdown
	ri.Cmd_AddCommand( "imagelist", R_ImageList_f );
	ri.Cmd_AddCommand( "shaderlist", R_ShaderList_f );
	ri.Cmd_AddCommand( "skinlist", R_SkinList_f );
	ri.Cmd_AddCommand( "fontlist", R_FontList_f );
#ifdef __ANDROID__
	ri.Cmd_AddCommand("screenshot", R_ScreenShotPNG_f);
#else
	ri.Cmd_AddCommand("screenshot", R_ScreenShotTGA_f);
#endif
	ri.Cmd_AddCommand( "screenshotTGA", R_ScreenShotTGA_f );
	ri.Cmd_AddCommand( "screenshotJPEG", R_ScreenShotJPEG_f );
	ri.Cmd_AddCommand( "screenshotPNG", R_ScreenShotPNG_f );
	ri.Cmd_AddCommand( "screenshotDOF", R_ScreenShotDOF_f );
	ri.Cmd_AddCommand( "gfxinfo", GfxInfo_f );
	ri.Cmd_AddCommand( "r_atihack", R_AtiHackToggle_f );
	ri.Cmd_AddCommand( "r_we", R_WorldEffect_f);
	ri.Cmd_AddCommand( "imagecacheinfo", RE_RegisterImages_Info_f);
	ri.Cmd_AddCommand( "modellist", R_Modellist_f );
	ri.Cmd_AddCommand( "modelist", R_ModeList_f );
	ri.Cmd_AddCommand( "modelcacheinfo", RE_RegisterModels_Info_f);
	ri.Cmd_AddCommand( "minimize", GLimp_Minimize );
	
	ri.Cmd_AddCommand( "capturestop", R_MME_Shutdown );
}


GLuint pboIds[4];
/*
===============
R_Init
===============
*/
extern void R_InitWorldEffects(void); //tr_WorldEffects.cpp
void R_Init( void ) {	
	int i;
	byte *ptr;

//	Com_Printf ("----- R_Init -----\n" );
	// clear all our internal state
	memset( &tr, 0, sizeof( tr ) );
	memset( &backEnd, 0, sizeof( backEnd ) );
	memset( &tess, 0, sizeof( tess ) );

#ifdef _WIN32
	tr.wv = (WinVars_t *)ri.GetWinVars();
#endif

//	Swap_Init();

#ifndef FINAL_BUILD
	if ( (intptr_t)tess.xyz & 15 ) {
		Com_Printf( "WARNING: tess.xyz not 16 byte aligned (%x)\n",(intptr_t)tess.xyz & 15 );
	}
#endif
	//
	// init function tables
	//
	for ( i = 0; i < FUNCTABLE_SIZE; i++ )
	{
		tr.sinTable[i]		= sin( DEG2RAD( i * 360.0f / ( ( float ) ( FUNCTABLE_SIZE - 1 ) ) ) );
		tr.squareTable[i]	= ( i < FUNCTABLE_SIZE/2 ) ? 1.0f : -1.0f;
		tr.sawToothTable[i] = (float)i / FUNCTABLE_SIZE;
		tr.inverseSawToothTable[i] = 1.0f - tr.sawToothTable[i];

		if ( i < FUNCTABLE_SIZE / 2 )
		{
			if ( i < FUNCTABLE_SIZE / 4 )
			{
				tr.triangleTable[i] = ( float ) i / ( FUNCTABLE_SIZE / 4 );
			}
			else
			{
				tr.triangleTable[i] = 1.0f - tr.triangleTable[i-FUNCTABLE_SIZE / 4];
			}
		}
		else
		{
			tr.triangleTable[i] = -tr.triangleTable[i-FUNCTABLE_SIZE/2];
		}
	}
	R_InitFogTable();

	R_NoiseInit();
	R_Register();

	R_MME_Init();

	R_BloomInit();
	
	max_polys = r_maxpolys->integer;
	if (max_polys < MAX_POLYS)
		max_polys = MAX_POLYS;

	max_polyverts = r_maxpolyverts->integer;
	if (max_polyverts < MAX_POLYVERTS)
		max_polyverts = MAX_POLYVERTS;

	ptr = (byte *)Hunk_Alloc( sizeof( *backEndData ) + sizeof(srfPoly_t) * max_polys + sizeof(polyVert_t) * max_polyverts, h_low);
	backEndData = (backEndData_t *) ptr;
	backEndData->polys = (srfPoly_t *) ((char *) ptr + sizeof( *backEndData ));
	backEndData->polyVerts = (polyVert_t *) ((char *) ptr + sizeof( *backEndData ) + sizeof(srfPoly_t) * max_polys);

	R_ToggleSmpFrame();
	
	for(i = 0; i < MAX_LIGHT_STYLES; i++)
	{
		RE_SetLightStyle(i, -1);
	}
	InitOpenGL();

	R_InitImages();
	R_InitShaders(qfalse);
	R_InitSkins();

	R_TerrainInit(); //rwwRMG - added

	R_InitFonts();

	R_ModelInit();
//	re.G2VertSpaceServer = &CMiniHeap_singleton;
	R_InitDecals ( );

	R_InitWorldEffects();

	int	err = qglGetError();
	if ( err != GL_NO_ERROR )
		Com_Printf ( "glGetError() = 0x%x\n", err);

//	Com_Printf ("----- finished R_Init -----\n" );
#ifndef HAVE_GLES
	{
		// create 2 pixel buffer objects, you need to delete them when program exits.
		// glBufferDataARB with NULL pointer reserves only memory space.
		int dataSize = glConfig.vidWidth * glConfig.vidHeight * 3;
		qglGenBuffersARB(4, pboIds);
		for ( int idx = 0; idx < 4; idx++ ) {
			qglBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, pboIds[idx]);
			qglBufferDataARB(GL_PIXEL_PACK_BUFFER_ARB, dataSize, 0, GL_STREAM_READ_ARB);
		}

		qglBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, 0);
	}
#endif
}

/*
===============
RE_Shutdown
===============
*/
void RE_Shutdown( qboolean destroyWindow ) {	

//	Com_Printf ("RE_Shutdown( %i )\n", destroyWindow );

	ri.Cmd_RemoveCommand ("imagelist");
	ri.Cmd_RemoveCommand ("shaderlist");
	ri.Cmd_RemoveCommand ("skinlist");
	ri.Cmd_RemoveCommand ("fontlist");
	ri.Cmd_RemoveCommand ("screenshot");
	ri.Cmd_RemoveCommand ("screenshotTGA");
	ri.Cmd_RemoveCommand ("screenshotJPEG");
	ri.Cmd_RemoveCommand ("screenshotPNG");
	ri.Cmd_RemoveCommand ("screenshotDOF");
	ri.Cmd_RemoveCommand ("gfxinfo");
	ri.Cmd_RemoveCommand ("r_atihack");
	ri.Cmd_RemoveCommand ("r_we");
	ri.Cmd_RemoveCommand ("imagecacheinfo");
	ri.Cmd_RemoveCommand ("modellist");
	ri.Cmd_RemoveCommand ("modelist");
	ri.Cmd_RemoveCommand ("modelcacheinfo");
	ri.Cmd_RemoveCommand ("minimize");

	ri.Cmd_RemoveCommand ("capturestop");
	ri.Cmd_RemoveCommand ("capturestopstereo");

#ifndef HAVE_GLES
	if ( r_DynamicGlow && r_DynamicGlow->integer )
	{
		// Release the Glow Vertex Shader.
		if ( tr.glowVShader )
		{
			qglDeleteProgramsARB( 1, &tr.glowVShader );
		}

		// Release Pixel Shader.
		if ( tr.glowPShader )
		{
			if ( qglCombinerParameteriNV  )
			{
				// Release the Glow Regcom call list.
				qglDeleteLists( tr.glowPShader, 1 );
			}
			else if ( qglGenProgramsARB )
			{
				// Release the Glow Fragment Shader.
				qglDeleteProgramsARB( 1, &tr.glowPShader );
			}
		}

		// Release the scene glow texture.
		qglDeleteTextures( 1, &tr.screenGlow );

		// Release the scene texture.
		qglDeleteTextures( 1, &tr.sceneImage );

		// Release the blur texture.
		qglDeleteTextures( 1, &tr.blurImage );
	}
#endif

	R_TerrainShutdown(); //rwwRMG - added

	R_ShutdownFonts();
	if ( tr.registered ) {
		R_SyncRenderThread();
		R_ShutdownCommandBuffers();
		if (destroyWindow)
		{
			R_DeleteTextures();		// only do this for vid_restart now, not during things like map load
		}
	}

	R_MME_Shutdown();

	// shut down platform specific OpenGL stuff
	if ( destroyWindow ) {

		R_FrameBuffer_Shutdown();

		GLimp_Shutdown();
	}

	tr.registered = qfalse;

	tr.cTable = CT_DEFAULT;
}

/*
=============
RE_EndRegistration

Touch all images to make sure they are resident
=============
*/
void RE_EndRegistration( void ) {
	R_SyncRenderThread();
	if (!ri.Sys_LowPhysicalMemory()) {
		RB_ShowImages();
	}
}

void RE_GetLightStyle(int style, color4ub_t color)
{
	if (style >= MAX_LIGHT_STYLES)
	{
	    Com_Error( ERR_FATAL, "RE_GetLightStyle: %d is out of range", (int)style );
		return;
	}

	*(int *)color = *(int *)styleColors[style];
}

void RE_SetLightStyle(int style, int color)
{
	if (style >= MAX_LIGHT_STYLES)
	{
	    Com_Error( ERR_FATAL, "RE_SetLightStyle: %d is out of range", (int)style );
		return;
	}

	if (*(int*)styleColors[style] != color)
	{
		*(int *)styleColors[style] = color;
	}
}

static void R_DemoRandomSeed(int time, float timeFraction) {
	srand(time + timeFraction);
}

static void R_ExtendedColors(colorTable_t cTable) {
	tr.cTable = cTable;
}

static void SetRangedFog( float range ) { tr.rangedFog = range; }

extern qboolean gG2_GBMNoReconstruct;
extern qboolean gG2_GBMUseSPMethod;
static void G2API_BoltMatrixReconstruction( qboolean reconstruct ) { gG2_GBMNoReconstruct = (qboolean)!reconstruct; }
static void G2API_BoltMatrixSPMethod( qboolean spMethod ) { gG2_GBMUseSPMethod = spMethod; }

extern float tr_distortionAlpha; //opaque
extern float tr_distortionStretch; //no stretch override
extern qboolean tr_distortionPrePost; //capture before postrender phase?
extern qboolean tr_distortionNegate; //negative blend mode
static void SetRefractionProperties( float distortionAlpha, float distortionStretch, qboolean distortionPrePost, qboolean distortionNegate ) {
	tr_distortionAlpha = distortionAlpha;
	tr_distortionStretch = distortionStretch;
	tr_distortionPrePost = distortionPrePost;
	tr_distortionNegate = distortionNegate;
}

static float GetDistanceCull( void ) { return tr.distanceCull; }

static void GetRealRes( int *w, int *h ) {
	*w = glConfig.vidWidth;
	*h = glConfig.vidHeight;
}

extern void R_SVModelInit( void ); //tr_model.cpp
extern void R_AutomapElevationAdjustment( float newHeight ); //tr_world.cpp
extern qboolean R_InitializeWireframeAutomap( void ); //tr_world.cpp

static void RE_LoadImage( const char *shortname, byte **pic, int *width, int *height, int *format ) { R_LoadImage( shortname, pic, width, height, (GLenum*)format ); }
extern void R_LoadDataImage( const char *name, byte **pic, int *width, int *height);
extern void R_InvertImage(byte *data, int width, int height, int depth);
extern void R_Resample(byte *source, int swidth, int sheight, byte *dest, int dwidth, int dheight, int components);
extern void R_CreateAutomapImage( const char *name, const byte *pic, int width, int height, qboolean mipmap, qboolean allowPicmip, qboolean allowTC, int glWrapClampMode );
extern qhandle_t RE_RegisterServerSkin( const char *name );
extern IGhoul2InfoArray &TheGhoul2InfoArray();

/*
@@@@@@@@@@@@@@@@@@@@@
GetRefAPI

@@@@@@@@@@@@@@@@@@@@@
*/
extern "C" {
Q_EXPORT refexport_t* QDECL GetRefAPI( int apiVersion, refimport_t *rimp ) {
	ri = *rimp;

	memset( &re, 0, sizeof( re ) );

	if ( apiVersion != REF_API_VERSION ) {
		Com_Printf ( "Mismatched REF_API_VERSION: expected %i, got %i\n", REF_API_VERSION, apiVersion );
		return NULL;
	}

	// the RE_ functions are Renderer Entry points

	re.Shutdown = RE_Shutdown;
	re.BeginRegistration					= RE_BeginRegistration;
	re.RegisterModel						= RE_RegisterModel;
	re.RegisterServerModel					= RE_RegisterServerModel;
	re.RegisterSkin							= RE_RegisterSkin;
	re.RegisterServerSkin					= RE_RegisterServerSkin;
	re.RegisterShader						= RE_RegisterShader;
	re.RegisterShaderNoMip					= RE_RegisterShaderNoMip;
	re.ShaderNameFromIndex					= RE_ShaderNameFromIndex;
	re.LoadWorld							= RE_LoadWorldMap;
	re.SetWorldVisData						= RE_SetWorldVisData;
	re.EndRegistration						= RE_EndRegistration;
	re.BeginFrame							= RE_BeginFrame;
	re.EndFrame								= RE_EndFrame;
	re.MarkFragments						= R_MarkFragments;
	re.LerpTag								= R_LerpTag;
	re.ModelBounds							= R_ModelBounds;
	re.DrawRotatePic						= RE_RotatePic;
	re.DrawRotatePic2						= RE_RotatePic2;
	re.ClearScene							= RE_ClearScene;
	re.ClearDecals							= RE_ClearDecals;
	re.AddRefEntityToScene					= RE_AddRefEntityToScene;
	re.AddMiniRefEntityToScene				= RE_AddMiniRefEntityToScene;
	re.AddPolyToScene						= RE_AddPolyToScene;
	re.AddDecalToScene						= RE_AddDecalToScene;
	re.LightForPoint						= R_LightForPoint;
#ifndef VV_LIGHTING
	re.AddLightToScene						= RE_AddLightToScene;
	re.AddAdditiveLightToScene				= RE_AddAdditiveLightToScene;
#endif
	re.RenderScene							= RE_RenderScene;
	re.SetColor								= RE_SetColor;
	re.DrawStretchPic						= RE_StretchPic;
	re.DrawStretchRaw						= RE_StretchRaw;
	re.UploadCinematic						= RE_UploadCinematic;

	re.RegisterFont							= RE_RegisterFont;
	re.Font_StrLenPixels					= RE_Font_StrLenPixels;
	re.Font_StrLenChars						= RE_Font_StrLenChars;
	re.Font_HeightPixels					= RE_Font_HeightPixels;
	re.Font_DrawString						= RE_Font_DrawString;
	re.Language_IsAsian						= Language_IsAsian;
	re.Language_UsesSpaces					= Language_UsesSpaces;
	re.AnyLanguage_ReadCharFromString		= AnyLanguage_ReadCharFromString;

	re.RemapShader							= R_RemapShader;
	re.GetEntityToken						= R_GetEntityToken;
	re.inPVS								= R_inPVS;
	re.GetLightStyle						= RE_GetLightStyle;
	re.SetLightStyle						= RE_SetLightStyle;
	re.GetBModelVerts						= RE_GetBModelVerts;

	// missing from 1.01
	re.SetRangedFog							= SetRangedFog;
	re.SetRefractionProperties				= SetRefractionProperties;
	re.GetDistanceCull						= GetDistanceCull;
	re.GetRealRes							= GetRealRes;
	re.AutomapElevationAdjustment			= R_AutomapElevationAdjustment; //tr_world.cpp
	re.InitializeWireframeAutomap			= R_InitializeWireframeAutomap; //tr_world.cpp
	re.AddWeatherZone						= RE_AddWeatherZone;
	re.WorldEffectCommand					= RE_WorldEffectCommand;
	re.InitRendererTerrain					= RE_InitRendererTerrain;
	re.RegisterMedia_LevelLoadBegin			= RE_RegisterMedia_LevelLoadBegin;
	re.RegisterMedia_LevelLoadEnd			= RE_RegisterMedia_LevelLoadEnd;
	re.RegisterMedia_GetLevel				= RE_RegisterMedia_GetLevel;
	re.RegisterImages_LevelLoadEnd			= RE_RegisterImages_LevelLoadEnd;
	re.RegisterModels_LevelLoadEnd			= RE_RegisterModels_LevelLoadEnd;

	// G2 stuff
	re.InitSkins							= R_InitSkins;
	re.InitShaders							= R_InitShaders;
	re.SVModelInit							= R_SVModelInit;
	re.HunkClearCrap						= RE_HunkClearCrap;

	// G2API
	re.G2API_AddBolt						= G2API_AddBolt;
	re.G2API_AddBoltSurfNum					= G2API_AddBoltSurfNum;
	re.G2API_AddSurface						= G2API_AddSurface;
	re.G2API_AnimateG2ModelsRag				= G2API_AnimateG2ModelsRag;
	re.G2API_AttachEnt						= G2API_AttachEnt;
	re.G2API_AttachG2Model					= G2API_AttachG2Model;
	re.G2API_AttachInstanceToEntNum			= G2API_AttachInstanceToEntNum;
	re.G2API_AbsurdSmoothing				= G2API_AbsurdSmoothing;
	re.G2API_BoltMatrixReconstruction		= G2API_BoltMatrixReconstruction;
	re.G2API_BoltMatrixSPMethod				= G2API_BoltMatrixSPMethod;
	re.G2API_CleanEntAttachments			= G2API_CleanEntAttachments;
	re.G2API_CleanGhoul2Models				= G2API_CleanGhoul2Models;
	re.G2API_ClearAttachedInstance			= G2API_ClearAttachedInstance;
	re.G2API_CollisionDetect				= G2API_CollisionDetect;
	re.G2API_CollisionDetectCache			= G2API_CollisionDetectCache;
	re.G2API_CopyGhoul2Instance				= G2API_CopyGhoul2Instance;
	re.G2API_CopySpecificG2Model			= G2API_CopySpecificG2Model;
	re.G2API_DetachG2Model					= G2API_DetachG2Model;
	re.G2API_DoesBoneExist					= G2API_DoesBoneExist;
	re.G2API_DuplicateGhoul2Instance		= G2API_DuplicateGhoul2Instance;
	re.G2API_FreeSaveBuffer					= G2API_FreeSaveBuffer;
	re.G2API_GetAnimFileName				= G2API_GetAnimFileName;
	re.G2API_GetAnimFileNameIndex			= G2API_GetAnimFileNameIndex;
	re.G2API_GetAnimRange					= G2API_GetAnimRange;
	re.G2API_GetBoltMatrix					= G2API_GetBoltMatrix;
	re.G2API_GetBoneAnim					= G2API_GetBoneAnim;
	re.G2API_GetBoneIndex					= G2API_GetBoneIndex;
	re.G2API_GetGhoul2ModelFlags			= G2API_GetGhoul2ModelFlags;
	re.G2API_GetGLAName						= G2API_GetGLAName;
	re.G2API_GetParentSurface				= G2API_GetParentSurface;
	re.G2API_GetRagBonePos					= G2API_GetRagBonePos;
	re.G2API_GetSurfaceIndex				= G2API_GetSurfaceIndex;
	re.G2API_GetSurfaceName					= G2API_GetSurfaceName;
	re.G2API_GetSurfaceOnOff				= G2API_GetSurfaceOnOff;
	re.G2API_GetSurfaceRenderStatus			= G2API_GetSurfaceRenderStatus;
	re.G2API_GetTime						= G2API_GetTime;
	re.G2API_Ghoul2Size						= G2API_Ghoul2Size;
	re.G2API_GiveMeVectorFromMatrix			= G2API_GiveMeVectorFromMatrix;
	re.G2API_HasGhoul2ModelOnIndex			= G2API_HasGhoul2ModelOnIndex;
	re.G2API_HaveWeGhoul2Models				= G2API_HaveWeGhoul2Models;
	re.G2API_IKMove							= G2API_IKMove;
	re.G2API_InitGhoul2Model				= G2API_InitGhoul2Model;
	re.G2API_IsPaused						= G2API_IsPaused;
	re.G2API_ListBones						= G2API_ListBones;
	re.G2API_ListSurfaces					= G2API_ListSurfaces;
	re.G2API_LoadGhoul2Models				= G2API_LoadGhoul2Models;
	re.G2API_LoadSaveCodeDestructGhoul2Info	= G2API_LoadSaveCodeDestructGhoul2Info;
	re.G2API_OverrideServerWithClientData	= G2API_OverrideServerWithClientData;
	re.G2API_PauseBoneAnim					= G2API_PauseBoneAnim;
	re.G2API_PrecacheGhoul2Model			= G2API_PrecacheGhoul2Model;
	re.G2API_RagEffectorGoal				= G2API_RagEffectorGoal;
	re.G2API_RagEffectorKick				= G2API_RagEffectorKick;
	re.G2API_RagForceSolve					= G2API_RagForceSolve;
	re.G2API_RagPCJConstraint				= G2API_RagPCJConstraint;
	re.G2API_RagPCJGradientSpeed			= G2API_RagPCJGradientSpeed;
	re.G2API_RemoveBolt						= G2API_RemoveBolt;
	re.G2API_RemoveBone						= G2API_RemoveBone;
	re.G2API_RemoveGhoul2Model				= G2API_RemoveGhoul2Model;
	re.G2API_RemoveGhoul2Models				= G2API_RemoveGhoul2Models;
	re.G2API_RemoveSurface					= G2API_RemoveSurface;
	re.G2API_ResetRagDoll					= G2API_ResetRagDoll;
	re.G2API_SaveGhoul2Models				= G2API_SaveGhoul2Models;
	re.G2API_SetBoltInfo					= G2API_SetBoltInfo;
	re.G2API_SetBoneAngles					= G2API_SetBoneAngles;
	re.G2API_SetBoneAnglesIndex				= G2API_SetBoneAnglesIndex;
	re.G2API_SetBoneAnglesMatrix			= G2API_SetBoneAnglesMatrix;
	re.G2API_SetBoneAnglesMatrixIndex		= G2API_SetBoneAnglesMatrixIndex;
	re.G2API_SetBoneAnim					= G2API_SetBoneAnim;
	re.G2API_SetBoneAnimIndex				= G2API_SetBoneAnimIndex;
	re.G2API_SetBoneIKState					= G2API_SetBoneIKState;
	re.G2API_SetGhoul2ModelIndexes			= G2API_SetGhoul2ModelIndexes;
	re.G2API_SetGhoul2ModelFlags			= G2API_SetGhoul2ModelFlags;
	re.G2API_SetLodBias						= G2API_SetLodBias;
	re.G2API_SetNewOrigin					= G2API_SetNewOrigin;
	re.G2API_SetRagDoll						= G2API_SetRagDoll;
	re.G2API_SetRootSurface					= G2API_SetRootSurface;
	re.G2API_SetShader						= G2API_SetShader;
	re.G2API_SetSkin						= G2API_SetSkin;
	re.G2API_SetSurfaceOnOff				= G2API_SetSurfaceOnOff;
	re.G2API_SetTime						= G2API_SetTime;
	re.G2API_SetTimeFraction				= G2API_SetTimeFraction;
	re.G2API_SkinlessModel					= G2API_SkinlessModel;
	re.G2API_StopBoneAngles					= G2API_StopBoneAngles;
	re.G2API_StopBoneAnglesIndex			= G2API_StopBoneAnglesIndex;
	re.G2API_StopBoneAnim					= G2API_StopBoneAnim;
	re.G2API_StopBoneAnimIndex				= G2API_StopBoneAnimIndex;

	#ifdef _G2_GORE
	re.G2API_GetNumGoreMarks				= G2API_GetNumGoreMarks;
	re.G2API_AddSkinGore					= G2API_AddSkinGore;
	re.G2API_ClearSkinGore					= G2API_ClearSkinGore;
	#endif // _SOF2

	// RMG / Terrain stuff
	re.LoadDataImage						= R_LoadDataImage;
	re.InvertImage							= R_InvertImage;
	re.Resample								= R_Resample;
	re.LoadImageJA							= RE_LoadImage;
	re.CreateAutomapImage					= R_CreateAutomapImage;
	re.SavePNG								= RE_SavePNG;

	re.TheGhoul2InfoArray					= TheGhoul2InfoArray;
	// this is set in R_Init
	//re.G2VertSpaceServer	= G2VertSpaceServer;

	//mme
	re.Capture = R_MME_Capture;
	re.BlurInfo = R_MME_BlurInfo;

	re.TimeFraction = R_MME_TimeFraction;
	
	re.DemoRandomSeed = R_DemoRandomSeed;
	re.ExtendedColors = R_ExtendedColors;

	re.FontRatioFix = RE_FontRatioFix;
	re.RotatePic2RatioFix = RE_RotatePic2RatioFix;

	return &re;
}

} //extern "C"
