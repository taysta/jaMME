//Anything above this #include will be ignored by the compiler
#include "../qcommon/exe_headers.h"

#include "tr_local.h"


/*

  for a projection shadow:

  point[x] += light vector * ( z - shadow plane )
  point[y] +=
  point[z] = shadow plane

  1 0 light[x] / light[z]

*/

#define _STENCIL_REVERSE

typedef struct {
	int		i2;
	int		facing;
} edgeDef_t;

#define	MAX_EDGE_DEFS	32

static	edgeDef_t	edgeDefs[SHADER_MAX_VERTEXES][MAX_EDGE_DEFS];
static	int			numEdgeDefs[SHADER_MAX_VERTEXES];
static	int			facing[SHADER_MAX_INDEXES/3];
#ifdef HAVE_GLES
static unsigned short indexes[6 * MAX_EDGE_DEFS * SHADER_MAX_VERTEXES + SHADER_MAX_INDEXES * 2];
static int idx = 0;
#endif

void R_AddEdgeDef( int i1, int i2, int facing ) {
	int		c;

	c = numEdgeDefs[ i1 ];
	if ( c == MAX_EDGE_DEFS ) {
		return;		// overflow
	}
	edgeDefs[ i1 ][ c ].i2 = i2;
	edgeDefs[ i1 ][ c ].facing = facing;

	numEdgeDefs[ i1 ]++;
}

void R_RenderShadowEdges( void ) {
	int		i;
	int		c;
	int		j;
	int		i2;
#if 0
	int		c_edges, c_rejected;
	int		c2, k;
	int		hit[2];
#endif
#ifdef _STENCIL_REVERSE
	int		numTris;
	int		o1, o2, o3;
#endif

	// an edge is NOT a silhouette edge if its face doesn't face the light,
	// or if it has a reverse paired edge that also faces the light.
	// A well behaved polyhedron would have exactly two faces for each edge,
	// but lots of models have dangling edges or overfanned edges
#if 0
	c_edges = 0;
	c_rejected = 0;
#endif

#ifdef HAVE_GLES
	idx = 0;
#endif

	for ( i = 0 ; i < tess.numVertexes ; i++ ) {
		c = numEdgeDefs[ i ];
		for ( j = 0 ; j < c ; j++ ) {
			if ( !edgeDefs[ i ][ j ].facing ) {
				continue;
			}

			//with this system we can still get edges shared by more than 2 tris which
			//produces artifacts including seeing the shadow through walls. So for now
			//we are going to render all edges even though it is a tiny bit slower. -rww
#if 1
			i2 = edgeDefs[ i ][ j ].i2;
#ifdef HAVE_GLES
			// A single drawing call is better than many. So I prefer a singe TRIANGLES call than many TRIANGLE_STRIP call
			// even if it seems less efficiant, it's faster on the PANDORA
			indexes[idx++] = i;
			indexes[idx++] = i + tess.numVertexes;
			indexes[idx++] = i2;
			indexes[idx++] = i2;
			indexes[idx++] = i + tess.numVertexes;
			indexes[idx++] = i2 + tess.numVertexes;
#else
			qglBegin( GL_TRIANGLE_STRIP );
				qglVertex3fv( tess.xyz[ i ] );
				qglVertex3fv( tess.xyz[ i + tess.numVertexes ] );
				qglVertex3fv( tess.xyz[ i2 ] );
				qglVertex3fv( tess.xyz[ i2 + tess.numVertexes ] );
			qglEnd();
#endif
#else
			hit[0] = 0;
			hit[1] = 0;

			i2 = edgeDefs[ i ][ j ].i2;
			c2 = numEdgeDefs[ i2 ];
			for ( k = 0 ; k < c2 ; k++ ) {
				if ( edgeDefs[ i2 ][ k ].i2 == i ) {
					hit[ edgeDefs[ i2 ][ k ].facing ]++;
				}
			}

			// if it doesn't share the edge with another front facing
			// triangle, it is a sil edge
			if (hit[1] != 1)
			{
				qglBegin( GL_TRIANGLE_STRIP );
				qglVertex3fv( tess.xyz[ i ] );
				qglVertex3fv( tess.xyz[ i + tess.numVertexes ] );
				qglVertex3fv( tess.xyz[ i2 ] );
				qglVertex3fv( tess.xyz[ i2 + tess.numVertexes ] );
				qglEnd();
				c_edges++;
			} else {
				c_rejected++;
			}
#endif
		}
	}

#ifdef _STENCIL_REVERSE
	//Carmack Reverse<tm> method requires that volumes
	//be capped properly -rww
	numTris = tess.numIndexes / 3;

	for ( i = 0 ; i < numTris ; i++ )
	{
		if ( !facing[i] )
		{
			continue;
		}

		o1 = tess.indexes[ i*3 + 0 ];
		o2 = tess.indexes[ i*3 + 1 ];
		o3 = tess.indexes[ i*3 + 2 ];

#ifdef HAVE_GLES
		indexes[idx++] = o1;
		indexes[idx++] = o2;
		indexes[idx++] = o3;
		indexes[idx++] = o3 + tess.numVertexes;
		indexes[idx++] = o2 + tess.numVertexes;
		indexes[idx++] = o1 + tess.numVertexes;
#else
		qglBegin(GL_TRIANGLES);
			qglVertex3fv(tess.xyz[o1]);
			qglVertex3fv(tess.xyz[o2]);
			qglVertex3fv(tess.xyz[o3]);
		qglEnd();
		qglBegin(GL_TRIANGLES);
			qglVertex3fv(tess.xyz[o3 + tess.numVertexes]);
			qglVertex3fv(tess.xyz[o2 + tess.numVertexes]);
			qglVertex3fv(tess.xyz[o1 + tess.numVertexes]);
		qglEnd();
#endif
	}
#ifdef HAVE_GLES
	qglDrawElements(GL_TRIANGLES, idx, GL_UNSIGNED_SHORT, indexes);
#endif
#endif
}

