/*
===========================================================================

Doom 3 GPL Source Code with Oculus rift support

Copyright (C) 2014 Guillaume Plourde
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "../oculus/Oculus.h"

#include "../extern/OculusSDK/LibOVR/Include/OVR_Kernel.h"
#include "../extern/OculusSDK/LibOVR/Src/OVR_CAPI_GL.h"
#include "../renderer/tr_local.h"

// This is a copy of Doom3 original render functions but with support for Oculus Rift

/*
===============
R_SetupProjection

This uses the "infinite far z" trick
===============
*/
void OR_SetupProjection(idRenderSystemLocal &tr)
{
	float	xmin, xmax, ymin, ymax;
	float	width, height;
	float	zNear;
	float	jitterx, jittery;
	static	idRandom random;

	// random jittering is usefull when multiple
	// frames are going to be blended together
	// for motion blurred anti-aliasing
	if (r_jitter.GetBool())
	{
		jitterx = random.RandomFloat();
		jittery = random.RandomFloat();
	}
	else
	{
		jitterx = jittery = 0;
	}

	//
	// set up projection matrix
	//

	zNear = r_znear.GetFloat();
	
	if (tr.viewDef->renderView.cramZNear)
	{
		zNear *= 0.25;
	}

	ymax = zNear * ovr.G_ovrEyeFov[tr.viewDef->eye].UpTan;
	ymin = -zNear * ovr.G_ovrEyeFov[tr.viewDef->eye].DownTan;

	xmax = zNear * ovr.G_ovrEyeFov[tr.viewDef->eye].LeftTan;
	xmin = -zNear * ovr.G_ovrEyeFov[tr.viewDef->eye].RightTan;

	width = xmax - xmin;
	height = ymax - ymin;

	jitterx = jitterx * width / (tr.viewDef->viewport.x2 - tr.viewDef->viewport.x1 + 1);
	xmin += jitterx;
	xmax += jitterx;
	jittery = jittery * height / (tr.viewDef->viewport.y2 - tr.viewDef->viewport.y1 + 1);
	ymin += jittery;
	ymax += jittery;

	tr.viewDef->projectionMatrix[0] = 2 * zNear / width;
	tr.viewDef->projectionMatrix[4] = 0;
	tr.viewDef->projectionMatrix[8] = (xmax + xmin) / width;	// normally 0
	tr.viewDef->projectionMatrix[12] = 0;

	tr.viewDef->projectionMatrix[1] = 0;
	tr.viewDef->projectionMatrix[5] = 2 * zNear / height;
	tr.viewDef->projectionMatrix[9] = (ymax + ymin) / height;	// normally 0
	tr.viewDef->projectionMatrix[13] = 0;

	// this is the far-plane-at-infinity formulation, and
	// crunches the Z range slightly so w=0 vertexes do not
	// rasterize right at the wraparound point
	tr.viewDef->projectionMatrix[2] = 0;
	tr.viewDef->projectionMatrix[6] = 0;
	tr.viewDef->projectionMatrix[10] = -0.999f;
	tr.viewDef->projectionMatrix[14] = -2.0f * zNear;

	tr.viewDef->projectionMatrix[3] = 0;
	tr.viewDef->projectionMatrix[7] = 0;
	tr.viewDef->projectionMatrix[11] = -1;
	tr.viewDef->projectionMatrix[15] = 0;
}

/*
=================
R_SetupViewFrustum

Setup that culling frustum planes for the current view
FIXME: derive from modelview matrix times projection matrix
=================
*/

void OR_SetupViewFrustum(idRenderSystemLocal &tr)
{
	int		i;
	float	xs, xc;
	float	ang;

	int FOV = 105;
	ang = DEG2RAD(FOV) * 0.5f;
	idMath::SinCos(ang, xs, xc);

	tr.viewDef->frustum[0] = xs * tr.viewDef->renderView.viewaxis[0] + xc * tr.viewDef->renderView.viewaxis[1];
	tr.viewDef->frustum[1] = xs * tr.viewDef->renderView.viewaxis[0] - xc * tr.viewDef->renderView.viewaxis[1];

	ang = DEG2RAD(FOV) * 0.5f;
	idMath::SinCos(ang, xs, xc);

	tr.viewDef->frustum[2] = xs * tr.viewDef->renderView.viewaxis[0] + xc * tr.viewDef->renderView.viewaxis[2];
	tr.viewDef->frustum[3] = xs * tr.viewDef->renderView.viewaxis[0] - xc * tr.viewDef->renderView.viewaxis[2];
	 
	// plane four is the front clipping plane
	tr.viewDef->frustum[4] = /* vec3_origin - */ tr.viewDef->renderView.viewaxis[0];

	for (i = 0; i < 5; i++) {
		// flip direction so positive side faces out (FIXME: globally unify this)
		tr.viewDef->frustum[i] = -tr.viewDef->frustum[i].Normal();
		tr.viewDef->frustum[i][3] = -(tr.viewDef->renderView.vieworg * tr.viewDef->frustum[i].Normal());
	}

	// eventually, plane five will be the rear clipping plane for fog

	float dNear, dFar, dLeft, dUp;

	dNear = r_znear.GetFloat();
	if (tr.viewDef->renderView.cramZNear) {
		dNear *= 0.25f;
	}

	dFar = MAX_WORLD_SIZE;
	dLeft = dFar * tan(DEG2RAD(FOV * 0.5f));
	dUp = dFar * tan(DEG2RAD(FOV * 0.5f));
	tr.viewDef->viewFrustum.SetOrigin(tr.viewDef->renderView.vieworg);
	tr.viewDef->viewFrustum.SetAxis(tr.viewDef->renderView.viewaxis);
	tr.viewDef->viewFrustum.SetSize(dNear, dFar, dLeft, dUp);
}

