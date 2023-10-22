/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
/*
 *
 * dynamic opengl loading - courtesy of Jeff Slutter
 *
 *
 */

#ifndef __LOADGL_H__
#define __LOADGL_H__

#if !DXX_USE_OGL
#error "This file can only be included in OpenGL enabled builds."
#endif

#ifdef _WIN32
#include <windows.h>
#define OGLFUNCCALL	__stdcall
#else
#define OGLFUNCCALL
#endif

#include <GL/gl.h>
#include "pstypes.h"

//gl extensions.
#ifndef GL_ARB_multitexture
#define GL_ARB_multitexture 1
#define GL_TEXTURE0_ARB 0x84C0
#define GL_TEXTURE1_ARB 0x84C1
#define GL_MAX_TEXTURE_UNITS_ARB 0x84E2
#endif

#ifndef GL_SGIS_multitexture
#define GL_SGIS_multitexture 1
#define GL_TEXTURE0_SGIS 0x835F
#define GL_TEXTURE1_SGIS 0x8360
#define GL_MAX_TEXTURES_SGIS 0x835D
#endif

#ifndef GL_EXT_paletted_texture
#define GL_EXT_paletted_texture 1
#define GL_COLOR_INDEX8_EXT 0x80E5
#define GL_TEXTURE_INDEX_SIZE_EXT 0x80ED
#endif

#ifndef GL_TEXTURE_INDEX_SIZE_EXT
#define GL_TEXTURE_INDEX_SIZE_EXT 0x80ED
#endif

#ifndef GL_SCISSOR_TEST
#define GL_SCISSOR_TEST 0x0C11
#endif

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

#ifndef GL_NV_register_combiners
#define GL_NV_register_combiners 1
#define GL_REGISTER_COMBINERS_NV          0x8522
#define GL_VARIABLE_A_NV                  0x8523
#define GL_VARIABLE_B_NV                  0x8524
#define GL_VARIABLE_C_NV                  0x8525
#define GL_VARIABLE_D_NV                  0x8526
#define GL_VARIABLE_E_NV                  0x8527
#define GL_VARIABLE_F_NV                  0x8528
#define GL_VARIABLE_G_NV                  0x8529
#define GL_CONSTANT_COLOR0_NV             0x852A
#define GL_CONSTANT_COLOR1_NV             0x852B
#define GL_PRIMARY_COLOR_NV               0x852C
#define GL_SECONDARY_COLOR_NV             0x852D
#define GL_SPARE0_NV                      0x852E
#define GL_SPARE1_NV                      0x852F
#define GL_DISCARD_NV                     0x8530
#define GL_E_TIMES_F_NV                   0x8531
#define GL_SPARE0_PLUS_SECONDARY_COLOR_NV 0x8532
#define GL_UNSIGNED_IDENTITY_NV           0x8536
#define GL_UNSIGNED_INVERT_NV             0x8537
#define GL_EXPAND_NORMAL_NV               0x8538
#define GL_EXPAND_NEGATE_NV               0x8539
#define GL_HALF_BIAS_NORMAL_NV            0x853A
#define GL_HALF_BIAS_NEGATE_NV            0x853B
#define GL_SIGNED_IDENTITY_NV             0x853C
#define GL_SIGNED_NEGATE_NV               0x853D
#define GL_SCALE_BY_TWO_NV                0x853E
#define GL_SCALE_BY_FOUR_NV               0x853F
#define GL_SCALE_BY_ONE_HALF_NV           0x8540
#define GL_BIAS_BY_NEGATIVE_ONE_HALF_NV   0x8541
#define GL_COMBINER_INPUT_NV              0x8542
#define GL_COMBINER_MAPPING_NV            0x8543
#define GL_COMBINER_COMPONENT_USAGE_NV    0x8544
#define GL_COMBINER_AB_DOT_PRODUCT_NV     0x8545
#define GL_COMBINER_CD_DOT_PRODUCT_NV     0x8546
#define GL_COMBINER_MUX_SUM_NV            0x8547
#define GL_COMBINER_SCALE_NV              0x8548
#define GL_COMBINER_BIAS_NV               0x8549
#define GL_COMBINER_AB_OUTPUT_NV          0x854A
#define GL_COMBINER_CD_OUTPUT_NV          0x854B
#define GL_COMBINER_SUM_OUTPUT_NV         0x854C
#define GL_MAX_GENERAL_COMBINERS_NV       0x854D
#define GL_NUM_GENERAL_COMBINERS_NV       0x854E
#define GL_COLOR_SUM_CLAMP_NV             0x854F
#define GL_COMBINER0_NV                   0x8550
#define GL_COMBINER1_NV                   0x8551
#define GL_COMBINER2_NV                   0x8552
#define GL_COMBINER3_NV                   0x8553
#define GL_COMBINER4_NV                   0x8554
#define GL_COMBINER5_NV                   0x8555
#define GL_COMBINER6_NV                   0x8556
#define GL_COMBINER7_NV                   0x8557
#endif

