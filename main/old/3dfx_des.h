/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.  
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#ifndef __3DFX_DES_H__
#define __3DFX_DES_H__

#ifdef _3DFX

#include "gr.h"
#include "3d.h"
#include "texmap.h"

#define GLIDE_HARDWARE
#include <glide.h>
#include <stdio.h>

#define _3DFX_MAX_HANDLES 98 * 1

extern int _3dfx_tex_combine, _3dfx_tex_combine_bottom, _3dfx_tex_combine_top;
extern int _3dfx_triangles_rendered_pre_clip;
extern int _3dfx_triangles_rendered_post_clip;
extern int _3dfx_current_bitmap_index;
extern int _3dfx_skip_ddraw, _3dfx_no_texture, _3dfx_allow_transparency;

#define fix_to_float   ( 1.0F / 65536.0F )
#define fix_to_rgb     ( ( 255.0 / 32.0F ) / 65536.0F )
#define fix_to_st      ( ( 255.0 / 63.0 ) / ( 65536.0F ) )

#define _3DFX_TF_DOWNLOADED  0x00000001
#define _3DFX_TF_IN_MEMORY   0x10000000
#define _3DFX_TF_SUPERX      0x00000010
#define _3DFX_TF_TRANSPARENT 0x00000020

typedef struct
{
   int   flags;
   int   handle;
   int   mem_required;
   void *data;
} _3dfxTextureInfo;

extern _3dfxTextureInfo _3dfx_texture_info[];

extern int _3dfx_handle_to_index[_3DFX_MAX_HANDLES];
extern int _3dfx_current_handle;

extern int _3dfx_bytes_downloaded_this_frame;
extern int _3dfx_download_requests_made;
extern int _3dfx_download_requests_granted;
extern int _3dfx_drawing_polygon_model;
extern int _3dfx_should_sync;
extern int _3dfx_available;

extern int _3dfx_rendering_poly_obj;

extern int   _3dfx_tex_combine_orientation;
extern int   _3dfx_tex_combine_superx;
extern int   _3dfx_tex_combine_top_flags, _3dfx_tex_combine_bottom_flags;
extern int   _3dfx_should_sync;
extern float _3dfx_stencil;

void _3dfx_DownloadTexture( int index );
void _3dfx_DebugOut( const char *fmt, ... );
void _3dfx_decompose_tmap_and_draw( int nv, g3s_point **pointlist, void *uvl_copy, grs_bitmap *bm );
void _3dfx_LoadTexture( int index, const char *_name );
void _3dfx_BufferSwap( void );
int  _3dfx_Init( void );
void _3dfx_DownloadAndUseTexture( unsigned long index );
void _3dfx_DrawFlatShadedPoly( const g3ds_tmap *tmap, FxU32 argb );
void _3dfx_BlitScale( grs_bitmap *source_bmp, grs_bitmap *dest_bmp,
                      int x0, int y0, int x1, int y1,
                      fix u0, fix v0,  fix u1, fix v1, int orient );
void _3dfx_Blit( int x, int y, grs_bitmap *bp );
void _3dfx_InitFogForPaletteTricks( void );

unsigned long _3dfx_PaletteToARGB( int pindex );

#define SIZEOF_64x64_16BIT_TEXTURE 10928

#endif

#endif
