/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */

/* OpenGL Synchronization code:
 * either use fence sync objects or glFinish() to prevent the GPU from
 * lagging behind too much.
 */

#pragma once

#include "args.h"
#include "ogl_extensions.h"

class ogl_sync {
	private:
		SyncGLMethod method;
		fix wait_timeout;
		GLsync fence;
	public:
		ogl_sync();
		~ogl_sync();

		void before_swap();
		void after_swap();
		void init(SyncGLMethod sync_method, int wait);
		void deinit();
};