//#define _DEBUG_STENCIL_SHADOWS

/*
=================
RB_ShadowTessEnd

triangleFromEdge[ v1 ][ v2 ]


  set triangle from edge( v1, v2, tri )
  if ( facing[ triangleFromEdge[ v1 ][ v2 ] ] && !facing[ triangleFromEdge[ v2 ][ v1 ] ) {
  }
=================
*/
void RB_DoShadowTessEnd( vec3_t lightPos );
void RB_ShadowTessEnd( void )
{
#if 0
	if (backEnd.currentEntity &&
		(backEnd.currentEntity->directedLight[0] ||
			backEnd.currentEntity->directedLight[1] ||
			backEnd.currentEntity->directedLight[2]))
	{ //an ent that has its light set for it
		RB_DoShadowTessEnd(NULL);
		return;
	}

//	if (!tess.dlightBits)
//	{
//		return;
//	}

	int i = 0;
	dlight_t *dl;

	R_TransformDlights( backEnd.refdef.num_dlights, backEnd.refdef.dlights, &backEnd.ori );
/*	while (i < tr.refdef.num_dlights)
	{
		if (tess.dlightBits & (1 << i))
		{
			dl = &tr.refdef.dlights[i];

			RB_DoShadowTessEnd(dl->transformed);
		}
        
		i++;
	}
	*/
			dl = &tr.refdef.dlights[0];

			RB_DoShadowTessEnd(dl->transformed);

#else //old ents-only way
	RB_DoShadowTessEnd(NULL);
#endif
}

