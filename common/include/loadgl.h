/*
 *
 * dynamic opengl loading - courtesy of Jeff Slutter
 *
 *
 */

#ifndef __LOADGL_H__
#define __LOADGL_H__

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

#ifdef _cplusplus
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

#define glAccum dglAccum
#define glAlphaFunc dglAlphaFunc
#define glAreTexturesResident dglAreTexturesResident
#define glArrayElement dglArrayElement
#define glBegin dglBegin
#define glBindTexture dglBindTexture
#define glBitmap dglBitmap
#define glBlendFunc dglBlendFunc
#define glCallList dglCallList
#define glCallLists dglCallLists
#define glClear dglClear
#define glClearAccum dglClearAccum
#define glClearColor dglClearColor
#define glClearDepth dglClearDepth
#define glClearIndex dglClearIndex
#define glClearStencil dglClearStencil
#define glClipPlane dglClipPlane
#define glColor3b dglColor3b
#define glColor3bv dglColor3bv
#define glColor3d dglColor3d
#define glColor3dv dglColor3dv
#define glColor3f dglColor3f
#define glColor3fv dglColor3fv
#define glColor3i dglColor3i
#define glColor3iv dglColor3iv
#define glColor3s dglColor3s
#define glColor3sv dglColor3sv
#define glColor3ub dglColor3ub
#define glColor3ubv dglColor3ubv
#define glColor3ui dglColor3ui
#define glColor3uiv dglColor3uiv
#define glColor3us dglColor3us
#define glColor3usv dglColor3usv
#define glColor4b dglColor4b
#define glColor4bv dglColor4bv
#define glColor4d dglColor4d
#define glColor4dv dglColor4dv
#define glColor4f dglColor4f
#define glColor4fv dglColor4fv
#define glColor4i dglColor4i
#define glColor4iv dglColor4iv
#define glColor4s dglColor4s
#define glColor4sv dglColor4sv
#define glColor4ub dglColor4ub
#define glColor4ubv dglColor4ubv
#define glColor4ui dglColor4ui
#define glColor4uiv dglColor4uiv
#define glColor4us dglColor4us
#define glColor4usv dglColor4usv
#define glColorMask dglColorMask
#define glColorMaterial dglColorMaterial
#define glColorPointer dglColorPointer
#define glCopyPixels dglCopyPixels
#define glCopyTexImage1D dglCopyTexImage1D
#define glCopyTexImage2D dglCopyTexImage2D
#define glCopyTexSubImage1D dglCopyTexSubImage1D
#define glCopyTexSubImage2D dglCopyTexSubImage2D
#define glCullFace dglCullFace
#define glDeleteLists dglDeleteLists
#define glDeleteTextures dglDeleteTextures
#define glDepthFunc dglDepthFunc
#define glDepthMask dglDepthMask
#define glDepthRange dglDepthRange
#define glDisable dglDisable
#define glDisableClientState dglDisableClientState
#define glDrawArrays dglDrawArrays
#define glDrawBuffer dglDrawBuffer
#define glDrawElements dglDrawElements
#define glDrawPixels dglDrawPixels
#define glEdgeFlag dglEdgeFlag
#define glEdgeFlagPointer dglEdgeFlagPointer
#define glEdgeFlagv dglEdgeFlagv
#define glEnable dglEnable
#define glEnableClientState dglEnableClientState
#define glEnd dglEnd
#define glEndList dglEndList
#define glEvalCoord1d dglEvalCoord1d
#define glEvalCoord1dv dglEvalCoord1dv
#define glEvalCoord1f dglEvalCoord1f
#define glEvalCoord1fv dglEvalCoord1fv
#define glEvalCoord2d dglEvalCoord2d
#define glEvalCoord2dv dglEvalCoord2dv
#define glEvalCoord2f dglEvalCoord2f
#define glEvalCoord2fv dglEvalCoord2fv
#define glEvalMesh1 dglEvalMesh1
#define glEvalMesh2 dglEvalMesh2
#define glEvalPoint1 dglEvalPoint1
#define glEvalPoint2 dglEvalPoint2
#define glFeedbackBuffer dglFeedbackBuffer
#define glFinish dglFinish
#define glFlush dglFlush
#define glFogf dglFogf
#define glFogfv dglFogfv
#define glFogi dglFogi
#define glFogiv dglFogiv
#define glFrontFace dglFrontFace
#define glFrustum dglFrustum
#define glGenLists dglGenLists
#define glGenTextures dglGenTextures
#define glGetBooleanv dglGetBooleanv
#define glGetClipPlane dglGetClipPlane
#define glGetDoublev dglGetDoublev
#define glGetError dglGetError
#define glGetFloatv dglGetFloatv
#define glGetIntegerv dglGetIntegerv
#define glGetLightfv dglGetLightfv
#define glGetLightiv dglGetLightiv
#define glGetMapdv dglGetMapdv
#define glGetMapfv dglGetMapfv
#define glGetMapiv dglGetMapiv
#define glGetMaterialfv dglGetMaterialfv
#define glGetMaterialiv dglGetMaterialiv
#define glGetPixelMapfv dglGetPixelMapfv
#define glGetPixelMapuiv dglGetPixelMapuiv
#define glGetPixelMapusv dglGetPixelMapusv
#define glGetPointerv dglGetPointerv
#define glGetPolygonStipple dglGetPolygonStipple
#define glGetString dglGetString
#define glGetTexEnvfv dglGetTexEnvfv
#define glGetTexEnviv dglGetTexEnviv
#define glGetTexGendv dglGetTexGendv
#define glGetTexGenfv dglGetTexGenfv
#define glGetTexGeniv dglGetTexGeniv
#define glGetTexImage dglGetTexImage
#define glGetTexLevelParameterfv dglGetTexLevelParameterfv
#define glGetTexLevelParameteriv dglGetTexLevelParameteriv
#define glGetTexParameterfv dglGetTexParameterfv
#define glGetTexParameteriv dglGetTexParameteriv
#define glHint dglHint
#define glIndexMask dglIndexMask
#define glIndexPointer dglIndexPointer
#define glIndexd dglIndexd
#define glIndexdv dglIndexdv
#define glIndexf dglIndexf
#define glIndexfv dglIndexfv
#define glIndexi dglIndexi
#define glIndexiv dglIndexiv
#define glIndexs dglIndexs
#define glIndexsv dglIndexsv
#define glIndexub dglIndexub
#define glIndexubv dglIndexubv
#define glInitNames dglInitNames
#define glInterleavedArrays dglInterleavedArrays
#define glIsEnabled dglIsEnabled
#define glIsList dglIsList
#define glIsTexture dglIsTexture
#define glLightModelf dglLightModelf
#define glLightModelfv dglLightModelfv
#define glLightModeli dglLightModeli
#define glLightModeliv dglLightModeliv
#define glLightf dglLightf
#define glLightfv dglLightfv
#define glLighti dglLighti
#define glLightiv dglLightiv
#define glLineStipple dglLineStipple
#define glLineWidth dglLineWidth
#define glListBase dglListBase
#define glLoadIdentity dglLoadIdentity
#define glLoadMatrixd dglLoadMatrixd
#define glLoadMatrixf dglLoadMatrixf
#define glLoadName dglLoadName
#define glLogicOp dglLogicOp
#define glMap1d dglMap1d
#define glMap1f dglMap1f
#define glMap2d dglMap2d
#define glMap2f dglMap2f
#define glMapGrid1d dglMapGrid1d
#define glMapGrid1f dglMapGrid1f
#define glMapGrid2d dglMapGrid2d
#define glMapGrid2f dglMapGrid2f
#define glMaterialf dglMaterialf
#define glMaterialfv dglMaterialfv
#define glMateriali dglMateriali
#define glMaterialiv dglMaterialiv
#define glMatrixMode dglMatrixMode
#define glMultMatrixd dglMultMatrixd
#define glMultMatrixf dglMultMatrixf
#define glNewList dglNewList
#define glNormal3b dglNormal3b
#define glNormal3bv dglNormal3bv
#define glNormal3d dglNormal3d
#define glNormal3dv dglNormal3dv
#define glNormal3f dglNormal3f
#define glNormal3fv dglNormal3fv
#define glNormal3i dglNormal3i
#define glNormal3iv dglNormal3iv
#define glNormal3s dglNormal3s
#define glNormal3sv dglNormal3sv
#define glNormalPointer dglNormalPointer
#define glOrtho dglOrtho
#define glPassThrough dglPassThrough
#define glPixelMapfv dglPixelMapfv
#define glPixelMapuiv dglPixelMapuiv
#define glPixelMapusv dglPixelMapusv
#define glPixelStoref dglPixelStoref
#define glPixelStorei dglPixelStorei
#define glPixelTransferf dglPixelTransferf
#define glPixelTransferi dglPixelTransferi
#define glPixelZoom dglPixelZoom
#define glPointSize dglPointSize
#define glPolygonMode dglPolygonMode
#define glPolygonOffset dglPolygonOffset
#define glPolygonStipple dglPolygonStipple
#define glPopAttrib dglPopAttrib
#define glPopClientAttrib dglPopClientAttrib
#define glPopMatrix dglPopMatrix
#define glPopName dglPopName
#define glPrioritizeTextures dglPrioritizeTextures
#define glPushAttrib dglPushAttrib
#define glPushClientAttrib dglPushClientAttrib
#define glPushMatrix dglPushMatrix
#define glPushName dglPushName
#define glRasterPos2d dglRasterPos2d
#define glRasterPos2dv dglRasterPos2dv
#define glRasterPos2f dglRasterPos2f
#define glRasterPos2fv dglRasterPos2fv
#define glRasterPos2i dglRasterPos2i
#define glRasterPos2iv dglRasterPos2iv
#define glRasterPos2s dglRasterPos2s
#define glRasterPos2sv dglRasterPos2sv
#define glRasterPos3d dglRasterPos3d
#define glRasterPos3dv dglRasterPos3dv
#define glRasterPos3f dglRasterPos3f
#define glRasterPos3fv dglRasterPos3fv
#define glRasterPos3i dglRasterPos3i
#define glRasterPos3iv dglRasterPos3iv
#define glRasterPos3s dglRasterPos3s
#define glRasterPos3sv dglRasterPos3sv
#define glRasterPos4d dglRasterPos4d
#define glRasterPos4dv dglRasterPos4dv
#define glRasterPos4f dglRasterPos4f
#define glRasterPos4fv dglRasterPos4fv
#define glRasterPos4i dglRasterPos4i
#define glRasterPos4iv dglRasterPos4iv
#define glRasterPos4s dglRasterPos4s
#define glRasterPos4sv dglRasterPos4sv
#define glReadBuffer dglReadBuffer
#define glReadPixels dglReadPixels
#define glRectd dglRectd
#define glRectdv dglRectdv
#define glRectf dglRectf
#define glRectfv dglRectfv
#define glRecti dglRecti
#define glRectiv dglRectiv
#define glRects dglRects
#define glRectsv dglRectsv
#define glRenderMode dglRenderMode
#define glRotated dglRotated
#define glRotatef dglRotatef
#define glScaled dglScaled
#define glScalef dglScalef
#define glScissor dglScissor
#define glSelectBuffer dglSelectBuffer
#define glShadeModel dglShadeModel
#define glStencilFunc dglStencilFunc
#define glStencilMask dglStencilMask
#define glStencilOp dglStencilOp
#define glTexCoord1d dglTexCoord1d
#define glTexCoord1dv dglTexCoord1dv
#define glTexCoord1f dglTexCoord1f
#define glTexCoord1fv dglTexCoord1fv
#define glTexCoord1i dglTexCoord1i
#define glTexCoord1iv dglTexCoord1iv
#define glTexCoord1s dglTexCoord1s
#define glTexCoord1sv dglTexCoord1sv
#define glTexCoord2d dglTexCoord2d
#define glTexCoord2dv dglTexCoord2dv
#define glTexCoord2f dglTexCoord2f
#define glTexCoord2fv dglTexCoord2fv
#define glTexCoord2i dglTexCoord2i
#define glTexCoord2iv dglTexCoord2iv
#define glTexCoord2s dglTexCoord2s
#define glTexCoord2sv dglTexCoord2sv
#define glTexCoord3d dglTexCoord3d
#define glTexCoord3dv dglTexCoord3dv
#define glTexCoord3f dglTexCoord3f
#define glTexCoord3fv dglTexCoord3fv
#define glTexCoord3i dglTexCoord3i
#define glTexCoord3iv dglTexCoord3iv
#define glTexCoord3s dglTexCoord3s
#define glTexCoord3sv dglTexCoord3sv
#define glTexCoord4d dglTexCoord4d
#define glTexCoord4dv dglTexCoord4dv
#define glTexCoord4f dglTexCoord4f
#define glTexCoord4fv dglTexCoord4fv
#define glTexCoord4i dglTexCoord4i
#define glTexCoord4iv dglTexCoord4iv
#define glTexCoord4s dglTexCoord4s
#define glTexCoord4sv dglTexCoord4sv
#define glTexCoordPointer dglTexCoordPointer
#define glTexEnvf dglTexEnvf
#define glTexEnvfv dglTexEnvfv
#define glTexEnvi dglTexEnvi
#define glTexEnviv dglTexEnviv
#define glTexGend dglTexGend
#define glTexGendv dglTexGendv
#define glTexGenf dglTexGenf
#define glTexGenfv dglTexGenfv
#define glTexGeni dglTexGeni
#define glTexGeniv dglTexGeniv
#define glTexImage1D dglTexImage1D
#define glTexImage2D dglTexImage2D
#define glTexParameterf dglTexParameterf
#define glTexParameterfv dglTexParameterfv
#define glTexParameteri dglTexParameteri
#define glTexParameteriv dglTexParameteriv
#define glTexSubImage1D dglTexSubImage1D
#define glTexSubImage2D dglTexSubImage2D
#define glTranslated dglTranslated
#define glTranslatef dglTranslatef
#define glVertex2d dglVertex2d
#define glVertex2dv dglVertex2dv
#define glVertex2f dglVertex2f
#define glVertex2fv dglVertex2fv
#define glVertex2i dglVertex2i
#define glVertex2iv dglVertex2iv
#define glVertex2s dglVertex2s
#define glVertex2sv dglVertex2sv
#define glVertex3d dglVertex3d
#define glVertex3dv dglVertex3dv
#define glVertex3f dglVertex3f
#define glVertex3fv dglVertex3fv
#define glVertex3i dglVertex3i
#define glVertex3iv dglVertex3iv
#define glVertex3s dglVertex3s
#define glVertex3sv dglVertex3sv
#define glVertex4d dglVertex4d
#define glVertex4dv dglVertex4dv
#define glVertex4f dglVertex4f
#define glVertex4fv dglVertex4fv
#define glVertex4i dglVertex4i
#define glVertex4iv dglVertex4iv
#define glVertex4s dglVertex4s
#define glVertex4sv dglVertex4sv
#define glVertexPointer dglVertexPointer
#define glViewport dglViewport

#define glMultiTexCoord2fARB dglMultiTexCoord2fARB
#define glActiveTextureARB dglActiveTextureARB
#define glMultiTexCoord2fSGIS dglMultiTexCoord2fSGIS
#define glSelectTextureSGIS dglSelectTextureSGIS

#define glColorTableEXT dglColorTableEXT

#define glCombinerParameteriNV dglCombinerParameteriNV
#define glCombinerInputNV dglCombinerInputNV
#define glCombinerOutputNV dglCombinerOutputNV
#define glFinalCombinerInputNV dglFinalCombinerInputNV

#ifdef _WIN32
#define wglCopyContext dwglCopyContext
#define wglCreateContext dwglCreateContext
#define wglCreateLayerContext dwglCreateLayerContext
#define wglDeleteContext dwglDeleteContext
#define wglGetCurrentContext dwglGetCurrentContext
#define wglGetCurrentDC dwglGetCurrentDC
#define wglGetProcAddress dwglGetProcAddress
#define wglMakeCurrent dwglMakeCurrent
#define wglShareLists dwglShareLists
#define wglUseFontBitmapsA dwglUseFontBitmapsA
#define wglUseFontBitmapsW dwglUseFontBitmapsW
#define wglUseFontOutlinesA dwglUseFontOutlinesA
#define wglUseFontOutlinesW dwglUseFontOutlinesW
#define wglDescribeLayerPlane dwglDescribeLayerPlane
#define wglSetLayerPaletteEntries dwglSetLayerPaletteEntries
#define wglGetLayerPaletteEntries dwglGetLayerPaletteEntries
#define wglRealizeLayerPalette dwglRealizeLayerPalette
#define wglSwapLayerBuffers dwglSwapLayerBuffers
#if (WINVER >= 0x0500)
#define wglSwapMultipleBuffers dwglSwapMultipleBuffers
#endif
#endif

