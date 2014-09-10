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

#include "Oculus.h"
#include "../sys/win32/win_local.h"

#include "../extern/OculusSDK/LibOVR/Include/OVR_Kernel.h"
#include "../extern/OculusSDK/LibOVR/Src/OVR_Stereo.h"
#include "../extern/OculusSDK/LibOVR/Src/OVR_CAPI_GL.h"

using namespace OVR;

// VR setting cvar definitions
idCVar vr_enableOculusRiftRendering("vr_enableOculusRiftRendering", "0", CVAR_BOOL | CVAR_SYSTEM | CVAR_NOCHEAT, "Enable Oculus Rift SDK rendering path");
idCVar vr_enableOculusRiftTracking("vr_enableOculusRiftTracking", "1", CVAR_BOOL | CVAR_SYSTEM | CVAR_NOCHEAT, "Enable Oculus Rift SDK head motion tracking");
idCVar vr_enableLowPersistence("vr_enableLowPersistence", "1", CVAR_BOOL | CVAR_SYSTEM | CVAR_NOCHEAT, "Enable Oculus Rift low persistence display");
idCVar vr_enableDynamicPrediction("vr_enableDynamicPrediction", "1", CVAR_BOOL | CVAR_SYSTEM | CVAR_NOCHEAT, "Enable Oculus Rift dynamic prediction");
idCVar vr_enableVsync("vr_enableVsync", "1", CVAR_BOOL | CVAR_SYSTEM | CVAR_NOCHEAT, "Enable vsync");
idCVar vr_enableMirror("vr_enableMirror", "1", CVAR_BOOL | CVAR_SYSTEM | CVAR_NOCHEAT, "Enable mirror to window mode");
idCVar vr_enablezeroipd("vr_enablezeroipd", "0", CVAR_BOOL | CVAR_SYSTEM | CVAR_NOCHEAT, "Disable depth rendering");

OculusHmd::OculusHmd()
{

}

OculusHmd::~OculusHmd()
{
	this->Shutdown();
}

/*
====================
OvrSystemLocal::Init

Init Oculus Rift HMD, get HMD properties and start head tracking
====================
*/
int OculusHmd::Init()
{
	// Initialize the Oculus VR library
	ovr_Initialize();

	if (!(Hmd = ovrHmd_Create(0))) {
		isDebughmd = true;
		common->Printf("Oculus HMD not found. Fall back to debug device.\n");
		if (!(Hmd = ovrHmd_CreateDebug(DEBUGHMDTYPE))) {
			Sys_Error("Error during debug HMD initialization");
			return false;
		}
	}

	common->Printf("--- OVR Initialization Complete ---\n");
	common->Printf("Device: %s - %s\n", Hmd->Manufacturer, Hmd->ProductName);

	// Set Hardware caps.
	unsigned hmdCaps;
	
	if (!vr_enableVsync.GetBool())
	{
		hmdCaps |= ovrHmdCap_NoVSync;
		common->Printf("VSync: Disabled\n");
	}
	
	if (vr_enableLowPersistence.GetBool())
	{
		hmdCaps |= ovrHmdCap_LowPersistence;
		common->Printf("Low Persistence mode: Enabled\n");
	}
	
	if (vr_enableDynamicPrediction.GetBool())
	{
		hmdCaps |= ovrHmdCap_DynamicPrediction;
		common->Printf("Dynamic Prediction: Enabled\n");
	}

	if (!vr_enableMirror.GetBool())
	{
		hmdCaps |= ovrHmdCap_NoMirrorToWindow;
		common->Printf("Mirror mode: Disabled\n");
	}

	// Do something if we are in extended mode
	if ((Hmd->HmdCaps & ovrHmdCap_ExtendDesktop))
	{
		common->Printf("HMD Device in Extended display mode\n");
	}

	ovrHmd_SetEnabledCaps(Hmd, hmdCaps);

	// Enable position and rotation tracking
	if (vr_enableOculusRiftTracking.GetBool())
	{
		if (!InitHmdPositionTracking())
			Sys_Error("Error during HMD head tracking initialization");
	}

	return 1;
};

int OculusHmd::InitRendering()
{
	ovrHmd_AttachToWindow(Hmd, win32.hWnd, NULL, NULL);

	GLInitExtensions();

	if (!SetupView())
	{
		Sys_Error("Error during HMD rendereing initialization");
		return 0;
	}

	return 1;
}