#define GL_SCISSOR_TEST 0x0C11

#ifdef __cplusplus
#define OEXTERN	extern "C"
#else
#define OEXTERN extern
#define true 1
#define false 0
#endif

#ifdef DECLARE_VARS
#define DEFVAR
#else
#define DEFVAR	OEXTERN
#endif

#define glAlphaFunc dglAlphaFunc
#define glBindTexture dglBindTexture
#define glBlendFunc dglBlendFunc
#define glClear dglClear
#define glClearColor dglClearColor
#define glColor4f dglColor4f
#define glColorPointer dglColorPointer
#define glCullFace dglCullFace
#define glDeleteTextures dglDeleteTextures
#define glDepthFunc dglDepthFunc
#define glDepthMask dglDepthMask
#define glDisable dglDisable
#define glDisableClientState dglDisableClientState
#define glDrawArrays dglDrawArrays
#define glEnable dglEnable
#define glEnableClientState dglEnableClientState
#define glEnd dglEnd
#define glFinish dglFinish
#define glGenTextures dglGenTextures
#define glGetFloatv dglGetFloatv
#define glGetIntegerv dglGetIntegerv
#define glGetString dglGetString
#define glGetTexLevelParameteriv dglGetTexLevelParameteriv
#define glHint dglHint
#define glLineWidth dglLineWidth
#define glLoadIdentity dglLoadIdentity
#define glMatrixMode dglMatrixMode
#define glOrtho dglOrtho
#define glPointSize dglPointSize
#define glPopMatrix dglPopMatrix
#define glPrioritizeTextures dglPrioritizeTextures
#define glPushMatrix dglPushMatrix
#define glReadBuffer dglReadBuffer
#define glReadPixels dglReadPixels
#define glScalef dglScalef
#define glShadeModel dglShadeModel
#define glTexCoordPointer dglTexCoordPointer
#define glTexEnvi dglTexEnvi
#define glTexImage2D dglTexImage2D
#define glTexParameterf dglTexParameterf
#define glTexParameteri dglTexParameteri
#define glTranslatef dglTranslatef
#define glVertexPointer dglVertexPointer
#define glViewport dglViewport

