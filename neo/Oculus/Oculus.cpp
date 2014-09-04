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

#include "../sys/win32/win_local.h"

#include "../extern/OculusSDK/LibOVR/Include/OVR_Kernel.h"
#include "../extern/OculusSDK/LibOVR/Src/OVR_Stereo.h"
#include "../extern/OculusSDK/LibOVR/Src/OVR_CAPI_GL.h"

using namespace OVR;

/*
====================
OvrSystemLocal

====================
*/

OculusHmd::OculusHmd()
{

}

OculusHmd::~OculusHmd()
{
	Hmd = NULL;
}
/*
====================
OvrSystemLocal::Init

Init Oculus Rift HMD, get HMD properties and start head tracking
====================
*/
bool OculusHmd::Init()
{
	// Init the SDK
	ovr_Initialize();

	// Create Hmd device
	Hmd = ovrHmd_Create(0);

	// Check if we have real hardware or else create a debug HMD for testing
	if (!Hmd)
	{
		Hmd = ovrHmd_CreateDebug(ovrHmd_DK2);
		isDebughmd = true;

		if (!Hmd)
			return false;
	}

	// Set HMD Resolution description, this is the Hmd screen resolution

	Resolution.description = "Oculus VR HMD";
	Resolution.width = Hmd->Resolution.w;
	Resolution.height = Hmd->Resolution.h;

	ovrHmd_SetEnabledCaps(Hmd, ovrHmdCap_LowPersistence | ovrHmdCap_DynamicPrediction);	
	ovrHmd_ConfigureTracking(Hmd, ovrTrackingCap_Orientation | ovrTrackingCap_MagYawCorrection | ovrTrackingCap_Position, 0);

	float FovSideTanLimit = FovPort::Max(Hmd->MaxEyeFov[0], Hmd->MaxEyeFov[1]).GetMaxSideTan();
	float FovSideTanMax = FovPort::Max(Hmd->DefaultEyeFov[0], Hmd->DefaultEyeFov[1]).GetMaxSideTan();

	eyeFov[0] = Hmd->DefaultEyeFov[0];
	eyeFov[1] = Hmd->DefaultEyeFov[1];
	
	eyeFov[0] = OVR::FovPort::Min(eyeFov[0], FovPort(FovSideTanMax));
	eyeFov[1] = OVR::FovPort::Min(eyeFov[1], FovPort(FovSideTanMax));

	//Configure Stereo settings.
	Sizei recommenedTex0Size = ovrHmd_GetFovTextureSize(Hmd, ovrEye_Left, Hmd->DefaultEyeFov[0], 1.0f);
	Sizei recommenedTex1Size = ovrHmd_GetFovTextureSize(Hmd, ovrEye_Right, Hmd->DefaultEyeFov[1], 1.0f);
	
	//RenderWidth = Hmd->Resolution.w;
	//RenderHeight = Hmd->Resolution.h;

	RenderWidth = (recommenedTex0Size.w>recommenedTex1Size.w ? recommenedTex0Size.w : recommenedTex1Size.w);
	RenderHeight = (recommenedTex0Size.h>recommenedTex1Size.h ? recommenedTex0Size.h : recommenedTex1Size.h);

	return true;
};

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
CreateTexture

Create opengl texture
=======================
*/

static GLuint CreateTexture(int width, int height)
{
	/*
	GLuint texid;
	unsigned int* texdata;
	const unsigned int size = (width * height) * 4;

	texdata = (unsigned int*)new GLuint[(size * sizeof(unsigned int))];
	ZeroMemory(texdata, size * sizeof(unsigned int));

	glGenTextures(1, &texid);
	glBindTexture(GL_TEXTURE_2D, texid);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, texdata);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	delete[] texdata;
	return texid;
	*/
	GLuint i;
	glGenTextures(1, &i);
	glBindTexture(GL_TEXTURE_2D, i);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	return i;
}

/*
=======================
CreateOculusTexture

Generate an ovrTexture with our opengl texture
=======================
*/
static ovrTexture CreateOculusTexture(GLuint id, int width, int height)
{
	ovrTexture tex;

	OVR::Sizei newRTSize(width, height);

	ovrGLTextureData* texData = (ovrGLTextureData*)&tex;

	texData->Header.API = ovrRenderAPI_OpenGL;
	texData->Header.TextureSize = newRTSize;
	texData->Header.RenderViewport = Recti(newRTSize);
	texData->TexId = id;

	return tex;
}