/*
====================
OvrSystemLocal::Shutdown

Disconnect from hmd
====================
*/
void OculusHmd::Shutdown()
{
	ovrHmd_Destroy(Hmd);
	ovr_Shutdown();
};

/*
=======================
GetHeadTrackingOrientation

=======================
*/

float degtorad(float r)
{
	return (r * (180.0f / idMath::PI));
}

idVec3 OculusHmd::GetHeadTrackingOrientation()
{
	idVec3 angles;

	if (!Hmd)
		return angles;

	ovrTrackingState ts = ovrHmd_GetTrackingState(Hmd, ovr_GetTimeInSeconds());
	Posef pose = ts.HeadPose.ThePose;

	if (ts.StatusFlags & (ovrStatus_OrientationTracked | ovrStatus_PositionTracked))
	{
		float y, p, r;
		pose.Rotation.GetEulerAngles<Axis_Y, Axis_X, Axis_Z>(&y, &p, &r);

		angles.x = -degtorad(p);
		angles.y = degtorad(y);
		angles.z = -degtorad(r);
	}

	return angles;
}

/*
=======================
GetHeadTrackingPosition

=======================
*/

idVec3 OculusHmd::GetHeadTrackingPosition()
{
	idVec3 position;

	if (!Hmd)
		return position;

	ovrTrackingState ts = ovrHmd_GetTrackingState(Hmd, ovr_GetTimeInSeconds());

	Posef pose = ts.HeadPose.ThePose;

	if (ts.StatusFlags & (ovrStatus_OrientationTracked | ovrStatus_PositionTracked))
	{
		position.x = pose.Translation.x;
		position.y = pose.Translation.y;
		position.z = pose.Translation.z;
	}
	return position;
}

/*
=======================
CreateOculusTexture

Generate an ovrTexture with our opengl texture
=======================
*/
ovrTexture OculusHmd::Fn_GenOvrTexture(int eye)
{
	ovrTexture tex;

	OVR::Sizei newRTSize(G_ovrRenderWidth, G_ovrRenderHeight);

	ovrGLTextureData* texData = (ovrGLTextureData*)&tex;

	texData->Header.API = ovrRenderAPI_OpenGL;
	texData->Header.TextureSize = newRTSize;
	texData->Header.RenderViewport = Recti(newRTSize);
	texData->TexId = EyeTexture[eye];

	return tex;
}


/*
======================
GLInitExtensions

Init OpenGL extentions that are required by Doom3 OCulus Rift port and not the original code.
======================
*/

PFNGLGENFRAMEBUFFERSPROC				glGenFramebuffers;
PFNGLDELETEFRAMEBUFFERSPROC				glDeleteFramebuffers;
PFNGLCHECKFRAMEBUFFERSTATUSPROC			glCheckFramebufferStatus;
PFNGLFRAMEBUFFERRENDERBUFFERPROC		glFramebufferRenderbuffer;
PFNGLFRAMEBUFFERTEXTURE2DPROC			glFramebufferTexture2D;
PFNGLBINDFRAMEBUFFERPROC				glBindFramebuffer;
PFNGLGENRENDERBUFFERSPROC				glGenRenderBuffers;
PFNGLBINDRENDERBUFFERPROC				glBindRenderbuffer;
PFNGLRENDERBUFFERSTORAGEEXTPROC			glRenderbufferStorage;
//PFNGLDRAWBUFFERSPROC					glDrawBuffers;
//PFNGLBLITFRAMEBUFFERPROC				glBlitFramebuffer;