typedef void (OGLFUNCCALL *glAlphaFunc_fp)(GLenum func, GLclampf ref);
typedef void (OGLFUNCCALL *glBindTexture_fp)(GLenum target, GLuint texture);
typedef void (OGLFUNCCALL *glBlendFunc_fp)(GLenum sfactor, GLenum dfactor);
typedef void (OGLFUNCCALL *glClear_fp)(GLbitfield mask);
typedef void (OGLFUNCCALL *glClearColor_fp)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
typedef void (OGLFUNCCALL *glColor4f_fp)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
typedef void (OGLFUNCCALL *glColorPointer_fp)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
typedef void (OGLFUNCCALL *glCullFace_fp)(GLenum mode);
typedef void (OGLFUNCCALL *glDeleteTextures_fp)(GLsizei n, const GLuint *textures);
typedef void (OGLFUNCCALL *glDepthFunc_fp)(GLenum func);
typedef void (OGLFUNCCALL *glDepthMask_fp)(GLboolean flag);
typedef void (OGLFUNCCALL *glDisable_fp)(GLenum cap);
typedef void (OGLFUNCCALL *glDisableClientState_fp)(GLenum array);
typedef void (OGLFUNCCALL *glDrawArrays_fp)(GLenum mode, GLint first, GLsizei count);
typedef void (OGLFUNCCALL *glEnable_fp)(GLenum cap);
typedef void (OGLFUNCCALL *glEnableClientState_fp)(GLenum array);
typedef void (OGLFUNCCALL *glEnd_fp)(void);
typedef void (OGLFUNCCALL *glFinish_fp)(void);
typedef void (OGLFUNCCALL *glGenTextures_fp)(GLsizei n, GLuint *textures);
typedef void (OGLFUNCCALL *glGetFloatv_fp)(GLenum pname, GLfloat *params);
typedef void (OGLFUNCCALL *glGetIntegerv_fp)(GLenum pname, GLint *params);
typedef const GLubyte *(OGLFUNCCALL *glGetString_fp)(GLenum name);
typedef void (OGLFUNCCALL *glGetTexLevelParameteriv_fp)(GLenum target, GLint level, GLenum pname, GLint *params);
typedef void (OGLFUNCCALL *glHint_fp)(GLenum target, GLenum mode);
typedef void (OGLFUNCCALL *glLineWidth_fp)(GLfloat width);
typedef void (OGLFUNCCALL *glLoadIdentity_fp)(void);
typedef void (OGLFUNCCALL *glMatrixMode_fp)(GLenum mode);
typedef void (OGLFUNCCALL *glOrtho_fp)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
typedef void (OGLFUNCCALL *glPointSize_fp)(GLfloat size);
typedef void (OGLFUNCCALL *glPopMatrix_fp)(void);
typedef void (OGLFUNCCALL *glPrioritizeTextures_fp)(GLsizei n, const GLuint *textures, const GLclampf *priorities);
typedef void (OGLFUNCCALL *glPushMatrix_fp)(void);
typedef void (OGLFUNCCALL *glReadBuffer_fp)(GLenum mode);
typedef void (OGLFUNCCALL *glReadPixels_fp)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
typedef void (OGLFUNCCALL *glScalef_fp)(GLfloat x, GLfloat y, GLfloat z);
typedef void (OGLFUNCCALL *glShadeModel_fp)(GLenum mode);
typedef void (OGLFUNCCALL *glTexCoordPointer_fp)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
typedef void (OGLFUNCCALL *glTexEnvi_fp)(GLenum target, GLenum pname, GLint param);
typedef void (OGLFUNCCALL *glTexImage2D_fp)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
typedef void (OGLFUNCCALL *glTexParameterf_fp)(GLenum target, GLenum pname, GLfloat param);
typedef void (OGLFUNCCALL *glTexParameteri_fp)(GLenum target, GLenum pname, GLint param);
typedef void (OGLFUNCCALL *glTranslatef_fp)(GLfloat x, GLfloat y, GLfloat z);
typedef void (OGLFUNCCALL *glVertexPointer_fp)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
typedef void (OGLFUNCCALL *glViewport_fp)(GLint x, GLint y, GLsizei width, GLsizei height);

typedef void (OGLFUNCCALL *glMultiTexCoord2fARB_fp)(GLenum target, GLfloat s, GLfloat t);
typedef void (OGLFUNCCALL *glActiveTextureARB_fp)(GLenum target);
typedef void (OGLFUNCCALL *glMultiTexCoord2fSGIS_fp)(GLenum target, GLfloat s, GLfloat t);
typedef void (OGLFUNCCALL *glSelectTextureSGIS_fp)(GLenum target);

#ifdef _WIN32
typedef PROC  (OGLFUNCCALL *wglGetProcAddress_fp)(LPCSTR);
#endif

