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

char _3dfx_rcsid[] = "$Id: 3dfx.c,v 1.1.1.1 2001-01-19 03:30:14 bradleyb Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "mono.h"
#include "bm.h"
#include "gr.h"
#include "3d.h"
#include "segment.h"
#include "piggy.h"
#include "3dfx_des.h"
#include "game.h"

#ifdef _3DFX

GrHwConfiguration  hwconfig;
int                _3dfx_triangles_rendered_pre_clip;
int                _3dfx_triangles_rendered_post_clip;
int                _3dfx_tex_combine, _3dfx_tex_combine_top, _3dfx_tex_combine_bottom;
int                _3dfx_current_handle;
int                _3dfx_handle_to_index[_3DFX_MAX_HANDLES];
int                _3dfx_bytes_downloaded_this_frame;
int                _3dfx_download_requests_made;
int                _3dfx_download_requests_granted;
int                _3dfx_drawing_polygon_model;
int                _3dfx_current_bitmap_index;
int                _3dfx_skip_ddraw, _3dfx_no_texture, _3dfx_allow_transparency;
float              _3dfx_oo_stencil;
float              _3dfx_stencil;
int                _3dfx_should_sync;
int                _3dfx_rendering_poly_obj;
int                _3dfx_tex_combine_orientation;
int                _3dfx_tex_combine_superx;
int                _3dfx_tex_combine_top_flags,
                   _3dfx_tex_combine_bottom_flags;
float              _3dfx_stencil;
int                _3dfx_no_bilinear;
int                _3dfx_available;

typedef struct BitmapFile       {
	char                    name[15];
} BitmapFile;
extern BitmapFile AllBitmaps[ MAX_BITMAP_FILES ];

_3dfxTextureInfo   _3dfx_texture_info[MAX_BITMAP_FILES];

void MyErrorHandler( const char *s, FxBool fatal )
{
   mprintf( ( 0, "3Dfx err_handler - %s\n", s ) );
   if ( fatal )
   {
      exit( 2 );
   }
}

int _3dfx_Init( void )
{
   int         i;

   mprintf( ( 0, "Initializing 3Dfx Interactive Voodoo Graphics hardware\n" ) );
   grGlideInit();
   grErrorSetCallback( MyErrorHandler );

   if ( !grSstQueryHardware( &hwconfig ) )
   {
      mprintf( ( 0, "3Dfx Interactive Voodoo Graphics not found!" ) );
      return 0;
   }
   grSstSelect( 0 );
   if ( !grSstOpen( GR_RESOLUTION_640x480,
                    GR_REFRESH_60Hz,
                    GR_COLORFORMAT_ARGB,
                    GR_ORIGIN_UPPER_LEFT,
                    GR_SMOOTHING_ENABLE,
                    2 ) )
   {
      mprintf( ( 0, "3Dfx Interactive Voodoo Graphics not opened!" ) );
      return 0;
   }

   if ( getenv( "GLIDE_OFF" ) != 0 )
   {
      mprintf( ( 0, "3Dfx Interactive Voodoo Graphics disabled!" ) );
      return 0;
   }

   grDepthBufferMode( GR_DEPTHBUFFER_DISABLE );
   grBufferClear( 0x00000000, 0, 0 );
   grCullMode( GR_CULL_DISABLE );

   /*
   ** configure environment variable controlled options
   */
   if ( getenv( "GLIDE_NO_BILINEAR" ) )
      _3dfx_no_bilinear = 1;
   if ( getenv( "GLIDE_NO_DDRAW" ) )
      _3dfx_skip_ddraw = 1;
   if ( getenv( "GLIDE_NO_TEXTURE" ) )
      _3dfx_no_texture = 1;
   if ( getenv( "GLIDE_NO_TRANSPARENCY" ) )
      _3dfx_allow_transparency = 0;
   else
      _3dfx_allow_transparency = 1;
   if ( getenv( "GLIDE_NO_SYNC" ) )
      _3dfx_should_sync = 0;
   else
      _3dfx_should_sync = 1;

   /*
   ** allocate texture memory
   */
   for ( i = 0; i < MAX_BITMAP_FILES; i++ )
   {
      _3dfx_texture_info[i].handle = GR_NULL_MIPMAP_HANDLE;
   }

   _3dfx_current_handle = 0;
   for ( i = 0; i < _3DFX_MAX_HANDLES; i++ )
   {
      GrMipMapId_t id;

      id = grTexAllocateMemory( GR_TMU0,
                                GR_MIPMAPLEVELMASK_BOTH,
                                64, 64,
                                GR_TEXFMT_ARGB_1555,
                                GR_MIPMAP_NEAREST,
                                GR_LOD_1, GR_LOD_64,
                                GR_ASPECT_1x1,
                                GR_TEXTURECLAMP_WRAP, GR_TEXTURECLAMP_WRAP,
                                GR_TEXTUREFILTER_BILINEAR, _3dfx_no_bilinear ? GR_TEXTUREFILTER_POINT_SAMPLED : GR_TEXTUREFILTER_BILINEAR,
                                1.0F,
                                FXFALSE );

      if ( id == GR_NULL_MIPMAP_HANDLE )
      {
         mprintf( ( 0, "            - unexpected null mmid returned\n" ) );
         exit( 1 );
      }
   }

   return 1;
}