void OculusHmd::GLInitExtensions()
{
	if (glGenFramebuffers)
		return;

	glGenFramebuffers =				(PFNGLGENFRAMEBUFFERSPROC)GLGetProcAddress("glGenFramebuffersEXT");
	glDeleteFramebuffers =			(PFNGLDELETEFRAMEBUFFERSPROC)GLGetProcAddress("glDeleteFramebuffersEXT");
	glCheckFramebufferStatus =		(PFNGLCHECKFRAMEBUFFERSTATUSPROC)GLGetProcAddress("glCheckFramebufferStatusEXT");
	glFramebufferRenderbuffer =		(PFNGLFRAMEBUFFERRENDERBUFFERPROC)GLGetProcAddress("glFramebufferRenderbufferEXT");
	glFramebufferTexture2D =		(PFNGLFRAMEBUFFERTEXTURE2DPROC)GLGetProcAddress("glFramebufferTexture2DEXT");
	glBindFramebuffer =				(PFNGLBINDFRAMEBUFFERPROC)GLGetProcAddress("glBindFramebufferEXT");
	glGenRenderBuffers =			(PFNGLGENRENDERBUFFERSPROC)GLGetProcAddress("glGenRenderbuffersEXT");
	glBindRenderbuffer =			(PFNGLBINDRENDERBUFFERPROC)GLGetProcAddress("glBindRenderbufferEXT");
	glRenderbufferStorage =			(PFNGLRENDERBUFFERSTORAGEEXTPROC)GLGetProcAddress("glRenderbufferStorageEXT");
	//glDrawBuffers =				(PFNGLDRAWBUFFERSPROC)GLGetProcAddress("glDrawBuffersEXT");
	//glBlitFramebuffer =			(PFNGLBLITFRAMEBUFFERPROC)GLGetProcAddress("glBlitFramebufferEXT");
	//glDeleteRenderbuffers =		(PFNGLDELETERENDERBUFFERSPROC)GLGetProcAddress("glDeleteRenderbuffersEXT");
}

/*
=======================
GetViewAdjustVector

This is how much we shift the eyes left and right
=======================
*/

idVec3 OculusHmd::GetViewAdjustVector(int eye)
{
	// From http://idtech4.com/wp-content/uploads/2011/10/Doom-3-Multiplayer-Level-Editing-Guide.pdf
	//  DOOM 3 player models stand to a total of 74 game units
	// 74 / 6 = 12.33 so 1 Doom unit equal 1 inch equal 2.5 cm i guess 
	// Make this a cvar so people can adjust

	idVec3 a;

	a.x = (G_ovrEyeRenderDesc[eye].ViewAdjust.x * 100) / OVR2IDUNITS;
	a.y = (G_ovrEyeRenderDesc[eye].ViewAdjust.y * 100) / OVR2IDUNITS;
	a.z = (G_ovrEyeRenderDesc[eye].ViewAdjust.z * 100) / OVR2IDUNITS;

	return a;
}

int OculusHmd::InitHmdPositionTracking()
{
	ovrHmd_ConfigureTracking(Hmd, ovrTrackingCap_Orientation | ovrTrackingCap_MagYawCorrection | ovrTrackingCap_Position, 0);
	return 1;
}

void OculusHmd::SelectBuffer(int idx, GLuint &fbo)
{
	if (idx == LEFT_EYE_TARGET)
	{
		fbo = G_GLFrameBuffer[0];
		return;
	}
	else if (idx == RIGHT_EYE_TARGET)
	{
		fbo = G_GLFrameBuffer[1];
		return;
	}
	else if (idx == GUI_TARGET)
	{
		//fbo = G_GLGuiFrameBuffer;
		return;
	}
	return;
}

int OculusHmd::Fn_SetupFrameBuffer(int idx)
{
	if (!G_GLFrameBuffer[idx])
	{
		glGenFramebuffers(1, &G_GLFrameBuffer[idx]);
	}

	glGenTextures(1, &EyeTexture[idx]);
	glBindTexture(GL_TEXTURE_2D, EyeTexture[idx]);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, G_FrameBufferWidth, G_FrameBufferHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

	glGenTextures(1, &G_GLDepthTexture[idx]);
	glBindTexture(GL_TEXTURE_2D, G_GLDepthTexture[idx]);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, G_FrameBufferWidth, G_FrameBufferHeight, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);

	glBindFramebuffer(GL_FRAMEBUFFER, G_GLFrameBuffer[idx]);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, EyeTexture[idx], 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, G_GLDepthTexture[idx], 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		Sys_Error("Error in Fn_SetupFrameBuffer. Framebuffer is invalid.");
		return 0;
	}
	
	glBindFramebuffer(GL_FRAMEBUFFER, G_GLFrameBuffer[idx]);
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); 

	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return 1;
}

extern int MakePowerOfTwo(int num);

