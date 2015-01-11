#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"

// OCULUS BEGIN

idCVar vr_maxaimpitch("vr_maxaimpitch", "89", CVAR_GAME | CVAR_NETWORKSYNC | CVAR_FLOAT, "");
idCVar vr_aimdragangle("vr_maxaimpitch", "16", CVAR_GAME | CVAR_NETWORKSYNC | CVAR_INTEGER, "");

idCVar vr_laserSightWidth("vr_laserSightWidth", "0.2", CVAR_FLOAT | CVAR_ARCHIVE, "laser sight beam width");
idCVar vr_laserSightLength("vr_laserSightLength", "128", CVAR_FLOAT | CVAR_ARCHIVE, "laser sight beam length");

/*
================
idActor::GetDeltaAimAngles
================
*/
const idAngles &idActor::GetDeltaAimAngles( void ) const {
	return deltaAimAngles;
}

/*
================
idActor::SetDeltaAimAngles
================
*/
void idActor::SetDeltaAimAngles( const idAngles &delta ) {
	deltaAimAngles = delta;
}

/*
================
idPlayer::UpdateDeltaAimAngles
================
*/
void idPlayer::VR_UpdateDeltaAimAngles( const idAngles &angles ) {
	// set the delta angle
	idAngles delta;
	for( int i = 0; i < 3; i++ ) {
		delta[ i ] = angles[ i ] - SHORT2ANGLE( usercmd.angles[ i ] );
	}
	SetDeltaAimAngles( delta );
}

/*
================
idPlayer::UpdateDeltaViewAngles
================
*/
void idPlayer::VR_UpdateDeltaViewAngles( const idAngles &angles ) {
	// set the delta angle
	idAngles delta;
	for( int i = 0; i < 3; i++ ) {
		delta[ i ] = angles[ i ] - hmdAngles[ i ];
	}
	SetDeltaViewAngles( delta );
}