void RB_DoShadowTessEnd( vec3_t lightPos )
{
	int		i;
	int		numTris;
	vec3_t	lightDir;

	// we can only do this if we have enough space in the vertex buffers
	if ( tess.numVertexes >= SHADER_MAX_VERTEXES / 2 ) {
		return;
	}

	if ( glConfig.stencilBits < 4 ) {
		return;
	}

#if 1 //controlled method - try to keep shadows in range so they don't show through so much -rww
	vec3_t	worldxyz;
	vec3_t	entLight;
	float	groundDist;

	VectorCopy( backEnd.currentEntity->lightDir, entLight );
	entLight[2] = 0.0f;
	VectorNormalize(entLight);

	//Oh well, just cast them straight down no matter what onto the ground plane.
	//This presets no chance of screwups and still looks better than a stupid
	//shader blob.
	VectorSet(lightDir, entLight[0]*0.3f, entLight[1]*0.3f, 1.0f);
	// project vertexes away from light direction
	for ( i = 0 ; i < tess.numVertexes ; i++ ) {
		//add or.origin to vert xyz to end up with world oriented coord, then figure
		//out the ground pos for the vert to project the shadow volume to
		VectorAdd(tess.xyz[i], backEnd.ori.origin, worldxyz);
		groundDist = worldxyz[2] - backEnd.currentEntity->e.shadowPlane;
		groundDist += 16.0f; //fudge factor
		VectorMA( tess.xyz[i], -groundDist, lightDir, tess.xyz[i+tess.numVertexes] );
	}
#else
	if (lightPos)
	{
		for ( i = 0 ; i < tess.numVertexes ; i++ )
		{
			tess.xyz[i+tess.numVertexes][0] = tess.xyz[i][0]+(( tess.xyz[i][0]-lightPos[0] )*128.0f);
			tess.xyz[i+tess.numVertexes][1] = tess.xyz[i][1]+(( tess.xyz[i][1]-lightPos[1] )*128.0f);
			tess.xyz[i+tess.numVertexes][2] = tess.xyz[i][2]+(( tess.xyz[i][2]-lightPos[2] )*128.0f);
		}
	}
	else
	{
		VectorCopy( backEnd.currentEntity->lightDir, lightDir );

		// project vertexes away from light direction
		for ( i = 0 ; i < tess.numVertexes ; i++ ) {
			VectorMA( tess.xyz[i], -512, lightDir, tess.xyz[i+tess.numVertexes] );
		}
	}
#endif
	// decide which triangles face the light
	memset( numEdgeDefs, 0, 4 * tess.numVertexes );

	numTris = tess.numIndexes / 3;
	for ( i = 0 ; i < numTris ; i++ ) {
		int		i1, i2, i3;
		vec3_t	d1, d2, normal;
		float	*v1, *v2, *v3;
		float	d;

		i1 = tess.indexes[ i*3 + 0 ];
		i2 = tess.indexes[ i*3 + 1 ];
		i3 = tess.indexes[ i*3 + 2 ];

		v1 = tess.xyz[ i1 ];
		v2 = tess.xyz[ i2 ];
		v3 = tess.xyz[ i3 ];

		if (!lightPos)
		{
			VectorSubtract( v2, v1, d1 );
			VectorSubtract( v3, v1, d2 );
			CrossProduct( d1, d2, normal );

			d = DotProduct( normal, lightDir );
		}
		else
		{
			float planeEq[4];
			planeEq[0] = v1[1]*(v2[2]-v3[2]) + v2[1]*(v3[2]-v1[2]) + v3[1]*(v1[2]-v2[2]);
			planeEq[1] = v1[2]*(v2[0]-v3[0]) + v2[2]*(v3[0]-v1[0]) + v3[2]*(v1[0]-v2[0]);
			planeEq[2] = v1[0]*(v2[1]-v3[1]) + v2[0]*(v3[1]-v1[1]) + v3[0]*(v1[1]-v2[1]);
			planeEq[3] = -( v1[0]*( v2[1]*v3[2] - v3[1]*v2[2] ) +
						v2[0]*(v3[1]*v1[2] - v1[1]*v3[2]) +
						v3[0]*(v1[1]*v2[2] - v2[1]*v1[2]) );

			d = planeEq[0]*lightPos[0]+
				planeEq[1]*lightPos[1]+
				planeEq[2]*lightPos[2]+
				planeEq[3];
		}

		if ( d > 0 ) {
			facing[ i ] = 1;
		} else {
			facing[ i ] = 0;
		}

		// create the edges
		R_AddEdgeDef( i1, i2, facing[ i ] );
		R_AddEdgeDef( i2, i3, facing[ i ] );
		R_AddEdgeDef( i3, i1, facing[ i ] );
	}

	GL_Bind( tr.whiteImage );
	//qglEnable( GL_CULL_FACE );
	GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );

#ifndef _DEBUG_STENCIL_SHADOWS
	qglColor3f( 0.2f, 0.2f, 0.2f );

	// don't write to the color buffer
	qglColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE );

	qglEnable( GL_STENCIL_TEST );
	qglStencilFunc( GL_ALWAYS, 1, 255 );
