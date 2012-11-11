/*
 *  messagebox.h
 *  d1x-rebirth
 *
 *  Display an error or warning messagebox using the OS's window server.
 *
 */

#ifndef _MESSAGEBOX_H
#define _MESSAGEBOX_H

#ifdef __cplusplus
extern "C" {
#endif

// Display a warning in a messagebox
extern void msgbox_warning(const char *message);

// Display an error in a messagebox
extern void msgbox_error(const char *message);

#ifdef __cplusplus
}
#endif

#endif