void _3dfx_DownloadTexture( int index )
{
   GrMipMapId_t      id   = _3dfx_current_handle;
   _3dfxTextureInfo *info = &_3dfx_texture_info[index];
   int               texture_being_replaced;
   GrTextureFormat_t fmt;

   /*
   ** make sure the texture is in main memory
   */
   _3dfx_download_requests_made++;

   if ( !( info->flags & _3DFX_TF_IN_MEMORY ) )
   {
      _3dfx_LoadTexture( index, AllBitmaps[index].name );

      if ( !( info->flags & _3DFX_TF_IN_MEMORY ) )
         return;
   }

   /*
   ** if it's already in memory then we're cool
   */
   if ( info->flags & _3DFX_TF_DOWNLOADED )
   {
      return;
   }

   /*
   ** otherwise we gotta do a swaperoo.  So, find the old texture and
   ** give it the bad news.
   */
   texture_being_replaced = _3dfx_handle_to_index[id];

   _3dfx_texture_info[texture_being_replaced].flags &= ~_3DFX_TF_DOWNLOADED;

   /*
   ** update the new texture
   */
   _3dfx_handle_to_index[id] = index;
   info->handle              = id;
   info->flags              |= _3DFX_TF_DOWNLOADED;

   if ( info->flags & _3DFX_TF_SUPERX )
      fmt = GR_TEXFMT_ARGB_4444;
   else if ( info->flags & _3DFX_TF_TRANSPARENT )
      fmt = GR_TEXFMT_ARGB_1555;
   else
      fmt = GR_TEXFMT_RGB_565;

   grTexChangeAttributes( id,
                          64, 64,
                          fmt,
                          GR_MIPMAP_NEAREST,
                          GR_LOD_1, GR_LOD_64,
                          GR_ASPECT_1x1,
                          GR_TEXTURECLAMP_WRAP, GR_TEXTURECLAMP_WRAP,
                          -1, -1 );

   grTexDownloadMipMap( id, info->data, 0 );

   if ( ++_3dfx_current_handle >= _3DFX_MAX_HANDLES )
      _3dfx_current_handle = 0;

   _3dfx_bytes_downloaded_this_frame += SIZEOF_64x64_16BIT_TEXTURE;

   _3dfx_download_requests_granted++;
}

