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

#include <stdlib.h>
#include <SDL.h>

#include "args.h"
#include "console.h"
#include "maths.h"
#include "ogl_sync.h"
#include "timer.h"

namespace dcx {

ogl_sync::ogl_sync()
{
	method=SYNC_GL_NONE;
	wait_timeout = 0;
}

ogl_sync::~ogl_sync()
{
	if (fence)
		con_printf(CON_URGENT, "fence sync object was never destroyed!");
}

void ogl_sync::sync_deleter::operator()(GLsync fence_func) const
{
	glDeleteSyncFunc(fence_func);
}

void ogl_sync::before_swap()
{
	if (fence) {
		/// use a fence sync object to prevent the GPU from queuing up more than one frame
		if (method == SYNC_GL_FENCE_SLEEP) {
			while(glClientWaitSyncFunc(fence.get(), GL_SYNC_FLUSH_COMMANDS_BIT, 0ULL) == GL_TIMEOUT_EXPIRED) {
				timer_delay_ms(wait_timeout);
			}
		} else {
			glClientWaitSyncFunc(fence.get(), GL_SYNC_FLUSH_COMMANDS_BIT, 34000000ULL);
		}
		fence.reset();
	} else if (method == SYNC_GL_FINISH_BEFORE_SWAP) {
		glFinish();
	}
}
void ogl_sync::after_swap()
{
	if (method == SYNC_GL_FENCE || method == SYNC_GL_FENCE_SLEEP ) {
		fence.reset(glFenceSyncFunc(GL_SYNC_GPU_COMMANDS_COMPLETE, 0));
	} else if (method == SYNC_GL_FINISH_AFTER_SWAP) {
		glFinish();
	}
}

void ogl_sync::init(SyncGLMethod sync_method, int wait)
{
	method = sync_method;
	fence = NULL;
	fix a = i2f(wait);
	fix b = i2f(1000);
	wait_timeout = f2i(fixdiv(a, b) * 1000);

	bool need_ARB_sync;

	switch (method) {
		case SYNC_GL_FENCE:
		case SYNC_GL_FENCE_SLEEP:
		case SYNC_GL_AUTO:
			need_ARB_sync = true;
			break;
		default:
			need_ARB_sync = false;
	}

	if (method == SYNC_GL_AUTO) {
		if (!ogl_have_ARB_sync) {
			con_printf(CON_NORMAL, "GL_ARB_sync not available, disabling sync");
			method = SYNC_GL_NONE;
			need_ARB_sync = false;
		} else {
			method = SYNC_GL_FENCE_SLEEP;
		}
	}

	if (need_ARB_sync && !ogl_have_ARB_sync) {
		con_printf(CON_URGENT, "GL_ARB_sync not available, using fallback");
		method = SYNC_GL_FINISH_BEFORE_SWAP;
	}
	switch(method) {
		case SYNC_GL_FENCE:
			con_printf(CON_VERBOSE, "using GL_ARB_sync for synchronization (direct)");
			break;
		case SYNC_GL_FENCE_SLEEP:
			con_printf(CON_VERBOSE, "using GL_ARB_sync for synchronization with interval %dms", wait);
			break;
		case SYNC_GL_FINISH_AFTER_SWAP:
			con_printf(CON_VERBOSE, "using glFinish synchronization (method 1)");
			break;
		case SYNC_GL_FINISH_BEFORE_SWAP:
			con_printf(CON_VERBOSE, "using glFinish synchronization (method 2)");
			break;	
		default:	
			con_printf(CON_VERBOSE, "using no explicit GPU synchronization");
			break;
	}
}

void ogl_sync::deinit()
{
	fence.reset();
}

}
