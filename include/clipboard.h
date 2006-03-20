#ifndef _CLIPBOARD_H
#define _CLIPBOARD_H

#define MAX_PASTE_SIZE 36

// fills the specified buffer with clipboard text. returns number chars copied
extern int getClipboardText(char *text, int strlength);

#endif