/*
=============
RB_SetBuffer

=============
*/
static void	RB_SetBuffer(const void *data, int eye)
{
	GLuint fbo;

	const setBufferCommand_t	*cmd;
	cmd = (const setBufferCommand_t *)data;
	
	ovr.SelectBuffer(eye, fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	if (r_clear.GetFloat() || idStr::Length(r_clear.GetString()) != 1 || r_lockSurfaces.GetBool() || r_singleArea.GetBool() || r_showOverDraw.GetBool()) {
		float c[3];
		if (sscanf(r_clear.GetString(), "%f %f %f", &c[0], &c[1], &c[2]) == 3) {
			qglClearColor(c[0], c[1], c[2], 1);
		}
		else if (r_clear.GetInteger() == 2) {
			qglClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		}
		else if (r_showOverDraw.GetBool()) {
			qglClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		}
		else {
			qglClearColor(0.4f, 0.0f, 0.25f, 1.0f);
		}
		qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	}

	backEnd.frameCount = cmd->frameCount;

	return;
}
/*
=============
RB_CopyRender

Copy part of the current framebuffer to an image
=============
*/
const void RB_CopyRender(const void *data);

/*
=============
RBO_ExecuteBackEndCommands

=============
*/

void RBO_ExecuteBackEndCommands(const emptyCommand_t *allCmds)
{
	GLuint fbo;

	// needed for editor rendering
	//RB_SetDefaultGLState();

	// upload any image loads that have completed
	globalImages->CompleteBackgroundImageLoads();

	ovrPosef headPose[2];
	ovrFrameTiming hmdFrameTiming = ovrHmd_BeginFrame(ovr.Hmd, 0);

	bool needGuiDraw = true;

	for (int i = 0; i < 2; i++)
	{
		RB_SetDefaultGLState();

		for (const emptyCommand_t * cmds = allCmds; cmds != NULL; cmds = (const emptyCommand_t *)cmds->next)
		{
			switch (cmds->commandId)
			{
			case RC_NOP:
				break;
			case RC_DRAW_VIEW:
			{
				// Always left first. Oculus recommend that we let the HMD tell you what to render first so TODO
				int eye = ((const drawSurfsCommand_t *)cmds)->viewDef->eye;

				headPose[i] = ovrHmd_GetEyePose(ovr.Hmd, (ovrEyeType)i);

				if (((const drawSurfsCommand_t *)cmds)->viewDef->viewEntitys)
				{			
					// World
					if (i != eye)
					{
						// Wrong eye so skip the command
						continue;
					}
					RB_DrawView(cmds);
				}
				else
				{
					if (needGuiDraw)
					{
						//ovr.SelectBuffer(GUI_TARGET, fbo);
						//glBindFramebuffer(GL_FRAMEBUFFER, fbo);
						RB_DrawView(cmds);
						//RB_SetBuffer(cmds, i);
						//needGuiDraw = false;
					}
				}

				break;
			}
			case RC_SET_BUFFER:
				RB_SetBuffer(cmds, i);
				break;
			case RC_SWAP_BUFFERS:
				// Ignore this. The Oculus SDK handle that
				//glBindRenderbuffer(GL_RENDERBUFFER, 0);
				//glBindTexture(GL_TEXTURE_2D, 0);
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				break;
			case RC_COPY_RENDER:
				//RB_CopyRender(cmds);
				break;
			default:
				common->Error("RB_ExecuteBackEndCommands: bad commandId");
				break;
			}
		}
	}

	// Copy the GUI Framebuffer. Need to draw to a plane in the viewport instead at some point
	/*
	ovr.SelectBuffer(GUI_TARGET, fbo);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
	glReadBuffer(GL_COLOR_ATTACHMENT0);

	ovr.SelectBuffer(LEFT_EYE_TARGET, fbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	glBlitFramebuffer(0, 0, ovr.GetFrameBufferWidth(), ovr.GetFrameBufferHeight(),
		0, 0, ovr.GetFrameBufferWidth(), ovr.GetFrameBufferHeight(),
		GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT,
		GL_NEAREST);
	*/

	// Done rendering. Send this off to the Oculus SDK
	glViewport(0, 0, ovr.Hmd->Resolution.w, ovr.Hmd->Resolution.h);
	glScissor(0, 0, ovr.Hmd->Resolution.w, ovr.Hmd->Resolution.h);
	ovrHmd_EndFrame(ovr.Hmd, headPose, ovr.G_OvrTextures);
}