DEFVAR glAlphaFunc_fp dglAlphaFunc;
DEFVAR glBindTexture_fp dglBindTexture;
DEFVAR glBlendFunc_fp dglBlendFunc;
DEFVAR glClear_fp dglClear;
DEFVAR glClearColor_fp dglClearColor;
DEFVAR glColor4f_fp dglColor4f;
DEFVAR glColorPointer_fp dglColorPointer;
DEFVAR glCullFace_fp dglCullFace;
DEFVAR glDeleteTextures_fp dglDeleteTextures;
DEFVAR glDepthFunc_fp dglDepthFunc;
DEFVAR glDepthMask_fp dglDepthMask;
DEFVAR glDisable_fp dglDisable;
DEFVAR glDisableClientState_fp dglDisableClientState;
DEFVAR glDrawArrays_fp dglDrawArrays;
DEFVAR glEnable_fp dglEnable;
DEFVAR glEnableClientState_fp dglEnableClientState;
DEFVAR glEnd_fp dglEnd;
DEFVAR glFinish_fp dglFinish;
DEFVAR glGenTextures_fp dglGenTextures;
DEFVAR glGetFloatv_fp dglGetFloatv;
DEFVAR glGetIntegerv_fp dglGetIntegerv;
DEFVAR glGetString_fp dglGetString;
DEFVAR glGetTexLevelParameteriv_fp dglGetTexLevelParameteriv;
DEFVAR glHint_fp dglHint;
DEFVAR glLineWidth_fp dglLineWidth;
DEFVAR glLoadIdentity_fp dglLoadIdentity;
DEFVAR glMatrixMode_fp dglMatrixMode;
DEFVAR glOrtho_fp dglOrtho;
DEFVAR glPointSize_fp dglPointSize;
DEFVAR glPopMatrix_fp dglPopMatrix;
DEFVAR glPrioritizeTextures_fp dglPrioritizeTextures;
DEFVAR glPushMatrix_fp dglPushMatrix;
DEFVAR glReadBuffer_fp dglReadBuffer;
DEFVAR glReadPixels_fp dglReadPixels;
DEFVAR glScalef_fp dglScalef;
DEFVAR glShadeModel_fp dglShadeModel;
DEFVAR glTexCoordPointer_fp dglTexCoordPointer;
DEFVAR glTexEnvi_fp dglTexEnvi;
DEFVAR glTexImage2D_fp dglTexImage2D;
DEFVAR glTexParameterf_fp dglTexParameterf;
DEFVAR glTexParameteri_fp dglTexParameteri;
DEFVAR glTranslatef_fp dglTranslatef;
DEFVAR glVertexPointer_fp dglVertexPointer;
DEFVAR glViewport_fp dglViewport;

#ifdef DECLARE_VARS

// Dynamic module load functions
#ifdef _WIN32
static inline void *dll_LoadModule(const char *name)
{
	HINSTANCE handle;
	handle = LoadLibrary(name);
	return reinterpret_cast<void *>(handle);
}
static inline void dll_UnloadModule(void *hdl)
{
	if (hdl)
	{
		FreeLibrary(reinterpret_cast<HINSTANCE>(hdl));
	}
}
static void *dll_GetSymbol(void *dllhandle,const char *symname)
{
	if(!dllhandle)
		return NULL;
	return reinterpret_cast<void *>(GetProcAddress(reinterpret_cast<HINSTANCE>(dllhandle), symname));
}
#endif
#ifdef __unix__
#include <dlfcn.h>
static inline void *dll_LoadModule(const char *name)
{
	return (void *)dlopen(name,RTLD_NOW|RTLD_GLOBAL);
}
static inline void dll_UnloadModule(void *hdl)
{
	if(hdl)
	{
		dlclose(hdl);
	}
}
static void *dll_GetSymbol(void *dllhandle,const char *symname)
{
	if(!dllhandle)
		return NULL;
	return dlsym(dllhandle,symname);
}
#endif
#ifdef macintosh
#include <SDL.h>
static inline void *dll_LoadModule(const char *name)
{
	return SDL_GL_LoadLibrary(name) ? NULL : (void *) -1;	// return pointer is not dereferenced
}
static inline void dll_UnloadModule(void *hdl)
{
	hdl = hdl;	// SDL_GL_UnloadLibrary not exported by SDL
}
static void *dll_GetSymbol(void *dllhandle,const char *symname)
{
	if(!dllhandle)
		return NULL;
	return SDL_GL_GetProcAddress(symname);
}
#endif

#endif //DECLARE_VARS

// pass true to load the library
// pass false to unload it
bool OpenGL_LoadLibrary(bool load, const char *OglLibPath);//load=true removed because not c++
#ifdef DECLARE_VARS
static void OpenGL_SetFuncsToNull(void);