#else
	qglColor3f( 1.0f, 0.0f, 0.0f );
	qglPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	//qglDisable(GL_DEPTH_TEST);
#endif

#ifdef HAVE_GLES
	GLboolean text = qglIsEnabled(GL_TEXTURE_COORD_ARRAY);
	GLboolean glcol = qglIsEnabled(GL_COLOR_ARRAY);
	if (text)
		qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
	if (glcol)
		qglDisableClientState(GL_COLOR_ARRAY);
	qglVertexPointer(3, GL_FLOAT, 16, tess.xyz);
#endif

#ifdef _STENCIL_REVERSE
	qglDepthFunc(GL_LESS);

	//now using the Carmack Reverse<tm> -rww
	if ( backEnd.viewParms.isMirror ) {
		//qglCullFace( GL_BACK );
		GL_Cull(CT_BACK_SIDED);
		qglStencilOp( GL_KEEP, GL_INCR, GL_KEEP );

		R_RenderShadowEdges();

		//qglCullFace( GL_FRONT );
		GL_Cull(CT_FRONT_SIDED);
		qglStencilOp( GL_KEEP, GL_DECR, GL_KEEP );

#ifdef HAVE_GLES
		qglDrawElements(GL_TRIANGLES, idx, GL_UNSIGNED_SHORT, indexes);
#else
		R_RenderShadowEdges();
#endif
	} else {
		//qglCullFace( GL_FRONT );
		GL_Cull(CT_FRONT_SIDED);
		qglStencilOp( GL_KEEP, GL_INCR, GL_KEEP );

		R_RenderShadowEdges();

		//qglCullFace( GL_BACK );
		GL_Cull(CT_BACK_SIDED);
		qglStencilOp( GL_KEEP, GL_DECR, GL_KEEP );

#ifdef HAVE_GLES
		qglDrawElements(GL_TRIANGLES, idx, GL_UNSIGNED_SHORT, indexes);
#else
		R_RenderShadowEdges();
#endif
	}

	qglDepthFunc(GL_LEQUAL);
#else
	// mirrors have the culling order reversed
	if ( backEnd.viewParms.isMirror ) {
		qglCullFace( GL_FRONT );
		qglStencilOp( GL_KEEP, GL_KEEP, GL_INCR );

		R_RenderShadowEdges();

		qglCullFace( GL_BACK );
		qglStencilOp( GL_KEEP, GL_KEEP, GL_DECR );

#ifdef HAVE_GLES
		qglDrawElements(GL_TRIANGLES, idx, GL_UNSIGNED_SHORT, indexes);
#else
		R_RenderShadowEdges();
#endif
	} else {
		qglCullFace( GL_BACK );
		qglStencilOp( GL_KEEP, GL_KEEP, GL_INCR );

		R_RenderShadowEdges();

		qglCullFace( GL_FRONT );
		qglStencilOp( GL_KEEP, GL_KEEP, GL_DECR );

#ifdef HAVE_GLES
		qglDrawElements(GL_TRIANGLES, idx, GL_UNSIGNED_SHORT, indexes);
#else
		R_RenderShadowEdges();
#endif
	}
#endif

	// reenable writing to the color buffer
	qglColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );

#ifdef HAVE_GLES
	if (text)
		qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
	if (glcol)
		qglEnableClientState(GL_COLOR_ARRAY);
#endif

#ifdef _DEBUG_STENCIL_SHADOWS
	qglPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif
}


/*
=================
RB_ShadowFinish

Darken everything that is is a shadow volume.
We have to delay this until everything has been shadowed,
because otherwise shadows from different body parts would
overlap and double darken.
=================
*/
void RB_ShadowFinish( void ) {
	if ( r_shadows->integer != 2 ) {
		return;
	}
	if ( glConfig.stencilBits < 4 ) {
		return;
	}

#ifdef _DEBUG_STENCIL_SHADOWS
	return;
#endif

	qglEnable( GL_STENCIL_TEST );
	qglStencilFunc( GL_NOTEQUAL, 0, 255 );

	qglStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );

	bool planeZeroBack = false;
	if (qglIsEnabled(GL_CLIP_PLANE0))
	{
		planeZeroBack = true;
		qglDisable (GL_CLIP_PLANE0);
	}
	GL_Cull(CT_TWO_SIDED);
	//qglDisable (GL_CULL_FACE);

	GL_Bind( tr.whiteImage );

	qglPushMatrix();
    qglLoadIdentity ();

