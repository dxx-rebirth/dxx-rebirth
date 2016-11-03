/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */

/* OpenGL extensions:
 * We use global function pointer for any extension function we want to use.
 */

#include <string.h>
#include <SDL.h>

#include "ogl_extensions.h"
#include "console.h"

#include "dxxsconf.h"
#include "dsx-ns.h"
#include "compiler-array.h"

namespace dcx {

/* GL_ARB_sync */
bool ogl_have_ARB_sync = false;
PFNGLFENCESYNCPROC glFenceSyncFunc = NULL;
PFNGLDELETESYNCPROC glDeleteSyncFunc = NULL;
PFNGLCLIENTWAITSYNCPROC glClientWaitSyncFunc = NULL;

/* GL_EXT_texture_filter_anisotropic */
GLfloat ogl_maxanisotropy = 0.0f;

static array<long, 2> parse_version_str(const char *v)
{
	array<long, 2> version;
	version[0]=1;
	version[1]=0;
	if (v) {
		char *ptr;
		if (v[0] == 'O') {
			// OpenGL ES uses the format "OpenGL ES-xx major.minor"
			const char *prefix_gles="OpenGL ES-";
			if (!strncmp(v, prefix_gles, strlen(prefix_gles))) {
				// skip the prefix
				v += strlen(prefix_gles);
				// skip the profile marker
				if (v[0] && v[1]) {
					v +=2;
				}
			}
		}
		version[0]=strtol(v,&ptr,10);
		if (ptr[0]) 
			version[1]=strtol(ptr+1,NULL,10);
	}
	return version;
}

static bool is_ext_supported(const char *extensions, const char *name)
{
	if (extensions && name) {
		const char *found=strstr(extensions, name);
		if (found) {
			// check that the name is actually complete */
			char c = found[strlen(name)];
			if (c == ' ' || c == 0)
				return true;
		}
	}
	return false;
}

enum support_mode {
	NO_SUPPORT=0,
	SUPPORT_CORE=1,
	SUPPORT_EXT=2
};

static support_mode is_supported(const char *extensions, const array<long, 2> &version, const char *name, long major, long minor, long major_es, long minor_es)
{
#if DXX_USE_OGLES
	static_cast<void>(major);
	static_cast<void>(minor);
	if ( (major_es > 0) && ((version[0] > major_es) || (version[0] == major_es && version[1] >= minor_es)) )
		return SUPPORT_CORE;
#else
	static_cast<void>(major_es);
	static_cast<void>(minor_es);
	if ( (major > 0) && ((version[0] > major) || (version[0] == major && version[1] >= minor)) )
		return SUPPORT_CORE;
#endif

	if (is_ext_supported(extensions, name))
		return SUPPORT_EXT;
	return NO_SUPPORT;
}

void ogl_extensions_init()
{
	const auto *version_str = reinterpret_cast<const char *>(glGetString(GL_VERSION));
	if (!version_str) {
		con_printf(CON_URGENT, "no valid OpenGL context when querying GL extensions!");
		return;
	}
	const auto version = parse_version_str(version_str);
	const auto extension_str = reinterpret_cast<const char *>(glGetString(GL_EXTENSIONS));

	con_printf(CON_VERBOSE, "OpenGL%s: version %ld.%ld (%s)",
#if DXX_USE_OGLES
			" ES",
#else
			"",
#endif
			version[0], version[1], version_str);

	/* GL_EXT_texture_filter_anisotropic */
	if (is_supported(extension_str, version, "GL_EXT_texture_filter_anisotropic", -1, -1, -1, -1)) {
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &ogl_maxanisotropy);
		con_printf(CON_VERBOSE, "GL_EXT_texture_filter_anisotropic available, max anisotropy: %f", ogl_maxanisotropy);
	} else {
		ogl_maxanisotropy=0.0f;
		con_printf(CON_VERBOSE, "GL_EXT_texture_filter_anisotropic not available");
	}

	/* GL_ARB_sync */
	if (is_supported(extension_str, version, "GL_ARB_sync", 3, 2, 3, 0)) {
		glFenceSyncFunc = reinterpret_cast<PFNGLFENCESYNCPROC>(SDL_GL_GetProcAddress("glFenceSync"));
		glDeleteSyncFunc = reinterpret_cast<PFNGLDELETESYNCPROC>(SDL_GL_GetProcAddress("glDeleteSync"));
		glClientWaitSyncFunc = reinterpret_cast<PFNGLCLIENTWAITSYNCPROC>(SDL_GL_GetProcAddress("glClientWaitSync"));

	}
	if (glFenceSyncFunc && glDeleteSyncFunc && glClientWaitSyncFunc) {
		ogl_have_ARB_sync=true;
		con_printf(CON_VERBOSE, "GL_ARB_sync available");
	} else {
		con_printf(CON_VERBOSE, "GL_ARB_sync not available");
	}
}

}