void *OpenGLModuleHandle=NULL;
bool OpenGL_LoadLibrary(bool load, const char *OglLibPath)
{
	if(load && OpenGLModuleHandle)
		return true;

	OpenGL_SetFuncsToNull();

	if(!load)
	{
		if(OpenGLModuleHandle)
		{
			dll_UnloadModule(OpenGLModuleHandle);
			OpenGLModuleHandle = NULL;
		}
		return true;
	}else
	{
		OpenGLModuleHandle = dll_LoadModule(OglLibPath);
		if(!OpenGLModuleHandle)
			return false;

		dglAlphaFunc = reinterpret_cast<glAlphaFunc_fp>(dll_GetSymbol(OpenGLModuleHandle,"glAlphaFunc"));
		dglBindTexture = reinterpret_cast<glBindTexture_fp>(dll_GetSymbol(OpenGLModuleHandle,"glBindTexture"));
		dglBlendFunc = reinterpret_cast<glBlendFunc_fp>(dll_GetSymbol(OpenGLModuleHandle,"glBlendFunc"));
		dglClear = reinterpret_cast<glClear_fp>(dll_GetSymbol(OpenGLModuleHandle,"glClear"));
		dglClearColor = reinterpret_cast<glClearColor_fp>(dll_GetSymbol(OpenGLModuleHandle,"glClearColor"));
		dglColor4f = reinterpret_cast<glColor4f_fp>(dll_GetSymbol(OpenGLModuleHandle,"glColor4f"));
		dglColorPointer = reinterpret_cast<glColorPointer_fp>(dll_GetSymbol(OpenGLModuleHandle,"glColorPointer"));
		dglCullFace = reinterpret_cast<glCullFace_fp>(dll_GetSymbol(OpenGLModuleHandle,"glCullFace"));
		dglDeleteTextures = reinterpret_cast<glDeleteTextures_fp>(dll_GetSymbol(OpenGLModuleHandle,"glDeleteTextures"));
		dglDepthFunc = reinterpret_cast<glDepthFunc_fp>(dll_GetSymbol(OpenGLModuleHandle,"glDepthFunc"));
		dglDepthMask = reinterpret_cast<glDepthMask_fp>(dll_GetSymbol(OpenGLModuleHandle,"glDepthMask"));
		dglDisable = reinterpret_cast<glDisable_fp>(dll_GetSymbol(OpenGLModuleHandle,"glDisable"));
		dglDisableClientState = reinterpret_cast<glDisableClientState_fp>(dll_GetSymbol(OpenGLModuleHandle,"glDisableClientState"));
		dglDrawArrays = reinterpret_cast<glDrawArrays_fp>(dll_GetSymbol(OpenGLModuleHandle,"glDrawArrays"));
		dglEnable = reinterpret_cast<glEnable_fp>(dll_GetSymbol(OpenGLModuleHandle,"glEnable"));
		dglEnableClientState = reinterpret_cast<glEnableClientState_fp>(dll_GetSymbol(OpenGLModuleHandle,"glEnableClientState"));
		dglEnd = reinterpret_cast<glEnd_fp>(dll_GetSymbol(OpenGLModuleHandle,"glEnd"));
		dglFinish = reinterpret_cast<glFinish_fp>(dll_GetSymbol(OpenGLModuleHandle,"glFinish"));
		dglGenTextures = reinterpret_cast<glGenTextures_fp>(dll_GetSymbol(OpenGLModuleHandle,"glGenTextures"));
		dglGetFloatv = reinterpret_cast<glGetFloatv_fp>(dll_GetSymbol(OpenGLModuleHandle,"glGetFloatv"));
		dglGetIntegerv = reinterpret_cast<glGetIntegerv_fp>(dll_GetSymbol(OpenGLModuleHandle,"glGetIntegerv"));
		dglGetString = reinterpret_cast<glGetString_fp>(dll_GetSymbol(OpenGLModuleHandle,"glGetString"));
		dglGetTexLevelParameteriv = reinterpret_cast<glGetTexLevelParameteriv_fp>(dll_GetSymbol(OpenGLModuleHandle,"glGetTexLevelParameteriv"));
		dglHint = reinterpret_cast<glHint_fp>(dll_GetSymbol(OpenGLModuleHandle,"glHint"));
		dglLineWidth = reinterpret_cast<glLineWidth_fp>(dll_GetSymbol(OpenGLModuleHandle,"glLineWidth"));
		dglLoadIdentity = reinterpret_cast<glLoadIdentity_fp>(dll_GetSymbol(OpenGLModuleHandle,"glLoadIdentity"));
		dglMatrixMode = reinterpret_cast<glMatrixMode_fp>(dll_GetSymbol(OpenGLModuleHandle,"glMatrixMode"));
		dglOrtho = reinterpret_cast<glOrtho_fp>(dll_GetSymbol(OpenGLModuleHandle,"glOrtho"));
		dglPointSize = reinterpret_cast<glPointSize_fp>(dll_GetSymbol(OpenGLModuleHandle,"glPointSize"));
		dglPopMatrix = reinterpret_cast<glPopMatrix_fp>(dll_GetSymbol(OpenGLModuleHandle,"glPopMatrix"));
		dglPrioritizeTextures = reinterpret_cast<glPrioritizeTextures_fp>(dll_GetSymbol(OpenGLModuleHandle,"glPrioritizeTextures"));
		dglPushMatrix = reinterpret_cast<glPushMatrix_fp>(dll_GetSymbol(OpenGLModuleHandle,"glPushMatrix"));
		dglReadBuffer = reinterpret_cast<glReadBuffer_fp>(dll_GetSymbol(OpenGLModuleHandle,"glReadBuffer"));
		dglReadPixels = reinterpret_cast<glReadPixels_fp>(dll_GetSymbol(OpenGLModuleHandle,"glReadPixels"));
		dglScalef = reinterpret_cast<glScalef_fp>(dll_GetSymbol(OpenGLModuleHandle,"glScalef"));
		dglShadeModel = reinterpret_cast<glShadeModel_fp>(dll_GetSymbol(OpenGLModuleHandle,"glShadeModel"));
		dglTexCoordPointer = reinterpret_cast<glTexCoordPointer_fp>(dll_GetSymbol(OpenGLModuleHandle,"glTexCoordPointer"));
		dglTexEnvi = reinterpret_cast<glTexEnvi_fp>(dll_GetSymbol(OpenGLModuleHandle,"glTexEnvi"));
		dglTexImage2D = reinterpret_cast<glTexImage2D_fp>(dll_GetSymbol(OpenGLModuleHandle,"glTexImage2D"));
		dglTexParameterf = reinterpret_cast<glTexParameterf_fp>(dll_GetSymbol(OpenGLModuleHandle,"glTexParameterf"));
		dglTexParameteri = reinterpret_cast<glTexParameteri_fp>(dll_GetSymbol(OpenGLModuleHandle,"glTexParameteri"));
		dglTranslatef = reinterpret_cast<glTranslatef_fp>(dll_GetSymbol(OpenGLModuleHandle,"glTranslatef"));
		dglVertexPointer = reinterpret_cast<glVertexPointer_fp>(dll_GetSymbol(OpenGLModuleHandle,"glVertexPointer"));
		dglViewport = reinterpret_cast<glViewport_fp>(dll_GetSymbol(OpenGLModuleHandle,"glViewport"));
	}

	return true;
}

