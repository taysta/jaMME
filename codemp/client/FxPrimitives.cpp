// this include must remain at the top of every CPP file
//Anything above this #include will be ignored by the compiler
#include "../qcommon/exe_headers.h"

#include "client.h"
#include "FxScheduler.h"

#ifdef VV_LIGHTING
#include "../renderer/tr_lightmanager.h"
#endif

extern int		drawnFx;

//--------------------------
//
// Base Effect Class
//
//--------------------------
CEffect::CEffect(void) :
	mMatImpactFX(MATIMPACTFX_NONE),
	mMatImpactParm(-1),
	mSoundVolume(-1),
	mSoundRadius(-1),
	mFlags(0)
{
	memset( &mRefEnt, 0, sizeof( mRefEnt ));
}

//--------------------------
//
// Derived Particle Class
//
//--------------------------

//----------------------------
void CParticle::Init(void)
{
	mRefEnt.radius = 0.0f;
	if (mFlags & FX_PLAYER_VIEW)
	{
		mOrigin1[0] = (int)(random() * 639);
		mOrigin1[1] = (int)(random() * 479);
	}
}

//----------------------------
void CParticle::Die(void)
{
	if ( mFlags & FX_DEATH_RUNS_FX && !(mFlags & FX_KILL_ON_IMPACT) )
	{
		vec3_t	norm;

		// Man, this just seems so, like, uncool and stuff...
		VectorSet( norm, crandom(), crandom(), crandom());
		VectorNormalize( norm );

		theFxScheduler.PlayEffect( mDeathFxID, mOrigin1, norm );
	}
}

//----------------------------
bool CParticle::Cull(void)
{
	vec3_t	dir;

	if (mFlags & FX_PLAYER_VIEW)
	{
		// this will be drawn as a 2D effect so don't cull it
		return false;
	}

	// Get the direction to the view
	VectorSubtract( mOrigin1, theFxHelper.refdef->vieworg, dir );

	// Check if it's behind the viewer
	if ( (DotProduct( theFxHelper.refdef->viewaxis[0], dir )) < 0) // cg.cosHalfFOV * (len - mRadius) )
	{
		return true;
	}

	// don't cull if this is hacked to show up close to the inview wpn
	if (mFlags & FX_DEPTH_HACK)
	{
		return false;
	}
	// Can't be too close
	float len = VectorLengthSquared( dir );
	if ( len < fx_nearCull->value )
	{
		return true;
	}

	return false;
}

//----------------------------
void CParticle::Draw(void)
{
	if ( mFlags & FX_DEPTH_HACK )
	{
		// Not sure if first person needs to be set, but it can't hurt?
		mRefEnt.renderfx |= RF_DEPTHHACK;
	}

	if (mFlags & FX_PLAYER_VIEW)
	{
		vec4_t	color;

		color[0] = mRefEnt.shaderRGBA[0] / 255.0;
		color[1] = mRefEnt.shaderRGBA[1] / 255.0;
		color[2] = mRefEnt.shaderRGBA[2] / 255.0;
		color[3] = mRefEnt.shaderRGBA[3] / 255.0;

		// add this 2D effect to the proper list. it will get drawn after the cgi.RenderScene call
		theFxScheduler.Add2DEffect(mOrigin1[0], mOrigin1[1], mRefEnt.radius, mRefEnt.radius, color, mRefEnt.customShader);
	}
	else
	{
		// Add our refEntity to the scene
		VectorCopy( mOrigin1, mRefEnt.origin );

		theFxHelper.AddFxToScene(&mRefEnt);
	}
	drawnFx++;	
}

//----------------------------
// Update
//----------------------------
bool CParticle::Update(void)
{
	// Game pausing can cause dumb time things to happen, so kill the effect in this instance
	if ( mTimeStart > theFxHelper.mTime )
	{
		return false;
	}

	if ( mFlags & FX_RELATIVE )
	{
		if ( !mGhoul2.IsValid())
		{	// the thing we are bolted to is no longer valid, so we may as well just die.
			return false;
		}

		vec3_t	org;
		vec3_t	ax[3];

		// Get our current position and direction
		if (!theFxHelper.GetOriginAxisFromBolt(&mGhoul2, mEntNum, mModelNum, mBoltNum, org, ax))
		{	//could not get bolt
			return false;
		}
		vec3_t 	realVel, realAccel;

		VectorMA( org, mOrgOffset[0], ax[0], org );
		VectorMA( org, mOrgOffset[1], ax[1], org );
		VectorMA( org, mOrgOffset[2], ax[2], org );

		const float	time = ((theFxHelper.mTime - mTimeStart) + theFxHelper.mTimeFraction) * 0.001f;
		// calc the real velocity and accel vectors
		VectorScale( ax[0], mVel[0], realVel );
		VectorMA( realVel, mVel[1], ax[1], realVel );
		VectorMA( realVel, mVel[2], ax[2], realVel );
		//realVel[2] += 0.5f * mGravity * time;

		VectorScale( ax[0], mAccel[0], realAccel );
		VectorMA( realAccel, mAccel[1], ax[1], realAccel );
		VectorMA( realAccel, mAccel[2], ax[2], realAccel );

		// Get our real velocity at the current time, taking into account the effects of acceleartion.  NOTE: not sure if this is even 100% correct math-wise
		VectorMA( realVel, time, realAccel, realVel );

		// Now move us to where we should be at the given time
		VectorMA( org, time, realVel, mOrigin1 );
		
	}
	else if (( mTimeStart < theFxHelper.mTime ) && UpdateOrigin() == false )
	{
		// we are marked for death
		return false;
	}


	if ( !Cull() )
	{	
		// Only update these if the thing is visible.	
		UpdateSize();
		UpdateRGB();
		UpdateAlpha();
		UpdateRotation();

		Draw();
	}

	return true;
}

//----------------------------
// Update Origin
//----------------------------
bool CParticle::UpdateOrigin(void)
{
	vec3_t	new_origin;
	int		FP = fx_forcePhysics->integer;

	VectorMA(mVel, theFxHelper.mRealTime, mAccel, mVel);
	// Predict the new position
	new_origin[0] = mOrigin1[0] + (theFxHelper.mRealTime * mVel[0]);// + (theFxHelper.mHalfRealTimeSq * mVel[0]);
	new_origin[1] = mOrigin1[1] + (theFxHelper.mRealTime * mVel[1]);// + (theFxHelper.mHalfRealTimeSq * mVel[1]);
	new_origin[2] = mOrigin1[2] + (theFxHelper.mRealTime * mVel[2]);// + (theFxHelper.mHalfRealTimeSq * mVel[2]);

	// Only perform physics if this object is tagged to do so
	if ((mFlags & FX_APPLY_PHYSICS || FP & FX_FORCE_PHYSICS_APPLY) && !(mFlags & FX_PLAYER_VIEW))
	{
		bool solid;

		if (mFlags & FX_EXPENSIVE_PHYSICS || FP & FX_FORCE_PHYSICS_EXPENSIVE)
		{
			solid = true; // by setting this to true, we force a real trace to happen
		}
		else
		{
			// if this returns solid, we need to do a trace
			if (!com_RMG || com_RMG->integer)
			{	// don't do this call for RMG maps
				TCGPointContents	*data = (TCGPointContents *)cl.mSharedMemory;

				VectorCopy(new_origin, data->mPoint);
				data->mPassEntityNum = ENTITYNUM_WORLD;

				// if this returns solid, we need to do a trace
				solid = !!(VM_Call( cgvm, CG_POINT_CONTENTS ) & MASK_SOLID); 
			}
			else
			{
				solid = false;
			}
		}

		if ( solid )
		{
			trace_t	trace;
			float	dot;

			if (mFlags & FX_USE_BBOX || FP & FX_FORCE_PHYSICS_BBOX)
			{
				if (mFlags & FX_GHOUL2_TRACE || FP & FX_FORCE_PHYSICS_GHOUL2)
				{
					theFxHelper.G2Trace( trace, mOrigin1, mMin, mMax, new_origin, -1, MASK_SOLID );
				}
				else
				{
					theFxHelper.Trace( trace, mOrigin1, mMin, mMax, new_origin, -1, MASK_SOLID );
				}
			}
			else
			{
				if (mFlags & FX_GHOUL2_TRACE || FP & FX_FORCE_PHYSICS_GHOUL2)
				{
					theFxHelper.G2Trace( trace, mOrigin1, NULL, NULL, new_origin, -1, MASK_PLAYERSOLID );
				}
				else
				{
					theFxHelper.Trace( trace, mOrigin1, NULL, NULL, new_origin, -1, MASK_SOLID );
				}
			}

			// Hit something
			if (trace.startsolid || trace.allsolid)
			{
				VectorClear( mVel );
				VectorClear( mAccel );

				if ((mFlags & FX_GHOUL2_TRACE || FP & FX_FORCE_PHYSICS_GHOUL2) && (mFlags & FX_IMPACT_RUNS_FX))
				{
					static vec3_t bsNormal = {0, 1, 0};

					theFxScheduler.PlayEffect( mImpactFxID, trace.endpos, bsNormal );
				}

				mFlags &= ~(FX_APPLY_PHYSICS | FX_IMPACT_RUNS_FX);

				return true;
			}
			else if ( trace.fraction < 1.0f )//&& !trace.startsolid && !trace.allsolid )
			{
				if ( mFlags & FX_IMPACT_RUNS_FX && !(trace.surfaceFlags & SURF_NOIMPACT ))
				{
					theFxScheduler.PlayEffect( mImpactFxID, trace.endpos, trace.plane.normal );
				}

				// may need to interact with the material type we hit
				theFxScheduler.MaterialImpact(&trace, (CEffect*)this);

				if ( mFlags & FX_KILL_ON_IMPACT	)
				{
					// time to die
					return false;
				}

				VectorMA( mVel, theFxHelper.mRealTime * trace.fraction, mAccel, mVel );

				dot = DotProduct( mVel, trace.plane.normal );

				VectorMA( mVel, -2.0f * dot, trace.plane.normal, mVel );

				VectorScale( mVel, mElasticity, mVel );
				mElasticity *= 0.5f;

				// If the velocity is too low, make it stop moving, rotating, and turn off physics to avoid
				//	doing expensive operations when they aren't needed
				//if ( trace.plane.normal[2] > 0.33f && mVel[2] < 10.0f )
				if (VectorLengthSquared(mVel) < 1.0f)
				{
					VectorClear( mVel );
					VectorClear( mAccel );

					mFlags &= ~(FX_APPLY_PHYSICS | FX_IMPACT_RUNS_FX);
				}

				// Set the origin to the exact impact point
				VectorMA( trace.endpos, 1.0f, trace.plane.normal, mOrigin1 );
				return true;
			}
		}
	}

	// No physics were done to this object, move it
	VectorCopy( new_origin, mOrigin1 );

	if (mFlags & FX_PLAYER_VIEW)
	{
		if (mOrigin1[0] < 0 || mOrigin1[0] > 639 || mOrigin1[1] < 0 || mOrigin1[1] > 479)
		{
			return false;
		}
	}

	return true;
}