//	qglColor3f( 0.6f, 0.6f, 0.6f );
//	GL_State( GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO );

//	qglColor3f( 1, 0, 0 );
//	GL_State( GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );

	qglColor4f( 0.0f, 0.0f, 0.0f, 0.5f );
	//GL_State( GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
	GL_State( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );

#ifdef HAVE_GLES
	static GLfloat vtx[] = {
		-100.0f,  100.0f, -10.0f,
		100.0f,  100.0f, -10.0f,
		100.0f, -100.0f, -10.0f,
		-100.0f, -100.0f, -10.0f
	};
	GLboolean text = qglIsEnabled(GL_TEXTURE_COORD_ARRAY);
	GLboolean glcol = qglIsEnabled(GL_COLOR_ARRAY);
	if (text)
		qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
	if (glcol)
		qglDisableClientState(GL_COLOR_ARRAY);
	qglVertexPointer(3, GL_FLOAT, 0, vtx);
	qglDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	if (text)
		qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
	if (glcol)
		qglEnableClientState(GL_COLOR_ARRAY);
#else
	qglBegin( GL_QUADS );
	qglVertex3f( -100.0f, 100.0f, -10.0f);
	qglVertex3f( 100.0f, 100.0f, -10.0f);
	qglVertex3f( 100.0f, -100.0f, -10.0f);
	qglVertex3f( -100.0f, -100.0f, -10.0f);
	qglEnd();
#endif

	qglColor4f(1,1,1,1);
	qglDisable( GL_STENCIL_TEST );
	if (planeZeroBack)
	{
		qglEnable (GL_CLIP_PLANE0);
	}
	qglPopMatrix();
}


/*
=================
RB_ProjectionShadowDeform

=================
*/
void RB_ProjectionShadowDeform( void ) {
	float	*xyz;
	int		i;
	float	h;
	vec3_t	ground;
	vec3_t	light;
	float	groundDist;
	float	d;
	vec3_t	lightDir;

	xyz = ( float * ) tess.xyz;

	ground[0] = backEnd.ori.axis[0][2];
	ground[1] = backEnd.ori.axis[1][2];
	ground[2] = backEnd.ori.axis[2][2];

	groundDist = backEnd.ori.origin[2] - backEnd.currentEntity->e.shadowPlane;

	VectorCopy( backEnd.currentEntity->lightDir, lightDir );
	d = DotProduct( lightDir, ground );
	// don't let the shadows get too long or go negative
	if ( d < 0.5 ) {
		VectorMA( lightDir, (0.5 - d), ground, lightDir );
		d = DotProduct( lightDir, ground );
	}
	d = 1.0 / d;

	light[0] = lightDir[0] * d;
	light[1] = lightDir[1] * d;
	light[2] = lightDir[2] * d;

	for ( i = 0; i < tess.numVertexes; i++, xyz += 4 ) {
		h = DotProduct( xyz, ground ) + groundDist;

		xyz[0] -= light[0] * h;
		xyz[1] -= light[1] * h;
		xyz[2] -= light[2] * h;
	}
}

