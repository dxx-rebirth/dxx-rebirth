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

char poly_acc_rcsid[] = "$Id: poly_acc.c,v 1.1.1.1 2001-01-19 03:30:14 bradleyb Exp $";
static unsigned poly_count;

//
//                                                         Includes
//
#include <math.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pa_enabl.h"        // needs to occur early.

#if defined(POLY_ACC)

#include "pstypes.h"
#include "fix.h"
#include "3d.h"
#include "gr.h"
#include "grdef.h"
#include "texmap.h"
#include "mono.h"
#include "debug.h"
#include "error.h"
#include "poly_acc.h"

//
//                                                          Defines
//
#define TEX_SIZE        64
#define TEX_CACHE       (64)
#define CHROMA_COLOR    0x0001
#define MODE_110        S3DTK_MODE640x480x15           // 640x480x15
#define DMA_SIZE        1024
#define DMA_VERT_WORDS  (sizeof(S3DTK_VERTEX_TEX) / sizeof(unsigned long))
#define NDL             5               // stolen from main/game.h. how many filter levels there are.

enum { OP_TMAP, OP_FLAT, OP_BLIT, OP_BLIT_TRANSPARENT, OP_BLIT_SCALE };

//
//                                                           Variables
//
extern ubyte scale_rle_data[1024];      // from 2d\scale.c
unsigned short pa_clut[256];            // translate from 8 bit pixels to 15 bit pixels.
static short pa_clut_save_buf[256];     // used to save/restore the clut.
int pa_filter_mode = NDL-1;             //  For Virge filtering control, set in Custom Detail Level menu.


static pa_initted = 0;
static pa_card_initted = 0;     //$$hack, library doesn't seem to correctly init card twice.
static ULONG old_mode;

/* S3D Toolkit function list */
static S3DTK_LPFUNCTIONLIST pS3DTK_Funct;
static S3DTK_VERTEX_TEX verts[MAX_TMAP_VERTS];
static S3DTK_LPVERTEX_TEX trifan[MAX_TMAP_VERTS];

/* buffer definitions */
static int backBuffer = 1;                    /* index of back buffer                          */
static S3DTK_SURFACE displaySurf[2];          /* one for front buffer and one for back buffer  */
static S3DTK_RECTAREA fullScreenRect = { 0, 0, 640, 480 };

/* physical properties */
static ULONG screenFormat = S3DTK_VIDEORGB15;   /* format of the surface as defined in S3DTK.H */
static float width=640, height=480;             /* screen's dimension                          */

// memory manager variables.
static ULONG totalMemory;                  /* frame buffer size                           */
static ULONG memoryUsed;                   /* frame buffer allocated                      */
static char *frameBuffer;                  /* linear address of frame buffer starts at    */

// cache variables.
static S3DTK_SURFACE cache_slots[TEX_CACHE];
static struct
{
    uint tag;
    uint age;
} cache_tags[TEX_CACHE];
static int cache_age = 0;

// dma vars.
static unsigned long dma_buf[DMA_SIZE];     // simulated dma ring buffer.
static int dma_head, dma_tail;              // head and tail of ring buffer.
static op_extra_words[] =                   // how many extra words are needed besides the vertex info
{ 7, 5, 10, 11, 5 };                        // plus 1. avoids (head == tail) == (tail == head).
static filter_tmap[] = {
    S3DTK_TEX1TPP, S3DTK_TEXM1TPP, S3DTK_TEX4TPP, S3DTK_TEX4TPP, S3DTK_TEXM4TPP
};
static filter_scale[] = {
    S3DTK_TEX1TPP, S3DTK_TEX4TPP, S3DTK_TEX4TPP, S3DTK_TEX4TPP, S3DTK_TEX4TPP
};
static filter_robot[] = {       // actually how to filter non-perspective triangles.
    S3DTK_TEX1TPP, S3DTK_TEXM1TPP, S3DTK_TEXM1TPP, S3DTK_TEX4TPP, S3DTK_TEXM4TPP
};

// misc
static int pa_3d_x_off = 0, pa_3d_y_off = 0;

// local prototypes.
static int pa_cache_search(grs_bitmap *bp);
static BOOL allocSurf(S3DTK_SURFACE *surf, ULONG width, ULONG height, ULONG bpp, ULONG format);
extern uint texmerge_get_unique_id( grs_bitmap * bmp );
extern void decode_row(grs_bitmap * bmp, int y);
static unsigned get_pow2(unsigned wh);
static unsigned long dma_get(void);
static void dma_put(unsigned long d);
static int dma_space(void);
static int CheckForSpecificCard(void);