int _3dfx_LoadTextureFromDisk( const char *name, int index )
{
   Gu3dfInfo info;
   int       success_or_fail = 0;

   if ( gu3dfGetInfo( name, &info ) )
   {
      _3dfx_texture_info[index].flags = 0;

      if ( info.header.large_lod != GR_LOD_64 )
      {
         goto done;
      }

      if ( info.header.aspect_ratio != GR_ASPECT_1x1 )
      {
         goto done;
      }

      #ifdef malloc
      #  undef malloc
      #endif
      info.data = ( void * ) malloc( info.mem_required );

      if ( info.data == 0 )
      {
         mprintf( ( 0, "info.data == 0\n" ) );
         goto done;
      }

      if ( !gu3dfLoad( name, &info ) )
      {
         mprintf( ( 0, "couldn't gu3dfLoad() '%s'\n", name ) );
         free( info.data );
         goto done;
      }

      _3dfx_texture_info[index].data         = info.data;
      _3dfx_texture_info[index].flags        = _3DFX_TF_IN_MEMORY;
      _3dfx_texture_info[index].mem_required = info.mem_required;

      if ( info.header.format == GR_TEXFMT_ARGB_1555 )
         _3dfx_texture_info[index].flags |= _3DFX_TF_TRANSPARENT;
      else if ( info.header.format == GR_TEXFMT_ARGB_4444 )
         _3dfx_texture_info[index].flags |= ( _3DFX_TF_SUPERX | _3DFX_TF_TRANSPARENT );
   }

   success_or_fail = 1;
done:
   return success_or_fail;
}

/*
** _3dfx_LoadTexture
*/
void _3dfx_LoadTexture( int index, const char *_name )
{
   /*
   ** load a non-ABM file
   */
   if ( !strchr( _name, '#' ) )
   {
      char name[256];

      strcpy( name, _name );
      sprintf( name, "3DF\\%s.3df", _name );

      _3dfx_LoadTextureFromDisk( name, index );
   }
   /*
   ** handle ABM files
   */
   else
   {
      char _3dfx_filename[256], _3dfx_name[256], _3dfx_extension[256];

      strcpy( _3dfx_name, _name );
     *strchr( _3dfx_name, '#' ) = 0;
      strcpy( _3dfx_extension, strchr( _name, '#' ) + 1 );
      sprintf( _3dfx_filename, "ABM\\%s.%03s", _3dfx_name, _3dfx_extension );

      _3dfx_LoadTextureFromDisk( _3dfx_filename, index );
   }
}

/*
** _3dfx_BufferSwap
*/
void _3dfx_BufferSwap( void )
{
   if ( _3dfx_available )
   {
      while ( ( ( grSstStatus() >> 28 ) & 7 ) > 0 )
         ;

      grBufferSwap( _3dfx_should_sync );
      grBufferClear( 0xFFFF0000, 0, 0 );
   }

   _3dfx_bytes_downloaded_this_frame  = 0;
   _3dfx_download_requests_made       = 0;
   _3dfx_download_requests_granted    = 0;
   _3dfx_triangles_rendered_pre_clip  = 0;
   _3dfx_triangles_rendered_post_clip = 0;
}

/*
** _3dfx_DrawFlatShadedPoly
*/
#define SNAP( a ) ( ( fix ) ( (a) & ( 0xFFFFF000L ) ) )

