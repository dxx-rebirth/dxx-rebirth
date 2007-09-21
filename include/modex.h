#ifndef _MODEX_H
#define _MODEX_H
extern int  gr_modex_setmode(short mode);
extern void gr_modex_setplane(short plane);
extern void gr_modex_setstart(short x, short y, int wait_for_retrace);
extern void gr_modex_uscanline( short x1, short x2, short y, unsigned char color );
extern void gr_modex_line();
extern void gr_sync_display();

extern int modex_line_vertincr;
extern int modex_line_incr1;
extern int modex_line_incr2;    
extern int modex_line_x1;       
extern int modex_line_y1;       
extern int modex_line_x2;       
extern int modex_line_y2;       
extern ubyte modex_line_Color;
#endif