//----------------------------
// Update Size
//----------------------------
void CParticle::UpdateSize(void)
{
	// completely biased towards start if it doesn't get overridden
	float	perc1 = 1.0f, perc2 = 1.0f; 

	if ( (mFlags & FX_SIZE_LINEAR) )
	{ 
		// calculate element biasing
		perc1 = 1.0f - ((float)(theFxHelper.mTime - mTimeStart) + theFxHelper.mTimeFraction) / (float)(mTimeEnd - mTimeStart);
		if (perc1 < 0)
			perc1 = 0;
		else if (perc1 > 1)
			perc1 = 1;
	}

	// We can combine FX_LINEAR with _either_ FX_NONLINEAR, FX_WAVE, or FX_CLAMP
	if (( mFlags & FX_SIZE_PARM_MASK ) == FX_SIZE_NONLINEAR )
	{
		if ((double)((double)theFxHelper.mTime + (double)theFxHelper.mTimeFraction) > (double)mSizeParm)
		{ 
			// get percent done, using parm as the start of the non-linear fade
			perc2 = 1.0f - ((float)((float)theFxHelper.mTime - mSizeParm) + theFxHelper.mTimeFraction) / (float)(mTimeEnd - mSizeParm);
			if (perc2 < 0)
				perc2 = 0;
			else if (perc2 > 1)
				perc2 = 1;
		}

		if ( mFlags & FX_SIZE_LINEAR )
		{
			// do an even blend
			perc1 = perc1 * 0.5f + perc2 * 0.5f;
		}
		else
		{ 
			// just copy it over...sigh
			perc1 = perc2;
		}
	}
	else if (( mFlags & FX_SIZE_PARM_MASK ) == FX_SIZE_WAVE )
	{ 
		// wave gen, with parm being the frequency multiplier
		perc1 = perc1 * cosf(((float)(theFxHelper.mTime - mTimeStart) + theFxHelper.mTimeFraction) * mSizeParm);
	}
	else if (( mFlags & FX_SIZE_PARM_MASK ) == FX_SIZE_CLAMP )
	{
		if ((double)((double)theFxHelper.mTime + (double)theFxHelper.mTimeFraction) < (double)mSizeParm)
		{ 
			// get percent done, using parm as the start of the non-linear fade
			perc2 = ((float)(mSizeParm - (float)theFxHelper.mTime) - theFxHelper.mTimeFraction) / (float)(mSizeParm - mTimeStart);
			if (perc2 < 0)
				perc2 = 0;
			else if (perc2 > 1)
				perc2 = 1;
		}
		else
		{
			perc2 = 0.0f; // make it full size??
		}

		if ( (mFlags & FX_SIZE_LINEAR) )
		{ 
			// do an even blend
			perc1 = perc1 * 0.5f + perc2 * 0.5f;
		}
		else
		{ 
			// just copy it over...sigh
			perc1 = perc2;
		}
	}

	// If needed, RAND can coexist with linear and either non-linear or wave.
	if ( mFlags & FX_SIZE_RAND )
	{ 
		// Random simply modulates the existing value
		perc1 = random() * perc1;
	}

	mRefEnt.radius = (double)(((double)mSizeStart * (double)perc1) + ((double)mSizeEnd * (double)(1.0f - perc1)));
}

inline int VectorToInt(vec3_t vec)
{
	int			tmp, retval;

	// FIXME: unix compatibility needed
#ifdef _WIN32
	_asm
	{
		push	edx
		mov		edx, [vec]
		fld		dword ptr[edx + 0]
		fld		dword ptr[edx + 4]
		fld		dword ptr[edx + 8]

		mov		eax, 0xff00

		fistp	tmp	   
		mov		al, byte ptr [tmp]
		shl		eax, 16
		
		fistp	tmp
		mov		ah, byte ptr [tmp]

		fistp	tmp
		mov		al, byte ptr [tmp]

		mov		[retval], eax
		pop		edx
	}
#endif
	return(retval);
}

//----------------------------
// Update RGB
//----------------------------
void CParticle::UpdateRGB(void)
{
	// completely biased towards start if it doesn't get overridden
	float	perc1 = 1.0f, perc2 = 1.0f; 
	vec3_t	res;

	if ( (mFlags & FX_RGB_LINEAR) )
	{ 
		// calculate element biasing
		perc1 = 1.0f - ((float)(theFxHelper.mTime - mTimeStart) + theFxHelper.mTimeFraction) / (float)(mTimeEnd - mTimeStart);
		if (perc1 < 0)
			perc1 = 0;
		else if (perc1 > 1)
			perc1 = 1;
	}

	// We can combine FX_LINEAR with _either_ FX_NONLINEAR, FX_WAVE, or FX_CLAMP
	if (( mFlags & FX_RGB_PARM_MASK ) == FX_RGB_NONLINEAR )
	{
		if ((double)((double)theFxHelper.mTime + (double)theFxHelper.mTimeFraction) > (double)mRGBParm)
		{ 
			// get percent done, using parm as the start of the non-linear fade
			perc2 = 1.0f - ((float)((float)theFxHelper.mTime - mRGBParm) + theFxHelper.mTimeFraction) / (float)(mTimeEnd - mRGBParm);
			if (perc2 < 0)
				perc2 = 0;
			else if (perc2 > 1)
				perc2 = 1;
		}

		if ( (mFlags & FX_RGB_LINEAR) )
		{ 
			// do an even blend
			perc1 = perc1 * 0.5f + perc2 * 0.5f;
		}
		else
		{ 
			// just copy it over...sigh
			perc1 = perc2;
		}
	}
	else if (( mFlags & FX_RGB_PARM_MASK ) == FX_RGB_WAVE )
	{ 
		// wave gen, with parm being the frequency multiplier
		perc1 = perc1 * cosf(((float)(theFxHelper.mTime - mTimeStart) + theFxHelper.mTimeFraction) * mRGBParm);
	}
	else if (( mFlags & FX_RGB_PARM_MASK ) == FX_RGB_CLAMP )
	{
		if ((double)((double)theFxHelper.mTime + (double)theFxHelper.mTimeFraction) < (double)mRGBParm)
		{ 
			// get percent done, using parm as the start of the non-linear fade
			perc2 = ((float)(mRGBParm - (float)theFxHelper.mTime) - theFxHelper.mTimeFraction) / (float)(mRGBParm - mTimeStart);
			if (perc2 < 0)
				perc2 = 0;
			else if (perc2 > 1)
				perc2 = 1;
		}
		else
		{
			perc2 = 0.0f; // make it full size??
		}

		if (( mFlags & FX_RGB_LINEAR ))
		{ 
			// do an even blend
			perc1 = perc1 * 0.5f + perc2 * 0.5f;
		}
		else
		{ 
			// just copy it over...sigh
			perc1 = perc2;
		}
	}

	// If needed, RAND can coexist with linear and either non-linear or wave.
	if ( mFlags & FX_RGB_RAND )
	{ 
		// Random simply modulates the existing value
		perc1 = random() * perc1;
	}

	// Now get the correct color 
	VectorScale( mRGBStart, perc1, res );
	VectorMA( res, 1.0f - perc1, mRGBEnd, res );

	res[0] = Com_Clamp(0.0f, 1.0f, res[0]) * 255.0f;
	res[1] = Com_Clamp(0.0f, 1.0f, res[1]) * 255.0f;
	res[2] = Com_Clamp(0.0f, 1.0f, res[2]) * 255.0f;

#ifdef _WIN32
	*(int *)mRefEnt.shaderRGBA = VectorToInt(res);
#else
    mRefEnt.shaderRGBA[0] = (char)res[0];
    mRefEnt.shaderRGBA[1] = (char)res[1];
    mRefEnt.shaderRGBA[2] = (char)res[2];
#endif

}

//----------------------------
// Update Alpha
//----------------------------
void CParticle::UpdateAlpha(void)
{
	int		alpha;

	// completely biased towards start if it doesn't get overridden
	float	perc1 = 1.0f, perc2 = 1.0f;

	if ( (mFlags & FX_ALPHA_LINEAR) )
	{ 
		// calculate element biasing
		perc1 = 1.0f - ((float)(theFxHelper.mTime - mTimeStart) + theFxHelper.mTimeFraction) / (float)(mTimeEnd - mTimeStart);
		if (perc1 < 0)
			perc1 = 0;
		else if (perc1 > 1)
			perc1 = 1;
	}

	// We can combine FX_LINEAR with _either_ FX_NONLINEAR, FX_WAVE, or FX_CLAMP
	if (( mFlags & FX_ALPHA_PARM_MASK ) == FX_ALPHA_NONLINEAR )
	{
		if ((double)((double)theFxHelper.mTime + (double)theFxHelper.mTimeFraction) > (double)mAlphaParm)
		{ 
			// get percent done, using parm as the start of the non-linear fade
			perc2 = 1.0f - ((float)((float)theFxHelper.mTime - mAlphaParm) + theFxHelper.mTimeFraction) / (float)(mTimeEnd - mAlphaParm);
			if (perc2 < 0)
				perc2 = 0;
			else if (perc2 > 1)
				perc2 = 1;
		}

		if (( mFlags & FX_ALPHA_LINEAR ))
		{ 
			// do an even blend
			perc1 = perc1 * 0.5f + perc2 * 0.5f;
		}
		else
		{ 
			// just copy it over...sigh
			perc1 = perc2;
		}
	}
	else if (( mFlags & FX_ALPHA_PARM_MASK ) == FX_ALPHA_WAVE )
	{ 
		// wave gen, with parm being the frequency multiplier
		perc1 = perc1 * cosf(((float)(theFxHelper.mTime - mTimeStart) + theFxHelper.mTimeFraction) * mAlphaParm);
	}
	else if (( mFlags & FX_ALPHA_PARM_MASK ) == FX_ALPHA_CLAMP )
	{
		if ((double)((double)theFxHelper.mTime + (double)theFxHelper.mTimeFraction) < (double)mAlphaParm)
		{ 
			// get percent done, using parm as the start of the non-linear fade
			perc2 = ((float)(mAlphaParm - (float)theFxHelper.mTime) - theFxHelper.mTimeFraction) / (float)(mAlphaParm - mTimeStart);
			if (perc2 < 0)
				perc2 = 0;
			else if (perc2 > 1)
				perc2 = 1;
		}
		else
		{
			perc2 = 0.0f; // make it full size??
		}

		if (( mFlags & FX_ALPHA_LINEAR ))
		{ 
			// do an even blend
			perc1 = perc1 * 0.5f + perc2 * 0.5f;
		}
		else
		{ 
			// just copy it over...sigh
			perc1 = perc2;
		}
	}

	perc1 = (double)((double)((double)mAlphaStart * (double)perc1) + (double)((double)mAlphaEnd * (double)(1.0f - perc1)));

	// We should be in the right range, but clamp to ensure
	perc1 = Com_Clamp(0.0f, 1.0f, perc1);

	// If needed, RAND can coexist with linear and either non-linear or wave.
	if ( mFlags & FX_ALPHA_RAND )
	{ 
		// Random simply modulates the existing value
		perc1 = random() * perc1;
	}

	alpha = Com_Clamp(0, 255, perc1 * 255.0f);
	if ( mFlags & FX_USE_ALPHA )
	{
		// should use this when using art that has an alpha channel
		 mRefEnt.shaderRGBA[3] = (byte)alpha;
	}
	else
	{
		// Modulate the rgb fields by the alpha value to do the fade, works fine for additive blending
		mRefEnt.shaderRGBA[0] = ((int)mRefEnt.shaderRGBA[0] * alpha) >> 8;
		mRefEnt.shaderRGBA[1] = ((int)mRefEnt.shaderRGBA[1] * alpha) >> 8;
		mRefEnt.shaderRGBA[2] = ((int)mRefEnt.shaderRGBA[2] * alpha) >> 8;
	}
}

