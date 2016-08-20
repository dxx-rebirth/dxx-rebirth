/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
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

#define glMultiTexCoord2fARB dglMultiTexCoord2fARB
#define glActiveTextureARB dglActiveTextureARB
#define glMultiTexCoord2fSGIS dglMultiTexCoord2fSGIS
#define glSelectTextureSGIS dglSelectTextureSGIS



#ifdef _WIN32
#define wglGetProcAddress dwglGetProcAddress
#if (WINVER >= 0x0500)
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

DEFVAR glMultiTexCoord2fARB_fp dglMultiTexCoord2fARB;
DEFVAR glActiveTextureARB_fp dglActiveTextureARB;
DEFVAR glMultiTexCoord2fSGIS_fp dglMultiTexCoord2fSGIS;
DEFVAR glSelectTextureSGIS_fp dglSelectTextureSGIS;

#ifdef _WIN32
DEFVAR wglGetProcAddress_fp dwglGetProcAddress;
#if (WINVER >= 0x0500)
#endif
#endif

#ifdef DECLARE_VARS

// Dynamic module load functions
#ifdef _WIN32
static inline void *dll_LoadModule(const char *name)
{
	HINSTANCE handle;
	handle = LoadLibrary(name);
	return (void *)handle;
}
static inline void dll_UnloadModule(void *hdl)
{
	HINSTANCE handle;
	handle = (HINSTANCE)hdl;

	if(hdl)
	{
		FreeLibrary(handle);
	}
}
static void *dll_GetSymbol(void *dllhandle,const char *symname)
{
	if(!dllhandle)
		return NULL;
	return (void *)GetProcAddress((HINSTANCE)dllhandle,symname);
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

		dglAlphaFunc = (glAlphaFunc_fp)dll_GetSymbol(OpenGLModuleHandle,"glAlphaFunc");
		dglBindTexture = (glBindTexture_fp)dll_GetSymbol(OpenGLModuleHandle,"glBindTexture");
		dglBlendFunc = (glBlendFunc_fp)dll_GetSymbol(OpenGLModuleHandle,"glBlendFunc");
		dglClear = (glClear_fp)dll_GetSymbol(OpenGLModuleHandle,"glClear");
		dglClearColor = (glClearColor_fp)dll_GetSymbol(OpenGLModuleHandle,"glClearColor");
		dglColor4f = (glColor4f_fp)dll_GetSymbol(OpenGLModuleHandle,"glColor4f");
		dglColorPointer = (glColorPointer_fp)dll_GetSymbol(OpenGLModuleHandle,"glColorPointer");
		dglCullFace = (glCullFace_fp)dll_GetSymbol(OpenGLModuleHandle,"glCullFace");
		dglDeleteTextures = (glDeleteTextures_fp)dll_GetSymbol(OpenGLModuleHandle,"glDeleteTextures");
		dglDepthFunc = (glDepthFunc_fp)dll_GetSymbol(OpenGLModuleHandle,"glDepthFunc");
		dglDepthMask = (glDepthMask_fp)dll_GetSymbol(OpenGLModuleHandle,"glDepthMask");
		dglDisable = (glDisable_fp)dll_GetSymbol(OpenGLModuleHandle,"glDisable");
		dglDisableClientState = (glDisableClientState_fp)dll_GetSymbol(OpenGLModuleHandle,"glDisableClientState");
		dglDrawArrays = (glDrawArrays_fp)dll_GetSymbol(OpenGLModuleHandle,"glDrawArrays");
		dglEnable = (glEnable_fp)dll_GetSymbol(OpenGLModuleHandle,"glEnable");
		dglEnableClientState = (glEnableClientState_fp)dll_GetSymbol(OpenGLModuleHandle,"glEnableClientState");
		dglEnd = (glEnd_fp)dll_GetSymbol(OpenGLModuleHandle,"glEnd");
		dglFinish = (glFinish_fp)dll_GetSymbol(OpenGLModuleHandle,"glFinish");
		dglGenTextures = (glGenTextures_fp)dll_GetSymbol(OpenGLModuleHandle,"glGenTextures");
		dglGetFloatv = (glGetFloatv_fp)dll_GetSymbol(OpenGLModuleHandle,"glGetFloatv");
		dglGetIntegerv = (glGetIntegerv_fp)dll_GetSymbol(OpenGLModuleHandle,"glGetIntegerv");
		dglGetString = (glGetString_fp)dll_GetSymbol(OpenGLModuleHandle,"glGetString");
		dglGetTexLevelParameteriv = (glGetTexLevelParameteriv_fp)dll_GetSymbol(OpenGLModuleHandle,"glGetTexLevelParameteriv");
		dglHint = (glHint_fp)dll_GetSymbol(OpenGLModuleHandle,"glHint");
		dglLineWidth = (glLineWidth_fp)dll_GetSymbol(OpenGLModuleHandle,"glLineWidth");
		dglLoadIdentity = (glLoadIdentity_fp)dll_GetSymbol(OpenGLModuleHandle,"glLoadIdentity");
		dglMatrixMode = (glMatrixMode_fp)dll_GetSymbol(OpenGLModuleHandle,"glMatrixMode");
		dglOrtho = (glOrtho_fp)dll_GetSymbol(OpenGLModuleHandle,"glOrtho");
		dglPointSize = (glPointSize_fp)dll_GetSymbol(OpenGLModuleHandle,"glPointSize");
		dglPopMatrix = (glPopMatrix_fp)dll_GetSymbol(OpenGLModuleHandle,"glPopMatrix");
		dglPrioritizeTextures = (glPrioritizeTextures_fp)dll_GetSymbol(OpenGLModuleHandle,"glPrioritizeTextures");
		dglPushMatrix = (glPushMatrix_fp)dll_GetSymbol(OpenGLModuleHandle,"glPushMatrix");
		dglReadBuffer = (glReadBuffer_fp)dll_GetSymbol(OpenGLModuleHandle,"glReadBuffer");
		dglReadPixels = (glReadPixels_fp)dll_GetSymbol(OpenGLModuleHandle,"glReadPixels");
		dglScalef = (glScalef_fp)dll_GetSymbol(OpenGLModuleHandle,"glScalef");
		dglShadeModel = (glShadeModel_fp)dll_GetSymbol(OpenGLModuleHandle,"glShadeModel");
		dglTexCoordPointer = (glTexCoordPointer_fp)dll_GetSymbol(OpenGLModuleHandle,"glTexCoordPointer");
		dglTexEnvi = (glTexEnvi_fp)dll_GetSymbol(OpenGLModuleHandle,"glTexEnvi");
		dglTexImage2D = (glTexImage2D_fp)dll_GetSymbol(OpenGLModuleHandle,"glTexImage2D");
		dglTexParameterf = (glTexParameterf_fp)dll_GetSymbol(OpenGLModuleHandle,"glTexParameterf");
		dglTexParameteri = (glTexParameteri_fp)dll_GetSymbol(OpenGLModuleHandle,"glTexParameteri");
		dglTranslatef = (glTranslatef_fp)dll_GetSymbol(OpenGLModuleHandle,"glTranslatef");
		dglVertexPointer = (glVertexPointer_fp)dll_GetSymbol(OpenGLModuleHandle,"glVertexPointer");
		dglViewport = (glViewport_fp)dll_GetSymbol(OpenGLModuleHandle,"glViewport");

#ifdef _WIN32
		dwglGetProcAddress = (wglGetProcAddress_fp)dll_GetSymbol(OpenGLModuleHandle,"wglGetProcAddress");
		#if (WINVER >= 0x0500)
		#endif
#endif

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

	dglMultiTexCoord2fARB = NULL;
	dglActiveTextureARB = NULL;
	dglMultiTexCoord2fSGIS = NULL;
	dglSelectTextureSGIS = NULL;


#ifdef _WIN32
	dwglGetProcAddress = NULL;
	#if (WINVER >= 0x0500)
	#endif
#endif

}
#endif


#endif //!__LOADGL_H__