static void OpenGL_SetFuncsToNull(void)
{
	dglAlphaFunc = NULL;
	dglBindTexture = NULL;
	dglBlendFunc = NULL;
	dglClear = NULL;
	dglClearColor = NULL;
	dglColor4f = NULL;
	dglColorPointer = NULL;
	dglCullFace = NULL;
	dglDeleteTextures = NULL;
	dglDepthFunc = NULL;
	dglDepthMask = NULL;
	dglDisable = NULL;
	dglDisableClientState = NULL;
	dglDrawArrays = NULL;
	dglEnable = NULL;
	dglEnableClientState = NULL;
	dglEnd = NULL;
	dglFinish = NULL;
	dglGenTextures = NULL;
	dglGetFloatv = NULL;
	dglGetIntegerv = NULL;
	dglGetString = NULL;
	dglGetTexLevelParameteriv = NULL;
	dglHint = NULL;
	dglLineWidth = NULL;
	dglLoadIdentity = NULL;
	dglMatrixMode = NULL;
	dglOrtho = NULL;
	dglPointSize = NULL;
	dglPopMatrix = NULL;
	dglPrioritizeTextures = NULL;
	dglPushMatrix = NULL;
	dglReadBuffer = NULL;
	dglReadPixels = NULL;
	dglScalef = NULL;
	dglShadeModel = NULL;
	dglTexCoordPointer = NULL;
	dglTexEnvi = NULL;
	dglTexImage2D = NULL;
	dglTexParameterf = NULL;
	dglTexParameteri = NULL;
	dglTranslatef = NULL;
	dglVertexPointer = NULL;
	dglViewport = NULL;
}
#endif


#endif //!__LOADGL_H__