//--------------------------
void CParticle::UpdateRotation(void)
{
	mRefEnt.rotation += (theFxHelper.mFrameTime * 0.01f) * mRotationDelta;
	mRotationDelta *= (1.0f - (theFxHelper.mFrameTime * 0.0007f)); // decay rotationDelta
}


//--------------------------------
//
// Derived Oriented Particle Class
//
//--------------------------------
COrientedParticle::COrientedParticle(void)
{
	mRefEnt.reType = RT_ORIENTED_QUAD;
}

//----------------------------
bool COrientedParticle::Cull(void)
{
	vec3_t	dir;
//	float	len;

	// Get the direction to the view
	VectorSubtract( mOrigin1, theFxHelper.refdef->vieworg, dir );

	// Check if it's behind the viewer
	if ( (DotProduct( theFxHelper.refdef->viewaxis[0], dir )) < 0 )
	{
		return true;
	}

//	len = VectorLengthSquared( dir );

	// don't cull stuff that's associated with inview wpns
	if ( mFlags & FX_DEPTH_HACK )
	{
		return false;
	}
	// Can't be too close
//	if ( len < fx_nearCull->value * fx_nearCull->value)
//	{
//		return true;
//	}

	return false;
}

//----------------------------
void COrientedParticle::Draw(void)
{
	if ( mFlags & FX_DEPTH_HACK )
	{
		// Not sure if first person needs to be set
		mRefEnt.renderfx |= RF_DEPTHHACK;
	}

	// Add our refEntity to the scene
	VectorCopy( mOrigin1, mRefEnt.origin );
	if ( !(mFlags&FX_RELATIVE) )
	{
		VectorCopy( mNormal, mRefEnt.axis[0] );
		MakeNormalVectors( mRefEnt.axis[0], mRefEnt.axis[1], mRefEnt.axis[2] );
	}

	theFxHelper.AddFxToScene( &mRefEnt );
	drawnFx++;	
}

//----------------------------
// Update
//----------------------------
bool COrientedParticle::Update(void)
{
	// Game pausing can cause dumb time things to happen, so kill the effect in this instance
	if ( mTimeStart > theFxHelper.mTime )
	{
		return false;
	}
		
	if ( mFlags & FX_RELATIVE )
	{
		if ( !mGhoul2.IsValid())
		{	// the thing we are bolted to is no longer valid, so we may as well just die.
			return false;
		}

		vec3_t	org;
		vec3_t	ax[3];

		// Get our current position and direction
		if (!theFxHelper.GetOriginAxisFromBolt(&mGhoul2, mEntNum, mModelNum, mBoltNum, org, ax))
		{	//could not get bolt
			return false;
		}
		vec3_t 	realVel, realAccel;

		VectorMA( org, mOrgOffset[0], ax[0], org );
		VectorMA( org, mOrgOffset[1], ax[1], org );
		VectorMA( org, mOrgOffset[2], ax[2], org );

		const float	time = ((theFxHelper.mTime - mTimeStart) + theFxHelper.mTimeFraction) * 0.001f;
		// calc the real velocity and accel vectors
		VectorScale( ax[0], mVel[0], realVel );
		VectorMA( realVel, mVel[1], ax[1], realVel );
		VectorMA( realVel, mVel[2], ax[2], realVel );
//		realVel[2] += 0.5f * mGravity * time;

		VectorScale( ax[0], mAccel[0], realAccel );
		VectorMA( realAccel, mAccel[1], ax[1], realAccel );
		VectorMA( realAccel, mAccel[2], ax[2], realAccel );

		// Get our real velocity at the current time, taking into account the effects of acceleartion.  NOTE: not sure if this is even 100% correct math-wise
		VectorMA( realVel, time, realAccel, realVel );

		// Now move us to where we should be at the given time
		VectorMA( org, time, realVel, mOrigin1 );

		//use the normalOffset and add that to the actual normal of the bolt
		//NOTE: not tested!!!
		VectorCopy( ax[0], mRefEnt.axis[0] );
		VectorCopy( ax[1], mRefEnt.axis[1] );
		VectorCopy( ax[2], mRefEnt.axis[2] );

		//vec3_t	offsetAngles;
		//VectorSet( offsetAngles, 0, 90, 90 );

		vec3_t	offsetAxis[3];
		//NOTE: mNormal is actually PITCH YAW and ROLL offsets
		AnglesToAxis( mNormal, offsetAxis );
		MatrixMultiply( offsetAxis, ax, mRefEnt.axis );
	}
	else if (( mTimeStart < theFxHelper.mTime ) && UpdateOrigin() == false )
	{
		// we are marked for death
		return false;
	}

	if ( !Cull() )
	{	// Only update these if the thing is visible.	
		UpdateSize();
		UpdateRGB();
		UpdateAlpha();
		UpdateRotation();

		Draw();
	}

	return true;
}


//----------------------------
//
// Derived Line Class
//
//----------------------------
CLine::CLine(void)
{
	mRefEnt.reType = RT_LINE;
}

//----------------------------
bool CLine::Cull( void )
{
	vec3_t	dir;

	VectorSubtract( mOrigin1, theFxHelper.refdef->vieworg, dir );

	//Check if it's in front of the viewer
	if ( (DotProduct( theFxHelper.refdef->viewaxis[0], dir )) >= 0 )
	{
		return false;	//don't cull
	}

	VectorSubtract( mOrigin2, theFxHelper.refdef->vieworg, dir );

	//Check if it's in front of the viewer
	if ( (DotProduct( theFxHelper.refdef->viewaxis[0], dir )) >= 0 )
	{
		return false;
	}

	return true; //all points behind viewer
}

//----------------------------
void CLine::Draw(void)
{
	if ( mFlags & FX_DEPTH_HACK )
	{
		// Not sure if first person needs to be set, but it can't hurt?
		mRefEnt.renderfx |= RF_DEPTHHACK;
	}

	VectorCopy( mOrigin1, mRefEnt.origin );
	VectorCopy( mOrigin2, mRefEnt.oldorigin );

	theFxHelper.AddFxToScene(&mRefEnt);
	drawnFx++;	
}

//----------------------------
bool CLine::Update(void)
{
	// Game pausing can cause dumb time things to happen, so kill the effect in this instance
	if ( mTimeStart > theFxHelper.mTime )
	{
		return false;
	}

	if ( mFlags & FX_RELATIVE )
	{
		if ( !mGhoul2.IsValid())
		{	// the thing we are bolted to is no longer valid, so we may as well just die.
			return false;
		}
		vec3_t	ax[3];
		// Get our current position and direction
		if (!theFxHelper.GetOriginAxisFromBolt(&mGhoul2, mEntNum, mModelNum, mBoltNum, mOrigin1, ax))
		{	//could not get bolt
			return false;
		}

		VectorAdd(mOrigin1, mOrgOffset, mOrigin1);	//add the offset to the bolt point
		
		VectorMA( mOrigin1, mVel[0], ax[0], mOrigin2 );
		VectorMA( mOrigin2, mVel[1], ax[1], mOrigin2 );
		VectorMA( mOrigin2, mVel[2], ax[2], mOrigin2 );
	}

	if ( !Cull())
	{	
		// Only update these if the thing is visible.	
		UpdateSize();
		UpdateRGB();
		UpdateAlpha();

		Draw();
	}

	return true;
}

//----------------------------
//
// Derived Electricity Class
//
//----------------------------
CElectricity::CElectricity(void)
{
	mRefEnt.reType = RT_ELECTRICITY;
}

//----------------------------
void CElectricity::Initialize(void)
{
	mRefEnt.frame = random() * 1265536.0f;
	mRefEnt.axis[0][2] = theFxHelper.mTime + (mTimeEnd - mTimeStart); // endtime

	if ( mFlags & FX_DEPTH_HACK )
	{
		mRefEnt.renderfx |= RF_DEPTHHACK;
	}

	if ( mFlags & FX_BRANCH )
	{
		mRefEnt.renderfx |= RF_FORKED;	
	}

	if ( mFlags & FX_TAPER )
	{
		mRefEnt.renderfx |= RF_TAPERED;
	}

	if ( mFlags & FX_GROW )
	{
		mRefEnt.renderfx |= RF_GROW;
	}
}

//----------------------------
void CElectricity::Draw(void)
{
	VectorCopy( mOrigin1, mRefEnt.origin );
	VectorCopy( mOrigin2, mRefEnt.oldorigin );
	mRefEnt.axis[0][0] = mChaos;
	mRefEnt.axis[0][1] = mTimeEnd - mTimeStart;
	mRefEnt.axis[1][0] = theFxHelper.mTimeFraction;

	theFxHelper.AddFxToScene( &mRefEnt );
	drawnFx++;	
}

