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


#if defined(POLY_ACC)
#if !defined(POLY_ACC_H)
#define POLY_ACC_H

//
//  A simplified interface to a variety of 3D cards.
//

#define SM_640x480x15xPA    23          // 640x480 15 bits per pixel, POLY ACC
#define BM_LINEAR15 5                   // 1555 format, may be able to replace with BM_RGB15. John indicated that
                                        // BM_RGB15 wasn't finished so I made a new type.
#define PA_BPP  2                       // bytes per pixel.

extern unsigned short pa_clut[256];     // translate from 8 bit pixels to 15 bit pixels.
extern int pa_filter_mode;              //  For Virge filtering control, set in Custom Detail Level menu.

int pa_init(void);                      // init library.
void pa_reset();                        // cleans up library.
int pa_detect(int mode);                // detect card and whether mode is supported.
void pa_set_mode(int mode);             // sets display mode if valid PA mode.
void pa_update_clut(                    // 8 bit to 15 bit table update.
    unsigned char *pal,
    int start, int count,
    int type);                          // type 0==0..63, 1==0..255.
void pa_restore_clut(void);
void pa_save_clut(void);
void pa_step_up(int r, int g, int b);

void *pa_get_buffer_address(int which); // returns a pointer to the front(0), or back(1) buffer.
                                        // NOTE: this only makes sense until the next page flip.
void pa_swap_buffer(void);              // performs the page flip.
void pa_ibitblt(void *src, void *dst, void *mask);

void pa_clear_buffer(int buffer, ushort color);
void pa_set_3d_offset(int x, int y);    // where on screen the triangles should be drawn.
void pa_draw_tmap(void /*$$grs_bitmap*/ * srcb, void /*$$g3ds_tmap*/ * tmap, int transparency_on, int lighting_on,
    int perspective, fix min_z);
void pa_draw_flat(void /*$$g3ds_tmap*/ * tmap, int color, int alpha);
void pa_blit(grs_bitmap *dst, int dx, int dy, grs_bitmap *src, int sx, int sy, int w, int h);
void pa_blit_scale(grs_bitmap *source_bmp, grs_bitmap *dest_bmp,
    int x0, int y0, int x1, int y1,
    fix u0, fix v0,  fix u1, fix v1, int orient
);
int pa_blit_lit(grs_bitmap *dst, int dx, int dy, grs_bitmap *src, int sx, int sy, int w, int h);

int pa_rect(int left,int top,int right,int bot);

void pa_init_cache(void);
void pa_cache_hit(int slot);
void pa_cache_miss(int slot, grs_bitmap * bmp);

int pa_idle(void);                      // allows program to poll whether the chip is busy.
void pa_sync(void);                     // waits for current async op to complete.
void pa_flush(void);                    // flushes dma and waits for everything to complete.

void pa_about_to_flip();					// used in Rendition version, but not in S3 version

void pa_set_write_mode (int mode);		// used by 3Dfx
void pa_set_frontbuffer_current(void);	// used by 3Dfx

#endif
#endif
