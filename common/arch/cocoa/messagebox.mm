/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
/*
 *  messagebox.mm
 *  d1x-rebirth
 *
 *  Display an error or warning messagebox using the OS's window server.
 *
 */

#import <Cocoa/Cocoa.h>

#include "window.h"
#include "event.h"
#include "messagebox.h"

namespace dcx {

void display_cocoa_alert(const char *message, int error)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	NSAlert *alert = [[NSAlert alloc] init];
	alert.alertStyle = error == 1 ? NSCriticalAlertStyle : NSWarningAlertStyle;
	alert.messageText = error ? @"Sorry, a critical error has occurred." : @"Attention!";
	alert.informativeText = [NSString stringWithUTF8String:message];
	
	[alert runModal];
	[alert release];
	[pool drain];
}

void msgbox_warning(const char *message)
{
	display_cocoa_alert(message, 0);
}

void msgbox_error(const char *message)
{
	display_cocoa_alert(message, 1);
}

}