//----------------------------
bool CElectricity::Update(void)
{
	// Game pausing can cause dumb time things to happen, so kill the effect in this instance
	if ( mTimeStart > theFxHelper.mTime )
	{
		return false;
	}

	if ( mFlags & FX_RELATIVE )
	{
		if ( !mGhoul2.IsValid())
		{	// the thing we are bolted to is no longer valid, so we may as well just die.
			return false;
		}
		vec3_t	ax[3];
		// Get our current position and direction
		if (!theFxHelper.GetOriginAxisFromBolt(&mGhoul2, mEntNum, mModelNum, mBoltNum, mOrigin1, ax))
		{	//could not get bolt
			return false;
		}

		VectorAdd(mOrigin1, mOrgOffset, mOrigin1);	//add the offset to the bolt point
		
		VectorMA( mOrigin1, mVel[0], ax[0], mOrigin2 );
		VectorMA( mOrigin2, mVel[1], ax[1], mOrigin2 );
		VectorMA( mOrigin2, mVel[2], ax[2], mOrigin2 );
	}

	if ( !Cull())
	{	
		// Only update these if the thing is visible.	
		UpdateSize();
		UpdateRGB();
		UpdateAlpha();
		
		Draw();
	}

	return true;
}

//----------------------------
//
// Derived Tail Class
//
//----------------------------
CTail::CTail(void)
{
	mRefEnt.reType = RT_LINE;
}

//----------------------------
void CTail::Draw(void)
{
	if ( mFlags & FX_DEPTH_HACK )
	{
		// Not sure if first person needs to be set
		mRefEnt.renderfx |= RF_DEPTHHACK;
	}

	VectorCopy( mOrigin1, mRefEnt.origin );

	theFxHelper.AddFxToScene(&mRefEnt);
	drawnFx++;	
}

//----------------------------
bool CTail::Update(void)
{
	// Game pausing can cause dumb time things to happen, so kill the effect in this instance
	if ( mTimeStart > theFxHelper.mTime )
	{
		return false;
	}
		
	if ( mFlags & FX_RELATIVE )
	{
		if ( !mGhoul2.IsValid())
		{	// the thing we are bolted to is no longer valid, so we may as well just die.
			return false;
		}
		// Get our current position and direction

		vec3_t	org;
		vec3_t	ax[3];
		if (mModelNum>=0 && mBoltNum>=0)	//bolt style
		{
			if (!theFxHelper.GetOriginAxisFromBolt(&mGhoul2, mEntNum, mModelNum, mBoltNum, org, ax))
			{	//could not get bolt
				return false;
			}
		}

		vec3_t 	realVel, realAccel;

		VectorMA( org, mOrgOffset[0], ax[0], org );
		VectorMA( org, mOrgOffset[1], ax[1], org );
		VectorMA( org, mOrgOffset[2], ax[2], org );

		// calc the real velocity and accel vectors
		// FIXME: if you want right and up movement in addition to the forward movement, you'll have to convert dir into a set of perp. axes and do some extra work
		VectorScale( ax[0], mVel[0], realVel );
		VectorMA( realVel, mVel[1], ax[1], realVel );
		VectorMA( realVel, mVel[2], ax[2], realVel );

		VectorScale( ax[0], mAccel[0], realAccel );
		VectorMA( realAccel, mAccel[1], ax[1], realAccel );
		VectorMA( realAccel, mAccel[2], ax[2], realAccel );

		const float	time = ((theFxHelper.mTime - mTimeStart) + theFxHelper.mTimeFraction) * 0.001f;

		// Get our real velocity at the current time, taking into account the effects of acceleration.  NOTE: not sure if this is even 100% correct math-wise
		VectorMA( realVel, time, realAccel, realVel );

		// Now move us to where we should be at the given time
		VectorMA( org, time, realVel, mOrigin1 );

		// Just calc an old point some time in the past, doesn't really matter when
		VectorMA( org, (time - 0.003f), realVel, mOldOrigin );
	}
#ifdef _DEBUG
	else if ( !fx_freeze->integer )
#else
	else
#endif
	{
		VectorCopy( mOrigin1, mOldOrigin );
	}

	if (( mTimeStart < theFxHelper.mTime ) && UpdateOrigin() == false )
	{
		// we are marked for death
		return false;
	}

	if ( !Cull() )
	{	
		// Only update these if the thing is visible.	
		UpdateSize();
		UpdateLength();
		UpdateRGB();
		UpdateAlpha();

		CalcNewEndpoint();
		Draw();
	}

	return true;
}

//----------------------------
void CTail::UpdateLength(void)
{
	// completely biased towards start if it doesn't get overridden
	float	perc1 = 1.0f, perc2 = 1.0f; 

	if ( mFlags & FX_LENGTH_LINEAR )
	{ 
		// calculate element biasing
		perc1 = 1.0f - ((float)(theFxHelper.mTime - mTimeStart) + theFxHelper.mTimeFraction) / (float)(mTimeEnd - mTimeStart);
		if (perc1 < 0)
			perc1 = 0;
		else if (perc1 > 1)
			perc1 = 1;
	}

	// We can combine FX_LINEAR with _either_ FX_NONLINEAR or FX_WAVE
	if (( mFlags & FX_LENGTH_PARM_MASK ) == FX_LENGTH_NONLINEAR )
	{
		if ((double)((double)theFxHelper.mTime + (double)theFxHelper.mTimeFraction) > (double)mLengthParm)
		{ 
			// get percent done, using parm as the start of the non-linear fade
			perc2 = 1.0f - ((float)((float)theFxHelper.mTime - mLengthParm) + theFxHelper.mTimeFraction) / (float)(mTimeEnd - mLengthParm);
			if (perc2 < 0)
				perc2 = 0;
			else if (perc2 > 1)
				perc2 = 1;
		}

		if ( mFlags & FX_LENGTH_LINEAR )
		{ 
			// do an even blend
			perc1 = perc1 * 0.5f + perc2 * 0.5f;
		}
		else
		{ 
			// just copy it over...sigh
			perc1 = perc2;
		}
	}
	else if (( mFlags & FX_LENGTH_PARM_MASK ) == FX_LENGTH_WAVE )
	{ 
		// wave gen, with parm being the frequency multiplier
		perc1 = perc1 * cosf(((float)(theFxHelper.mTime - mTimeStart) + theFxHelper.mTimeFraction) * mLengthParm);
	}
	else if (( mFlags & FX_LENGTH_PARM_MASK ) == FX_LENGTH_CLAMP )
	{
		if ((double)((double)theFxHelper.mTime + (double)theFxHelper.mTimeFraction) < (double)mLengthParm)
		{ 
			// get percent done, using parm as the start of the non-linear fade
			perc2 = ((float)(mLengthParm - (float)theFxHelper.mTime) - theFxHelper.mTimeFraction) / (float)(mLengthParm - mTimeStart);
			if (perc2 < 0)
				perc2 = 0;
			else if (perc2 > 1)
				perc2 = 1;
		}
		else
		{
			perc2 = 0.0f; // make it full size??
		}

		if ( mFlags & FX_LENGTH_LINEAR )
		{ 
			// do an even blend
			perc1 = perc1 * 0.5f + perc2 * 0.5f;
		}
		else
		{ 
			// just copy it over...sigh
			perc1 = perc2;
		}
	}

	// If needed, RAND can coexist with linear and either non-linear or wave.
	if ( mFlags & FX_LENGTH_RAND )
	{ 
		// Random simply modulates the existing value
		perc1 = random() * perc1;
	}

	mLength = (double)(((double)mLengthStart * (double)perc1) + ((double)mLengthEnd * (double)(1.0f - perc1)));
}


//----------------------------
void CTail::CalcNewEndpoint(void)
{
	vec3_t temp;

	// FIXME:  Hmmm, this looks dumb when physics are on and a bounce happens
//	VectorSubtract( mOldOrigin, mOrigin1, temp );
	//if old and new origins are too close to each other then direction is counted wrong...
	//let's be based on velocity
	VectorCopy( mVel, temp );

	// I wish we didn't have to do a VectorNormalize every frame..... but we have to, because direction could be changed :s
	VectorNormalize( temp );

	VectorMA( mOrigin1, -mLength, temp, mRefEnt.oldorigin );
}


//----------------------------
//
// Derived Cylinder Class
//
//----------------------------
CCylinder::CCylinder(void)
{
	mRefEnt.reType = RT_CYLINDER;
	mTraceEnd = qfalse;
}

bool CCylinder::Cull(void)
{
	if (mTraceEnd)
	{ //eh, don't cull variable-length cylinders
		return false;
	}

	return CTail::Cull();
}

void CCylinder::UpdateLength(void)
{
	if (mTraceEnd)
	{
		vec3_t temp;
		trace_t tr;

		VectorMA( mOrigin1, FX_MAX_TRACE_DIST, mRefEnt.axis[0], temp );
		theFxHelper.Trace( tr, mOrigin1, NULL, NULL, temp, -1, MASK_SOLID );
		VectorSubtract( tr.endpos, mOrigin1, temp );
		mLength = VectorLength(temp);
	}
	else
	{
		CTail::UpdateLength();
	}
}

//----------------------------
void CCylinder::Draw(void)
{
	if ( mFlags & FX_DEPTH_HACK )
	{
		// Not sure if first person needs to be set, but it can't hurt?
		mRefEnt.renderfx |= RF_DEPTHHACK;
	}

	VectorCopy( mOrigin1, mRefEnt.origin );
	VectorMA( mOrigin1, mLength, mRefEnt.axis[0], mRefEnt.oldorigin );

	theFxHelper.AddFxToScene(&mRefEnt);
	drawnFx++;	
}