/*
================
idPlayer::UpdateViewAngles
================
*/
void idPlayer::VR_UpdateViewAngles_Type0( void )
{
	int i;
	idAngles delta;

	hmdAngles = oculus->GetHeadTrackingOrientation();

	if ( !noclip && ( gameLocal.inCinematic || privateCameraView || gameLocal.GetCamera() || influenceActive == INFLUENCE_LEVEL2 || objectiveSystemOpen ) )
	{
		// no view changes at all, but we still want to update the deltas or else when
		// we get out of this mode, our view will snap to a kind of random angle
		VR_UpdateDeltaViewAngles(viewAngles);
		VR_UpdateDeltaAimAngles(aimAngles);

		return;
	}

	// if dead
	if ( health <= 0 )
	{
		if ( pm_thirdPersonDeath.GetBool() )
		{
			viewAngles.roll = 0.0f;
			viewAngles.pitch = 30.0f;
		} else {
			viewAngles.roll = 40.0f;
			viewAngles.pitch = -15.0f;
		}
		return;
	}

	// circularly clamp the angles with deltas
	for ( i = 0; i < 3; i++ ) {
		cmdAngles[i] = SHORT2ANGLE( usercmd.angles[i] );

		if ( influenceActive == INFLUENCE_LEVEL3 ) {
			aimAngles[i] += idMath::ClampFloat(-1.0f, 1.0f, idMath::AngleDelta(idMath::AngleNormalize180(cmdAngles[i] + deltaAimAngles[i]), aimAngles[i]));
		} else {
			aimAngles[i] = idMath::AngleNormalize180(cmdAngles[i] + deltaAimAngles[i] );
		}
	}

	//gameLocal.Printf("Command: %0.4f, %0.4f, %0.4f\n", cmdAngles[0], cmdAngles[1], cmdAngles[2]);
	//gameLocal.Printf("Delta Aim: %0.4f, %0.4f, %0.4f\n", deltaAimAngles[0], deltaAimAngles[1], deltaAimAngles[2]);

	for (i = 0; i < 3; i++) {
		viewAngles[i] = idMath::AngleNormalize180(hmdAngles[i] + deltaViewAngles[i]);
	}

	if ( !centerView.IsDone( gameLocal.time ) ) {
		viewAngles.pitch = centerView.GetCurrentValue(gameLocal.time);
	}

	// clamp the pitch
	if ( noclip ) {
		if ( viewAngles.pitch > 89.0f ) {
			viewAngles.pitch = 89.0f;
		} else if ( viewAngles.pitch < -89.0f ) {
			viewAngles.pitch = -89.0f;
		}
	} else {
		if ( viewAngles.pitch > pm_maxviewpitch.GetFloat() ) {
			viewAngles.pitch = pm_maxviewpitch.GetFloat();
		} else if ( viewAngles.pitch < pm_minviewpitch.GetFloat() ) {
			viewAngles.pitch = pm_minviewpitch.GetFloat();
		}
		
		if (aimAngles.pitch > pm_maxviewpitch.GetFloat())
		{
			aimAngles.pitch = pm_maxviewpitch.GetFloat();
		}
		else if (aimAngles.pitch < pm_minviewpitch.GetFloat())
		{
			aimAngles.pitch = pm_minviewpitch.GetFloat();
		}
		
	}

	float aimDistanceFromView = idMath::AngleDelta(viewAngles.yaw, aimAngles.yaw);
	float aimAngleDelta = idMath::AngleNormalize180(aimAngles.yaw - previousaimAngles.yaw);

	//gameLocal.Printf("Hmd Orientation: [%0.4f, %0.4f, %0.4f]\n", hmdAngles.yaw, hmdAngles.pitch, hmdAngles.roll);
	//gameLocal.Printf("View: [%0.4f, %0.4f, %0.4f]\n", viewAngles.yaw, viewAngles.pitch, viewAngles.roll);

	if ((aimDistanceFromView > 25.0f || aimDistanceFromView < -25.0f) && (previousaimAngles != aimAngles))
	{
		gameLocal.Printf("Abs: %0.4f, %0.4f\n", abs(aimDistanceFromView), previousDistance);
		viewAngles.yaw += aimAngleDelta;
	}

	previousDistance = abs(aimDistanceFromView);
	previousaimAngles = aimAngles;

	/*
	gameLocal.Printf("Hmd Orientation: [%0.4f, %0.4f, %0.4f]\n", hmdAngles.yaw, hmdAngles.pitch, hmdAngles.roll);
	gameLocal.Printf("View: [%0.4f, %0.4f, %0.4f]\n", viewAngles.yaw, viewAngles.pitch, viewAngles.roll);
	gameLocal.Printf("Position: [%0.4f, %0.4f, %0.4f]\n", firstPersonViewOrigin.x, firstPersonViewOrigin.y, firstPersonViewOrigin.z);
	*/

	VR_UpdateDeltaViewAngles(viewAngles);
	VR_UpdateDeltaAimAngles(aimAngles);

	// orient the model towards the direction we're looking
	SetAngles(idAngles(0, viewAngles.yaw, 0));
	
	// save in the log for analyzing weapon angle offsets
	loggedViewAngles[gameLocal.framenum & (NUM_LOGGED_VIEW_ANGLES - 1)] = viewAngles;
	previoushmdAngles = hmdAngles;
}

/*
==============
idPlayer::UpdateAimAngles
==============
*/
void idPlayer::UpdateAimAngles()
{
	/*
	const float scale = 0.08f;

	//mxDelta = usercmd.mx - previousmx;
	//myDelta = usercmd.my - previousmy;

	if (mxDelta != 0 || myDelta != 0)
	{
		//aimangles.yaw -= scale * (float)mxDelta;
		//aimangles.pitch -= scale * -(float)myDelta;

		//ClampAngle(35.0f, viewAngles.pitch, aimangles.pitch);
		//ClampAngle(35.0f, viewAngles.yaw, aimangles.yaw);

		//common->Printf("UpdateAimAngles: [pitch: %0.4f yaw: %0.4f] [pitch: %0.4f yaw: %0.4f]\n", aimangles.pitch, aimangles.yaw, viewAngles.pitch, viewAngles.yaw);
	}

	previousmx = usercmd.mx;
	previousmy = usercmd.my;
	*/
}

/*
==============
idPlayer::UpdateAimPointer
==============
*/

float dist(idVec3 a, idVec3 b)
{
	idVec3 c;

	c.x = a.x - b.x;
	c.y = a.y - b.y;
	c.z = a.z - b.z;
	return sqrt(c.x * c.x + c.y * c.y + c.z * c.z);
}

