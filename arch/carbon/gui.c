/*
 *  gui.c
 *  D2X (Descent2)
 *
 *  Created by Chris Taylor on Sat Jul 03 2004.
 *
 */

#ifdef HAVE_CONFIG_H
#include "conf.h"
#endif

#ifdef __APPLE__
#include <Carbon/Carbon.h>
#else
#include <Carbon.h>
#endif

void ErrorDialog(char *message)
{
    Str255 pascal_message;
    short itemHit;	// Insists on returning this
    
    CopyCStringToPascal(message, pascal_message);
    ShowCursor();
    StandardAlert(kAlertStopAlert, pascal_message, NULL, 0, &itemHit);
}

void WarningDialog(char *s)
{
    Str255 pascal_message;
    short itemHit;	// Insists on returning this
    
    CopyCStringToPascal(s, pascal_message);
    ShowCursor();
    StandardAlert(kAlertCautionAlert, pascal_message, NULL, 0, &itemHit);
    HideCursor();
}