//----------------------------
// Update Size2
//----------------------------
void CCylinder::UpdateSize2(void)
{
	// completely biased towards start if it doesn't get overridden
	float	perc1 = 1.0f, perc2 = 1.0f; 

	if ( mFlags & FX_SIZE2_LINEAR )
	{ 
		// calculate element biasing
		perc1 = 1.0f - ((float)(theFxHelper.mTime - mTimeStart) + theFxHelper.mTimeFraction) / (float)(mTimeEnd - mTimeStart);
		if (perc1 < 0)
			perc1 = 0;
		else if (perc1 > 1)
			perc1 = 1;
	}

	// We can combine FX_LINEAR with _either_ FX_NONLINEAR or FX_WAVE
	if (( mFlags & FX_SIZE2_PARM_MASK ) == FX_SIZE2_NONLINEAR )
	{
		if ((double)((double)theFxHelper.mTime + (double)theFxHelper.mTimeFraction) > (double)mSize2Parm)
		{ 
			// get percent done, using parm as the start of the non-linear fade
			perc2 = 1.0f - ((float)((float)theFxHelper.mTime - mSize2Parm) + theFxHelper.mTimeFraction) / (float)(mTimeEnd - mSize2Parm);
			if (perc2 < 0)
				perc2 = 0;
			else if (perc2 > 1)
				perc2 = 1;
		}

		if ( (mFlags & FX_SIZE2_LINEAR) )
		{ 
			// do an even blend
			perc1 = perc1 * 0.5f + perc2 * 0.5f;
		}
		else
		{ 
			// just copy it over...sigh
			perc1 = perc2;
		}
	}
	else if (( mFlags & FX_SIZE2_PARM_MASK ) == FX_SIZE2_WAVE )
	{ 
		// wave gen, with parm being the frequency multiplier
		perc1 = perc1 * cosf(((float)(theFxHelper.mTime - mTimeStart) + theFxHelper.mTimeFraction) * mSize2Parm);
	}
	else if (( mFlags & FX_SIZE2_PARM_MASK ) == FX_SIZE2_CLAMP )
	{
		if ((double)((double)theFxHelper.mTime + (double)theFxHelper.mTimeFraction) < (double)mSize2Parm)
		{ 
			// get percent done, using parm as the start of the non-linear fade
			perc2 = ((float)(mSize2Parm - (float)theFxHelper.mTime) - theFxHelper.mTimeFraction) / (float)(mSize2Parm - mTimeStart);
			if (perc2 < 0)
				perc2 = 0;
			else if (perc2 > 1)
				perc2 = 1;
		}
		else
		{
			perc2 = 0.0f; // make it full size??
		}

		if ( mFlags & FX_SIZE2_LINEAR )
		{ 
			// do an even blend
			perc1 = perc1 * 0.5f + perc2 * 0.5f;
		}
		else
		{ 
			// just copy it over...sigh
			perc1 = perc2;
		}
	}

	// If needed, RAND can coexist with linear and either non-linear or wave.
	if ( mFlags & FX_SIZE2_RAND )
	{ 
		// Random simply modulates the existing value
		perc1 = random() * perc1;
	}

	mRefEnt.rotation = (double)(((double)mSize2Start * (double)perc1) + ((double)mSize2End * (double)(1.0f - perc1)));
}

//----------------------------
bool CCylinder::Update(void)
{
	// Game pausing can cause dumb time things to happen, so kill the effect in this instance
	if ( mTimeStart > theFxHelper.mTime )
	{
		return false;
	}

	if ( mFlags & FX_RELATIVE )
	{
		if ( !mGhoul2.IsValid())
		{	// the thing we are bolted to is no longer valid, so we may as well just die.
			return false;
		}

		vec3_t	ax[3];
		// Get our current position and direction
		if (!theFxHelper.GetOriginAxisFromBolt(&mGhoul2, mEntNum, mModelNum, mBoltNum, mOrigin1, ax))
		{	//could not get bolt
			return false;
		}

		VectorAdd(mOrigin1, mOrgOffset, mOrigin1);	//add the offset to the bolt point
		
		VectorCopy( ax[0], mRefEnt.axis[0] );
		//FIXME: should mNormal be a modifier on the forward axis?
		/*
		VectorMA( mOrigin1, mNormal[0], ax[0], mOrigin2 );
		VectorMA( mOrigin2, mNormal[1], ax[1], mOrigin2 );
		VectorMA( mOrigin2, mNormal[2], ax[2], mOrigin2 );
		*/
	}
		
	if ( !Cull() )
	{	
		// Only update these if the thing is visible.	
		UpdateSize();
		UpdateSize2();
		UpdateLength();
		UpdateRGB();
		UpdateAlpha();
		
		Draw();
	}

	return true;
}


//----------------------------
//
// Derived Emitter Class
//
//----------------------------
CEmitter::CEmitter(void)
{
	// There may or may not be a model, but if there isn't one, 
	//	we just won't bother adding the refEnt in our Draw func
	mRefEnt.reType = RT_MODEL;
}

//----------------------------
CEmitter::~CEmitter(void)
{
}

//----------------------------
// Draw
//----------------------------
void CEmitter::Draw(void)
{
	// Emitters don't draw themselves, but they may need to add an attached model
	if ( mFlags & FX_ATTACHED_MODEL )
	{
		mRefEnt.nonNormalizedAxes = qtrue;

		VectorCopy( mOrigin1, mRefEnt.origin );

	 	VectorScale( mRefEnt.axis[0], mRefEnt.radius, mRefEnt.axis[0] );
	 	VectorScale( mRefEnt.axis[1], mRefEnt.radius, mRefEnt.axis[1] );
	 	VectorScale( mRefEnt.axis[2], mRefEnt.radius, mRefEnt.axis[2] );

		theFxHelper.AddFxToScene((miniRefEntity_t*)0);// I hate having to do this, but this needs to get added as a regular refEntity
		theFxHelper.AddFxToScene(&mRefEnt);
	}

	// If we are emitting effects, we had better be careful because just calling it every cgame frame could
	//	either choke up the effects system on a fast machine, or look really nasty on a low end one.
	if ( mFlags & FX_EMIT_FX )
	{
		vec3_t	org, v;
		float	ftime, time2, step;
		int		t, dif;

#define TRAIL_RATE		12 // we "think" at about a 60hz rate

		// Pick a target step distance and square it
		step = mDensity + crandom() * mVariance;
		step *= step;

		dif = 0;

		for ( t = mOldTime; t <= theFxHelper.mTime; t += TRAIL_RATE ) 
		{
			dif += TRAIL_RATE;

			// ?Not sure if it's better to update this before or after updating the origin
			VectorMA( mOldVelocity, dif * 0.001f, mAccel, v );

			// Calc the time differences
			ftime = dif * 0.001f;
			time2 = ftime * ftime * 0.5f;

			// Predict the new position
			org[0] = mOldOrigin[0] + (ftime * v[0]) + (time2 * v[0]);
			org[1] = mOldOrigin[1] + (ftime * v[1]) + (time2 * v[1]);
			org[2] = mOldOrigin[2] + (ftime * v[2]) + (time2 * v[2]);
	
			// Is it time to draw an effect?
			if ( DistanceSquared( org, mOldOrigin ) >= step )
			{ 
				// Pick a new target step distance and square it
				step = mDensity + crandom() * mVariance;
				step *= step;

				// We met the step criteria so, we should add in the effect
				theFxScheduler.PlayEffect( mEmitterFxID, org, mRefEnt.axis );

				VectorCopy( org, mOldOrigin );
				VectorCopy( v, mOldVelocity );
				dif = 0;
				mOldTime = t;
			}
		}
	}
	drawnFx++;	
}

//----------------------------
bool CEmitter::Update(void)
{
	// Game pausing can cause dumb time things to happen, so kill the effect in this instance
	if ( mTimeStart > theFxHelper.mTime )
	{
		return false;
	}
		
	// Use this to track if we've stopped moving
	VectorCopy( mOrigin1, mOldOrigin );
	VectorCopy( mVel, mOldVelocity );

	if ( mFlags & FX_RELATIVE )
	{
		if ( !mGhoul2.IsValid())
		{	// the thing we are bolted to is no longer valid, so we may as well just die.
			return false;
		}
		assert(0);//need this?
		
	}
	if (( mTimeStart < theFxHelper.mTime ) && UpdateOrigin() == false )
	{
		// we are marked for death
		return false;
	}

	bool moving = false;

	// If the thing is no longer moving, kill the angle delta, but don't do it too quickly or it will
	//	look very artificial.  Don't do it too slowly or it will look like there is no friction.
/*	if ( VectorCompare( mOldOrigin, mOrigin1 ))
	{
		VectorScale( mAngleDelta, 0.7f, mAngleDelta );
	}
	else
	{*/
		moving = true;
	//}

	if ( mFlags & FX_PAPER_PHYSICS )
	{
		// do this in a more framerate independant manner
		float sc;
		if (theFxHelper.mFrameTime)
			sc = ( 20.0f / theFxHelper.mFrameTime);
		else
			sc = 1.0f;

		// bah, evil clamping
		if ( sc > 1.0f )
		{
			sc = 1.0f;
		}

		if ( moving )
		{
			// scale the velocity down, basically inducing drag.  Acceleration ( gravity ) should keep it pulling down, which is what we want.
			VectorScale( mVel, (sc * 0.8f + 0.2f ) * 0.92f, mVel );

			// add some chaotic motion based on the way we are oriented
			VectorMA( mVel, (1.5f - sc) * 4.0f, mRefEnt.axis[0], mVel );
			VectorMA( mVel, (1.5f - sc) * 4.0f, mRefEnt.axis[1], mVel );
		}

		// make us settle so we can lay flat
		mAngles[0] *= (0.97f * (sc * 0.4f + 0.6f ));
		mAngles[2] *= (0.97f * (sc * 0.4f + 0.6f ));

		// decay our angle delta so we don't rotate as quickly
		VectorScale( mAngleDelta, (0.96f * (sc * 0.1f + 0.9f )), mAngleDelta );
	}

	UpdateAngles();
	UpdateSize();

	Draw();

	return true;
}

//----------------------------
void CEmitter::UpdateAngles(void)
{
	VectorMA(mAngles, theFxHelper.mFrameTime * 0.01f, mAngleDelta, mAngles); // was 0.001f, but then you really have to jack up the delta to even notice anything
	AnglesToAxis(mAngles, mRefEnt.axis);
}


//--------------------------
//
// Derived Light Class
//
//--------------------------

//----------------------------
void CLight::Draw(void)
{
#ifdef VV_LIGHTING
	VVLightMan.RE_AddLightToScene( mOrigin1, mRefEnt.radius, mRefEnt.origin[0], mRefEnt.origin[1], mRefEnt.origin[2] );
#else
	theFxHelper.AddLightToScene( mOrigin1, mRefEnt.radius, mRefEnt.origin[0], mRefEnt.origin[1], mRefEnt.origin[2] );
#endif
	drawnFx++;	
}

//----------------------------
// Update
//----------------------------
bool CLight::Update(void)
{
	// Game pausing can cause dumb time things to happen, so kill the effect in this instance
	if ( mTimeStart > theFxHelper.mTime )
	{
		return false;
	}

	if ( mFlags & FX_RELATIVE )
	{
		if ( !mGhoul2.IsValid())
		{	// the thing we are bolted to is no longer valid, so we may as well just die.
			return false;
		}

		vec3_t	ax[3];
		// Get our current position and direction
		if (!theFxHelper.GetOriginAxisFromBolt(&mGhoul2, mEntNum, mModelNum, mBoltNum, mOrigin1, ax))
		{	//could not get bolt
			return false;
		}
		VectorMA( mOrigin1, mOrgOffset[0], ax[0], mOrigin1 );
		VectorMA( mOrigin1, mOrgOffset[1], ax[1], mOrigin1 );
		VectorMA( mOrigin1, mOrgOffset[2], ax[2], mOrigin1 );
	}

	UpdateSize();
	UpdateRGB();
	
	Draw();

	return true;
}