bool idPlayer::CheckOutsideofCameraBound()
{
	float distance = idMath::AngleDelta(aimAngles.yaw, viewAngles.yaw);
	//common->Printf("Yaw delta: %0.4f\n", distance);
	if (distance > vr_aimdragangle.GetInteger() || distance < -vr_aimdragangle.GetInteger()) {
		return true;
	}

	return false;
}

void idPlayer::UpdateAimPointer()
{
	trace_t tr, test;
	idVec3 start = GetEyePosition();
	idVec4 color = idVec4(0.0f, 1.0f, 0.0f, 1.0f);

	//if (!weapon.GetEntity()->ShowCrosshair() || GuiActive() || AI_RELOAD || AI_ONLADDER || AI_DEAD || weapon.GetEntity()->IsHidden())
	if ( GuiActive() || AI_ONLADDER || AI_DEAD )
	{
		return;
	}

	// drag the cursor around when its outside the viewport bounds
	if (CheckOutsideofCameraBound())
	{
		color = idVec4(1.0f, 0.0f, 0.0f, 1.0f);
	}

	gameLocal.clip.TracePoint(tr, start, (start + aimAngles.ToForward() * 96.0f), CONTENTS_OPAQUE | MASK_SHOT_RENDERMODEL, this);
	gameLocal.clip.TracePoint(test, start, (start + viewAngles.ToForward() * 96.0f), CONTENTS_OPAQUE | MASK_SHOT_RENDERMODEL, this);

	float distance = dist(start, tr.endpos);
	float radius = 0.005f * distance;

	gameRenderWorld->DebugSphere(colorRed, idSphere(tr.endpos, radius));
	gameRenderWorld->DebugSphere(color, idSphere(test.endpos, 2.0f));
}

/*
==============
idPlayer::UpdateLaserSight
==============
*/

void idPlayer::UpdateLaserSight()
{
	idVec3	muzzleOrigin;
	idMat3	muzzleAxis;

	// In Multiplayer, weapon might not have been spawned yet.
	if (weapon.GetEntity() == NULL)
	{
		return;
	}

	if (!weapon.GetEntity()->ShowCrosshair() || AI_RELOAD || AI_ONLADDER || AI_DEAD || weapon.GetEntity()->IsHidden() || !weapon.GetEntity()->GetMuzzlePositionWithHacks(muzzleOrigin, muzzleAxis))
	{
		// hide it
		laserSightRenderEntity.allowSurfaceInViewID = -1;
		if (laserSightHandle == -1)
		{
			laserSightHandle = gameRenderWorld->AddEntityDef(&laserSightRenderEntity);
		}
		else {
			gameRenderWorld->UpdateEntityDef(laserSightHandle, &laserSightRenderEntity);
		}
		return;
	}

	// program the beam model

	// only show in the player's view
	laserSightRenderEntity.allowSurfaceInViewID = entityNumber + 1;
	laserSightRenderEntity.axis.Identity();

	laserSightRenderEntity.origin = muzzleOrigin - muzzleAxis[0] * 2.0f;
	idVec3	&target = *reinterpret_cast<idVec3 *>(&laserSightRenderEntity.shaderParms[SHADERPARM_BEAM_END_X]);

	trace_t tr;
	gameLocal.clip.TracePoint(tr, laserSightRenderEntity.origin, (laserSightRenderEntity.origin + viewAngles.ToForward() * 128.0f), CONTENTS_OPAQUE | MASK_SHOT_RENDERMODEL, this);
	target = tr.endpos;

	laserSightRenderEntity.shaderParms[SHADERPARM_BEAM_WIDTH] = vr_laserSightWidth.GetFloat();

	if (laserSightHandle == -1)
	{
		laserSightHandle = gameRenderWorld->AddEntityDef(&laserSightRenderEntity);
	}
	else
	{
		gameRenderWorld->UpdateEntityDef(laserSightHandle, &laserSightRenderEntity);
	}
}

/*
================
idPlayer::VR_SetAimAngles
================
*/
void idPlayer::VR_SetAimAngles(const idAngles &angles) {
	VR_UpdateDeltaAimAngles(angles);
	aimAngles = angles;
}

/*
================
idPlayer::RecenterView

================
*/
void idPlayer::RecenterView()
{

	if (oculus->isActivated) {
		oculus->RecenterHmd();
	}

	deltaViewAngles = ang_zero;
	SetViewAngles(ang_zero);
	oldViewYaw = viewAngles.yaw;
}
// OCULUS END