void _3dfx_DrawFlatShadedPoly( g3ds_tmap *Tmap1, unsigned long argb )
{
   GrVertex a, b, c;
   int      i;
   float    lowest_y = 10000, highest_y = 0;
   float    lowest_x = 10000, highest_x = 0;

   for ( i = 0; i < Tmap1->nv; i++ )
   {
      float x = SNAP( Tmap1->verts[i].x2d ) * fix_to_float;
      float y = SNAP( Tmap1->verts[i].y2d ) * fix_to_float;

      if ( x < lowest_x ) lowest_x = x;
      if ( x > highest_x ) highest_x = x;
      if ( y < lowest_y ) lowest_y = y;
      if ( y > highest_y ) highest_y = y;
   }

   if ( highest_x < 0 || highest_y < 0 || lowest_x > 639 || lowest_y > 479 )
   {
      mprintf( ( 0, "3dfx - rejecting fspoly way off screen\n" ) );
      return;
   }

   guColorCombineFunction( GR_COLORCOMBINE_CCRGB );
   grConstantColorValue( argb );

   a.x = SNAP( Tmap1->verts[0].x2d ) * fix_to_float;
   a.y = SNAP( Tmap1->verts[0].y2d ) * fix_to_float;

   for ( i = 1; i < Tmap1->nv - 1; i++ )
   {
      b.x   = SNAP( Tmap1->verts[i].x2d ) * fix_to_float;
      b.y   = SNAP( Tmap1->verts[i].y2d ) * fix_to_float;
      c.x   = SNAP( Tmap1->verts[i+1].x2d ) * fix_to_float;
      c.y   = SNAP( Tmap1->verts[i+1].y2d ) * fix_to_float;

      grDrawTriangle( &a, &b, &c );
   }
}

/*
** _3dfx_DownloadAndUseTexture
*/
void _3dfx_DownloadAndUseTexture( unsigned long index )
{
   _3dfx_DownloadTexture( index );
   if ( _3dfx_texture_info[index].handle != GR_NULL_MIPMAP_HANDLE )
      grTexSource( _3dfx_texture_info[index].handle );
}

/*
** _3dfx_decompose_tmap_and_draw
*/
void _3dfx_decompose_tmap_and_draw( int nv, g3s_point **pointlist, uvl *uvl_copy, grs_bitmap *bm )
{
   int        i;
   uvl        uvl_copy_3dfx[8];
   g3s_point *pointlist2[8];

   pointlist2[0]    = pointlist[0];
   uvl_copy_3dfx[0] = uvl_copy[0];

   _3dfx_DownloadAndUseTexture( bm->bm_handle );

   for ( i = 0; i < nv - 2; i++ )
   {
      int j;

      for ( j = 1; j < 3; j++ )
      {
         pointlist2[j]    = pointlist[i+j];
         uvl_copy_3dfx[j] = uvl_copy[i+j];
      }

      if ( _3dfx_available )
      {
         g3_draw_tmap( 3, pointlist2, (g3s_uvl *) uvl_copy_3dfx, bm );
      }
      _3dfx_triangles_rendered_pre_clip++;
   }
}

/*
** _3dfx_PaletteToARGB
*/
unsigned long _3dfx_PaletteToARGB( int pindex )
{
   unsigned long result = 0;

   result |= ( ( unsigned long ) gr_palette[pindex*3+0] ) << 18;
   result |= ( ( unsigned long ) gr_palette[pindex*3+1] ) << 10;
   result |= ( ( unsigned long ) gr_palette[pindex*3+2] ) << 2;

   return result;
}

/*
** _3dfx_BlitScale
*/
#define _3DFX_FB_WIDTH 1024

void _3dfx_BlitScale( grs_bitmap *source_bmp, grs_bitmap *dest_bmp,
                      int x0, int y0, int x1, int y1,
                      fix u0, fix v0, fix u1, fix v1, int orient )
{
   if ( _3dfx_available )
   {
      if ( source_bmp->bm_flags & BM_FLAG_RLE )
      {
         fix u, v, du, dv;
         int x, y;
         ubyte * sbits;
         unsigned long * dbits;
         unsigned long * fb;

         du = (u1-u0) / (x1-x0);
         dv = (v1-v0) / (y1-y0);

         v = v0;

         grLfbBegin();

         grLfbBypassMode( GR_LFBBYPASS_ENABLE );

         fb = ( unsigned long * ) grLfbGetWritePtr( GR_BUFFER_BACKBUFFER );
         grLfbWriteMode( GR_LFBWRITEMODE_8888 );

         guColorCombineFunction( GR_COLORCOMBINE_DECAL_TEXTURE );

         for ( y = y0; y <= y1; y++, v += dv )
         {
            extern ubyte scale_rle_data[];

            decode_row( source_bmp, f2i( v ) );

            sbits = scale_rle_data + f2i( u0 );
            dbits = fb + _3DFX_FB_WIDTH * y + x0;

            u = u0;

            for ( x = x0; x <= x1; x++ )
            {
               int pindex = sbits[ u >> 16 ];

               if ( pindex != 255 )
                  *dbits = _3dfx_PaletteToARGB( pindex );

               dbits++;
               u += du;
            }
         }
         grLfbBypassMode( GR_LFBBYPASS_DISABLE );

         grLfbEnd();
      }
      else
      {
         mprintf( ( 0, "_3dfx_BlitScale() - non RLE not implemented yet\n" ) );
      }
   }
}