//----------------------------
// Update Size
//----------------------------
void CLight::UpdateSize(void)
{
	// completely biased towards start if it doesn't get overridden
	float	perc1 = 1.0f, perc2 = 1.0f; 

	if ( mFlags & FX_SIZE_LINEAR )
	{ 
		// calculate element biasing
		perc1 = 1.0f - ((float)(theFxHelper.mTime - mTimeStart) + theFxHelper.mTimeFraction) / (float)(mTimeEnd - mTimeStart);
		if (perc1 < 0)
			perc1 = 0;
		else if (perc1 > 1)
			perc1 = 1;
	}

	// We can combine FX_LINEAR with _either_ FX_NONLINEAR or FX_WAVE
	if (( mFlags & FX_SIZE_PARM_MASK ) == FX_SIZE_NONLINEAR )
	{
		if ((double)((double)theFxHelper.mTime + (double)theFxHelper.mTimeFraction) > (double)mSizeParm)
		{ 
			// get percent done, using parm as the start of the non-linear fade
			perc2 = 1.0f - ((float)((float)theFxHelper.mTime - mSizeParm) + theFxHelper.mTimeFraction) / (float)(mTimeEnd - mSizeParm);
			if (perc2 < 0)
				perc2 = 0;
			else if (perc2 > 1)
				perc2 = 1;
		}

		if ( (mFlags & FX_SIZE_LINEAR) )
		{ 
			// do an even blend
			perc1 = perc1 * 0.5f + perc2 * 0.5f;
		}
		else
		{ 
			// just copy it over...sigh
			perc1 = perc2;
		}
	}
	else if (( mFlags & FX_SIZE_PARM_MASK ) == FX_SIZE_WAVE )
	{ 
		// wave gen, with parm being the frequency multiplier
		perc1 = perc1 * cosf(((float)(theFxHelper.mTime - mTimeStart) + theFxHelper.mTimeFraction) * mSizeParm);
	}
	else if (( mFlags & FX_SIZE_PARM_MASK ) == FX_SIZE_CLAMP )
	{
		if ((double)((double)theFxHelper.mTime + (double)theFxHelper.mTimeFraction) < (double)mSizeParm)
		{ 
			// get percent done, using parm as the start of the non-linear fade
			perc2 = ((float)(mSizeParm - (float)theFxHelper.mTime) - theFxHelper.mTimeFraction) / (float)(mSizeParm - mTimeStart);
			if (perc2 < 0)
				perc2 = 0;
			else if (perc2 > 1)
				perc2 = 1;
		}
		else
		{
			perc2 = 0.0f; // make it full size??
		}

		if ( mFlags & FX_SIZE_LINEAR )
		{ 
			// do an even blend
			perc1 = perc1 * 0.5f + perc2 * 0.5f;
		}
		else
		{ 
			// just copy it over...sigh
			perc1 = perc2;
		}
	}

	// If needed, RAND can coexist with linear and either non-linear or wave.
	if ( mFlags & FX_SIZE_RAND )
	{ 
		// Random simply modulates the existing value
		perc1 = random() * perc1;
	}

	mRefEnt.radius = (double)(((double)mSizeStart * (double)perc1) + ((double)mSizeEnd * (double)(1.0f - perc1)));
}

//----------------------------
// Update RGB
//----------------------------
void CLight::UpdateRGB(void)
{
	// completely biased towards start if it doesn't get overridden
	float	perc1 = 1.0f, perc2 = 1.0f; 
	vec3_t	res;

	if ( mFlags & FX_RGB_LINEAR )
	{ 
		// calculate element biasing
		perc1 = 1.0f - ((float)(theFxHelper.mTime - mTimeStart) + theFxHelper.mTimeFraction) / (float)(mTimeEnd - mTimeStart);
		if (perc1 < 0)
			perc1 = 0;
		else if (perc1 > 1)
			perc1 = 1;
	}

	// We can combine FX_LINEAR with _either_ FX_NONLINEAR or FX_WAVE
	if (( mFlags & FX_RGB_PARM_MASK ) == FX_RGB_NONLINEAR )
	{
		if ((double)((double)theFxHelper.mTime + (double)theFxHelper.mTimeFraction) > (double)mRGBParm)
		{ 
			// get percent done, using parm as the start of the non-linear fade
			perc2 = 1.0f - ((float)((float)theFxHelper.mTime - mRGBParm) + theFxHelper.mTimeFraction) / (float)(mTimeEnd - mRGBParm);
			if (perc2 < 0)
				perc2 = 0;
			else if (perc2 > 1)
				perc2 = 1;
		}

		if ( mFlags & FX_RGB_LINEAR )
		{ 
			// do an even blend
			perc1 = perc1 * 0.5f + perc2 * 0.5f;
		}
		else
		{ 
			// just copy it over...sigh
			perc1 = perc2;
		}
	}
	else if (( mFlags & FX_RGB_PARM_MASK ) == FX_RGB_WAVE )
	{ 
		// wave gen, with parm being the frequency multiplier
		perc1 = perc1 * cosf(((float)(theFxHelper.mTime - mTimeStart) + theFxHelper.mTimeFraction) * mRGBParm);
	}
	else if (( mFlags & FX_RGB_PARM_MASK ) == FX_RGB_CLAMP )
	{
		if ((double)((double)theFxHelper.mTime + (double)theFxHelper.mTimeFraction) < (double)mRGBParm)
		{ 
			// get percent done, using parm as the start of the non-linear fade
			perc2 = ((float)(mRGBParm - (float)theFxHelper.mTime) - theFxHelper.mTimeFraction) / (float)(mRGBParm - mTimeStart);
			if (perc2 < 0)
				perc2 = 0;
			else if (perc2 > 1)
				perc2 = 1;
		}
		else
		{
			perc2 = 0.0f; // make it full size??
		}

		if ( mFlags & FX_RGB_LINEAR )
		{ 
			// do an even blend
			perc1 = perc1 * 0.5f + perc2 * 0.5f;
		}
		else
		{ 
			// just copy it over...sigh
			perc1 = perc2;
		}
	}

	// If needed, RAND can coexist with linear and either non-linear or wave.
	if ( mFlags & FX_RGB_RAND )
	{ 
		// Random simply modulates the existing value
		perc1 = random() * perc1;
	}

	// Now get the correct color 
	VectorScale( mRGBStart, perc1, res );
	VectorMA(res, ( 1.0f - perc1 ), mRGBEnd, mRefEnt.origin);
}

//--------------------------
//
// Derived Trail Class
//
//--------------------------
#define NEW_MUZZLE	0
#define NEW_TIP		1
#define OLD_TIP		2
#define OLD_MUZZLE	3

//----------------------------
void CTrail::Draw()
{
	polyVert_t	verts[3];
//	vec3_t		color;

	// build the first tri out of the new muzzle...new tip...old muzzle
	VectorCopy( mVerts[NEW_MUZZLE].origin, verts[0].xyz );
	VectorCopy( mVerts[NEW_TIP].origin, verts[1].xyz );
	VectorCopy( mVerts[OLD_MUZZLE].origin, verts[2].xyz );

//	VectorScale( mVerts[NEW_MUZZLE].curRGB, mVerts[NEW_MUZZLE].curAlpha, color );
	verts[0].modulate[0] = mVerts[NEW_MUZZLE].rgb[0];
	verts[0].modulate[1] = mVerts[NEW_MUZZLE].rgb[1];
	verts[0].modulate[2] = mVerts[NEW_MUZZLE].rgb[2];
	verts[0].modulate[3] = mVerts[NEW_MUZZLE].alpha;

//	VectorScale( mVerts[NEW_TIP].curRGB, mVerts[NEW_TIP].curAlpha, color );
	verts[1].modulate[0] = mVerts[NEW_TIP].rgb[0];
	verts[1].modulate[1] = mVerts[NEW_TIP].rgb[1];
	verts[1].modulate[2] = mVerts[NEW_TIP].rgb[2];
	verts[1].modulate[3] = mVerts[NEW_TIP].alpha;

//	VectorScale( mVerts[OLD_MUZZLE].curRGB, mVerts[OLD_MUZZLE].curAlpha, color );
	verts[2].modulate[0] = mVerts[OLD_MUZZLE].rgb[0];
	verts[2].modulate[1] = mVerts[OLD_MUZZLE].rgb[1];
	verts[2].modulate[2] = mVerts[OLD_MUZZLE].rgb[2];
	verts[2].modulate[3] = mVerts[OLD_MUZZLE].alpha;

	verts[0].st[0] = mVerts[NEW_MUZZLE].curST[0];
	verts[0].st[1] = mVerts[NEW_MUZZLE].curST[1];
	verts[1].st[0] = mVerts[NEW_TIP].curST[0];
	verts[1].st[1] = mVerts[NEW_TIP].curST[1];
	verts[2].st[0] = mVerts[OLD_MUZZLE].curST[0];
	verts[2].st[1] = mVerts[OLD_MUZZLE].curST[1];

	// Add this tri
	theFxHelper.AddPolyToScene( mShader, 3, verts );

	// build the second tri out of the old muzzle...old tip...new tip
	VectorCopy( mVerts[OLD_MUZZLE].origin, verts[0].xyz );
	VectorCopy( mVerts[OLD_TIP].origin, verts[1].xyz );
	VectorCopy( mVerts[NEW_TIP].origin, verts[2].xyz );

//	VectorScale( mVerts[OLD_MUZZLE].curRGB, mVerts[OLD_MUZZLE].curAlpha, color );
	verts[0].modulate[0] = mVerts[OLD_MUZZLE].rgb[0];
	verts[0].modulate[1] = mVerts[OLD_MUZZLE].rgb[1];
	verts[0].modulate[2] = mVerts[OLD_MUZZLE].rgb[2];
	verts[0].modulate[3] = mVerts[OLD_MUZZLE].alpha;

//	VectorScale( mVerts[OLD_TIP].curRGB, mVerts[OLD_TIP].curAlpha, color );
	verts[1].modulate[0] = mVerts[OLD_TIP].rgb[0];
	verts[1].modulate[1] = mVerts[OLD_TIP].rgb[1];
	verts[1].modulate[2] = mVerts[OLD_TIP].rgb[2];
	verts[0].modulate[3] = mVerts[OLD_TIP].alpha;

//	VectorScale( mVerts[NEW_TIP].curRGB, mVerts[NEW_TIP].curAlpha, color );
	verts[2].modulate[0] = mVerts[NEW_TIP].rgb[0];
	verts[2].modulate[1] = mVerts[NEW_TIP].rgb[1];
	verts[2].modulate[2] = mVerts[NEW_TIP].rgb[2];
	verts[0].modulate[3] = mVerts[NEW_TIP].alpha;

	verts[0].st[0] = mVerts[OLD_MUZZLE].curST[0];
	verts[0].st[1] = mVerts[OLD_MUZZLE].curST[1];
	verts[1].st[0] = mVerts[OLD_TIP].curST[0];
	verts[1].st[1] = mVerts[OLD_TIP].curST[1];
	verts[2].st[0] = mVerts[NEW_TIP].curST[0];
	verts[2].st[1] = mVerts[NEW_TIP].curST[1];

	// Add this tri
	theFxHelper.AddPolyToScene( mShader, 3, verts );

	drawnFx++;
}