//
//                                                           Routines
//
int pa_init(void)                       // init library.
{
}

void pa_reset(void)                         // cleans up library.
{
}

int pa_detect(int mode)                 // detect card and whether mode is supported.
{
}

void pa_set_mode(int mode)           // sets display mode if valid PA mode.
{
}

// type 0==0..63, 1==0..255.
void pa_update_clut(unsigned char *pal, int start, int count, int type)
{
}

void pa_save_clut(void)
{
}

void pa_restore_clut(void)
{
}

void pa_step_up(int r, int g, int b)
{
}

// get the address of a card's framebuffer and lock it in.
void *pa_get_buffer_address(int which)
{
}

void pa_clear_buffer(int buffer, ushort color)
{
}


void pa_set_3d_offset(int x, int y)     // where on screen the triangles should be drawn.
{
}

void pa_draw_tmap(grs_bitmap * srcb, g3ds_tmap * tmap, int transparency_on, int lighting_on, int perspective, fix min_z)
{
}

void pa_draw_flat(g3ds_tmap *tmap, int color, int alpha)
{
}

//
//  perform a blit using hw if screen to screen, otherwise copy mem to mem.
//
void pa_blit(grs_bitmap *dst, int dx, int dy, grs_bitmap *src, int sx, int sy, int w, int h)
{
}

//
//  perform a transparent blit using hw if screen to screen, otherwise copy mem to mem.
//
void pa_blit_transparent(grs_bitmap *dst, int dx, int dy, grs_bitmap *src, int sx, int sy, int w, int h)
{
}

//
//  performs 2d scaling by applying a bitmap to a 3D sqaure and using the texture mapper to draw it.
//  NOTE:Only support RLE bitmaps.
//
void pa_blit_scale(grs_bitmap *source_bmp, grs_bitmap *dest_bmp,
    int x0, int y0, int x1, int y1,
    fix u0, fix v0,  fix u1, fix v1, int orient
)
{
}

int pa_blit_lit(grs_bitmap *dst, int dx, int dy, grs_bitmap *src, int sx, int sy, int w, int h)
{
}

int pa_rect(int sx, int sy, int ex, int ey)
{
}

void pa_swap_buffer(void)
{
}

#pragma off (unreferenced)
void pa_ibitblt(void *src, void *dst, void *mask)
#pragma on (unreferenced)
{
}

//
//    Allocate space for TEX_CACHE textures on the 3D card.
//
void pa_init_cache(void)
{
}

//
//  implements a primitive LRU to keep from destroying active textures. Primitive because it uses
//  an array search to find the oldest element.
//
static int pa_cache_search(grs_bitmap *bmp)
{
}

/***************************************************************************
 *
 *  Memory management routines
 *
 ***************************************************************************/
static BOOL initMemoryBuffer(void)
{
}

static void allocInit(S3DTK_LPFUNCTIONLIST pS3DTK_Funct)
{
}

static BOOL allocSurf(S3DTK_SURFACE *surf, ULONG width, ULONG height, ULONG bpp, ULONG format)
{
}

/* convert width/height to power of 2; if power of 2, return it. Else
   subtract 1 from next higher power of 2 */
static unsigned get_pow2(unsigned wh)
{
}

int pa_dma_poll(void)
{
}

static unsigned long dma_get(void)
{
}

static void dma_put(unsigned long d)
{
}

// return number of free entries in dma_buf
static int dma_space(void)
{
}

static void dma_add_verts(int nv)
{
}

static void dma_get_verts(void)
{
}

// see's if a command of 'type' is in dma buffer. Intended use is to serialize access to scale sprite buffer.
static int op_pending(int type)
{
}

//
//  waits for current async op to complete.
//
static pa_idle_cnt[2];      //$$
int pa_idle(void)
{
}

void pa_sync(void)
{
}

void pa_flush(void)
{
}

//used in Rendition version, but not in S3 version
void pa_about_to_flip()
{
}

// used by 3Dfx
void pa_set_write_mode (int mode)
{
}

// used by 3Dfx
void pa_set_frontbuffer_current(void)
{
}

#endif      // defined POLY_ACC