//update tr.screenImage
void RB_CaptureScreenImage(void)
{
	int radX = 2048;
	int radY = 2048;
	int x = glConfig.vidWidth/2;
	int y = glConfig.vidHeight/2;
	int cX, cY;

	GL_Bind( tr.screenImage );
	//using this method, we could pixel-filter the texture and all sorts of crazy stuff.
	//but, it is slow as hell.
	/*
	static byte *tmp = NULL;
	if (!tmp)
	{
		tmp = (byte *)Z_Malloc((sizeof(byte)*4)*(glConfig.vidWidth*glConfig.vidHeight), TAG_ICARUS, qtrue);
	}
	qglReadPixels(0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_RGBA, GL_UNSIGNED_BYTE, tmp);
	qglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 512, 512, 0, GL_RGBA, GL_UNSIGNED_BYTE, tmp);
	*/

	if (radX > glConfig.maxTextureSize)
	{
		radX = glConfig.maxTextureSize;
	}
	if (radY > glConfig.maxTextureSize)
	{
		radY = glConfig.maxTextureSize;
	}

	while (glConfig.vidWidth < radX)
	{
		radX /= 2;
	}
	while (glConfig.vidHeight < radY)
	{
		radY /= 2;
	}

	cX = x-(radX/2);
	cY = y-(radY/2);

	if (cX+radX > glConfig.vidWidth)
	{ //would it go off screen?
		cX = glConfig.vidWidth-radX;
	}
	else if (cX < 0)
	{ //cap it off at 0
		cX = 0;
	}

	if (cY+radY > glConfig.vidHeight)
	{ //would it go off screen?
		cY = glConfig.vidHeight-radY;
	}
	else if (cY < 0)
	{ //cap it off at 0
		cY = 0;
	}

#ifdef HAVE_GLES
	qglCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, cX, cY, radX, radY, 0);
#else
	qglCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16, cX, cY, radX, radY, 0);
#endif
}