typedef void (OGLFUNCCALL *glAccum_fp)(GLenum op, GLfloat value);
typedef void (OGLFUNCCALL *glAlphaFunc_fp)(GLenum func, GLclampf ref);
typedef GLboolean (OGLFUNCCALL *glAreTexturesResident_fp)(GLsizei n, const GLuint *textures, GLboolean *residences);
typedef void (OGLFUNCCALL *glArrayElement_fp)(GLint i);
typedef void (OGLFUNCCALL *glBegin_fp)(GLenum mode);
typedef void (OGLFUNCCALL *glBindTexture_fp)(GLenum target, GLuint texture);
typedef void (OGLFUNCCALL *glBitmap_fp)(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap);
typedef void (OGLFUNCCALL *glBlendFunc_fp)(GLenum sfactor, GLenum dfactor);
typedef void (OGLFUNCCALL *glCallList_fp)(GLuint list);
typedef void (OGLFUNCCALL *glCallLists_fp)(GLsizei n, GLenum type, const GLvoid *lists);
typedef void (OGLFUNCCALL *glClear_fp)(GLbitfield mask);
typedef void (OGLFUNCCALL *glClearAccum_fp)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
typedef void (OGLFUNCCALL *glClearColor_fp)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
typedef void (OGLFUNCCALL *glClearDepth_fp)(GLclampd depth);
typedef void (OGLFUNCCALL *glClearIndex_fp)(GLfloat c);
typedef void (OGLFUNCCALL *glClearStencil_fp)(GLint s);
typedef void (OGLFUNCCALL *glClipPlane_fp)(GLenum plane, const GLdouble *equation);
typedef void (OGLFUNCCALL *glColor3b_fp)(GLbyte red, GLbyte green, GLbyte blue);
typedef void (OGLFUNCCALL *glColor3bv_fp)(const GLbyte *v);
typedef void (OGLFUNCCALL *glColor3d_fp)(GLdouble red, GLdouble green, GLdouble blue);
typedef void (OGLFUNCCALL *glColor3dv_fp)(const GLdouble *v);
typedef void (OGLFUNCCALL *glColor3f_fp)(GLfloat red, GLfloat green, GLfloat blue);
typedef void (OGLFUNCCALL *glColor3fv_fp)(const GLfloat *v);
typedef void (OGLFUNCCALL *glColor3i_fp)(GLint red, GLint green, GLint blue);
typedef void (OGLFUNCCALL *glColor3iv_fp)(const GLint *v);
typedef void (OGLFUNCCALL *glColor3s_fp)(GLshort red, GLshort green, GLshort blue);
typedef void (OGLFUNCCALL *glColor3sv_fp)(const GLshort *v);
typedef void (OGLFUNCCALL *glColor3ub_fp)(GLubyte red, GLubyte green, GLubyte blue);
typedef void (OGLFUNCCALL *glColor3ubv_fp)(const GLubyte *v);
typedef void (OGLFUNCCALL *glColor3ui_fp)(GLuint red, GLuint green, GLuint blue);
typedef void (OGLFUNCCALL *glColor3uiv_fp)(const GLuint *v);
typedef void (OGLFUNCCALL *glColor3us_fp)(GLushort red, GLushort green, GLushort blue);
typedef void (OGLFUNCCALL *glColor3usv_fp)(const GLushort *v);
typedef void (OGLFUNCCALL *glColor4b_fp)(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha);
typedef void (OGLFUNCCALL *glColor4bv_fp)(const GLbyte *v);
typedef void (OGLFUNCCALL *glColor4d_fp)(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha);
typedef void (OGLFUNCCALL *glColor4dv_fp)(const GLdouble *v);
typedef void (OGLFUNCCALL *glColor4f_fp)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
typedef void (OGLFUNCCALL *glColor4fv_fp)(const GLfloat *v);
typedef void (OGLFUNCCALL *glColor4i_fp)(GLint red, GLint green, GLint blue, GLint alpha);
typedef void (OGLFUNCCALL *glColor4iv_fp)(const GLint *v);
typedef void (OGLFUNCCALL *glColor4s_fp)(GLshort red, GLshort green, GLshort blue, GLshort alpha);
typedef void (OGLFUNCCALL *glColor4sv_fp)(const GLshort *v);
typedef void (OGLFUNCCALL *glColor4ub_fp)(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
typedef void (OGLFUNCCALL *glColor4ubv_fp)(const GLubyte *v);
typedef void (OGLFUNCCALL *glColor4ui_fp)(GLuint red, GLuint green, GLuint blue, GLuint alpha);
typedef void (OGLFUNCCALL *glColor4uiv_fp)(const GLuint *v);
typedef void (OGLFUNCCALL *glColor4us_fp)(GLushort red, GLushort green, GLushort blue, GLushort alpha);
typedef void (OGLFUNCCALL *glColor4usv_fp)(const GLushort *v);
typedef void (OGLFUNCCALL *glColorMask_fp)(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
typedef void (OGLFUNCCALL *glColorMaterial_fp)(GLenum face, GLenum mode);
typedef void (OGLFUNCCALL *glColorPointer_fp)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
typedef void (OGLFUNCCALL *glCopyPixels_fp)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type);
typedef void (OGLFUNCCALL *glCopyTexImage1D_fp)(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border);
typedef void (OGLFUNCCALL *glCopyTexImage2D_fp)(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
typedef void (OGLFUNCCALL *glCopyTexSubImage1D_fp)(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width);
typedef void (OGLFUNCCALL *glCopyTexSubImage2D_fp)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
typedef void (OGLFUNCCALL *glCullFace_fp)(GLenum mode);
typedef void (OGLFUNCCALL *glDeleteLists_fp)(GLuint list, GLsizei range);
typedef void (OGLFUNCCALL *glDeleteTextures_fp)(GLsizei n, const GLuint *textures);
typedef void (OGLFUNCCALL *glDepthFunc_fp)(GLenum func);
typedef void (OGLFUNCCALL *glDepthMask_fp)(GLboolean flag);
typedef void (OGLFUNCCALL *glDepthRange_fp)(GLclampd zNear, GLclampd zFar);
typedef void (OGLFUNCCALL *glDisable_fp)(GLenum cap);
typedef void (OGLFUNCCALL *glDisableClientState_fp)(GLenum array);
typedef void (OGLFUNCCALL *glDrawArrays_fp)(GLenum mode, GLint first, GLsizei count);
typedef void (OGLFUNCCALL *glDrawBuffer_fp)(GLenum mode);
typedef void (OGLFUNCCALL *glDrawElements_fp)(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
typedef void (OGLFUNCCALL *glDrawPixels_fp)(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
typedef void (OGLFUNCCALL *glEdgeFlag_fp)(GLboolean flag);
typedef void (OGLFUNCCALL *glEdgeFlagPointer_fp)(GLsizei stride, const GLvoid *pointer);
typedef void (OGLFUNCCALL *glEdgeFlagv_fp)(const GLboolean *flag);
typedef void (OGLFUNCCALL *glEnable_fp)(GLenum cap);
typedef void (OGLFUNCCALL *glEnableClientState_fp)(GLenum array);
typedef void (OGLFUNCCALL *glEnd_fp)(void);
typedef void (OGLFUNCCALL *glEndList_fp)(void);
typedef void (OGLFUNCCALL *glEvalCoord1d_fp)(GLdouble u);
typedef void (OGLFUNCCALL *glEvalCoord1dv_fp)(const GLdouble *u);
typedef void (OGLFUNCCALL *glEvalCoord1f_fp)(GLfloat u);
typedef void (OGLFUNCCALL *glEvalCoord1fv_fp)(const GLfloat *u);
typedef void (OGLFUNCCALL *glEvalCoord2d_fp)(GLdouble u, GLdouble v);
typedef void (OGLFUNCCALL *glEvalCoord2dv_fp)(const GLdouble *u);
typedef void (OGLFUNCCALL *glEvalCoord2f_fp)(GLfloat u, GLfloat v);
typedef void (OGLFUNCCALL *glEvalCoord2fv_fp)(const GLfloat *u);
typedef void (OGLFUNCCALL *glEvalMesh1_fp)(GLenum mode, GLint i1, GLint i2);
typedef void (OGLFUNCCALL *glEvalMesh2_fp)(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2);
typedef void (OGLFUNCCALL *glEvalPoint1_fp)(GLint i);
typedef void (OGLFUNCCALL *glEvalPoint2_fp)(GLint i, GLint j);
typedef void (OGLFUNCCALL *glFeedbackBuffer_fp)(GLsizei size, GLenum type, GLfloat *buffer);
typedef void (OGLFUNCCALL *glFinish_fp)(void);
typedef void (OGLFUNCCALL *glFlush_fp)(void);
typedef void (OGLFUNCCALL *glFogf_fp)(GLenum pname, GLfloat param);
typedef void (OGLFUNCCALL *glFogfv_fp)(GLenum pname, const GLfloat *params);
typedef void (OGLFUNCCALL *glFogi_fp)(GLenum pname, GLint param);
typedef void (OGLFUNCCALL *glFogiv_fp)(GLenum pname, const GLint *params);
typedef void (OGLFUNCCALL *glFrontFace_fp)(GLenum mode);
typedef void (OGLFUNCCALL *glFrustum_fp)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
typedef GLuint (OGLFUNCCALL *glGenLists_fp)(GLsizei range);
typedef void (OGLFUNCCALL *glGenTextures_fp)(GLsizei n, GLuint *textures);
typedef void (OGLFUNCCALL *glGetBooleanv_fp)(GLenum pname, GLboolean *params);
typedef void (OGLFUNCCALL *glGetClipPlane_fp)(GLenum plane, GLdouble *equation);
typedef void (OGLFUNCCALL *glGetDoublev_fp)(GLenum pname, GLdouble *params);
typedef GLenum (OGLFUNCCALL *glGetError_fp)(void);
typedef void (OGLFUNCCALL *glGetFloatv_fp)(GLenum pname, GLfloat *params);
typedef void (OGLFUNCCALL *glGetIntegerv_fp)(GLenum pname, GLint *params);
typedef void (OGLFUNCCALL *glGetLightfv_fp)(GLenum light, GLenum pname, GLfloat *params);
typedef void (OGLFUNCCALL *glGetLightiv_fp)(GLenum light, GLenum pname, GLint *params);
typedef void (OGLFUNCCALL *glGetMapdv_fp)(GLenum target, GLenum query, GLdouble *v);
typedef void (OGLFUNCCALL *glGetMapfv_fp)(GLenum target, GLenum query, GLfloat *v);
typedef void (OGLFUNCCALL *glGetMapiv_fp)(GLenum target, GLenum query, GLint *v);
typedef void (OGLFUNCCALL *glGetMaterialfv_fp)(GLenum face, GLenum pname, GLfloat *params);
typedef void (OGLFUNCCALL *glGetMaterialiv_fp)(GLenum face, GLenum pname, GLint *params);
typedef void (OGLFUNCCALL *glGetPixelMapfv_fp)(GLenum map, GLfloat *values);
typedef void (OGLFUNCCALL *glGetPixelMapuiv_fp)(GLenum map, GLuint *values);
typedef void (OGLFUNCCALL *glGetPixelMapusv_fp)(GLenum map, GLushort *values);
typedef void (OGLFUNCCALL *glGetPointerv_fp)(GLenum pname, GLvoid* *params);
typedef void (OGLFUNCCALL *glGetPolygonStipple_fp)(GLubyte *mask);
typedef const GLubyte *(OGLFUNCCALL *glGetString_fp)(GLenum name);
typedef void (OGLFUNCCALL *glGetTexEnvfv_fp)(GLenum target, GLenum pname, GLfloat *params);
typedef void (OGLFUNCCALL *glGetTexEnviv_fp)(GLenum target, GLenum pname, GLint *params);
typedef void (OGLFUNCCALL *glGetTexGendv_fp)(GLenum coord, GLenum pname, GLdouble *params);
typedef void (OGLFUNCCALL *glGetTexGenfv_fp)(GLenum coord, GLenum pname, GLfloat *params);
typedef void (OGLFUNCCALL *glGetTexGeniv_fp)(GLenum coord, GLenum pname, GLint *params);
typedef void (OGLFUNCCALL *glGetTexImage_fp)(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels);
typedef void (OGLFUNCCALL *glGetTexLevelParameterfv_fp)(GLenum target, GLint level, GLenum pname, GLfloat *params);
typedef void (OGLFUNCCALL *glGetTexLevelParameteriv_fp)(GLenum target, GLint level, GLenum pname, GLint *params);
typedef void (OGLFUNCCALL *glGetTexParameterfv_fp)(GLenum target, GLenum pname, GLfloat *params);
typedef void (OGLFUNCCALL *glGetTexParameteriv_fp)(GLenum target, GLenum pname, GLint *params);
typedef void (OGLFUNCCALL *glHint_fp)(GLenum target, GLenum mode);
typedef void (OGLFUNCCALL *glIndexMask_fp)(GLuint mask);
typedef void (OGLFUNCCALL *glIndexPointer_fp)(GLenum type, GLsizei stride, const GLvoid *pointer);
typedef void (OGLFUNCCALL *glIndexd_fp)(GLdouble c);
typedef void (OGLFUNCCALL *glIndexdv_fp)(const GLdouble *c);
typedef void (OGLFUNCCALL *glIndexf_fp)(GLfloat c);
typedef void (OGLFUNCCALL *glIndexfv_fp)(const GLfloat *c);
typedef void (OGLFUNCCALL *glIndexi_fp)(GLint c);
typedef void (OGLFUNCCALL *glIndexiv_fp)(const GLint *c);
typedef void (OGLFUNCCALL *glIndexs_fp)(GLshort c);
typedef void (OGLFUNCCALL *glIndexsv_fp)(const GLshort *c);
typedef void (OGLFUNCCALL *glIndexub_fp)(GLubyte c);
typedef void (OGLFUNCCALL *glIndexubv_fp)(const GLubyte *c);
typedef void (OGLFUNCCALL *glInitNames_fp)(void);
typedef void (OGLFUNCCALL *glInterleavedArrays_fp)(GLenum format, GLsizei stride, const GLvoid *pointer);
typedef GLboolean (OGLFUNCCALL *glIsEnabled_fp)(GLenum cap);
typedef GLboolean (OGLFUNCCALL *glIsList_fp)(GLuint list);
typedef GLboolean (OGLFUNCCALL *glIsTexture_fp)(GLuint texture);
typedef void (OGLFUNCCALL *glLightModelf_fp)(GLenum pname, GLfloat param);
typedef void (OGLFUNCCALL *glLightModelfv_fp)(GLenum pname, const GLfloat *params);
typedef void (OGLFUNCCALL *glLightModeli_fp)(GLenum pname, GLint param);
typedef void (OGLFUNCCALL *glLightModeliv_fp)(GLenum pname, const GLint *params);
typedef void (OGLFUNCCALL *glLightf_fp)(GLenum light, GLenum pname, GLfloat param);
typedef void (OGLFUNCCALL *glLightfv_fp)(GLenum light, GLenum pname, const GLfloat *params);
typedef void (OGLFUNCCALL *glLighti_fp)(GLenum light, GLenum pname, GLint param);
typedef void (OGLFUNCCALL *glLightiv_fp)(GLenum light, GLenum pname, const GLint *params);
typedef void (OGLFUNCCALL *glLineStipple_fp)(GLint factor, GLushort pattern);
typedef void (OGLFUNCCALL *glLineWidth_fp)(GLfloat width);
typedef void (OGLFUNCCALL *glListBase_fp)(GLuint base);
typedef void (OGLFUNCCALL *glLoadIdentity_fp)(void);
typedef void (OGLFUNCCALL *glLoadMatrixd_fp)(const GLdouble *m);
typedef void (OGLFUNCCALL *glLoadMatrixf_fp)(const GLfloat *m);
typedef void (OGLFUNCCALL *glLoadName_fp)(GLuint name);
typedef void (OGLFUNCCALL *glLogicOp_fp)(GLenum opcode);
typedef void (OGLFUNCCALL *glMap1d_fp)(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points);
typedef void (OGLFUNCCALL *glMap1f_fp)(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points);
typedef void (OGLFUNCCALL *glMap2d_fp)(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points);
typedef void (OGLFUNCCALL *glMap2f_fp)(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points);
typedef void (OGLFUNCCALL *glMapGrid1d_fp)(GLint un, GLdouble u1, GLdouble u2);
typedef void (OGLFUNCCALL *glMapGrid1f_fp)(GLint un, GLfloat u1, GLfloat u2);
typedef void (OGLFUNCCALL *glMapGrid2d_fp)(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2);
typedef void (OGLFUNCCALL *glMapGrid2f_fp)(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2);
typedef void (OGLFUNCCALL *glMaterialf_fp)(GLenum face, GLenum pname, GLfloat param);
typedef void (OGLFUNCCALL *glMaterialfv_fp)(GLenum face, GLenum pname, const GLfloat *params);
typedef void (OGLFUNCCALL *glMateriali_fp)(GLenum face, GLenum pname, GLint param);
typedef void (OGLFUNCCALL *glMaterialiv_fp)(GLenum face, GLenum pname, const GLint *params);
typedef void (OGLFUNCCALL *glMatrixMode_fp)(GLenum mode);
typedef void (OGLFUNCCALL *glMultMatrixd_fp)(const GLdouble *m);
typedef void (OGLFUNCCALL *glMultMatrixf_fp)(const GLfloat *m);
typedef void (OGLFUNCCALL *glNewList_fp)(GLuint list, GLenum mode);
typedef void (OGLFUNCCALL *glNormal3b_fp)(GLbyte nx, GLbyte ny, GLbyte nz);
typedef void (OGLFUNCCALL *glNormal3bv_fp)(const GLbyte *v);
typedef void (OGLFUNCCALL *glNormal3d_fp)(GLdouble nx, GLdouble ny, GLdouble nz);
typedef void (OGLFUNCCALL *glNormal3dv_fp)(const GLdouble *v);
typedef void (OGLFUNCCALL *glNormal3f_fp)(GLfloat nx, GLfloat ny, GLfloat nz);
typedef void (OGLFUNCCALL *glNormal3fv_fp)(const GLfloat *v);
typedef void (OGLFUNCCALL *glNormal3i_fp)(GLint nx, GLint ny, GLint nz);
typedef void (OGLFUNCCALL *glNormal3iv_fp)(const GLint *v);
typedef void (OGLFUNCCALL *glNormal3s_fp)(GLshort nx, GLshort ny, GLshort nz);
typedef void (OGLFUNCCALL *glNormal3sv_fp)(const GLshort *v);
typedef void (OGLFUNCCALL *glNormalPointer_fp)(GLenum type, GLsizei stride, const GLvoid *pointer);
typedef void (OGLFUNCCALL *glOrtho_fp)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
typedef void (OGLFUNCCALL *glPassThrough_fp)(GLfloat token);
typedef void (OGLFUNCCALL *glPixelMapfv_fp)(GLenum map, GLsizei mapsize, const GLfloat *values);
typedef void (OGLFUNCCALL *glPixelMapuiv_fp)(GLenum map, GLsizei mapsize, const GLuint *values);
typedef void (OGLFUNCCALL *glPixelMapusv_fp)(GLenum map, GLsizei mapsize, const GLushort *values);
typedef void (OGLFUNCCALL *glPixelStoref_fp)(GLenum pname, GLfloat param);
typedef void (OGLFUNCCALL *glPixelStorei_fp)(GLenum pname, GLint param);
typedef void (OGLFUNCCALL *glPixelTransferf_fp)(GLenum pname, GLfloat param);
typedef void (OGLFUNCCALL *glPixelTransferi_fp)(GLenum pname, GLint param);
typedef void (OGLFUNCCALL *glPixelZoom_fp)(GLfloat xfactor, GLfloat yfactor);
typedef void (OGLFUNCCALL *glPointSize_fp)(GLfloat size);
typedef void (OGLFUNCCALL *glPolygonMode_fp)(GLenum face, GLenum mode);
typedef void (OGLFUNCCALL *glPolygonOffset_fp)(GLfloat factor, GLfloat units);
typedef void (OGLFUNCCALL *glPolygonStipple_fp)(const GLubyte *mask);
typedef void (OGLFUNCCALL *glPopAttrib_fp)(void);
typedef void (OGLFUNCCALL *glPopClientAttrib_fp)(void);
typedef void (OGLFUNCCALL *glPopMatrix_fp)(void);
typedef void (OGLFUNCCALL *glPopName_fp)(void);
typedef void (OGLFUNCCALL *glPrioritizeTextures_fp)(GLsizei n, const GLuint *textures, const GLclampf *priorities);
typedef void (OGLFUNCCALL *glPushAttrib_fp)(GLbitfield mask);
typedef void (OGLFUNCCALL *glPushClientAttrib_fp)(GLbitfield mask);
typedef void (OGLFUNCCALL *glPushMatrix_fp)(void);
typedef void (OGLFUNCCALL *glPushName_fp)(GLuint name);
typedef void (OGLFUNCCALL *glRasterPos2d_fp)(GLdouble x, GLdouble y);
typedef void (OGLFUNCCALL *glRasterPos2dv_fp)(const GLdouble *v);
typedef void (OGLFUNCCALL *glRasterPos2f_fp)(GLfloat x, GLfloat y);
typedef void (OGLFUNCCALL *glRasterPos2fv_fp)(const GLfloat *v);
typedef void (OGLFUNCCALL *glRasterPos2i_fp)(GLint x, GLint y);
typedef void (OGLFUNCCALL *glRasterPos2iv_fp)(const GLint *v);
typedef void (OGLFUNCCALL *glRasterPos2s_fp)(GLshort x, GLshort y);
typedef void (OGLFUNCCALL *glRasterPos2sv_fp)(const GLshort *v);
typedef void (OGLFUNCCALL *glRasterPos3d_fp)(GLdouble x, GLdouble y, GLdouble z);
typedef void (OGLFUNCCALL *glRasterPos3dv_fp)(const GLdouble *v);
typedef void (OGLFUNCCALL *glRasterPos3f_fp)(GLfloat x, GLfloat y, GLfloat z);
typedef void (OGLFUNCCALL *glRasterPos3fv_fp)(const GLfloat *v);
typedef void (OGLFUNCCALL *glRasterPos3i_fp)(GLint x, GLint y, GLint z);
typedef void (OGLFUNCCALL *glRasterPos3iv_fp)(const GLint *v);
typedef void (OGLFUNCCALL *glRasterPos3s_fp)(GLshort x, GLshort y, GLshort z);
typedef void (OGLFUNCCALL *glRasterPos3sv_fp)(const GLshort *v);
typedef void (OGLFUNCCALL *glRasterPos4d_fp)(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
typedef void (OGLFUNCCALL *glRasterPos4dv_fp)(const GLdouble *v);
typedef void (OGLFUNCCALL *glRasterPos4f_fp)(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
typedef void (OGLFUNCCALL *glRasterPos4fv_fp)(const GLfloat *v);
typedef void (OGLFUNCCALL *glRasterPos4i_fp)(GLint x, GLint y, GLint z, GLint w);
typedef void (OGLFUNCCALL *glRasterPos4iv_fp)(const GLint *v);
typedef void (OGLFUNCCALL *glRasterPos4s_fp)(GLshort x, GLshort y, GLshort z, GLshort w);
typedef void (OGLFUNCCALL *glRasterPos4sv_fp)(const GLshort *v);
typedef void (OGLFUNCCALL *glReadBuffer_fp)(GLenum mode);
typedef void (OGLFUNCCALL *glReadPixels_fp)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
typedef void (OGLFUNCCALL *glRectd_fp)(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2);
typedef void (OGLFUNCCALL *glRectdv_fp)(const GLdouble *v1, const GLdouble *v2);
typedef void (OGLFUNCCALL *glRectf_fp)(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2);
typedef void (OGLFUNCCALL *glRectfv_fp)(const GLfloat *v1, const GLfloat *v2);
typedef void (OGLFUNCCALL *glRecti_fp)(GLint x1, GLint y1, GLint x2, GLint y2);
typedef void (OGLFUNCCALL *glRectiv_fp)(const GLint *v1, const GLint *v2);
typedef void (OGLFUNCCALL *glRects_fp)(GLshort x1, GLshort y1, GLshort x2, GLshort y2);
typedef void (OGLFUNCCALL *glRectsv_fp)(const GLshort *v1, const GLshort *v2);
typedef GLint (OGLFUNCCALL *glRenderMode_fp)(GLenum mode);
typedef void (OGLFUNCCALL *glRotated_fp)(GLdouble angle, GLdouble x, GLdouble y, GLdouble z);
typedef void (OGLFUNCCALL *glRotatef_fp)(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
typedef void (OGLFUNCCALL *glScaled_fp)(GLdouble x, GLdouble y, GLdouble z);
typedef void (OGLFUNCCALL *glScalef_fp)(GLfloat x, GLfloat y, GLfloat z);
typedef void (OGLFUNCCALL *glScissor_fp)(GLint x, GLint y, GLsizei width, GLsizei height);
typedef void (OGLFUNCCALL *glSelectBuffer_fp)(GLsizei size, GLuint *buffer);
typedef void (OGLFUNCCALL *glShadeModel_fp)(GLenum mode);
typedef void (OGLFUNCCALL *glStencilFunc_fp)(GLenum func, GLint ref, GLuint mask);
typedef void (OGLFUNCCALL *glStencilMask_fp)(GLuint mask);
typedef void (OGLFUNCCALL *glStencilOp_fp)(GLenum fail, GLenum zfail, GLenum zpass);
typedef void (OGLFUNCCALL *glTexCoord1d_fp)(GLdouble s);
typedef void (OGLFUNCCALL *glTexCoord1dv_fp)(const GLdouble *v);
typedef void (OGLFUNCCALL *glTexCoord1f_fp)(GLfloat s);
typedef void (OGLFUNCCALL *glTexCoord1fv_fp)(const GLfloat *v);
typedef void (OGLFUNCCALL *glTexCoord1i_fp)(GLint s);
typedef void (OGLFUNCCALL *glTexCoord1iv_fp)(const GLint *v);
typedef void (OGLFUNCCALL *glTexCoord1s_fp)(GLshort s);
typedef void (OGLFUNCCALL *glTexCoord1sv_fp)(const GLshort *v);
typedef void (OGLFUNCCALL *glTexCoord2d_fp)(GLdouble s, GLdouble t);
typedef void (OGLFUNCCALL *glTexCoord2dv_fp)(const GLdouble *v);
typedef void (OGLFUNCCALL *glTexCoord2f_fp)(GLfloat s, GLfloat t);
typedef void (OGLFUNCCALL *glTexCoord2fv_fp)(const GLfloat *v);
typedef void (OGLFUNCCALL *glTexCoord2i_fp)(GLint s, GLint t);
typedef void (OGLFUNCCALL *glTexCoord2iv_fp)(const GLint *v);
typedef void (OGLFUNCCALL *glTexCoord2s_fp)(GLshort s, GLshort t);
typedef void (OGLFUNCCALL *glTexCoord2sv_fp)(const GLshort *v);
typedef void (OGLFUNCCALL *glTexCoord3d_fp)(GLdouble s, GLdouble t, GLdouble r);
typedef void (OGLFUNCCALL *glTexCoord3dv_fp)(const GLdouble *v);
typedef void (OGLFUNCCALL *glTexCoord3f_fp)(GLfloat s, GLfloat t, GLfloat r);
typedef void (OGLFUNCCALL *glTexCoord3fv_fp)(const GLfloat *v);
typedef void (OGLFUNCCALL *glTexCoord3i_fp)(GLint s, GLint t, GLint r);
typedef void (OGLFUNCCALL *glTexCoord3iv_fp)(const GLint *v);
typedef void (OGLFUNCCALL *glTexCoord3s_fp)(GLshort s, GLshort t, GLshort r);
typedef void (OGLFUNCCALL *glTexCoord3sv_fp)(const GLshort *v);
typedef void (OGLFUNCCALL *glTexCoord4d_fp)(GLdouble s, GLdouble t, GLdouble r, GLdouble q);
typedef void (OGLFUNCCALL *glTexCoord4dv_fp)(const GLdouble *v);
typedef void (OGLFUNCCALL *glTexCoord4f_fp)(GLfloat s, GLfloat t, GLfloat r, GLfloat q);
typedef void (OGLFUNCCALL *glTexCoord4fv_fp)(const GLfloat *v);
typedef void (OGLFUNCCALL *glTexCoord4i_fp)(GLint s, GLint t, GLint r, GLint q);
typedef void (OGLFUNCCALL *glTexCoord4iv_fp)(const GLint *v);
typedef void (OGLFUNCCALL *glTexCoord4s_fp)(GLshort s, GLshort t, GLshort r, GLshort q);
typedef void (OGLFUNCCALL *glTexCoord4sv_fp)(const GLshort *v);
typedef void (OGLFUNCCALL *glTexCoordPointer_fp)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
typedef void (OGLFUNCCALL *glTexEnvf_fp)(GLenum target, GLenum pname, GLfloat param);
typedef void (OGLFUNCCALL *glTexEnvfv_fp)(GLenum target, GLenum pname, const GLfloat *params);
typedef void (OGLFUNCCALL *glTexEnvi_fp)(GLenum target, GLenum pname, GLint param);
typedef void (OGLFUNCCALL *glTexEnviv_fp)(GLenum target, GLenum pname, const GLint *params);
typedef void (OGLFUNCCALL *glTexGend_fp)(GLenum coord, GLenum pname, GLdouble param);
typedef void (OGLFUNCCALL *glTexGendv_fp)(GLenum coord, GLenum pname, const GLdouble *params);
typedef void (OGLFUNCCALL *glTexGenf_fp)(GLenum coord, GLenum pname, GLfloat param);
typedef void (OGLFUNCCALL *glTexGenfv_fp)(GLenum coord, GLenum pname, const GLfloat *params);
typedef void (OGLFUNCCALL *glTexGeni_fp)(GLenum coord, GLenum pname, GLint param);
typedef void (OGLFUNCCALL *glTexGeniv_fp)(GLenum coord, GLenum pname, const GLint *params);
typedef void (OGLFUNCCALL *glTexImage1D_fp)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
typedef void (OGLFUNCCALL *glTexImage2D_fp)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
typedef void (OGLFUNCCALL *glTexParameterf_fp)(GLenum target, GLenum pname, GLfloat param);
typedef void (OGLFUNCCALL *glTexParameterfv_fp)(GLenum target, GLenum pname, const GLfloat *params);
typedef void (OGLFUNCCALL *glTexParameteri_fp)(GLenum target, GLenum pname, GLint param);
typedef void (OGLFUNCCALL *glTexParameteriv_fp)(GLenum target, GLenum pname, const GLint *params);
typedef void (OGLFUNCCALL *glTexSubImage1D_fp)(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels);
typedef void (OGLFUNCCALL *glTexSubImage2D_fp)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
typedef void (OGLFUNCCALL *glTranslated_fp)(GLdouble x, GLdouble y, GLdouble z);
typedef void (OGLFUNCCALL *glTranslatef_fp)(GLfloat x, GLfloat y, GLfloat z);
typedef void (OGLFUNCCALL *glVertex2d_fp)(GLdouble x, GLdouble y);
typedef void (OGLFUNCCALL *glVertex2dv_fp)(const GLdouble *v);
typedef void (OGLFUNCCALL *glVertex2f_fp)(GLfloat x, GLfloat y);
typedef void (OGLFUNCCALL *glVertex2fv_fp)(const GLfloat *v);
typedef void (OGLFUNCCALL *glVertex2i_fp)(GLint x, GLint y);
typedef void (OGLFUNCCALL *glVertex2iv_fp)(const GLint *v);
typedef void (OGLFUNCCALL *glVertex2s_fp)(GLshort x, GLshort y);
typedef void (OGLFUNCCALL *glVertex2sv_fp)(const GLshort *v);
typedef void (OGLFUNCCALL *glVertex3d_fp)(GLdouble x, GLdouble y, GLdouble z);
typedef void (OGLFUNCCALL *glVertex3dv_fp)(const GLdouble *v);
typedef void (OGLFUNCCALL *glVertex3f_fp)(GLfloat x, GLfloat y, GLfloat z);
typedef void (OGLFUNCCALL *glVertex3fv_fp)(const GLfloat *v);
typedef void (OGLFUNCCALL *glVertex3i_fp)(GLint x, GLint y, GLint z);
typedef void (OGLFUNCCALL *glVertex3iv_fp)(const GLint *v);
typedef void (OGLFUNCCALL *glVertex3s_fp)(GLshort x, GLshort y, GLshort z);
typedef void (OGLFUNCCALL *glVertex3sv_fp)(const GLshort *v);
typedef void (OGLFUNCCALL *glVertex4d_fp)(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
typedef void (OGLFUNCCALL *glVertex4dv_fp)(const GLdouble *v);
typedef void (OGLFUNCCALL *glVertex4f_fp)(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
typedef void (OGLFUNCCALL *glVertex4fv_fp)(const GLfloat *v);
typedef void (OGLFUNCCALL *glVertex4i_fp)(GLint x, GLint y, GLint z, GLint w);
typedef void (OGLFUNCCALL *glVertex4iv_fp)(const GLint *v);
typedef void (OGLFUNCCALL *glVertex4s_fp)(GLshort x, GLshort y, GLshort z, GLshort w);
typedef void (OGLFUNCCALL *glVertex4sv_fp)(const GLshort *v);
typedef void (OGLFUNCCALL *glVertexPointer_fp)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
typedef void (OGLFUNCCALL *glViewport_fp)(GLint x, GLint y, GLsizei width, GLsizei height);

typedef void (OGLFUNCCALL *glMultiTexCoord2fARB_fp)(GLenum target, GLfloat s, GLfloat t);
typedef void (OGLFUNCCALL *glActiveTextureARB_fp)(GLenum target);
typedef void (OGLFUNCCALL *glMultiTexCoord2fSGIS_fp)(GLenum target, GLfloat s, GLfloat t);
typedef void (OGLFUNCCALL *glSelectTextureSGIS_fp)(GLenum target);
typedef void (OGLFUNCCALL *glColorTableEXT_fp)(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *table);
typedef void (OGLFUNCCALL *glCombinerParameteriNV_fp)(GLenum, GLint);
typedef void (OGLFUNCCALL *glCombinerInputNV_fp)(GLenum, GLenum, GLenum, GLenum, GLenum, GLenum);
typedef void (OGLFUNCCALL *glCombinerOutputNV_fp)(GLenum, GLenum, GLenum, GLenum, GLenum, GLenum, GLenum, GLboolean, GLboolean, GLboolean);
typedef void (OGLFUNCCALL *glFinalCombinerInputNV_fp)(GLenum, GLenum, GLenum, GLenum);

#ifdef _WIN32
typedef BOOL  (OGLFUNCCALL *wglCopyContext_fp)(HGLRC, HGLRC, UINT);
typedef HGLRC (OGLFUNCCALL *wglCreateContext_fp)(HDC);
typedef HGLRC (OGLFUNCCALL *wglCreateLayerContext_fp)(HDC, int);
typedef BOOL  (OGLFUNCCALL *wglDeleteContext_fp)(HGLRC);
typedef HGLRC (OGLFUNCCALL *wglGetCurrentContext_fp)(VOID);
typedef HDC   (OGLFUNCCALL *wglGetCurrentDC_fp)(VOID);
typedef PROC  (OGLFUNCCALL *wglGetProcAddress_fp)(LPCSTR);
typedef BOOL  (OGLFUNCCALL *wglMakeCurrent_fp)(HDC, HGLRC);
typedef BOOL  (OGLFUNCCALL *wglShareLists_fp)(HGLRC, HGLRC);
typedef BOOL  (OGLFUNCCALL *wglUseFontBitmapsA_fp)(HDC, DWORD, DWORD, DWORD);
typedef BOOL  (OGLFUNCCALL *wglUseFontBitmapsW_fp)(HDC, DWORD, DWORD, DWORD);
typedef BOOL  (OGLFUNCCALL *wglUseFontOutlinesA_fp)(HDC, DWORD, DWORD, DWORD, FLOAT,FLOAT, int, LPGLYPHMETRICSFLOAT);
typedef BOOL  (OGLFUNCCALL *wglUseFontOutlinesW_fp)(HDC, DWORD, DWORD, DWORD, FLOAT,FLOAT, int, LPGLYPHMETRICSFLOAT);
typedef BOOL  (OGLFUNCCALL *wglDescribeLayerPlane_fp)(HDC, int, int, UINT,LPLAYERPLANEDESCRIPTOR);
typedef int   (OGLFUNCCALL *wglSetLayerPaletteEntries_fp)(HDC, int, int, int,CONST COLORREF *);
typedef int   (OGLFUNCCALL *wglGetLayerPaletteEntries_fp)(HDC, int, int, int,COLORREF *);
typedef BOOL  (OGLFUNCCALL *wglRealizeLayerPalette_fp)(HDC, int, BOOL);
typedef BOOL  (OGLFUNCCALL *wglSwapLayerBuffers_fp)(HDC, UINT);
#if (WINVER >= 0x0500)
typedef DWORD (OGLFUNCCALL *wglSwapMultipleBuffers_fp)(UINT, CONST WGLSWAP *);
#endif
#endif

DEFVAR glAccum_fp dglAccum;
DEFVAR glAlphaFunc_fp dglAlphaFunc;
DEFVAR glAreTexturesResident_fp dglAreTexturesResident;
DEFVAR glArrayElement_fp dglArrayElement;
DEFVAR glBegin_fp dglBegin;
DEFVAR glBindTexture_fp dglBindTexture;
DEFVAR glBitmap_fp dglBitmap;
DEFVAR glBlendFunc_fp dglBlendFunc;
DEFVAR glCallList_fp dglCallList;
DEFVAR glCallLists_fp dglCallLists;
DEFVAR glClear_fp dglClear;
DEFVAR glClearAccum_fp dglClearAccum;
DEFVAR glClearColor_fp dglClearColor;
DEFVAR glClearDepth_fp dglClearDepth;
DEFVAR glClearIndex_fp dglClearIndex;
DEFVAR glClearStencil_fp dglClearStencil;
DEFVAR glClipPlane_fp dglClipPlane;
DEFVAR glColor3b_fp dglColor3b;
DEFVAR glColor3bv_fp dglColor3bv;
DEFVAR glColor3d_fp dglColor3d;
DEFVAR glColor3dv_fp dglColor3dv;
DEFVAR glColor3f_fp dglColor3f;
DEFVAR glColor3fv_fp dglColor3fv;
DEFVAR glColor3i_fp dglColor3i;
DEFVAR glColor3iv_fp dglColor3iv;
DEFVAR glColor3s_fp dglColor3s;
DEFVAR glColor3sv_fp dglColor3sv;
DEFVAR glColor3ub_fp dglColor3ub;
DEFVAR glColor3ubv_fp dglColor3ubv;
DEFVAR glColor3ui_fp dglColor3ui;
DEFVAR glColor3uiv_fp dglColor3uiv;
DEFVAR glColor3us_fp dglColor3us;
DEFVAR glColor3usv_fp dglColor3usv;
DEFVAR glColor4b_fp dglColor4b;
DEFVAR glColor4bv_fp dglColor4bv;
DEFVAR glColor4d_fp dglColor4d;
DEFVAR glColor4dv_fp dglColor4dv;
DEFVAR glColor4f_fp dglColor4f;
DEFVAR glColor4fv_fp dglColor4fv;
DEFVAR glColor4i_fp dglColor4i;
DEFVAR glColor4iv_fp dglColor4iv;
DEFVAR glColor4s_fp dglColor4s;
DEFVAR glColor4sv_fp dglColor4sv;
DEFVAR glColor4ub_fp dglColor4ub;
DEFVAR glColor4ubv_fp dglColor4ubv;
DEFVAR glColor4ui_fp dglColor4ui;
DEFVAR glColor4uiv_fp dglColor4uiv;
DEFVAR glColor4us_fp dglColor4us;
DEFVAR glColor4usv_fp dglColor4usv;
DEFVAR glColorMask_fp dglColorMask;
DEFVAR glColorMaterial_fp dglColorMaterial;
DEFVAR glColorPointer_fp dglColorPointer;
DEFVAR glCopyPixels_fp dglCopyPixels;
DEFVAR glCopyTexImage1D_fp dglCopyTexImage1D;
DEFVAR glCopyTexImage2D_fp dglCopyTexImage2D;
DEFVAR glCopyTexSubImage1D_fp dglCopyTexSubImage1D;
DEFVAR glCopyTexSubImage2D_fp dglCopyTexSubImage2D;
DEFVAR glCullFace_fp dglCullFace;
DEFVAR glDeleteLists_fp dglDeleteLists;
DEFVAR glDeleteTextures_fp dglDeleteTextures;
DEFVAR glDepthFunc_fp dglDepthFunc;
DEFVAR glDepthMask_fp dglDepthMask;
DEFVAR glDepthRange_fp dglDepthRange;
DEFVAR glDisable_fp dglDisable;
DEFVAR glDisableClientState_fp dglDisableClientState;
DEFVAR glDrawArrays_fp dglDrawArrays;
DEFVAR glDrawBuffer_fp dglDrawBuffer;
DEFVAR glDrawElements_fp dglDrawElements;
DEFVAR glDrawPixels_fp dglDrawPixels;
DEFVAR glEdgeFlag_fp dglEdgeFlag;
DEFVAR glEdgeFlagPointer_fp dglEdgeFlagPointer;
DEFVAR glEdgeFlagv_fp dglEdgeFlagv;
DEFVAR glEnable_fp dglEnable;
DEFVAR glEnableClientState_fp dglEnableClientState;
DEFVAR glEnd_fp dglEnd;
DEFVAR glEndList_fp dglEndList;
DEFVAR glEvalCoord1d_fp dglEvalCoord1d;
DEFVAR glEvalCoord1dv_fp dglEvalCoord1dv;
DEFVAR glEvalCoord1f_fp dglEvalCoord1f;
DEFVAR glEvalCoord1fv_fp dglEvalCoord1fv;
DEFVAR glEvalCoord2d_fp dglEvalCoord2d;
DEFVAR glEvalCoord2dv_fp dglEvalCoord2dv;
DEFVAR glEvalCoord2f_fp dglEvalCoord2f;
DEFVAR glEvalCoord2fv_fp dglEvalCoord2fv;
DEFVAR glEvalMesh1_fp dglEvalMesh1;
DEFVAR glEvalMesh2_fp dglEvalMesh2;
DEFVAR glEvalPoint1_fp dglEvalPoint1;
DEFVAR glEvalPoint2_fp dglEvalPoint2;
DEFVAR glFeedbackBuffer_fp dglFeedbackBuffer;
DEFVAR glFinish_fp dglFinish;
DEFVAR glFlush_fp dglFlush;
DEFVAR glFogf_fp dglFogf;
DEFVAR glFogfv_fp dglFogfv;
DEFVAR glFogi_fp dglFogi;
DEFVAR glFogiv_fp dglFogiv;
DEFVAR glFrontFace_fp dglFrontFace;
DEFVAR glFrustum_fp dglFrustum;
DEFVAR glGenLists_fp dglGenLists;
DEFVAR glGenTextures_fp dglGenTextures;
DEFVAR glGetBooleanv_fp dglGetBooleanv;
DEFVAR glGetClipPlane_fp dglGetClipPlane;
DEFVAR glGetDoublev_fp dglGetDoublev;
DEFVAR glGetError_fp dglGetError;
DEFVAR glGetFloatv_fp dglGetFloatv;
DEFVAR glGetIntegerv_fp dglGetIntegerv;
DEFVAR glGetLightfv_fp dglGetLightfv;
DEFVAR glGetLightiv_fp dglGetLightiv;
DEFVAR glGetMapdv_fp dglGetMapdv;
DEFVAR glGetMapfv_fp dglGetMapfv;
DEFVAR glGetMapiv_fp dglGetMapiv;
DEFVAR glGetMaterialfv_fp dglGetMaterialfv;
DEFVAR glGetMaterialiv_fp dglGetMaterialiv;
DEFVAR glGetPixelMapfv_fp dglGetPixelMapfv;
DEFVAR glGetPixelMapuiv_fp dglGetPixelMapuiv;
DEFVAR glGetPixelMapusv_fp dglGetPixelMapusv;
DEFVAR glGetPointerv_fp dglGetPointerv;
DEFVAR glGetPolygonStipple_fp dglGetPolygonStipple;
DEFVAR glGetString_fp dglGetString;
DEFVAR glGetTexEnvfv_fp dglGetTexEnvfv;
DEFVAR glGetTexEnviv_fp dglGetTexEnviv;
DEFVAR glGetTexGendv_fp dglGetTexGendv;
DEFVAR glGetTexGenfv_fp dglGetTexGenfv;
DEFVAR glGetTexGeniv_fp dglGetTexGeniv;
DEFVAR glGetTexImage_fp dglGetTexImage;
DEFVAR glGetTexLevelParameterfv_fp dglGetTexLevelParameterfv;
DEFVAR glGetTexLevelParameteriv_fp dglGetTexLevelParameteriv;
DEFVAR glGetTexParameterfv_fp dglGetTexParameterfv;
DEFVAR glGetTexParameteriv_fp dglGetTexParameteriv;
DEFVAR glHint_fp dglHint;
DEFVAR glIndexMask_fp dglIndexMask;
DEFVAR glIndexPointer_fp dglIndexPointer;
DEFVAR glIndexd_fp dglIndexd;
DEFVAR glIndexdv_fp dglIndexdv;
DEFVAR glIndexf_fp dglIndexf;
DEFVAR glIndexfv_fp dglIndexfv;
DEFVAR glIndexi_fp dglIndexi;
DEFVAR glIndexiv_fp dglIndexiv;
DEFVAR glIndexs_fp dglIndexs;
DEFVAR glIndexsv_fp dglIndexsv;
DEFVAR glIndexub_fp dglIndexub;
DEFVAR glIndexubv_fp dglIndexubv;
DEFVAR glInitNames_fp dglInitNames;
DEFVAR glInterleavedArrays_fp dglInterleavedArrays;
DEFVAR glIsEnabled_fp dglIsEnabled;
DEFVAR glIsList_fp dglIsList;
DEFVAR glIsTexture_fp dglIsTexture;
DEFVAR glLightModelf_fp dglLightModelf;
DEFVAR glLightModelfv_fp dglLightModelfv;
DEFVAR glLightModeli_fp dglLightModeli;
DEFVAR glLightModeliv_fp dglLightModeliv;
DEFVAR glLightf_fp dglLightf;
DEFVAR glLightfv_fp dglLightfv;
DEFVAR glLighti_fp dglLighti;
DEFVAR glLightiv_fp dglLightiv;
DEFVAR glLineStipple_fp dglLineStipple;
DEFVAR glLineWidth_fp dglLineWidth;
DEFVAR glListBase_fp dglListBase;
DEFVAR glLoadIdentity_fp dglLoadIdentity;
DEFVAR glLoadMatrixd_fp dglLoadMatrixd;
DEFVAR glLoadMatrixf_fp dglLoadMatrixf;
DEFVAR glLoadName_fp dglLoadName;
DEFVAR glLogicOp_fp dglLogicOp;
DEFVAR glMap1d_fp dglMap1d;
DEFVAR glMap1f_fp dglMap1f;
DEFVAR glMap2d_fp dglMap2d;
DEFVAR glMap2f_fp dglMap2f;
DEFVAR glMapGrid1d_fp dglMapGrid1d;
DEFVAR glMapGrid1f_fp dglMapGrid1f;
DEFVAR glMapGrid2d_fp dglMapGrid2d;
DEFVAR glMapGrid2f_fp dglMapGrid2f;
DEFVAR glMaterialf_fp dglMaterialf;
DEFVAR glMaterialfv_fp dglMaterialfv;
DEFVAR glMateriali_fp dglMateriali;
DEFVAR glMaterialiv_fp dglMaterialiv;
DEFVAR glMatrixMode_fp dglMatrixMode;
DEFVAR glMultMatrixd_fp dglMultMatrixd;
DEFVAR glMultMatrixf_fp dglMultMatrixf;
DEFVAR glNewList_fp dglNewList;
DEFVAR glNormal3b_fp dglNormal3b;
DEFVAR glNormal3bv_fp dglNormal3bv;
DEFVAR glNormal3d_fp dglNormal3d;
DEFVAR glNormal3dv_fp dglNormal3dv;
DEFVAR glNormal3f_fp dglNormal3f;
DEFVAR glNormal3fv_fp dglNormal3fv;
DEFVAR glNormal3i_fp dglNormal3i;
DEFVAR glNormal3iv_fp dglNormal3iv;
DEFVAR glNormal3s_fp dglNormal3s;
DEFVAR glNormal3sv_fp dglNormal3sv;
DEFVAR glNormalPointer_fp dglNormalPointer;
DEFVAR glOrtho_fp dglOrtho;
DEFVAR glPassThrough_fp dglPassThrough;
DEFVAR glPixelMapfv_fp dglPixelMapfv;
DEFVAR glPixelMapuiv_fp dglPixelMapuiv;
DEFVAR glPixelMapusv_fp dglPixelMapusv;
DEFVAR glPixelStoref_fp dglPixelStoref;
DEFVAR glPixelStorei_fp dglPixelStorei;
DEFVAR glPixelTransferf_fp dglPixelTransferf;
DEFVAR glPixelTransferi_fp dglPixelTransferi;
DEFVAR glPixelZoom_fp dglPixelZoom;
DEFVAR glPointSize_fp dglPointSize;
DEFVAR glPolygonMode_fp dglPolygonMode;
DEFVAR glPolygonOffset_fp dglPolygonOffset;
DEFVAR glPolygonStipple_fp dglPolygonStipple;
DEFVAR glPopAttrib_fp dglPopAttrib;
DEFVAR glPopClientAttrib_fp dglPopClientAttrib;
DEFVAR glPopMatrix_fp dglPopMatrix;
DEFVAR glPopName_fp dglPopName;
DEFVAR glPrioritizeTextures_fp dglPrioritizeTextures;
DEFVAR glPushAttrib_fp dglPushAttrib;
DEFVAR glPushClientAttrib_fp dglPushClientAttrib;
DEFVAR glPushMatrix_fp dglPushMatrix;
DEFVAR glPushName_fp dglPushName;
DEFVAR glRasterPos2d_fp dglRasterPos2d;
DEFVAR glRasterPos2dv_fp dglRasterPos2dv;
DEFVAR glRasterPos2f_fp dglRasterPos2f;
DEFVAR glRasterPos2fv_fp dglRasterPos2fv;
DEFVAR glRasterPos2i_fp dglRasterPos2i;
DEFVAR glRasterPos2iv_fp dglRasterPos2iv;
DEFVAR glRasterPos2s_fp dglRasterPos2s;
DEFVAR glRasterPos2sv_fp dglRasterPos2sv;
DEFVAR glRasterPos3d_fp dglRasterPos3d;
DEFVAR glRasterPos3dv_fp dglRasterPos3dv;
DEFVAR glRasterPos3f_fp dglRasterPos3f;
DEFVAR glRasterPos3fv_fp dglRasterPos3fv;
DEFVAR glRasterPos3i_fp dglRasterPos3i;
DEFVAR glRasterPos3iv_fp dglRasterPos3iv;
DEFVAR glRasterPos3s_fp dglRasterPos3s;
DEFVAR glRasterPos3sv_fp dglRasterPos3sv;
DEFVAR glRasterPos4d_fp dglRasterPos4d;
DEFVAR glRasterPos4dv_fp dglRasterPos4dv;
DEFVAR glRasterPos4f_fp dglRasterPos4f;
DEFVAR glRasterPos4fv_fp dglRasterPos4fv;
DEFVAR glRasterPos4i_fp dglRasterPos4i;
DEFVAR glRasterPos4iv_fp dglRasterPos4iv;
DEFVAR glRasterPos4s_fp dglRasterPos4s;
DEFVAR glRasterPos4sv_fp dglRasterPos4sv;
DEFVAR glReadBuffer_fp dglReadBuffer;
DEFVAR glReadPixels_fp dglReadPixels;
DEFVAR glRectd_fp dglRectd;
DEFVAR glRectdv_fp dglRectdv;
DEFVAR glRectf_fp dglRectf;
DEFVAR glRectfv_fp dglRectfv;
DEFVAR glRecti_fp dglRecti;
DEFVAR glRectiv_fp dglRectiv;
DEFVAR glRects_fp dglRects;
DEFVAR glRectsv_fp dglRectsv;
DEFVAR glRenderMode_fp dglRenderMode;
DEFVAR glRotated_fp dglRotated;
DEFVAR glRotatef_fp dglRotatef;
DEFVAR glScaled_fp dglScaled;
DEFVAR glScalef_fp dglScalef;
DEFVAR glScissor_fp dglScissor;
DEFVAR glSelectBuffer_fp dglSelectBuffer;
DEFVAR glShadeModel_fp dglShadeModel;
DEFVAR glStencilFunc_fp dglStencilFunc;
DEFVAR glStencilMask_fp dglStencilMask;
DEFVAR glStencilOp_fp dglStencilOp;
DEFVAR glTexCoord1d_fp dglTexCoord1d;
DEFVAR glTexCoord1dv_fp dglTexCoord1dv;
DEFVAR glTexCoord1f_fp dglTexCoord1f;
DEFVAR glTexCoord1fv_fp dglTexCoord1fv;
DEFVAR glTexCoord1i_fp dglTexCoord1i;
DEFVAR glTexCoord1iv_fp dglTexCoord1iv;
DEFVAR glTexCoord1s_fp dglTexCoord1s;
DEFVAR glTexCoord1sv_fp dglTexCoord1sv;
DEFVAR glTexCoord2d_fp dglTexCoord2d;
DEFVAR glTexCoord2dv_fp dglTexCoord2dv;
DEFVAR glTexCoord2f_fp dglTexCoord2f;
DEFVAR glTexCoord2fv_fp dglTexCoord2fv;
DEFVAR glTexCoord2i_fp dglTexCoord2i;
DEFVAR glTexCoord2iv_fp dglTexCoord2iv;
DEFVAR glTexCoord2s_fp dglTexCoord2s;
DEFVAR glTexCoord2sv_fp dglTexCoord2sv;
DEFVAR glTexCoord3d_fp dglTexCoord3d;
DEFVAR glTexCoord3dv_fp dglTexCoord3dv;
DEFVAR glTexCoord3f_fp dglTexCoord3f;
DEFVAR glTexCoord3fv_fp dglTexCoord3fv;
DEFVAR glTexCoord3i_fp dglTexCoord3i;
DEFVAR glTexCoord3iv_fp dglTexCoord3iv;
DEFVAR glTexCoord3s_fp dglTexCoord3s;
DEFVAR glTexCoord3sv_fp dglTexCoord3sv;
DEFVAR glTexCoord4d_fp dglTexCoord4d;
DEFVAR glTexCoord4dv_fp dglTexCoord4dv;
DEFVAR glTexCoord4f_fp dglTexCoord4f;
DEFVAR glTexCoord4fv_fp dglTexCoord4fv;
DEFVAR glTexCoord4i_fp dglTexCoord4i;
DEFVAR glTexCoord4iv_fp dglTexCoord4iv;
DEFVAR glTexCoord4s_fp dglTexCoord4s;
DEFVAR glTexCoord4sv_fp dglTexCoord4sv;
DEFVAR glTexCoordPointer_fp dglTexCoordPointer;
DEFVAR glTexEnvf_fp dglTexEnvf;
DEFVAR glTexEnvfv_fp dglTexEnvfv;
DEFVAR glTexEnvi_fp dglTexEnvi;
DEFVAR glTexEnviv_fp dglTexEnviv;
DEFVAR glTexGend_fp dglTexGend;
DEFVAR glTexGendv_fp dglTexGendv;
DEFVAR glTexGenf_fp dglTexGenf;
DEFVAR glTexGenfv_fp dglTexGenfv;
DEFVAR glTexGeni_fp dglTexGeni;
DEFVAR glTexGeniv_fp dglTexGeniv;
DEFVAR glTexImage1D_fp dglTexImage1D;
DEFVAR glTexImage2D_fp dglTexImage2D;
DEFVAR glTexParameterf_fp dglTexParameterf;
DEFVAR glTexParameterfv_fp dglTexParameterfv;
DEFVAR glTexParameteri_fp dglTexParameteri;
DEFVAR glTexParameteriv_fp dglTexParameteriv;
DEFVAR glTexSubImage1D_fp dglTexSubImage1D;
DEFVAR glTexSubImage2D_fp dglTexSubImage2D;
DEFVAR glTranslated_fp dglTranslated;
DEFVAR glTranslatef_fp dglTranslatef;
DEFVAR glVertex2d_fp dglVertex2d;
DEFVAR glVertex2dv_fp dglVertex2dv;
DEFVAR glVertex2f_fp dglVertex2f;
DEFVAR glVertex2fv_fp dglVertex2fv;
DEFVAR glVertex2i_fp dglVertex2i;
DEFVAR glVertex2iv_fp dglVertex2iv;
DEFVAR glVertex2s_fp dglVertex2s;
DEFVAR glVertex2sv_fp dglVertex2sv;
DEFVAR glVertex3d_fp dglVertex3d;
DEFVAR glVertex3dv_fp dglVertex3dv;
DEFVAR glVertex3f_fp dglVertex3f;
DEFVAR glVertex3fv_fp dglVertex3fv;
DEFVAR glVertex3i_fp dglVertex3i;
DEFVAR glVertex3iv_fp dglVertex3iv;
DEFVAR glVertex3s_fp dglVertex3s;
DEFVAR glVertex3sv_fp dglVertex3sv;
DEFVAR glVertex4d_fp dglVertex4d;
DEFVAR glVertex4dv_fp dglVertex4dv;
DEFVAR glVertex4f_fp dglVertex4f;
DEFVAR glVertex4fv_fp dglVertex4fv;
DEFVAR glVertex4i_fp dglVertex4i;
DEFVAR glVertex4iv_fp dglVertex4iv;
DEFVAR glVertex4s_fp dglVertex4s;
DEFVAR glVertex4sv_fp dglVertex4sv;
DEFVAR glVertexPointer_fp dglVertexPointer;
DEFVAR glViewport_fp dglViewport;

DEFVAR glMultiTexCoord2fARB_fp dglMultiTexCoord2fARB;
DEFVAR glActiveTextureARB_fp dglActiveTextureARB;
DEFVAR glMultiTexCoord2fSGIS_fp dglMultiTexCoord2fSGIS;
DEFVAR glSelectTextureSGIS_fp dglSelectTextureSGIS;
DEFVAR glColorTableEXT_fp dglColorTableEXT;
DEFVAR glCombinerParameteriNV_fp dglCombinerParameteriNV;
DEFVAR glCombinerInputNV_fp dglCombinerInputNV;
DEFVAR glCombinerOutputNV_fp dglCombinerOutputNV;
DEFVAR glFinalCombinerInputNV_fp dglFinalCombinerInputNV;

#ifdef _WIN32
DEFVAR wglCopyContext_fp dwglCopyContext;
DEFVAR wglCreateContext_fp dwglCreateContext;
DEFVAR wglCreateLayerContext_fp dwglCreateLayerContext;
DEFVAR wglDeleteContext_fp dwglDeleteContext;
DEFVAR wglGetCurrentContext_fp dwglGetCurrentContext;
DEFVAR wglGetCurrentDC_fp dwglGetCurrentDC;
DEFVAR wglGetProcAddress_fp dwglGetProcAddress;
DEFVAR wglMakeCurrent_fp dwglMakeCurrent;
DEFVAR wglShareLists_fp dwglShareLists;
DEFVAR wglUseFontBitmapsA_fp dwglUseFontBitmapsA;
DEFVAR wglUseFontBitmapsW_fp dwglUseFontBitmapsW;
DEFVAR wglUseFontOutlinesA_fp dwglUseFontOutlinesA;
DEFVAR wglUseFontOutlinesW_fp dwglUseFontOutlinesW;
DEFVAR wglDescribeLayerPlane_fp dwglDescribeLayerPlane;
DEFVAR wglSetLayerPaletteEntries_fp dwglSetLayerPaletteEntries;
DEFVAR wglGetLayerPaletteEntries_fp dwglGetLayerPaletteEntries;
DEFVAR wglRealizeLayerPalette_fp dwglRealizeLayerPalette;
DEFVAR wglSwapLayerBuffers_fp dwglSwapLayerBuffers;
#if (WINVER >= 0x0500)
DEFVAR wglSwapMultipleBuffers_fp dwglSwapMultipleBuffers;
#endif
#endif

#ifdef DECLARE_VARS

// Dynamic module load functions
#ifdef _WIN32
void *dll_LoadModule(const char *name)
{
	HINSTANCE handle;
	handle = LoadLibrary(name);
	return (void *)handle;
}
void dll_UnloadModule(void *hdl)
{
	HINSTANCE handle;
	handle = (HINSTANCE)hdl;

	if(hdl)
	{
		FreeLibrary(handle);
	}
}
void *dll_GetSymbol(void *dllhandle,const char *symname)
{
	if(!dllhandle)
		return NULL;
	return (void *)GetProcAddress((HINSTANCE)dllhandle,symname);
}
#endif
#ifdef __unix__
#include <dlfcn.h>
void *dll_LoadModule(const char *name)
{
	return (void *)dlopen(name,RTLD_NOW|RTLD_GLOBAL);
}
void dll_UnloadModule(void *hdl)
{
	if(hdl)
	{
		dlclose(hdl);
	}
}
void *dll_GetSymbol(void *dllhandle,const char *symname)
{
	if(!dllhandle)
		return NULL;
	return dlsym(dllhandle,symname);
}
#endif
#ifdef macintosh
#include <SDL.h>
void *dll_LoadModule(const char *name)
{
	return SDL_GL_LoadLibrary(name) ? NULL : (void *) -1;	// return pointer is not dereferenced
}
void dll_UnloadModule(void *hdl)
{
	hdl = hdl;	// SDL_GL_UnloadLibrary not exported by SDL
}
void *dll_GetSymbol(void *dllhandle,const char *symname)
{
	if(!dllhandle)
		return NULL;
	return SDL_GL_GetProcAddress(symname);
}
#endif

#endif //DECLARE_VARS

void OpenGL_SetFuncsToNull(void);

extern char *OglLibPath;

#ifndef DECLARE_VARS
// pass true to load the library
// pass false to unload it
bool OpenGL_LoadLibrary(bool load);//load=true removed because not c++
#else
void *OpenGLModuleHandle=NULL;
//char *OglLibPath="opengl32.dll";
bool OpenGL_LoadLibrary(bool load)
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

		dglAccum = (glAccum_fp)dll_GetSymbol(OpenGLModuleHandle,"glAccum");
		dglAlphaFunc = (glAlphaFunc_fp)dll_GetSymbol(OpenGLModuleHandle,"glAlphaFunc");
		dglAreTexturesResident = (glAreTexturesResident_fp)dll_GetSymbol(OpenGLModuleHandle,"glAreTexturesResident");
		dglArrayElement = (glArrayElement_fp)dll_GetSymbol(OpenGLModuleHandle,"glArrayElement");
		dglBegin = (glBegin_fp)dll_GetSymbol(OpenGLModuleHandle,"glBegin");
		dglBindTexture = (glBindTexture_fp)dll_GetSymbol(OpenGLModuleHandle,"glBindTexture");
		dglBitmap = (glBitmap_fp)dll_GetSymbol(OpenGLModuleHandle,"glBitmap");
		dglBlendFunc = (glBlendFunc_fp)dll_GetSymbol(OpenGLModuleHandle,"glBlendFunc");
		dglCallList = (glCallList_fp)dll_GetSymbol(OpenGLModuleHandle,"glCallList");
		dglCallLists = (glCallLists_fp)dll_GetSymbol(OpenGLModuleHandle,"glCallLists");
		dglClear = (glClear_fp)dll_GetSymbol(OpenGLModuleHandle,"glClear");
		dglClearAccum = (glClearAccum_fp)dll_GetSymbol(OpenGLModuleHandle,"glClearAccum");
		dglClearColor = (glClearColor_fp)dll_GetSymbol(OpenGLModuleHandle,"glClearColor");
		dglClearDepth = (glClearDepth_fp)dll_GetSymbol(OpenGLModuleHandle,"glClearDepth");
		dglClearIndex = (glClearIndex_fp)dll_GetSymbol(OpenGLModuleHandle,"glClearIndex");
		dglClearStencil = (glClearStencil_fp)dll_GetSymbol(OpenGLModuleHandle,"glClearStencil");
		dglClipPlane = (glClipPlane_fp)dll_GetSymbol(OpenGLModuleHandle,"glClipPlane");
		dglColor3b = (glColor3b_fp)dll_GetSymbol(OpenGLModuleHandle,"glColor3b");
		dglColor3bv = (glColor3bv_fp)dll_GetSymbol(OpenGLModuleHandle,"glColor3bv");
		dglColor3d = (glColor3d_fp)dll_GetSymbol(OpenGLModuleHandle,"glColor3d");
		dglColor3dv = (glColor3dv_fp)dll_GetSymbol(OpenGLModuleHandle,"glColor3dv");
		dglColor3f = (glColor3f_fp)dll_GetSymbol(OpenGLModuleHandle,"glColor3f");
		dglColor3fv = (glColor3fv_fp)dll_GetSymbol(OpenGLModuleHandle,"glColor3fv");
		dglColor3i = (glColor3i_fp)dll_GetSymbol(OpenGLModuleHandle,"glColor3i");
		dglColor3iv = (glColor3iv_fp)dll_GetSymbol(OpenGLModuleHandle,"glColor3iv");
		dglColor3s = (glColor3s_fp)dll_GetSymbol(OpenGLModuleHandle,"glColor3s");
		dglColor3sv = (glColor3sv_fp)dll_GetSymbol(OpenGLModuleHandle,"glColor3sv");
		dglColor3ub = (glColor3ub_fp)dll_GetSymbol(OpenGLModuleHandle,"glColor3ub");
		dglColor3ubv = (glColor3ubv_fp)dll_GetSymbol(OpenGLModuleHandle,"glColor3ubv");
		dglColor3ui = (glColor3ui_fp)dll_GetSymbol(OpenGLModuleHandle,"glColor3ui");
		dglColor3uiv = (glColor3uiv_fp)dll_GetSymbol(OpenGLModuleHandle,"glColor3uiv");
		dglColor3us = (glColor3us_fp)dll_GetSymbol(OpenGLModuleHandle,"glColor3us");
		dglColor3usv = (glColor3usv_fp)dll_GetSymbol(OpenGLModuleHandle,"glColor3usv");
		dglColor4b = (glColor4b_fp)dll_GetSymbol(OpenGLModuleHandle,"glColor4b");
		dglColor4bv = (glColor4bv_fp)dll_GetSymbol(OpenGLModuleHandle,"glColor4bv");
		dglColor4d = (glColor4d_fp)dll_GetSymbol(OpenGLModuleHandle,"glColor4d");
		dglColor4dv = (glColor4dv_fp)dll_GetSymbol(OpenGLModuleHandle,"glColor4dv");
		dglColor4f = (glColor4f_fp)dll_GetSymbol(OpenGLModuleHandle,"glColor4f");
		dglColor4fv = (glColor4fv_fp)dll_GetSymbol(OpenGLModuleHandle,"glColor4fv");
		dglColor4i = (glColor4i_fp)dll_GetSymbol(OpenGLModuleHandle,"glColor4i");
		dglColor4iv = (glColor4iv_fp)dll_GetSymbol(OpenGLModuleHandle,"glColor4iv");
		dglColor4s = (glColor4s_fp)dll_GetSymbol(OpenGLModuleHandle,"glColor4s");
		dglColor4sv = (glColor4sv_fp)dll_GetSymbol(OpenGLModuleHandle,"glColor4sv");
		dglColor4ub = (glColor4ub_fp)dll_GetSymbol(OpenGLModuleHandle,"glColor4ub");
		dglColor4ubv = (glColor4ubv_fp)dll_GetSymbol(OpenGLModuleHandle,"glColor4ubv");
		dglColor4ui = (glColor4ui_fp)dll_GetSymbol(OpenGLModuleHandle,"glColor4ui");
		dglColor4uiv = (glColor4uiv_fp)dll_GetSymbol(OpenGLModuleHandle,"glColor4uiv");
		dglColor4us = (glColor4us_fp)dll_GetSymbol(OpenGLModuleHandle,"glColor4us");
		dglColor4usv = (glColor4usv_fp)dll_GetSymbol(OpenGLModuleHandle,"glColor4usv");
		dglColorMask = (glColorMask_fp)dll_GetSymbol(OpenGLModuleHandle,"glColorMask");
		dglColorMaterial = (glColorMaterial_fp)dll_GetSymbol(OpenGLModuleHandle,"glColorMaterial");
		dglColorPointer = (glColorPointer_fp)dll_GetSymbol(OpenGLModuleHandle,"glColorPointer");
		dglCopyPixels = (glCopyPixels_fp)dll_GetSymbol(OpenGLModuleHandle,"glCopyPixels");
		dglCopyTexImage1D = (glCopyTexImage1D_fp)dll_GetSymbol(OpenGLModuleHandle,"glCopyTexImage1D");
		dglCopyTexImage2D = (glCopyTexImage2D_fp)dll_GetSymbol(OpenGLModuleHandle,"glCopyTexImage2D");
		dglCopyTexSubImage1D = (glCopyTexSubImage1D_fp)dll_GetSymbol(OpenGLModuleHandle,"glCopyTexSubImage1D");
		dglCopyTexSubImage2D = (glCopyTexSubImage2D_fp)dll_GetSymbol(OpenGLModuleHandle,"glCopyTexSubImage2D");
		dglCullFace = (glCullFace_fp)dll_GetSymbol(OpenGLModuleHandle,"glCullFace");
		dglDeleteLists = (glDeleteLists_fp)dll_GetSymbol(OpenGLModuleHandle,"glDeleteLists");
		dglDeleteTextures = (glDeleteTextures_fp)dll_GetSymbol(OpenGLModuleHandle,"glDeleteTextures");
		dglDepthFunc = (glDepthFunc_fp)dll_GetSymbol(OpenGLModuleHandle,"glDepthFunc");
		dglDepthMask = (glDepthMask_fp)dll_GetSymbol(OpenGLModuleHandle,"glDepthMask");
		dglDepthRange = (glDepthRange_fp)dll_GetSymbol(OpenGLModuleHandle,"glDepthRange");
		dglDisable = (glDisable_fp)dll_GetSymbol(OpenGLModuleHandle,"glDisable");
		dglDisableClientState = (glDisableClientState_fp)dll_GetSymbol(OpenGLModuleHandle,"glDisableClientState");
		dglDrawArrays = (glDrawArrays_fp)dll_GetSymbol(OpenGLModuleHandle,"glDrawArrays");
		dglDrawBuffer = (glDrawBuffer_fp)dll_GetSymbol(OpenGLModuleHandle,"glDrawBuffer");
		dglDrawElements = (glDrawElements_fp)dll_GetSymbol(OpenGLModuleHandle,"glDrawElements");
		dglDrawPixels = (glDrawPixels_fp)dll_GetSymbol(OpenGLModuleHandle,"glDrawPixels");
		dglEdgeFlag = (glEdgeFlag_fp)dll_GetSymbol(OpenGLModuleHandle,"glEdgeFlag");
		dglEdgeFlagPointer = (glEdgeFlagPointer_fp)dll_GetSymbol(OpenGLModuleHandle,"glEdgeFlagPointer");
		dglEdgeFlagv = (glEdgeFlagv_fp)dll_GetSymbol(OpenGLModuleHandle,"glEdgeFlagv");
		dglEnable = (glEnable_fp)dll_GetSymbol(OpenGLModuleHandle,"glEnable");
		dglEnableClientState = (glEnableClientState_fp)dll_GetSymbol(OpenGLModuleHandle,"glEnableClientState");
		dglEnd = (glEnd_fp)dll_GetSymbol(OpenGLModuleHandle,"glEnd");
		dglEndList = (glEndList_fp)dll_GetSymbol(OpenGLModuleHandle,"glEndList");
		dglEvalCoord1d = (glEvalCoord1d_fp)dll_GetSymbol(OpenGLModuleHandle,"glEvalCoord1d");
		dglEvalCoord1dv = (glEvalCoord1dv_fp)dll_GetSymbol(OpenGLModuleHandle,"glEvalCoord1dv");
		dglEvalCoord1f = (glEvalCoord1f_fp)dll_GetSymbol(OpenGLModuleHandle,"glEvalCoord1f");
		dglEvalCoord1fv = (glEvalCoord1fv_fp)dll_GetSymbol(OpenGLModuleHandle,"glEvalCoord1fv");
		dglEvalCoord2d = (glEvalCoord2d_fp)dll_GetSymbol(OpenGLModuleHandle,"glEvalCoord2d");
		dglEvalCoord2dv = (glEvalCoord2dv_fp)dll_GetSymbol(OpenGLModuleHandle,"glEvalCoord2dv");
		dglEvalCoord2f = (glEvalCoord2f_fp)dll_GetSymbol(OpenGLModuleHandle,"glEvalCoord2f");
		dglEvalCoord2fv = (glEvalCoord2fv_fp)dll_GetSymbol(OpenGLModuleHandle,"glEvalCoord2fv");
		dglEvalMesh1 = (glEvalMesh1_fp)dll_GetSymbol(OpenGLModuleHandle,"glEvalMesh1");
		dglEvalMesh2 = (glEvalMesh2_fp)dll_GetSymbol(OpenGLModuleHandle,"glEvalMesh2");
		dglEvalPoint1 = (glEvalPoint1_fp)dll_GetSymbol(OpenGLModuleHandle,"glEvalPoint1");
		dglEvalPoint2 = (glEvalPoint2_fp)dll_GetSymbol(OpenGLModuleHandle,"glEvalPoint2");
		dglFeedbackBuffer = (glFeedbackBuffer_fp)dll_GetSymbol(OpenGLModuleHandle,"glFeedbackBuffer");
		dglFinish = (glFinish_fp)dll_GetSymbol(OpenGLModuleHandle,"glFinish");
		dglFlush = (glFlush_fp)dll_GetSymbol(OpenGLModuleHandle,"glFlush");
		dglFogf = (glFogf_fp)dll_GetSymbol(OpenGLModuleHandle,"glFogf");
		dglFogfv = (glFogfv_fp)dll_GetSymbol(OpenGLModuleHandle,"glFogfv");
		dglFogi = (glFogi_fp)dll_GetSymbol(OpenGLModuleHandle,"glFogi");
		dglFogiv = (glFogiv_fp)dll_GetSymbol(OpenGLModuleHandle,"glFogiv");
		dglFrontFace = (glFrontFace_fp)dll_GetSymbol(OpenGLModuleHandle,"glFrontFace");
		dglFrustum = (glFrustum_fp)dll_GetSymbol(OpenGLModuleHandle,"glFrustum");
		dglGenLists = (glGenLists_fp)dll_GetSymbol(OpenGLModuleHandle,"glGenLists");
		dglGenTextures = (glGenTextures_fp)dll_GetSymbol(OpenGLModuleHandle,"glGenTextures");
		dglGetBooleanv = (glGetBooleanv_fp)dll_GetSymbol(OpenGLModuleHandle,"glGetBooleanv");
		dglGetClipPlane = (glGetClipPlane_fp)dll_GetSymbol(OpenGLModuleHandle,"glGetClipPlane");
		dglGetDoublev = (glGetDoublev_fp)dll_GetSymbol(OpenGLModuleHandle,"glGetDoublev");
		dglGetError = (glGetError_fp)dll_GetSymbol(OpenGLModuleHandle,"glGetError");
		dglGetFloatv = (glGetFloatv_fp)dll_GetSymbol(OpenGLModuleHandle,"glGetFloatv");
		dglGetIntegerv = (glGetIntegerv_fp)dll_GetSymbol(OpenGLModuleHandle,"glGetIntegerv");
		dglGetLightfv = (glGetLightfv_fp)dll_GetSymbol(OpenGLModuleHandle,"glGetLightfv");
		dglGetLightiv = (glGetLightiv_fp)dll_GetSymbol(OpenGLModuleHandle,"glGetLightiv");
		dglGetMapdv = (glGetMapdv_fp)dll_GetSymbol(OpenGLModuleHandle,"glGetMapdv");
		dglGetMapfv = (glGetMapfv_fp)dll_GetSymbol(OpenGLModuleHandle,"glGetMapfv");
		dglGetMapiv = (glGetMapiv_fp)dll_GetSymbol(OpenGLModuleHandle,"glGetMapiv");
		dglGetMaterialfv = (glGetMaterialfv_fp)dll_GetSymbol(OpenGLModuleHandle,"glGetMaterialfv");
		dglGetMaterialiv = (glGetMaterialiv_fp)dll_GetSymbol(OpenGLModuleHandle,"glGetMaterialiv");
		dglGetPixelMapfv = (glGetPixelMapfv_fp)dll_GetSymbol(OpenGLModuleHandle,"glGetPixelMapfv");
		dglGetPixelMapuiv = (glGetPixelMapuiv_fp)dll_GetSymbol(OpenGLModuleHandle,"glGetPixelMapuiv");
		dglGetPixelMapusv = (glGetPixelMapusv_fp)dll_GetSymbol(OpenGLModuleHandle,"glGetPixelMapusv");
		dglGetPointerv = (glGetPointerv_fp)dll_GetSymbol(OpenGLModuleHandle,"glGetPointerv");
		dglGetPolygonStipple = (glGetPolygonStipple_fp)dll_GetSymbol(OpenGLModuleHandle,"glGetPolygonStipple");
		dglGetString = (glGetString_fp)dll_GetSymbol(OpenGLModuleHandle,"glGetString");
		dglGetTexEnvfv = (glGetTexEnvfv_fp)dll_GetSymbol(OpenGLModuleHandle,"glGetTexEnvfv");
		dglGetTexEnviv = (glGetTexEnviv_fp)dll_GetSymbol(OpenGLModuleHandle,"glGetTexEnviv");
		dglGetTexGendv = (glGetTexGendv_fp)dll_GetSymbol(OpenGLModuleHandle,"glGetTexGendv");
		dglGetTexGenfv = (glGetTexGenfv_fp)dll_GetSymbol(OpenGLModuleHandle,"glGetTexGenfv");
		dglGetTexGeniv = (glGetTexGeniv_fp)dll_GetSymbol(OpenGLModuleHandle,"glGetTexGeniv");
		dglGetTexImage = (glGetTexImage_fp)dll_GetSymbol(OpenGLModuleHandle,"glGetTexImage");
		dglGetTexLevelParameterfv = (glGetTexLevelParameterfv_fp)dll_GetSymbol(OpenGLModuleHandle,"glGetTexLevelParameterfv");
		dglGetTexLevelParameteriv = (glGetTexLevelParameteriv_fp)dll_GetSymbol(OpenGLModuleHandle,"glGetTexLevelParameteriv");
		dglGetTexParameterfv = (glGetTexParameterfv_fp)dll_GetSymbol(OpenGLModuleHandle,"glGetTexParameterfv");
		dglGetTexParameteriv = (glGetTexParameteriv_fp)dll_GetSymbol(OpenGLModuleHandle,"glGetTexParameteriv");
		dglHint = (glHint_fp)dll_GetSymbol(OpenGLModuleHandle,"glHint");
		dglIndexMask = (glIndexMask_fp)dll_GetSymbol(OpenGLModuleHandle,"glIndexMask");
		dglIndexPointer = (glIndexPointer_fp)dll_GetSymbol(OpenGLModuleHandle,"glIndexPointer");
		dglIndexd = (glIndexd_fp)dll_GetSymbol(OpenGLModuleHandle,"glIndexd");
		dglIndexdv = (glIndexdv_fp)dll_GetSymbol(OpenGLModuleHandle,"glIndexdv");
		dglIndexf = (glIndexf_fp)dll_GetSymbol(OpenGLModuleHandle,"glIndexf");
		dglIndexfv = (glIndexfv_fp)dll_GetSymbol(OpenGLModuleHandle,"glIndexfv");
		dglIndexi = (glIndexi_fp)dll_GetSymbol(OpenGLModuleHandle,"glIndexi");
		dglIndexiv = (glIndexiv_fp)dll_GetSymbol(OpenGLModuleHandle,"glIndexiv");
		dglIndexs = (glIndexs_fp)dll_GetSymbol(OpenGLModuleHandle,"glIndexs");
		dglIndexsv = (glIndexsv_fp)dll_GetSymbol(OpenGLModuleHandle,"glIndexsv");
		dglIndexub = (glIndexub_fp)dll_GetSymbol(OpenGLModuleHandle,"glIndexub");
		dglIndexubv = (glIndexubv_fp)dll_GetSymbol(OpenGLModuleHandle,"glIndexubv");
		dglInitNames = (glInitNames_fp)dll_GetSymbol(OpenGLModuleHandle,"glInitNames");
		dglInterleavedArrays = (glInterleavedArrays_fp)dll_GetSymbol(OpenGLModuleHandle,"glInterleavedArrays");
		dglIsEnabled = (glIsEnabled_fp)dll_GetSymbol(OpenGLModuleHandle,"glIsEnabled");
		dglIsList = (glIsList_fp)dll_GetSymbol(OpenGLModuleHandle,"glIsList");
		dglIsTexture = (glIsTexture_fp)dll_GetSymbol(OpenGLModuleHandle,"glIsTexture");
		dglLightModelf = (glLightModelf_fp)dll_GetSymbol(OpenGLModuleHandle,"glLightModelf");
		dglLightModelfv = (glLightModelfv_fp)dll_GetSymbol(OpenGLModuleHandle,"glLightModelfv");
		dglLightModeli = (glLightModeli_fp)dll_GetSymbol(OpenGLModuleHandle,"glLightModeli");
		dglLightModeliv = (glLightModeliv_fp)dll_GetSymbol(OpenGLModuleHandle,"glLightModeliv");
		dglLightf = (glLightf_fp)dll_GetSymbol(OpenGLModuleHandle,"glLightf");
		dglLightfv = (glLightfv_fp)dll_GetSymbol(OpenGLModuleHandle,"glLightfv");
		dglLighti = (glLighti_fp)dll_GetSymbol(OpenGLModuleHandle,"glLighti");
		dglLightiv = (glLightiv_fp)dll_GetSymbol(OpenGLModuleHandle,"glLightiv");
		dglLineStipple = (glLineStipple_fp)dll_GetSymbol(OpenGLModuleHandle,"glLineStipple");
		dglLineWidth = (glLineWidth_fp)dll_GetSymbol(OpenGLModuleHandle,"glLineWidth");
		dglListBase = (glListBase_fp)dll_GetSymbol(OpenGLModuleHandle,"glListBase");
		dglLoadIdentity = (glLoadIdentity_fp)dll_GetSymbol(OpenGLModuleHandle,"glLoadIdentity");
		dglLoadMatrixd = (glLoadMatrixd_fp)dll_GetSymbol(OpenGLModuleHandle,"glLoadMatrixd");
		dglLoadMatrixf = (glLoadMatrixf_fp)dll_GetSymbol(OpenGLModuleHandle,"glLoadMatrixf");
		dglLoadName = (glLoadName_fp)dll_GetSymbol(OpenGLModuleHandle,"glLoadName");
		dglLogicOp = (glLogicOp_fp)dll_GetSymbol(OpenGLModuleHandle,"glLogicOp");
		dglMap1d = (glMap1d_fp)dll_GetSymbol(OpenGLModuleHandle,"glMap1d");
		dglMap1f = (glMap1f_fp)dll_GetSymbol(OpenGLModuleHandle,"glMap1f");
		dglMap2d = (glMap2d_fp)dll_GetSymbol(OpenGLModuleHandle,"glMap2d");
		dglMap2f = (glMap2f_fp)dll_GetSymbol(OpenGLModuleHandle,"glMap2f");
		dglMapGrid1d = (glMapGrid1d_fp)dll_GetSymbol(OpenGLModuleHandle,"glMapGrid1d");
		dglMapGrid1f = (glMapGrid1f_fp)dll_GetSymbol(OpenGLModuleHandle,"glMapGrid1f");
		dglMapGrid2d = (glMapGrid2d_fp)dll_GetSymbol(OpenGLModuleHandle,"glMapGrid2d");
		dglMapGrid2f = (glMapGrid2f_fp)dll_GetSymbol(OpenGLModuleHandle,"glMapGrid2f");
		dglMaterialf = (glMaterialf_fp)dll_GetSymbol(OpenGLModuleHandle,"glMaterialf");
		dglMaterialfv = (glMaterialfv_fp)dll_GetSymbol(OpenGLModuleHandle,"glMaterialfv");
		dglMateriali = (glMateriali_fp)dll_GetSymbol(OpenGLModuleHandle,"glMateriali");
		dglMaterialiv = (glMaterialiv_fp)dll_GetSymbol(OpenGLModuleHandle,"glMaterialiv");
		dglMatrixMode = (glMatrixMode_fp)dll_GetSymbol(OpenGLModuleHandle,"glMatrixMode");
		dglMultMatrixd = (glMultMatrixd_fp)dll_GetSymbol(OpenGLModuleHandle,"glMultMatrixd");
		dglMultMatrixf = (glMultMatrixf_fp)dll_GetSymbol(OpenGLModuleHandle,"glMultMatrixf");
		dglNewList = (glNewList_fp)dll_GetSymbol(OpenGLModuleHandle,"glNewList");
		dglNormal3b = (glNormal3b_fp)dll_GetSymbol(OpenGLModuleHandle,"glNormal3b");
		dglNormal3bv = (glNormal3bv_fp)dll_GetSymbol(OpenGLModuleHandle,"glNormal3bv");
		dglNormal3d = (glNormal3d_fp)dll_GetSymbol(OpenGLModuleHandle,"glNormal3d");
		dglNormal3dv = (glNormal3dv_fp)dll_GetSymbol(OpenGLModuleHandle,"glNormal3dv");
		dglNormal3f = (glNormal3f_fp)dll_GetSymbol(OpenGLModuleHandle,"glNormal3f");
		dglNormal3fv = (glNormal3fv_fp)dll_GetSymbol(OpenGLModuleHandle,"glNormal3fv");
		dglNormal3i = (glNormal3i_fp)dll_GetSymbol(OpenGLModuleHandle,"glNormal3i");
		dglNormal3iv = (glNormal3iv_fp)dll_GetSymbol(OpenGLModuleHandle,"glNormal3iv");
		dglNormal3s = (glNormal3s_fp)dll_GetSymbol(OpenGLModuleHandle,"glNormal3s");
		dglNormal3sv = (glNormal3sv_fp)dll_GetSymbol(OpenGLModuleHandle,"glNormal3sv");
		dglNormalPointer = (glNormalPointer_fp)dll_GetSymbol(OpenGLModuleHandle,"glNormalPointer");
		dglOrtho = (glOrtho_fp)dll_GetSymbol(OpenGLModuleHandle,"glOrtho");
		dglPassThrough = (glPassThrough_fp)dll_GetSymbol(OpenGLModuleHandle,"glPassThrough");
		dglPixelMapfv = (glPixelMapfv_fp)dll_GetSymbol(OpenGLModuleHandle,"glPixelMapfv");
		dglPixelMapuiv = (glPixelMapuiv_fp)dll_GetSymbol(OpenGLModuleHandle,"glPixelMapuiv");
		dglPixelMapusv = (glPixelMapusv_fp)dll_GetSymbol(OpenGLModuleHandle,"glPixelMapusv");
		dglPixelStoref = (glPixelStoref_fp)dll_GetSymbol(OpenGLModuleHandle,"glPixelStoref");
		dglPixelStorei = (glPixelStorei_fp)dll_GetSymbol(OpenGLModuleHandle,"glPixelStorei");
		dglPixelTransferf = (glPixelTransferf_fp)dll_GetSymbol(OpenGLModuleHandle,"glPixelTransferf");
		dglPixelTransferi = (glPixelTransferi_fp)dll_GetSymbol(OpenGLModuleHandle,"glPixelTransferi");
		dglPixelZoom = (glPixelZoom_fp)dll_GetSymbol(OpenGLModuleHandle,"glPixelZoom");
		dglPointSize = (glPointSize_fp)dll_GetSymbol(OpenGLModuleHandle,"glPointSize");
		dglPolygonMode = (glPolygonMode_fp)dll_GetSymbol(OpenGLModuleHandle,"glPolygonMode");
		dglPolygonOffset = (glPolygonOffset_fp)dll_GetSymbol(OpenGLModuleHandle,"glPolygonOffset");
		dglPolygonStipple = (glPolygonStipple_fp)dll_GetSymbol(OpenGLModuleHandle,"glPolygonStipple");
		dglPopAttrib = (glPopAttrib_fp)dll_GetSymbol(OpenGLModuleHandle,"glPopAttrib");
		dglPopClientAttrib = (glPopClientAttrib_fp)dll_GetSymbol(OpenGLModuleHandle,"glPopClientAttrib");
		dglPopMatrix = (glPopMatrix_fp)dll_GetSymbol(OpenGLModuleHandle,"glPopMatrix");
		dglPopName = (glPopName_fp)dll_GetSymbol(OpenGLModuleHandle,"glPopName");
		dglPrioritizeTextures = (glPrioritizeTextures_fp)dll_GetSymbol(OpenGLModuleHandle,"glPrioritizeTextures");
		dglPushAttrib = (glPushAttrib_fp)dll_GetSymbol(OpenGLModuleHandle,"glPushAttrib");
		dglPushClientAttrib = (glPushClientAttrib_fp)dll_GetSymbol(OpenGLModuleHandle,"glPushClientAttrib");
		dglPushMatrix = (glPushMatrix_fp)dll_GetSymbol(OpenGLModuleHandle,"glPushMatrix");
		dglPushName = (glPushName_fp)dll_GetSymbol(OpenGLModuleHandle,"glPushName");
		dglRasterPos2d = (glRasterPos2d_fp)dll_GetSymbol(OpenGLModuleHandle,"glRasterPos2d");
		dglRasterPos2dv = (glRasterPos2dv_fp)dll_GetSymbol(OpenGLModuleHandle,"glRasterPos2dv");
		dglRasterPos2f = (glRasterPos2f_fp)dll_GetSymbol(OpenGLModuleHandle,"glRasterPos2f");
		dglRasterPos2fv = (glRasterPos2fv_fp)dll_GetSymbol(OpenGLModuleHandle,"glRasterPos2fv");
		dglRasterPos2i = (glRasterPos2i_fp)dll_GetSymbol(OpenGLModuleHandle,"glRasterPos2i");
		dglRasterPos2iv = (glRasterPos2iv_fp)dll_GetSymbol(OpenGLModuleHandle,"glRasterPos2iv");
		dglRasterPos2s = (glRasterPos2s_fp)dll_GetSymbol(OpenGLModuleHandle,"glRasterPos2s");
		dglRasterPos2sv = (glRasterPos2sv_fp)dll_GetSymbol(OpenGLModuleHandle,"glRasterPos2sv");
		dglRasterPos3d = (glRasterPos3d_fp)dll_GetSymbol(OpenGLModuleHandle,"glRasterPos3d");
		dglRasterPos3dv = (glRasterPos3dv_fp)dll_GetSymbol(OpenGLModuleHandle,"glRasterPos3dv");
		dglRasterPos3f = (glRasterPos3f_fp)dll_GetSymbol(OpenGLModuleHandle,"glRasterPos3f");
		dglRasterPos3fv = (glRasterPos3fv_fp)dll_GetSymbol(OpenGLModuleHandle,"glRasterPos3fv");
		dglRasterPos3i = (glRasterPos3i_fp)dll_GetSymbol(OpenGLModuleHandle,"glRasterPos3i");
		dglRasterPos3iv = (glRasterPos3iv_fp)dll_GetSymbol(OpenGLModuleHandle,"glRasterPos3iv");
		dglRasterPos3s = (glRasterPos3s_fp)dll_GetSymbol(OpenGLModuleHandle,"glRasterPos3s");
		dglRasterPos3sv = (glRasterPos3sv_fp)dll_GetSymbol(OpenGLModuleHandle,"glRasterPos3sv");
		dglRasterPos4d = (glRasterPos4d_fp)dll_GetSymbol(OpenGLModuleHandle,"glRasterPos4d");
		dglRasterPos4dv = (glRasterPos4dv_fp)dll_GetSymbol(OpenGLModuleHandle,"glRasterPos4dv");
		dglRasterPos4f = (glRasterPos4f_fp)dll_GetSymbol(OpenGLModuleHandle,"glRasterPos4f");
		dglRasterPos4fv = (glRasterPos4fv_fp)dll_GetSymbol(OpenGLModuleHandle,"glRasterPos4fv");
		dglRasterPos4i = (glRasterPos4i_fp)dll_GetSymbol(OpenGLModuleHandle,"glRasterPos4i");
		dglRasterPos4iv = (glRasterPos4iv_fp)dll_GetSymbol(OpenGLModuleHandle,"glRasterPos4iv");
		dglRasterPos4s = (glRasterPos4s_fp)dll_GetSymbol(OpenGLModuleHandle,"glRasterPos4s");
		dglRasterPos4sv = (glRasterPos4sv_fp)dll_GetSymbol(OpenGLModuleHandle,"glRasterPos4sv");
		dglReadBuffer = (glReadBuffer_fp)dll_GetSymbol(OpenGLModuleHandle,"glReadBuffer");
		dglReadPixels = (glReadPixels_fp)dll_GetSymbol(OpenGLModuleHandle,"glReadPixels");
		dglRectd = (glRectd_fp)dll_GetSymbol(OpenGLModuleHandle,"glRectd");
		dglRectdv = (glRectdv_fp)dll_GetSymbol(OpenGLModuleHandle,"glRectdv");
		dglRectf = (glRectf_fp)dll_GetSymbol(OpenGLModuleHandle,"glRectf");
		dglRectfv = (glRectfv_fp)dll_GetSymbol(OpenGLModuleHandle,"glRectfv");
		dglRecti = (glRecti_fp)dll_GetSymbol(OpenGLModuleHandle,"glRecti");
		dglRectiv = (glRectiv_fp)dll_GetSymbol(OpenGLModuleHandle,"glRectiv");
		dglRects = (glRects_fp)dll_GetSymbol(OpenGLModuleHandle,"glRects");
		dglRectsv = (glRectsv_fp)dll_GetSymbol(OpenGLModuleHandle,"glRectsv");
		dglRenderMode = (glRenderMode_fp)dll_GetSymbol(OpenGLModuleHandle,"glRenderMode");
		dglRotated = (glRotated_fp)dll_GetSymbol(OpenGLModuleHandle,"glRotated");
		dglRotatef = (glRotatef_fp)dll_GetSymbol(OpenGLModuleHandle,"glRotatef");
		dglScaled = (glScaled_fp)dll_GetSymbol(OpenGLModuleHandle,"glScaled");
		dglScalef = (glScalef_fp)dll_GetSymbol(OpenGLModuleHandle,"glScalef");
		dglScissor = (glScissor_fp)dll_GetSymbol(OpenGLModuleHandle,"glScissor");
		dglSelectBuffer = (glSelectBuffer_fp)dll_GetSymbol(OpenGLModuleHandle,"glSelectBuffer");
		dglShadeModel = (glShadeModel_fp)dll_GetSymbol(OpenGLModuleHandle,"glShadeModel");
		dglStencilFunc = (glStencilFunc_fp)dll_GetSymbol(OpenGLModuleHandle,"glStencilFunc");
		dglStencilMask = (glStencilMask_fp)dll_GetSymbol(OpenGLModuleHandle,"glStencilMask");
		dglStencilOp = (glStencilOp_fp)dll_GetSymbol(OpenGLModuleHandle,"glStencilOp");
		dglTexCoord1d = (glTexCoord1d_fp)dll_GetSymbol(OpenGLModuleHandle,"glTexCoord1d");
		dglTexCoord1dv = (glTexCoord1dv_fp)dll_GetSymbol(OpenGLModuleHandle,"glTexCoord1dv");
		dglTexCoord1f = (glTexCoord1f_fp)dll_GetSymbol(OpenGLModuleHandle,"glTexCoord1f");
		dglTexCoord1fv = (glTexCoord1fv_fp)dll_GetSymbol(OpenGLModuleHandle,"glTexCoord1fv");
		dglTexCoord1i = (glTexCoord1i_fp)dll_GetSymbol(OpenGLModuleHandle,"glTexCoord1i");
		dglTexCoord1iv = (glTexCoord1iv_fp)dll_GetSymbol(OpenGLModuleHandle,"glTexCoord1iv");
		dglTexCoord1s = (glTexCoord1s_fp)dll_GetSymbol(OpenGLModuleHandle,"glTexCoord1s");
		dglTexCoord1sv = (glTexCoord1sv_fp)dll_GetSymbol(OpenGLModuleHandle,"glTexCoord1sv");
		dglTexCoord2d = (glTexCoord2d_fp)dll_GetSymbol(OpenGLModuleHandle,"glTexCoord2d");
		dglTexCoord2dv = (glTexCoord2dv_fp)dll_GetSymbol(OpenGLModuleHandle,"glTexCoord2dv");
		dglTexCoord2f = (glTexCoord2f_fp)dll_GetSymbol(OpenGLModuleHandle,"glTexCoord2f");
		dglTexCoord2fv = (glTexCoord2fv_fp)dll_GetSymbol(OpenGLModuleHandle,"glTexCoord2fv");
		dglTexCoord2i = (glTexCoord2i_fp)dll_GetSymbol(OpenGLModuleHandle,"glTexCoord2i");
		dglTexCoord2iv = (glTexCoord2iv_fp)dll_GetSymbol(OpenGLModuleHandle,"glTexCoord2iv");
		dglTexCoord2s = (glTexCoord2s_fp)dll_GetSymbol(OpenGLModuleHandle,"glTexCoord2s");
		dglTexCoord2sv = (glTexCoord2sv_fp)dll_GetSymbol(OpenGLModuleHandle,"glTexCoord2sv");
		dglTexCoord3d = (glTexCoord3d_fp)dll_GetSymbol(OpenGLModuleHandle,"glTexCoord3d");
		dglTexCoord3dv = (glTexCoord3dv_fp)dll_GetSymbol(OpenGLModuleHandle,"glTexCoord3dv");
		dglTexCoord3f = (glTexCoord3f_fp)dll_GetSymbol(OpenGLModuleHandle,"glTexCoord3f");
		dglTexCoord3fv = (glTexCoord3fv_fp)dll_GetSymbol(OpenGLModuleHandle,"glTexCoord3fv");
		dglTexCoord3i = (glTexCoord3i_fp)dll_GetSymbol(OpenGLModuleHandle,"glTexCoord3i");
		dglTexCoord3iv = (glTexCoord3iv_fp)dll_GetSymbol(OpenGLModuleHandle,"glTexCoord3iv");
		dglTexCoord3s = (glTexCoord3s_fp)dll_GetSymbol(OpenGLModuleHandle,"glTexCoord3s");
		dglTexCoord3sv = (glTexCoord3sv_fp)dll_GetSymbol(OpenGLModuleHandle,"glTexCoord3sv");
		dglTexCoord4d = (glTexCoord4d_fp)dll_GetSymbol(OpenGLModuleHandle,"glTexCoord4d");
		dglTexCoord4dv = (glTexCoord4dv_fp)dll_GetSymbol(OpenGLModuleHandle,"glTexCoord4dv");
		dglTexCoord4f = (glTexCoord4f_fp)dll_GetSymbol(OpenGLModuleHandle,"glTexCoord4f");
		dglTexCoord4fv = (glTexCoord4fv_fp)dll_GetSymbol(OpenGLModuleHandle,"glTexCoord4fv");
		dglTexCoord4i = (glTexCoord4i_fp)dll_GetSymbol(OpenGLModuleHandle,"glTexCoord4i");
		dglTexCoord4iv = (glTexCoord4iv_fp)dll_GetSymbol(OpenGLModuleHandle,"glTexCoord4iv");
		dglTexCoord4s = (glTexCoord4s_fp)dll_GetSymbol(OpenGLModuleHandle,"glTexCoord4s");
		dglTexCoord4sv = (glTexCoord4sv_fp)dll_GetSymbol(OpenGLModuleHandle,"glTexCoord4sv");
		dglTexCoordPointer = (glTexCoordPointer_fp)dll_GetSymbol(OpenGLModuleHandle,"glTexCoordPointer");
		dglTexEnvf = (glTexEnvf_fp)dll_GetSymbol(OpenGLModuleHandle,"glTexEnvf");
		dglTexEnvfv = (glTexEnvfv_fp)dll_GetSymbol(OpenGLModuleHandle,"glTexEnvfv");
		dglTexEnvi = (glTexEnvi_fp)dll_GetSymbol(OpenGLModuleHandle,"glTexEnvi");
		dglTexEnviv = (glTexEnviv_fp)dll_GetSymbol(OpenGLModuleHandle,"glTexEnviv");
		dglTexGend = (glTexGend_fp)dll_GetSymbol(OpenGLModuleHandle,"glTexGend");
		dglTexGendv = (glTexGendv_fp)dll_GetSymbol(OpenGLModuleHandle,"glTexGendv");
		dglTexGenf = (glTexGenf_fp)dll_GetSymbol(OpenGLModuleHandle,"glTexGenf");
		dglTexGenfv = (glTexGenfv_fp)dll_GetSymbol(OpenGLModuleHandle,"glTexGenfv");
		dglTexGeni = (glTexGeni_fp)dll_GetSymbol(OpenGLModuleHandle,"glTexGeni");
		dglTexGeniv = (glTexGeniv_fp)dll_GetSymbol(OpenGLModuleHandle,"glTexGeniv");
		dglTexImage1D = (glTexImage1D_fp)dll_GetSymbol(OpenGLModuleHandle,"glTexImage1D");
		dglTexImage2D = (glTexImage2D_fp)dll_GetSymbol(OpenGLModuleHandle,"glTexImage2D");
		dglTexParameterf = (glTexParameterf_fp)dll_GetSymbol(OpenGLModuleHandle,"glTexParameterf");
		dglTexParameterfv = (glTexParameterfv_fp)dll_GetSymbol(OpenGLModuleHandle,"glTexParameterfv");
		dglTexParameteri = (glTexParameteri_fp)dll_GetSymbol(OpenGLModuleHandle,"glTexParameteri");
		dglTexParameteriv = (glTexParameteriv_fp)dll_GetSymbol(OpenGLModuleHandle,"glTexParameteriv");
		dglTexSubImage1D = (glTexSubImage1D_fp)dll_GetSymbol(OpenGLModuleHandle,"glTexSubImage1D");
		dglTexSubImage2D = (glTexSubImage2D_fp)dll_GetSymbol(OpenGLModuleHandle,"glTexSubImage2D");
		dglTranslated = (glTranslated_fp)dll_GetSymbol(OpenGLModuleHandle,"glTranslated");
		dglTranslatef = (glTranslatef_fp)dll_GetSymbol(OpenGLModuleHandle,"glTranslatef");
		dglVertex2d = (glVertex2d_fp)dll_GetSymbol(OpenGLModuleHandle,"glVertex2d");
		dglVertex2dv = (glVertex2dv_fp)dll_GetSymbol(OpenGLModuleHandle,"glVertex2dv");
		dglVertex2f = (glVertex2f_fp)dll_GetSymbol(OpenGLModuleHandle,"glVertex2f");
		dglVertex2fv = (glVertex2fv_fp)dll_GetSymbol(OpenGLModuleHandle,"glVertex2fv");
		dglVertex2i = (glVertex2i_fp)dll_GetSymbol(OpenGLModuleHandle,"glVertex2i");
		dglVertex2iv = (glVertex2iv_fp)dll_GetSymbol(OpenGLModuleHandle,"glVertex2iv");
		dglVertex2s = (glVertex2s_fp)dll_GetSymbol(OpenGLModuleHandle,"glVertex2s");
		dglVertex2sv = (glVertex2sv_fp)dll_GetSymbol(OpenGLModuleHandle,"glVertex2sv");
		dglVertex3d = (glVertex3d_fp)dll_GetSymbol(OpenGLModuleHandle,"glVertex3d");
		dglVertex3dv = (glVertex3dv_fp)dll_GetSymbol(OpenGLModuleHandle,"glVertex3dv");
		dglVertex3f = (glVertex3f_fp)dll_GetSymbol(OpenGLModuleHandle,"glVertex3f");
		dglVertex3fv = (glVertex3fv_fp)dll_GetSymbol(OpenGLModuleHandle,"glVertex3fv");
		dglVertex3i = (glVertex3i_fp)dll_GetSymbol(OpenGLModuleHandle,"glVertex3i");
		dglVertex3iv = (glVertex3iv_fp)dll_GetSymbol(OpenGLModuleHandle,"glVertex3iv");
		dglVertex3s = (glVertex3s_fp)dll_GetSymbol(OpenGLModuleHandle,"glVertex3s");
		dglVertex3sv = (glVertex3sv_fp)dll_GetSymbol(OpenGLModuleHandle,"glVertex3sv");
		dglVertex4d = (glVertex4d_fp)dll_GetSymbol(OpenGLModuleHandle,"glVertex4d");
		dglVertex4dv = (glVertex4dv_fp)dll_GetSymbol(OpenGLModuleHandle,"glVertex4dv");
		dglVertex4f = (glVertex4f_fp)dll_GetSymbol(OpenGLModuleHandle,"glVertex4f");
		dglVertex4fv = (glVertex4fv_fp)dll_GetSymbol(OpenGLModuleHandle,"glVertex4fv");
		dglVertex4i = (glVertex4i_fp)dll_GetSymbol(OpenGLModuleHandle,"glVertex4i");
		dglVertex4iv = (glVertex4iv_fp)dll_GetSymbol(OpenGLModuleHandle,"glVertex4iv");
		dglVertex4s = (glVertex4s_fp)dll_GetSymbol(OpenGLModuleHandle,"glVertex4s");
		dglVertex4sv = (glVertex4sv_fp)dll_GetSymbol(OpenGLModuleHandle,"glVertex4sv");
		dglVertexPointer = (glVertexPointer_fp)dll_GetSymbol(OpenGLModuleHandle,"glVertexPointer");
		dglViewport = (glViewport_fp)dll_GetSymbol(OpenGLModuleHandle,"glViewport");

#ifdef _WIN32
		dwglCopyContext = (wglCopyContext_fp)dll_GetSymbol(OpenGLModuleHandle,"wglCopyContext");
		dwglCreateContext = (wglCreateContext_fp)dll_GetSymbol(OpenGLModuleHandle,"wglCreateContext");
		dwglCreateLayerContext = (wglCreateLayerContext_fp)dll_GetSymbol(OpenGLModuleHandle,"wglCreateLayerContext");
		dwglDeleteContext = (wglDeleteContext_fp)dll_GetSymbol(OpenGLModuleHandle,"wglDeleteContext");
		dwglGetCurrentContext = (wglGetCurrentContext_fp)dll_GetSymbol(OpenGLModuleHandle,"wglGetCurrentContext");
		dwglGetCurrentDC = (wglGetCurrentDC_fp)dll_GetSymbol(OpenGLModuleHandle,"wglGetCurrentDC");
		dwglGetProcAddress = (wglGetProcAddress_fp)dll_GetSymbol(OpenGLModuleHandle,"wglGetProcAddress");
		dwglMakeCurrent = (wglMakeCurrent_fp)dll_GetSymbol(OpenGLModuleHandle,"wglMakeCurrent");
		dwglShareLists = (wglShareLists_fp)dll_GetSymbol(OpenGLModuleHandle,"wglShareLists");
		dwglUseFontBitmapsA = (wglUseFontBitmapsA_fp)dll_GetSymbol(OpenGLModuleHandle,"wglUseFontBitmapsA");
		dwglUseFontBitmapsW = (wglUseFontBitmapsW_fp)dll_GetSymbol(OpenGLModuleHandle,"wglUseFontBitmapsW");
		dwglUseFontOutlinesA = (wglUseFontOutlinesA_fp)dll_GetSymbol(OpenGLModuleHandle,"wglUseFontOutlinesA");
		dwglUseFontOutlinesW = (wglUseFontOutlinesW_fp)dll_GetSymbol(OpenGLModuleHandle,"wglUseFontOutlinesW");
		dwglDescribeLayerPlane = (wglDescribeLayerPlane_fp)dll_GetSymbol(OpenGLModuleHandle,"wglDescribeLayerPlane");
		dwglSetLayerPaletteEntries = (wglSetLayerPaletteEntries_fp)dll_GetSymbol(OpenGLModuleHandle,"wglSetLayerPaletteEntries");
		dwglGetLayerPaletteEntries = (wglGetLayerPaletteEntries_fp)dll_GetSymbol(OpenGLModuleHandle,"wglGetLayerPaletteEntries");
		dwglRealizeLayerPalette = (wglRealizeLayerPalette_fp)dll_GetSymbol(OpenGLModuleHandle,"wglRealizeLayerPalette");
		dwglSwapLayerBuffers = (wglSwapLayerBuffers_fp)dll_GetSymbol(OpenGLModuleHandle,"wglSwapLayerBuffers");
		#if (WINVER >= 0x0500)
		dwglSwapMultipleBuffers = (wglSwapMultipleBuffers_fp)dll_GetSymbol(OpenGLModuleHandle,"wglSwapMultipleBuffers");
		#endif
#endif

	}

	return true;
}

void OpenGL_SetFuncsToNull(void)
{
	dglAccum = NULL;
	dglAlphaFunc = NULL;
	dglAreTexturesResident = NULL;
	dglArrayElement = NULL;
	dglBegin = NULL;
	dglBindTexture = NULL;
	dglBitmap = NULL;
	dglBlendFunc = NULL;
	dglCallList = NULL;
	dglCallLists = NULL;
	dglClear = NULL;
	dglClearAccum = NULL;
	dglClearColor = NULL;
	dglClearDepth = NULL;
	dglClearIndex = NULL;
	dglClearStencil = NULL;
	dglClipPlane = NULL;
	dglColor3b = NULL;
	dglColor3bv = NULL;
	dglColor3d = NULL;
	dglColor3dv = NULL;
	dglColor3f = NULL;
	dglColor3fv = NULL;
	dglColor3i = NULL;
	dglColor3iv = NULL;
	dglColor3s = NULL;
	dglColor3sv = NULL;
	dglColor3ub = NULL;
	dglColor3ubv = NULL;
	dglColor3ui = NULL;
	dglColor3uiv = NULL;
	dglColor3us = NULL;
	dglColor3usv = NULL;
	dglColor4b = NULL;
	dglColor4bv = NULL;
	dglColor4d = NULL;
	dglColor4dv = NULL;
	dglColor4f = NULL;
	dglColor4fv = NULL;
	dglColor4i = NULL;
	dglColor4iv = NULL;
	dglColor4s = NULL;
	dglColor4sv = NULL;
	dglColor4ub = NULL;
	dglColor4ubv = NULL;
	dglColor4ui = NULL;
	dglColor4uiv = NULL;
	dglColor4us = NULL;
	dglColor4usv = NULL;
	dglColorMask = NULL;
	dglColorMaterial = NULL;
	dglColorPointer = NULL;
	dglCopyPixels = NULL;
	dglCopyTexImage1D = NULL;
	dglCopyTexImage2D = NULL;
	dglCopyTexSubImage1D = NULL;
	dglCopyTexSubImage2D = NULL;
	dglCullFace = NULL;
	dglDeleteLists = NULL;
	dglDeleteTextures = NULL;
	dglDepthFunc = NULL;
	dglDepthMask = NULL;
	dglDepthRange = NULL;
	dglDisable = NULL;
	dglDisableClientState = NULL;
	dglDrawArrays = NULL;
	dglDrawBuffer = NULL;
	dglDrawElements = NULL;
	dglDrawPixels = NULL;
	dglEdgeFlag = NULL;
	dglEdgeFlagPointer = NULL;
	dglEdgeFlagv = NULL;
	dglEnable = NULL;
	dglEnableClientState = NULL;
	dglEnd = NULL;
	dglEndList = NULL;
	dglEvalCoord1d = NULL;
	dglEvalCoord1dv = NULL;
	dglEvalCoord1f = NULL;
	dglEvalCoord1fv = NULL;
	dglEvalCoord2d = NULL;
	dglEvalCoord2dv = NULL;
	dglEvalCoord2f = NULL;
	dglEvalCoord2fv = NULL;
	dglEvalMesh1 = NULL;
	dglEvalMesh2 = NULL;
	dglEvalPoint1 = NULL;
	dglEvalPoint2 = NULL;
	dglFeedbackBuffer = NULL;
	dglFinish = NULL;
	dglFlush = NULL;
	dglFogf = NULL;
	dglFogfv = NULL;
	dglFogi = NULL;
	dglFogiv = NULL;
	dglFrontFace = NULL;
	dglFrustum = NULL;
	dglGenLists = NULL;
	dglGenTextures = NULL;
	dglGetBooleanv = NULL;
	dglGetClipPlane = NULL;
	dglGetDoublev = NULL;
	dglGetError = NULL;
	dglGetFloatv = NULL;
	dglGetIntegerv = NULL;
	dglGetLightfv = NULL;
	dglGetLightiv = NULL;
	dglGetMapdv = NULL;
	dglGetMapfv = NULL;
	dglGetMapiv = NULL;
	dglGetMaterialfv = NULL;
	dglGetMaterialiv = NULL;
	dglGetPixelMapfv = NULL;
	dglGetPixelMapuiv = NULL;
	dglGetPixelMapusv = NULL;
	dglGetPointerv = NULL;
	dglGetPolygonStipple = NULL;
	dglGetString = NULL;
	dglGetTexEnvfv = NULL;
	dglGetTexEnviv = NULL;
	dglGetTexGendv = NULL;
	dglGetTexGenfv = NULL;
	dglGetTexGeniv = NULL;
	dglGetTexImage = NULL;
	dglGetTexLevelParameterfv = NULL;
	dglGetTexLevelParameteriv = NULL;
	dglGetTexParameterfv = NULL;
	dglGetTexParameteriv = NULL;
	dglHint = NULL;
	dglIndexMask = NULL;
	dglIndexPointer = NULL;
	dglIndexd = NULL;
	dglIndexdv = NULL;
	dglIndexf = NULL;
	dglIndexfv = NULL;
	dglIndexi = NULL;
	dglIndexiv = NULL;
	dglIndexs = NULL;
	dglIndexsv = NULL;
	dglIndexub = NULL;
	dglIndexubv = NULL;
	dglInitNames = NULL;
	dglInterleavedArrays = NULL;
	dglIsEnabled = NULL;
	dglIsList = NULL;
	dglIsTexture = NULL;
	dglLightModelf = NULL;
	dglLightModelfv = NULL;
	dglLightModeli = NULL;
	dglLightModeliv = NULL;
	dglLightf = NULL;
	dglLightfv = NULL;
	dglLighti = NULL;
	dglLightiv = NULL;
	dglLineStipple = NULL;
	dglLineWidth = NULL;
	dglListBase = NULL;
	dglLoadIdentity = NULL;
	dglLoadMatrixd = NULL;
	dglLoadMatrixf = NULL;
	dglLoadName = NULL;
	dglLogicOp = NULL;
	dglMap1d = NULL;
	dglMap1f = NULL;
	dglMap2d = NULL;
	dglMap2f = NULL;
	dglMapGrid1d = NULL;
	dglMapGrid1f = NULL;
	dglMapGrid2d = NULL;
	dglMapGrid2f = NULL;
	dglMaterialf = NULL;
	dglMaterialfv = NULL;
	dglMateriali = NULL;
	dglMaterialiv = NULL;
	dglMatrixMode = NULL;
	dglMultMatrixd = NULL;
	dglMultMatrixf = NULL;
	dglNewList = NULL;
	dglNormal3b = NULL;
	dglNormal3bv = NULL;
	dglNormal3d = NULL;
	dglNormal3dv = NULL;
	dglNormal3f = NULL;
	dglNormal3fv = NULL;
	dglNormal3i = NULL;
	dglNormal3iv = NULL;
	dglNormal3s = NULL;
	dglNormal3sv = NULL;
	dglNormalPointer = NULL;
	dglOrtho = NULL;
	dglPassThrough = NULL;
	dglPixelMapfv = NULL;
	dglPixelMapuiv = NULL;
	dglPixelMapusv = NULL;
	dglPixelStoref = NULL;
	dglPixelStorei = NULL;
	dglPixelTransferf = NULL;
	dglPixelTransferi = NULL;
	dglPixelZoom = NULL;
	dglPointSize = NULL;
	dglPolygonMode = NULL;
	dglPolygonOffset = NULL;
	dglPolygonStipple = NULL;
	dglPopAttrib = NULL;
	dglPopClientAttrib = NULL;
	dglPopMatrix = NULL;
	dglPopName = NULL;
	dglPrioritizeTextures = NULL;
	dglPushAttrib = NULL;
	dglPushClientAttrib = NULL;
	dglPushMatrix = NULL;
	dglPushName = NULL;
	dglRasterPos2d = NULL;
	dglRasterPos2dv = NULL;
	dglRasterPos2f = NULL;
	dglRasterPos2fv = NULL;
	dglRasterPos2i = NULL;
	dglRasterPos2iv = NULL;
	dglRasterPos2s = NULL;
	dglRasterPos2sv = NULL;
	dglRasterPos3d = NULL;
	dglRasterPos3dv = NULL;
	dglRasterPos3f = NULL;
	dglRasterPos3fv = NULL;
	dglRasterPos3i = NULL;
	dglRasterPos3iv = NULL;
	dglRasterPos3s = NULL;
	dglRasterPos3sv = NULL;
	dglRasterPos4d = NULL;
	dglRasterPos4dv = NULL;
	dglRasterPos4f = NULL;
	dglRasterPos4fv = NULL;
	dglRasterPos4i = NULL;
	dglRasterPos4iv = NULL;
	dglRasterPos4s = NULL;
	dglRasterPos4sv = NULL;
	dglReadBuffer = NULL;
	dglReadPixels = NULL;
	dglRectd = NULL;
	dglRectdv = NULL;
	dglRectf = NULL;
	dglRectfv = NULL;
	dglRecti = NULL;
	dglRectiv = NULL;
	dglRects = NULL;
	dglRectsv = NULL;
	dglRenderMode = NULL;
	dglRotated = NULL;
	dglRotatef = NULL;
	dglScaled = NULL;
	dglScalef = NULL;
	dglScissor = NULL;
	dglSelectBuffer = NULL;
	dglShadeModel = NULL;
	dglStencilFunc = NULL;
	dglStencilMask = NULL;
	dglStencilOp = NULL;
	dglTexCoord1d = NULL;
	dglTexCoord1dv = NULL;
	dglTexCoord1f = NULL;
	dglTexCoord1fv = NULL;
	dglTexCoord1i = NULL;
	dglTexCoord1iv = NULL;
	dglTexCoord1s = NULL;
	dglTexCoord1sv = NULL;
	dglTexCoord2d = NULL;
	dglTexCoord2dv = NULL;
	dglTexCoord2f = NULL;
	dglTexCoord2fv = NULL;
	dglTexCoord2i = NULL;
	dglTexCoord2iv = NULL;
	dglTexCoord2s = NULL;
	dglTexCoord2sv = NULL;
	dglTexCoord3d = NULL;
	dglTexCoord3dv = NULL;
	dglTexCoord3f = NULL;
	dglTexCoord3fv = NULL;
	dglTexCoord3i = NULL;
	dglTexCoord3iv = NULL;
	dglTexCoord3s = NULL;
	dglTexCoord3sv = NULL;
	dglTexCoord4d = NULL;
	dglTexCoord4dv = NULL;
	dglTexCoord4f = NULL;
	dglTexCoord4fv = NULL;
	dglTexCoord4i = NULL;
	dglTexCoord4iv = NULL;
	dglTexCoord4s = NULL;
	dglTexCoord4sv = NULL;
	dglTexCoordPointer = NULL;
	dglTexEnvf = NULL;
	dglTexEnvfv = NULL;
	dglTexEnvi = NULL;
	dglTexEnviv = NULL;
	dglTexGend = NULL;
	dglTexGendv = NULL;
	dglTexGenf = NULL;
	dglTexGenfv = NULL;
	dglTexGeni = NULL;
	dglTexGeniv = NULL;
	dglTexImage1D = NULL;
	dglTexImage2D = NULL;
	dglTexParameterf = NULL;
	dglTexParameterfv = NULL;
	dglTexParameteri = NULL;
	dglTexParameteriv = NULL;
	dglTexSubImage1D = NULL;
	dglTexSubImage2D = NULL;
	dglTranslated = NULL;
	dglTranslatef = NULL;
	dglVertex2d = NULL;
	dglVertex2dv = NULL;
	dglVertex2f = NULL;
	dglVertex2fv = NULL;
	dglVertex2i = NULL;
	dglVertex2iv = NULL;
	dglVertex2s = NULL;
	dglVertex2sv = NULL;
	dglVertex3d = NULL;
	dglVertex3dv = NULL;
	dglVertex3f = NULL;
	dglVertex3fv = NULL;
	dglVertex3i = NULL;
	dglVertex3iv = NULL;
	dglVertex3s = NULL;
	dglVertex3sv = NULL;
	dglVertex4d = NULL;
	dglVertex4dv = NULL;
	dglVertex4f = NULL;
	dglVertex4fv = NULL;
	dglVertex4i = NULL;
	dglVertex4iv = NULL;
	dglVertex4s = NULL;
	dglVertex4sv = NULL;
	dglVertexPointer = NULL;
	dglViewport = NULL;

	dglMultiTexCoord2fARB = NULL;
	dglActiveTextureARB = NULL;
	dglMultiTexCoord2fSGIS = NULL;
	dglSelectTextureSGIS = NULL;
	dglColorTableEXT = NULL;

	dglCombinerParameteriNV = NULL;
	dglCombinerInputNV = NULL;
	dglCombinerOutputNV = NULL;
	dglFinalCombinerInputNV = NULL;

#ifdef _WIN32
	dwglCopyContext = NULL;
	dwglCreateContext = NULL;
	dwglCreateLayerContext = NULL;
	dwglDeleteContext = NULL;
	dwglGetCurrentContext = NULL;
	dwglGetCurrentDC = NULL;
	dwglGetProcAddress = NULL;
	dwglMakeCurrent = NULL;
	dwglShareLists = NULL;
	dwglUseFontBitmapsA = NULL;
	dwglUseFontBitmapsW = NULL;
	dwglUseFontOutlinesA = NULL;
	dwglUseFontOutlinesW = NULL;
	dwglDescribeLayerPlane = NULL;
	dwglSetLayerPaletteEntries = NULL;
	dwglGetLayerPaletteEntries = NULL;
	dwglRealizeLayerPalette = NULL;
	dwglSwapLayerBuffers = NULL;
	#if (WINVER >= 0x0500)
	dwglSwapMultipleBuffers = NULL;
	#endif
#endif

}
#endif


#endif //!__LOADGL_H__
