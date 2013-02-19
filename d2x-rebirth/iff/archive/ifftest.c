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

#include <conio.h>
#include <stdio.h>
#include <stdlib.h>

#include "iff.h"
#include "vga.h"
#include "palette.h"
#include "mem.h"

//#define ANIM_TEST 1		//if defined, read in anim brush

rle_span(ubyte *dest,ubyte *src,int len);


ubyte test_span[] = {0,1,2,3,4,4,5,6,7,8,8,8,8,8,9,10,11,11};
ubyte new_span[256];

extern void gr_pal_setblock( int start, int number, unsigned char * pal );

main(int argc,char **argv)
{
	int ret;
	grs_bitmap my_bitmap;
	ubyte my_palette[256*3];
	grs_bitmap *bm_list[100];
	int n_bitmaps;
	char key;

#if 0
	{
		int new_len,i;
		new_len=rle_span(new_span,test_span,sizeof(test_span));
		printf("old span (%d): ",sizeof(test_span));
		for (i=0;i<sizeof(test_span);i++) printf("%d ",test_span[i]);
		printf("\nnew span (%d): ",new_len);
		for (i=0;i<new_len;i++) printf("%d ",new_span[i]);
		exit(0);
	}
#endif

#ifdef ANIM_TEST
	ret = iff_read_animbrush(argv[1],bm_list,100,&n_bitmaps,&my_palette);
#else
	ret = iff_read_bitmap(argv[1],&my_bitmap,BM_LINEAR,&my_palette);
	bm_list[0] = &my_bitmap;
	n_bitmaps = 1;
#endif

	printf("ret = %d\n",ret);
	printf("error message = <%s>",iff_errormsg(ret));

	if (ret == IFF_NO_ERROR) {
		int i;

		vga_init();
		gr_init();
		vga_set_mode(SM_320x200C);

		for (i=0;i<n_bitmaps;) {

			if (argc>2) {
				ret = iff_write_bitmap(argv[2],bm_list[i],&my_palette);
				printf("ret = %d\n",ret);
			}

			//gr_pal_setblock(0,256,&my_palette);
			gr_palette_load(&my_palette);
			//gr_pal_fade_in(grd_curscreen->pal);	//in case palette is blacked

			gr_ubitmap(0,0,bm_list[i]);

			key = getch();

			if (key=='-') {if (i) i--;}
			else i++;

		}

		gr_close();

		for (i=0;i<n_bitmaps;i++) {
			free(bm_list[i]->bm_data);

			#ifdef ANIM_TEST
				free(bm_list[i]);
			#endif

		}
	}

}