//yeah.. not really shadow-related.. but it's stencil-related. -rww
float tr_distortionAlpha = 1.0f; //opaque
float tr_distortionStretch = 0.0f; //no stretch override
qboolean tr_distortionPrePost = qfalse; //capture before postrender phase?
qboolean tr_distortionNegate = qfalse; //negative blend mode
void RB_DistortionFill(void)
{
	float alpha = tr_distortionAlpha;
	float spost = 0.0f;
	float spost2 = 0.0f;

	if ( glConfig.stencilBits < 4 )
	{
		return;
	}

	//ok, cap the stupid thing now I guess
	if (!tr_distortionPrePost)
	{
		RB_CaptureScreenImage();
	}

	qglEnable(GL_STENCIL_TEST);
	qglStencilFunc(GL_NOTEQUAL, 0, 0xFFFFFFFF);
	qglStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	qglDisable (GL_CLIP_PLANE0);
	GL_Cull( CT_TWO_SIDED );

	//reset the view matrices and go into ortho mode
	qglMatrixMode(GL_PROJECTION);
	qglPushMatrix();
	qglLoadIdentity();
	qglOrtho(0, glConfig.vidWidth, glConfig.vidHeight, 32, -1, 1);
	qglMatrixMode(GL_MODELVIEW);
	qglPushMatrix();
	qglLoadIdentity();

	if (tr_distortionStretch)
	{ //override
		spost = tr_distortionStretch;
		spost2 = tr_distortionStretch;
	}
	else
	{ //do slow stretchy effect
		spost = sin(tr.refdef.time * 0.0005 + tr.refdef.timeFraction * 0.0005);
		if (spost < 0.0f)
		{
			spost = -spost;
		}
		spost *= 0.2f;

		spost2 = sin(tr.refdef.time * 0.0005 + tr.refdef.timeFraction * 0.0005);
		if (spost2 < 0.0f)
		{
			spost2 = -spost2;
		}
		spost2 *= 0.08f;
	}

	if (alpha != 1.0f)
	{ //blend
		GL_State(GLS_SRCBLEND_SRC_ALPHA|GLS_DSTBLEND_SRC_ALPHA);
	}
	else
	{ //be sure to reset the draw state
		GL_State(0);
	}


#ifdef HAVE_GLES
	qglColor4f(1.0f, 1.0f, 1.0f, alpha);
	GLboolean text = qglIsEnabled(GL_TEXTURE_COORD_ARRAY);
	GLboolean glcol = qglIsEnabled(GL_COLOR_ARRAY);
	if (!text)
		qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
	if (glcol)
		qglDisableClientState(GL_COLOR_ARRAY);
	GLfloat tex[] = {
		0 + spost2, 1 - spost,
		0 + spost2, 0 + spost,
		1 - spost2, 0 + spost,
		1 - spost2, 1 - spost
	};
	GLfloat vtx[] = {
		0, 0,
		0, glConfig.vidHeight,
		glConfig.vidWidth, glConfig.vidHeight,
		glConfig.vidWidth, 0
	};
	qglTexCoordPointer(2, GL_FLOAT, 0, tex);
	qglVertexPointer(2, GL_FLOAT, 0, vtx);
	qglDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	/*	if (glcol)
	qglEnableClientState( GL_COLOR_ARRAY );
	if (!text)
	qglDisableClientState( GL_TEXTURE_COORD_ARRAY );*/
#else
	qglBegin(GL_QUADS);
		qglColor4f(1.0f, 1.0f, 1.0f, alpha);
		qglTexCoord2f(0+spost2, 1-spost);
		qglVertex2f(0, 0);

		qglTexCoord2f(0+spost2, 0+spost);
		qglVertex2f(0, glConfig.vidHeight);

		qglTexCoord2f(1-spost2, 0+spost);
		qglVertex2f(glConfig.vidWidth, glConfig.vidHeight);

		qglTexCoord2f(1-spost2, 1-spost);
		qglVertex2f(glConfig.vidWidth, 0);
	qglEnd();
#endif

	if (tr_distortionAlpha == 1.0f && tr_distortionStretch == 0.0f)
	{ //no overrides
		if (tr_distortionNegate)
		{ //probably the crazy alternate saber trail
			alpha = 0.8f;
			GL_State(GLS_SRCBLEND_ZERO|GLS_DSTBLEND_ONE_MINUS_SRC_COLOR);
		}
		else
		{
			alpha = 0.5f;
			GL_State(GLS_SRCBLEND_SRC_ALPHA|GLS_DSTBLEND_SRC_ALPHA);
		}

		spost = sin(tr.refdef.time * 0.0008 + tr.refdef.timeFraction * 0.0008);
		if (spost < 0.0f)
		{
			spost = -spost;
		}
		spost *= 0.08f;

		spost2 = sin(tr.refdef.time * 0.0008 + tr.refdef.timeFraction * 0.0008);
		if (spost2 < 0.0f)
		{
			spost2 = -spost2;
		}
		spost2 *= 0.2f;


#ifdef HAVE_GLES
		qglColor4f(1.0f, 1.0f, 1.0f, alpha);
		/*		GLboolean text = qglIsEnabled(GL_TEXTURE_COORD_ARRAY);
		GLboolean glcol = qglIsEnabled(GL_COLOR_ARRAY);
		if (!text)
		qglEnableClientState( GL_TEXTURE_COORD_ARRAY );
		if (glcol)
		qglDisableClientState( GL_COLOR_ARRAY );*/
		GLfloat tex[] = {
			0 + spost2, 1 - spost,
			0 + spost2, 0 + spost,
			1 - spost2, 0 + spost,
			1 - spost2, 1 - spost
		};
		GLfloat vtx[] = {
			0, 0,
			0, glConfig.vidHeight,
			glConfig.vidWidth, glConfig.vidHeight,
			glConfig.vidWidth, 0
		};
		qglTexCoordPointer(2, GL_FLOAT, 0, tex);
		qglVertexPointer(2, GL_FLOAT, 0, vtx);
		qglDrawArrays(GL_TRIANGLE_FAN, 0, 4);
#else
		qglBegin(GL_QUADS);
			qglColor4f(1.0f, 1.0f, 1.0f, alpha);
			qglTexCoord2f(0+spost2, 1-spost);
			qglVertex2f(0, 0);

			qglTexCoord2f(0+spost2, 0+spost);
			qglVertex2f(0, glConfig.vidHeight);

			qglTexCoord2f(1-spost2, 0+spost);
			qglVertex2f(glConfig.vidWidth, glConfig.vidHeight);

			qglTexCoord2f(1-spost2, 1-spost);
			qglVertex2f(glConfig.vidWidth, 0);
		qglEnd();
#endif
	}
#ifdef HAVE_GLES
	if (glcol)
		qglEnableClientState(GL_COLOR_ARRAY);
	if (!text)
		qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
#endif

	//pop the view matrices back
	qglMatrixMode(GL_PROJECTION);
	qglPopMatrix();
	qglMatrixMode(GL_MODELVIEW);
	qglPopMatrix();

	qglDisable( GL_STENCIL_TEST );
}

