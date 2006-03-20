// Muhahaha! And you though d_conv.h was the end of it!
// You were wrong! This file converts internal game structs to and fro.

// Please place this header file last or as close as possible on your
// list of #includes.

#ifndef _D_GAMECV_H
#define _D_GAMECV_H
#include "d_conv.h"


#ifdef _AISTRUCT_H // Ai structures
// These should be fixed; replaced with constants.
#define SIZEOF_AI_LOCAL sizeof(ai_local)
#define SIZEOF_POINT_SEG sizeof(point_seg)
extern void d_import_ai_local(ai_local *dest, char *src);
extern void d_export_ai_local(char *dest, ai_local *src);
extern void d_import_point_seg(point_seg *dest, char *src);
extern void d_export_point_seg(char *dest, point_seg *src);
// Probably should do ai_static as well here.
#endif // _AISTRUCT_H

#ifdef _BM_H
 // Structs from bm.h
#endif // _BM_H

#endif // _D_GAMECV_H