//----------------------------
// Update
//----------------------------
bool CTrail::Update()
{
	// Game pausing can cause dumb time things to happen, so kill the effect in this instance
	if ( mTimeStart > theFxHelper.mTime )
	{
		return false;
	}
	
	float perc = ((float)(mTimeEnd - theFxHelper.mTime) - theFxHelper.mTimeFraction) / (float)(mTimeEnd - mTimeStart);

	for ( int t = 0; t < 4; t++ )
	{
//		mVerts[t].curAlpha = mVerts[t].alpha * perc + mVerts[t].destAlpha * ( 1.0f - perc );
//		if ( mVerts[t].curAlpha < 0.0f )
//		{
//			mVerts[t].curAlpha = 0.0f;
//		}

//		VectorScale( mVerts[t].rgb, perc, mVerts[t].curRGB );
//		VectorMA( mVerts[t].curRGB, ( 1.0f - perc ), mVerts[t].destrgb, mVerts[t].curRGB );
		mVerts[t].curST[0] = mVerts[t].ST[0] * perc + mVerts[t].destST[0] * ( 1.0f - perc );
		if ( mVerts[t].curST[0] > 1.0f )
		{
			mVerts[t].curST[0] = 1.0f;
		}
		mVerts[t].curST[1] = mVerts[t].ST[1] * perc + mVerts[t].destST[1] * ( 1.0f - perc );
	}

	Draw();

	return true;
}

//--------------------------
//
// Derived Poly Class
//
//--------------------------

//----------------------------
bool CPoly::Cull(void)
{
	vec3_t	dir;

	// Get the direction to the view
	VectorSubtract( mOrigin1, theFxHelper.refdef->vieworg, dir );

	// Check if it's behind the viewer
	if ( (DotProduct( theFxHelper.refdef->viewaxis[0], dir )) < 0 )
	{
		return true;
	}

	float len = VectorLengthSquared( dir );

	// Can't be too close
	if ( len < fx_nearCull->value * fx_nearCull->value)
	{
		return true;
	}

	return false;
}

//----------------------------
void CPoly::Draw(void)
{
	polyVert_t	verts[MAX_CPOLY_VERTS];

	for ( int i = 0; i < mCount; i++ )
	{
		// Add our midpoint and vert offset to get the actual vertex
		VectorAdd( mOrigin1, mOrg[i], verts[i].xyz );

		// Assign the same color to each vert
		*(int *)verts[i].modulate = *(int *)mRefEnt.shaderRGBA;

		// Copy the ST coords
		Vector2Copy( mST[i], verts[i].st );
	}

	// Add this poly
	theFxHelper.AddPolyToScene( mRefEnt.customShader, mCount, verts );
	drawnFx++;	
}

//----------------------------
void CPoly::CalcRotateMatrix(void)
{
	float	cosX, cosZ;
	float	sinX, sinZ;
	float	rad;

	// rotate around Z
	rad = DEG2RAD(mRotDelta[YAW] * (theFxHelper.mFrameTime * 0.01f));
	cosZ = cosf(rad);
	sinZ = sinf(rad);
	// rotate around X
	rad = DEG2RAD(mRotDelta[PITCH] * (theFxHelper.mFrameTime * 0.01f));
	cosX = cosf(rad);
	sinX = sinf(rad);

/*Pitch - aroundx  Yaw - around z
1 0  0			 c -s 0
0 c -s			 s  c 0 
0 s  c			 0  0 1
*/
	mRot[0][0] = cosZ;
	mRot[1][0] = -sinZ;
	mRot[2][0] = 0;
	mRot[0][1] = cosX * sinZ;
	mRot[1][1] = cosX * cosZ;
	mRot[2][1] = -sinX;
	mRot[0][2] = sinX * sinZ;
	mRot[1][2] = sinX * cosZ;
	mRot[2][2] = cosX;
/*
// ROLL is not supported unless anyone complains, if it needs to be added, use this format
Roll

 c 0 s
 0 1 0 
-s 0 c
*/
	mLastFrameTime = theFxHelper.mFrameTime;
}

//--------------------------------
void CPoly::Rotate(void)
{
	vec3_t	temp[MAX_CPOLY_VERTS];
	float	dif = fabs(mLastFrameTime - theFxHelper.mFrameTime);
	
	if ( dif > 0.1f * mLastFrameTime )
	{
		CalcRotateMatrix();
	}

	// Multiply our rotation matrix by each of the offset verts to get their new position
	for ( int i = 0; i < mCount; i++ )
	{
		VectorRotate( mOrg[i], mRot, temp[i] );
		VectorCopy( temp[i], mOrg[i] );
	}
}

//----------------------------
// Update
//----------------------------
bool CPoly::Update(void)
{
	// Game pausing can cause dumb time things to happen, so kill the effect in this instance
	if ( mTimeStart > theFxHelper.mTime )
	{
		return false;
	}
		
	// If our timestamp hasn't exired yet, we won't even consider doing any kind of motion
	if ( theFxHelper.mTime > mTimeStamp )
	{
		vec3_t mOldOrigin;

		VectorCopy( mOrigin1, mOldOrigin );

		if (( mTimeStart < theFxHelper.mTime ) && UpdateOrigin() == false )
		{
			// we are marked for death
			return false;
		}

		// Only rotate whilst moving
		if ( !VectorCompare( mOldOrigin, mOrigin1 ))
		{
			Rotate();
		}
	}

	if ( !Cull())
	{	
		// Only update these if the thing is visible.	
		UpdateRGB();
		UpdateAlpha();
		
		Draw();
	}

	return true;
}

//----------------------------
void CPoly::PolyInit(void)
{
	if ( mCount < 3 )
	{
		return;
	}

	int		i;
	vec3_t	org = {0.0f, 0.0f ,0.0f};

	// Find our midpoint
	for ( i = 0; i < mCount; i++ )
	{
		VectorAdd( org, mOrg[i], org );
	}

	VectorScale( org, (float)(1.0f / mCount), org );

	// now store our midpoint for physics purposes
	VectorCopy( org, mOrigin1 );

	// Now we process the passed in points and make it so that they aren't actually the point...
	//	rather, they are the offset from mOrigin1.
	for ( i = 0; i < mCount; i++ )
	{
		VectorSubtract( mOrg[i], mOrigin1, mOrg[i] );
	}

	CalcRotateMatrix();
}

/*
-------------------------
CBezier

Bezier curve line
-------------------------
*/
bool CBezier::Cull( void )
{
	vec3_t	dir;

	VectorSubtract( mOrigin1, theFxHelper.refdef->vieworg, dir );

	//Check if it's in front of the viewer
	if ( (DotProduct( theFxHelper.refdef->viewaxis[0], dir )) >= 0 )
	{
		return false;	//don't cull
	}

	VectorSubtract( mOrigin2, theFxHelper.refdef->vieworg, dir );

	//Check if it's in front of the viewer
	if ( (DotProduct( theFxHelper.refdef->viewaxis[0], dir )) >= 0 )
	{
		return false;
	}

	VectorSubtract( mControl1, theFxHelper.refdef->vieworg, dir );

	//Check if it's in front of the viewer
	if ( (DotProduct( theFxHelper.refdef->viewaxis[0], dir )) >= 0 )
	{
		return false;
	}

	return true; //all points behind viewer
}

//----------------------------
bool CBezier::Update( void )
{
	float	ftime, time2;

	ftime = theFxHelper.mFrameTime * 0.001f;
	time2 = ftime * ftime * 0.5f;

	mControl1[0] = mControl1[0] + (ftime * mControl1Vel[0]) + (time2 * mControl1Vel[0]);
	mControl2[0] = mControl2[0] + (ftime * mControl2Vel[0]) + (time2 * mControl2Vel[0]);
	mControl1[1] = mControl1[1] + (ftime * mControl1Vel[1]) + (time2 * mControl1Vel[1]);
	mControl2[1] = mControl2[1] + (ftime * mControl2Vel[1]) + (time2 * mControl2Vel[1]);
	mControl1[2] = mControl1[2] + (ftime * mControl1Vel[2]) + (time2 * mControl1Vel[2]);
	mControl2[2] = mControl2[2] + (ftime * mControl2Vel[2]) + (time2 * mControl2Vel[2]);

	if ( Cull() == false )
	{	
		// Only update these if the thing is visible.	
		UpdateSize();
		UpdateRGB();
		UpdateAlpha();

		Draw();
	}
	return true;
}

