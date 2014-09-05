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

	int					GLSetupFrameBuffer();
	GLuint				G_GLFrameBuffer;
	GLuint				G_GLDepthBuffer;
	ovrTexture			G_OvrTextures[2];

	ovrHmd				Hmd = 0;
	ovrFovPort			G_ovrEyeFov[2];

	int					SetupView();
	int					GetRenderWidth()	{ return G_ovrRenderWidth; }
	int					GetRenderHeight()	{ return G_ovrRenderHeight; }
	idVec3				GetViewAdjustVector(int id);
	
	int					FuncUpdateFrameBuffer(int idx);

private:
	ovrEyeRenderDesc	G_ovrEyeRenderDesc[2];
	ovrTexture			FuncGenOvrTexture(int w, int h);
	int					InitHmdPositionTracking();
	int					G_ovrRenderWidth;
	int					G_ovrRenderHeight;
	GLuint				FuncGenGLTexture(int w, int h);
	void				GLInitExtensions();
	void*				GLGetProcAddress(const char* func) { return wglGetProcAddress(func); }
};

extern OculusHmd ovr;