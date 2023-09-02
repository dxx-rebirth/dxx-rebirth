/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */

/* OpenGL Synchronization code:
 * either use fence sync objects or glFinish() to prevent the GPU from
 * lagging behind too much.
 */

#pragma once

#include <memory>
#include "maths.h"
#include "args.h"
#include "ogl_extensions.h"

namespace dcx {

#if DXX_USE_OGL
class ogl_sync {
	class sync_deleter
	{
	public:
		typedef GLsync pointer;
		void operator()(pointer p) const;
	};
	private:
		SyncGLMethod method{SYNC_GL_NONE};
		fix wait_timeout{0};
		std::unique_ptr<GLsync, sync_deleter> fence;
	public:
		~ogl_sync();

		void before_swap();
		void after_swap();
		void init(SyncGLMethod sync_method, int wait);
		void deinit();
};
#endif

}
