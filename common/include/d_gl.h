/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */

#include "dxxsconf.h"
#if !DXX_USE_OGL
#error "This file can only be included in OpenGL enabled builds."
#endif

#ifdef _WIN32
#include "loadgl.h"
#else
#	define GL_GLEXT_LEGACY
#	if defined(__APPLE__) && defined(__MACH__)
#		include <OpenGL/gl.h>
#		include <OpenGL/glu.h>
#	else
#		define GL_GLEXT_PROTOTYPES
#		if DXX_USE_OGLES
#			include <GLES/gl.h>
#		else
#			include <GL/gl.h>
#			include <GL/glext.h>
#		endif
#	endif
#endif