int OculusHmd::SetupView()
{
	G_ovrEyeFov[0] = Hmd->DefaultEyeFov[0];
	G_ovrEyeFov[1] = Hmd->DefaultEyeFov[1];

	float FovSideTanLimit = FovPort::Max(Hmd->MaxEyeFov[0], Hmd->MaxEyeFov[1]).GetMaxSideTan();
	float FovSideTanMax = FovPort::Max(Hmd->DefaultEyeFov[0], Hmd->DefaultEyeFov[1]).GetMaxSideTan();

	G_ovrEyeFov[0] = FovPort::Min(G_ovrEyeFov[0], FovPort(FovSideTanMax));
	G_ovrEyeFov[1] = FovPort::Min(G_ovrEyeFov[1], FovPort(FovSideTanMax));

	if (vr_enablezeroipd.GetBool())
	{
		G_ovrEyeFov[0] = FovPort::Max(G_ovrEyeFov[0], G_ovrEyeFov[1]);
		G_ovrEyeFov[1] = G_ovrEyeFov[0];

		Sizei _textureSize = ovrHmd_GetFovTextureSize(Hmd, ovrEye_Left, G_ovrEyeFov[0], PIXELDENSITY);

		G_ovrRenderWidth = _textureSize.w;
		G_ovrRenderHeight = _textureSize.h;

		G_FrameBufferWidth = G_ovrRenderWidth;
		G_FrameBufferHeight = G_ovrRenderHeight;

		Fn_SetupFrameBuffer(0);
		Fn_SetupFrameBuffer(1);
		//Fn_SetupGuiFrameBuffer();

		G_OvrTextures[0] = Fn_GenOvrTexture(0);
		G_OvrTextures[1] = G_OvrTextures[0];
	}
	else
	{
		Sizei _textureSizeLeft = ovrHmd_GetFovTextureSize(Hmd, ovrEye_Left, G_ovrEyeFov[0], PIXELDENSITY);
		Sizei _textureSizeRight = ovrHmd_GetFovTextureSize(Hmd, ovrEye_Right, G_ovrEyeFov[0], PIXELDENSITY);

		G_ovrRenderWidth = _textureSizeLeft.w;
		G_ovrRenderHeight = _textureSizeLeft.h;

		G_FrameBufferWidth = G_ovrRenderWidth;
		G_FrameBufferHeight = G_ovrRenderHeight;

		Fn_SetupFrameBuffer(0);
		Fn_SetupFrameBuffer(1);
		//Fn_SetupGuiFrameBuffer();

		G_OvrTextures[0] = Fn_GenOvrTexture(0);
		G_OvrTextures[1] = Fn_GenOvrTexture(1);
	}


	if (vr_enablezeroipd.GetBool())
	{
		// Remove IPD adjust
		G_ovrEyeRenderDesc[0].ViewAdjust = Vector3f(0);
		G_ovrEyeRenderDesc[1].ViewAdjust = Vector3f(0);
	}

	ovrGLConfig glcfg;
	glcfg.OGL.Header.API = ovrRenderAPI_OpenGL;
	glcfg.OGL.Header.RTSize = Sizei(Hmd->Resolution.w, Hmd->Resolution.h);
	glcfg.OGL.Header.Multisample = 1;
	glcfg.OGL.Window = win32.hWnd;
	glcfg.OGL.DC = win32.hDC;

	unsigned distortionCaps = ovrDistortionCap_Chromatic | ovrDistortionCap_Vignette;

	/*
	if (SupportsSrgb)
		distortionCaps |= ovrDistortionCap_SRGB;
	if (PixelLuminanceOverdrive)
		distortionCaps |= ovrDistortionCap_Overdrive;
	if (TimewarpEnabled)
		distortionCaps |= ovrDistortionCap_TimeWarp;
	if (TimewarpNoJitEnabled)
		distortionCaps |= ovrDistortionCap_ProfileNoTimewarpSpinWaits;
	*/

	if (!ovrHmd_ConfigureRendering(Hmd, &glcfg.Config, distortionCaps, G_ovrEyeFov, G_ovrEyeRenderDesc))
	{
		// Shutdown
		return 0;
	}

	return 1;
}

// We will refer to this in the Doom3 code when we need stuff from the Hmd
OculusHmd ovr;