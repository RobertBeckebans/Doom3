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

#ifndef __OCULUS_H__
#define __OCULUS_H__

// Enable Oculus HMD Code
#define ENABLE_OCULUS_HMD

#define GAME_NAME_VR "DOOM 3 VR"
#define	WIN32_VR_WINDOW_CLASS_NAME "DOOM3_VR"

#include <GL/GL.h>

#undef strncmp
#include "../extern/OculusSDK/LibOVR/Src/OVR_CAPI.h"

#define OVR2IDUNITS 2.5f
#define PIXELDENSITY 1.5f
#define DEBUGHMDTYPE ovrHmd_DK2
#define LEFT_EYE_TARGET 0
#define RIGHT_EYE_TARGET 1
#define GUI_TARGET 2

class Oculus
{
public:

	virtual				~Oculus() {};
	
	virtual int			Init( void ) = 0;
	virtual int			InitPositionTracking( void ) = 0;
	virtual void		InitScratch( void ) = 0;
	virtual void		InitOpenGL( void ) = 0;

	virtual void		Shutdown( void ) = 0;

	virtual idAngles	GetHeadTrackingOrientation( void ) = 0;
	virtual idVec3		GetHeadTrackingPosition( void ) = 0;

	virtual bool		isMotionTrackingEnabled( void ) = 0;

	bool				isDebughmd;
	bool				isActivated;

	int					multiSamples;

	ovrTexture			G_OvrTextures[2];
	GLuint				EyeTexture[2];
	GLuint				Scratch[2];
	GLuint				currentRenderImage[2];

	ovrHmd				Hmd;
	ovrFovPort			G_ovrEyeFov[2];

	int					GetCurrentFrambufferIndex() { return currentEye; }
	void				SetCurrentFrambufferIndex(int i) { currentEye = i; }

	virtual int			SetupView();
	int					GetRenderWidth()		{ return G_ovrRenderWidth; }
	int					GetRenderHeight()		{ return G_ovrRenderHeight; }
	int					GetFrameBufferWidth()	{ return G_FrameBufferWidth; }
	int					GetFrameBufferHeight()	{ return G_FrameBufferHeight; }
	virtual void		SelectBuffer(int i, GLuint &fbo);
	idVec3				GetViewAdjustVector(int id);
	
protected:

	int					currentEye;
	GLuint				G_GLFrameBuffer[2];
	GLuint				G_GLDepthTexture[2];

	ovrEyeRenderDesc	G_ovrEyeRenderDesc[2];
	ovrTexture			Fn_GenOvrTexture(int i);
	int					Fn_SetupFrameBuffer(int idx);
	int					G_ovrRenderWidth;
	int					G_ovrRenderHeight;
	int					G_FrameBufferWidth;
	int					G_FrameBufferHeight;

	void*				GLGetProcAddress(const char* func) { return wglGetProcAddress(func); }

// Windows
#ifdef WIN32
public:
	virtual int			InitRendering(HWND h, HDC dc) = 0;
protected:
	HWND				hWnd;
	HDC					dc;
#endif
};

extern Oculus * oculus;

#endif /* __OCULUS_H__ */