void* GetFunction(const char* functionName)
{
	return wglGetProcAddress(functionName);
}

PFNGLGENFRAMEBUFFERSPROC				glGenFramebuffers;
PFNGLDELETEFRAMEBUFFERSPROC				glDeleteFramebuffers;
PFNGLCHECKFRAMEBUFFERSTATUSPROC			glCheckFramebufferStatus;
PFNGLFRAMEBUFFERRENDERBUFFERPROC		glFramebufferRenderbuffer;
PFNGLFRAMEBUFFERTEXTURE2DPROC			glFramebufferTexture2D;
PFNGLBINDFRAMEBUFFERPROC				glBindFramebuffer;
PFNGLGENRENDERBUFFERSPROC				glGenRenderBuffers;
PFNGLBINDRENDERBUFFERPROC				glBindRenderbuffer;
PFNGLRENDERBUFFERSTORAGEEXTPROC			glRenderbufferStorage;

void InitGLExtensions()
{
	if (glGenFramebuffers)
		return;

	glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)GetFunction("glGenFramebuffersEXT");
	glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)GetFunction("glDeleteFramebuffersEXT");
	glCheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)GetFunction("glCheckFramebufferStatusEXT");
	glFramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC)GetFunction("glFramebufferRenderbufferEXT");
	glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC)GetFunction("glFramebufferTexture2DEXT");
	glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)GetFunction("glBindFramebufferEXT");
	glGenRenderBuffers = (PFNGLGENRENDERBUFFERSPROC)GetFunction("glGenRenderbuffersEXT");
	glBindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC)GetFunction("glBindRenderbufferEXT");
	glRenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEEXTPROC)GetFunction("glRenderbufferStorageEXT");
//    glDeleteRenderbuffers =             (PFNGLDELETERENDERBUFFERSPROC)             GetFunction("glDeleteRenderbuffersEXT");
}

/*
=======================
InitRenderTarget

Create the render targets and setup opengl rendering to our window
=======================
*/

bool OculusHmd::InitRenderTarget()
{
	InitGLExtensions();

	glGenFramebuffers(1, &Framebuffer);
	glGenRenderBuffers(1, &rbo);

	for (int i = 0; i < 3; i++)
	{
		RenderTargetTexture[i] = CreateTexture(RenderWidth, RenderHeight);
	}

	EyeTexture[0] = CreateOculusTexture(RenderTargetTexture[0], RenderWidth, RenderHeight);
	EyeTexture[1] = CreateOculusTexture(RenderTargetTexture[1], RenderWidth, RenderHeight);

	eyeFov[0] = Hmd->DefaultEyeFov[0];
	eyeFov[1] = Hmd->DefaultEyeFov[1];

	unsigned distortionCaps = NULL;

	if (!isDebughmd)
		distortionCaps = ovrDistortionCap_Chromatic | ovrDistortionCap_Vignette;

	ovrGLConfig glcfg;
	glcfg.OGL.Header.API = ovrRenderAPI_OpenGL;
	glcfg.OGL.Header.RTSize = Sizei(Hmd->Resolution.w, Hmd->Resolution.h);
	glcfg.OGL.Header.Multisample = 1;
	glcfg.OGL.Window = win32.hWnd;
	glcfg.OGL.DC = win32.hDC;

	ovrBool result = ovrHmd_ConfigureRendering(Hmd, &glcfg.Config, distortionCaps, eyeFov, EyeRenderDesc);

	return true;
}

/*
=======================
GetRenderWidth

Get HMD rendering resolution width
=======================
*/

int OculusHmd::GetRenderWidth()
{
	return RenderWidth;
}

/*
=======================
GetRenderHeight

Get HMD rendering resolution height
=======================
*/
int OculusHmd::GetRenderHeight()
{
	return RenderHeight;
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

	float scale = 2.5f;

	a.x = (EyeRenderDesc[eye].ViewAdjust.x * 100) / scale;
	a.y = (EyeRenderDesc[eye].ViewAdjust.y * 100) / scale;
	a.z = (EyeRenderDesc[eye].ViewAdjust.z * 100) / scale;

	return a;
}

// We will refer to this in the Doom3 code when we need stuff from the Hmd
OculusHmd ovr;