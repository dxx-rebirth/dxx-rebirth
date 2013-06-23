/*
 *  messagebox.h
 *  d1x-rebirth
 *
 *  Display an error or warning messagebox using the OS's window server.
 *
 */

#ifndef _MESSAGEBOX_H
#define _MESSAGEBOX_H

// Display a warning in a messagebox
extern void msgbox_warning(char *message);

// Display an error in a messagebox
extern void msgbox_error(const char *message);

#endif
