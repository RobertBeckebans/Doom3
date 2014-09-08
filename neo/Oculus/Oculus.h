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

// Enable Oculus HMD Code
#define ENABLE_OCULUS_HMD

#define GAME_NAME_VR "DOOM 3 VR"
#define	WIN32_VR_WINDOW_CLASS_NAME "DOOM3_VR"

#include <GL/GL.h>

#undef strncmp
#include "../extern/OculusSDK/LibOVR/Src/OVR_CAPI.h"

#define OVR2IDUNITS 2.5f
#define PIXELDENSITY 1.0f
#define DEBUGHMDTYPE ovrHmd_DK2
#define LEFT_EYE_TARGET 0
#define RIGHT_EYE_TARGET 1
#define GUI_TARGET 2

typedef struct ovrvidmode_t
{
    const char *description;
    int         width, height;
} ovrvidmode_t;

class OculusHmd
{
public:

	OculusHmd();
	~OculusHmd();

	int					Init();
	void				Shutdown();

	idVec3				GetHeadTrackingOrientation();
	idVec3				GetHeadTrackingPosition();

	int					InitRendering();

	bool				isDebughmd;
	int					multiSamples;

	ovrTexture			G_OvrTextures[2];

	ovrHmd				Hmd = 0;
	ovrFovPort			G_ovrEyeFov[2];

	GLuint				EyeTexture[2];

	int					SetupView();
	int					GetRenderWidth()		{ return G_ovrRenderWidth; }
	int					GetRenderHeight()		{ return G_ovrRenderHeight; }
	int					GetFrameBufferWidth()	{ return G_FrameBufferWidth; }
	int					GetFrameBufferHeight()	{ return G_FrameBufferHeight; }
	void				SelectBuffer(int i, GLuint &fbo);
	idVec3				GetViewAdjustVector(int id);
	
private:
	
	int					GLSetupGuiFrameBuffer();
	GLuint				G_GLFrameBuffer[2];
	GLuint				G_GLGuiFrameBuffer;
	GLuint				G_GLDepthBuffer[3];
	GLuint				G_GLDepthTexture[3];
	GLuint				G_GLGuiTexture;

	ovrEyeRenderDesc	G_ovrEyeRenderDesc[2];
	ovrTexture			Fn_GenOvrTexture(int i);
	int					Fn_SetupFrameBuffer(int idx);
	int					Fn_SetupGuiFrameBuffer();
	int					InitHmdPositionTracking();
	int					G_ovrRenderWidth;
	int					G_ovrRenderHeight;
	int					G_FrameBufferWidth;
	int					G_FrameBufferHeight;

	void				GLInitExtensions();
	void*				GLGetProcAddress(const char* func) { return wglGetProcAddress(func); }
};

extern OculusHmd ovr;