/*
** _3dfx_Blit
*/
#define _3DFX_FB_WIDTH 1024

void _3dfx_Blit( int x, int y, grs_bitmap *bm )
{
   if ( _3dfx_available )
   {
      if ( bm->bm_flags & BM_FLAG_RLE )
      {
         ubyte * sbits;
         unsigned long * dbits;
         unsigned long * fb;
         int             v;

         grLfbBegin();

         grLfbBypassMode( GR_LFBBYPASS_ENABLE );

         fb = ( unsigned long * ) grLfbGetWritePtr( GR_BUFFER_BACKBUFFER );
         grLfbWriteMode( GR_LFBWRITEMODE_8888 );

         guColorCombineFunction( GR_COLORCOMBINE_DECAL_TEXTURE );

         for ( v = 0; v < bm->bm_h; v++ )
         {
            extern ubyte scale_rle_data[];
            int          u;

            decode_row( bm, v );

            sbits = scale_rle_data;
            dbits = fb + _3DFX_FB_WIDTH * ( y + v ) + x;

            for ( u = 0; u < bm->bm_w; u++ )
            {
               int pindex = sbits[ u ];

               if ( pindex != 255 )
                  *dbits = _3dfx_PaletteToARGB( pindex );
               dbits++;
            }
         }
         grLfbBypassMode( GR_LFBBYPASS_DISABLE );

         grLfbEnd();
      }
      else
      {
         mprintf( ( 0, "_3dfx_Blit() - non RLE not implemented yet\n" ) );
      }
   }
}

/*
** _3dfx_InitFogForPaletteTricks
*/
void _3dfx_InitFogForPaletteTricks( void )
{
   GrFog_t _3dfx_fog_table[GR_FOG_TABLE_SIZE];

   if ( PaletteRedAdd + PaletteGreenAdd + PaletteBlueAdd )
   {
      int i;
      unsigned long fog_color_value;
      unsigned long max_palette_add = PaletteRedAdd;
      float         palette_add_intensity;
      float         palette_to_255 = 255.0 / 31.0;

      if ( PaletteGreenAdd > max_palette_add ) max_palette_add = PaletteGreenAdd;
      if ( PaletteBlueAdd > max_palette_add )  max_palette_add = PaletteBlueAdd;

      palette_add_intensity = max_palette_add / 31.0;

      fog_color_value = ( ( ( unsigned long ) ( palette_to_255 * PaletteRedAdd ) << 16 ) |
                          ( ( unsigned long ) ( palette_to_255 * PaletteGreenAdd ) << 8 ) |
                          ( ( unsigned long ) ( palette_to_255 * PaletteBlueAdd ) ) );

      grFogMode( GR_FOG_WITH_TABLE );
      grFogColorValue( fog_color_value );

      for ( i = 0; i < GR_FOG_TABLE_SIZE; i++ )
      {
         _3dfx_fog_table[i] = 256 * palette_add_intensity;
      }
      grFogTable( _3dfx_fog_table );
   }
   else
   {
      grFogMode( GR_FOG_DISABLE );
   }
}


#endif