//----------------------------
inline void CBezier::DrawSegment( vec3_t start, vec3_t end, float texcoord1, float texcoord2, float segPercent, float lastSegPercent )
{
	vec3_t			lineDir, cross, viewDir;
	static vec3_t	lastEnd[2];
	polyVert_t		verts[4];
	float			scaleBottom = 0.0f, scaleTop = 0.0f;

	VectorSubtract( end, start, lineDir );
	VectorSubtract( end, theFxHelper.refdef->vieworg, viewDir );
	CrossProduct( lineDir, viewDir, cross );
	VectorNormalize( cross );

	// scaleBottom is the width of the bottom edge of the quad, scaleTop is the width of the top edge
	scaleBottom = (mSizeStart + ((mSizeEnd - mSizeStart) * lastSegPercent)) * 0.5f;
	scaleTop = (mSizeStart + ((mSizeEnd - mSizeStart) * segPercent)) * 0.5f;

	//Construct the oriented quad
	if ( mInit )
	{
		VectorCopy( lastEnd[0], verts[0].xyz );
		VectorCopy( lastEnd[1], verts[1].xyz );
	}
	else
	{
		VectorMA( start, -scaleBottom, cross, verts[0].xyz );
		VectorMA( start, scaleBottom, cross, verts[1].xyz );
	}

	verts[0].st[0] = 0.0f;
	verts[0].st[1] = texcoord1;

	verts[0].modulate[0] = mRefEnt.shaderRGBA[0] * ( 1.0f - texcoord1 );
	verts[0].modulate[1] = mRefEnt.shaderRGBA[1] * ( 1.0f - texcoord1 );
	verts[0].modulate[2] = mRefEnt.shaderRGBA[2] * ( 1.0f - texcoord1 );
	verts[0].modulate[3] = mRefEnt.shaderRGBA[3];

	verts[1].st[0] = 1.0f;
	verts[1].st[1] = texcoord1;

	verts[1].modulate[0] = mRefEnt.shaderRGBA[0] * ( 1.0f - texcoord1 );
	verts[1].modulate[1] = mRefEnt.shaderRGBA[1] * ( 1.0f - texcoord1 );
	verts[1].modulate[2] = mRefEnt.shaderRGBA[2] * ( 1.0f - texcoord1 );
	verts[1].modulate[3] = mRefEnt.shaderRGBA[3];

	if ( texcoord1 == 0.0f )
	{
		*(int *)verts[0].modulate = 0;
		*(int *)verts[1].modulate = 0;
	}

	VectorMA( end, scaleTop, cross, verts[2].xyz );
	verts[2].st[0] = 1.0f;
	verts[2].st[1] = texcoord2;

	verts[2].modulate[0] = mRefEnt.shaderRGBA[0] * ( 1.0f - texcoord2 );
	verts[2].modulate[1] = mRefEnt.shaderRGBA[1] * ( 1.0f - texcoord2 );
	verts[2].modulate[2] = mRefEnt.shaderRGBA[2] * ( 1.0f - texcoord2 );
	verts[2].modulate[3] = mRefEnt.shaderRGBA[3];

	VectorMA( end, -scaleTop, cross, verts[3].xyz );
	verts[3].st[0] = 0.0f;
	verts[3].st[1] = texcoord2;

	verts[3].modulate[0] = mRefEnt.shaderRGBA[0] * ( 1.0f - texcoord2 );
	verts[3].modulate[1] = mRefEnt.shaderRGBA[1] * ( 1.0f - texcoord2 );
	verts[3].modulate[2] = mRefEnt.shaderRGBA[2] * ( 1.0f - texcoord2 );
	verts[3].modulate[3] = mRefEnt.shaderRGBA[3];

	theFxHelper.AddPolyToScene( mRefEnt.customShader, 4, verts );

	VectorCopy( verts[2].xyz, lastEnd[1] );
	VectorCopy( verts[3].xyz, lastEnd[0] );

	mInit = true;
}

const	float	BEZIER_RESOLUTION	= 16.0f;

//----------------------------
void CBezier::Draw( void )	
{
	vec3_t	pos, old_pos;
    float	mu, mum1;
	float	incr = 1.0f / BEZIER_RESOLUTION, tex = 1.0f, tc1, tc2;
	int		i = 0;

	VectorCopy( mOrigin1, old_pos );

	mInit = false;	//Signify a new batch for vert gluing

	float mum13, mu3, group1, group2;

	tc1 = 0.0f;

	for ( mu = incr; mu <= 1.0f; mu += incr)
	{
		//Four point curve
		mum1	= 1 - mu;
		mum13	= mum1 * mum1 * mum1;
		mu3		= mu * mu * mu;
		group1	= 3 * mu * mum1 * mum1;
		group2	= 3 * mu * mu *mum1;

		for ( i = 0; i < 3; i++ )
		{
			pos[i] = mum13 * mOrigin1[i] + group1 * mControl1[i] + group2 * mControl2[i] + mu3 * mOrigin2[i];
		}

		tc2 = mu * tex;

		//Draw it
		DrawSegment( old_pos, pos, tc1, tc2, mu, mu - incr );

		VectorCopy( pos, old_pos );
		tc1 = tc2;
	}
	drawnFx++;	
}

/*
-------------------------
CFlash

Full screen flash
-------------------------
*/

//----------------------------
bool CFlash::Update( void )
{
	if ( UpdateOrigin() == false )
	{
		// we are marked for death
		return false;
	}

	UpdateSize();
	mRefEnt.radius *= mRadiusModifier;
	UpdateRGB();
	UpdateAlpha();

	Draw();
	return true;
}

bool FX_WorldToScreen(vec3_t worldCoord, float *x, float *y)
{
	int	xcenter, ycenter;
	vec3_t	local, transformed;
	vec3_t	vfwd, vright, vup;

	//NOTE: did it this way because most draw functions expect virtual 640x480 coords
	//	and adjust them for current resolution
	xcenter = 640 / 2;//gives screen coords in virtual 640x480, to be adjusted when drawn
	ycenter = 480 / 2;//gives screen coords in virtual 640x480, to be adjusted when drawn

	VectorSubtract (worldCoord, theFxHelper.refdef->vieworg, local);

	AngleVectors (theFxHelper.refdef->viewangles, vfwd, vright, vup);

	transformed[0] = DotProduct(local,vright);
	transformed[1] = DotProduct(local,vup);
	transformed[2] = DotProduct(local,vfwd);		

	// Make sure Z is not negative.
	if(transformed[2] < 0.01)
	{
		return false;
	}
	// Simple convert to screen coords.
	float xzi = xcenter / transformed[2] * (90.0/theFxHelper.refdef->fov_x);
	float yzi = ycenter / transformed[2] * (90.0/theFxHelper.refdef->fov_y);

	*x = (xcenter + xzi * transformed[0]);
	*y = (ycenter - yzi * transformed[1]);

	return true;
}

//----------------------------
void CFlash::Init( void )
{
	// 10/19/01 kef -- maybe we want to do something different here for localized flashes, but right
	//now I want to be sure that whatever RGB changes occur to a non-localized flash will also occur
	//to a localized flash (so I'll have the same initial RGBA values for both...I need them sync'd for an effect)

	vec3_t	dif;
	float	mod = 1.0f, dis = 0.0f, maxRange = 900;

	VectorSubtract( mOrigin1, theFxHelper.refdef->vieworg, dif );
	dis = VectorNormalize( dif );

	mod = DotProduct( dif, theFxHelper.refdef->viewaxis[0] );

	if ( dis > maxRange || ( mod < 0.5f && dis > 100 ))
	{
		mod = 0.0f;
	}
	else if ( mod < 0.5f && dis <= 100 )
	{
		mod += 1.1f;
	}

	mod *= (1.0f - ((dis * dis) / (maxRange * maxRange)));

	VectorScale( mRGBStart, mod, mRGBStart );
	VectorScale( mRGBEnd, mod, mRGBEnd );

	if ( mFlags & FX_LOCALIZED_FLASH )
	{
		FX_WorldToScreen(mOrigin1, &mScreenX, &mScreenY);

		// modify size of localized flash based on distance to effect (but not orientation)
		if (dis > 100 && dis < maxRange)
		{
			mRadiusModifier = (1.0f - ((dis * dis) / (maxRange * maxRange)));
		}
	}
}

//----------------------------
void CFlash::Draw( void )	
{
    // Interestingly, if znear is set > than this, then the flash
    // doesn't appear at all.
    const float FLASH_DISTANCE_FROM_VIEWER = 12.0f;
	mRefEnt.reType = RT_SPRITE;

	if ( mFlags & FX_LOCALIZED_FLASH )
	{
		vec4_t	color;

		color[0] = mRefEnt.shaderRGBA[0] / 255.0;
		color[1] = mRefEnt.shaderRGBA[1] / 255.0;
		color[2] = mRefEnt.shaderRGBA[2] / 255.0;
		color[3] = mRefEnt.shaderRGBA[3] / 255.0;

		// add this 2D effect to the proper list. it will get drawn after the cgi.RenderScene call
		theFxScheduler.Add2DEffect(mScreenX, mScreenY, mRefEnt.radius, mRefEnt.radius, color, mRefEnt.customShader);
	}
	else
	{
		float stereoSep, stereoCoeff = 1.0f;
		VectorCopy( theFxHelper.refdef->vieworg, mRefEnt.origin );
		VectorMA( mRefEnt.origin, FLASH_DISTANCE_FROM_VIEWER, theFxHelper.refdef->viewaxis[0], mRefEnt.origin );

        // This is assuming that the screen is wider than it is tall.
		stereoSep = Cvar_VariableValue( "r_stereoSeparation" );
		if (stereoSep != 0)
			stereoCoeff = stereoSep / 0.017f; //not accurate but works
        mRefEnt.radius = FLASH_DISTANCE_FROM_VIEWER * tan (DEG2RAD (theFxHelper.refdef->fov_x * 0.5f)) * stereoCoeff;

		theFxHelper.AddFxToScene( &mRefEnt );
	}
	drawnFx++;	
}

void FX_AddPrimitive( CEffect **pEffect, int killTime );
void FX_FeedTrail(effectTrailArgStruct_t *a)
{
	CTrail *fx = new CTrail;
	int i = 0;

	while (i < 4)
	{
		VectorCopy(a->mVerts[i].origin, fx->mVerts[i].origin);
		VectorCopy(a->mVerts[i].rgb, fx->mVerts[i].rgb);
		VectorCopy(a->mVerts[i].destrgb, fx->mVerts[i].destrgb);
		VectorCopy(a->mVerts[i].curRGB, fx->mVerts[i].curRGB);
		fx->mVerts[i].alpha = a->mVerts[i].alpha;
		fx->mVerts[i].destAlpha = a->mVerts[i].destAlpha;
		fx->mVerts[i].curAlpha = a->mVerts[i].curAlpha;
		fx->mVerts[i].ST[0] = a->mVerts[i].ST[0];
		fx->mVerts[i].ST[1] = a->mVerts[i].ST[1];
		fx->mVerts[i].destST[0] = a->mVerts[i].destST[0];
		fx->mVerts[i].destST[1] = a->mVerts[i].destST[1];
		fx->mVerts[i].curST[0] = a->mVerts[i].curST[0];
		fx->mVerts[i].curST[1] = a->mVerts[i].curST[1];
		i++;
	}

	fx->SetFlags(a->mSetFlags);

	fx->mShader = a->mShader;

	FX_AddPrimitive((CEffect **)&fx, a->mKillTime);
}
// end
