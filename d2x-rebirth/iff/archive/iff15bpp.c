#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>


#include "iff.h"

#define INDEX_TO_15BPP(i) ((WORD)((((palptr[(i)].r/2)&31)<<10)+(((palptr[(i)].g/2)&31)<<5)+((palptr[(i)].b/2 )&31)))

extern int parse_iff(FILE *ifile,struct bitmap_header *bitmap_header);

int x,y,pl,bc;
int bytes_per_row,color;
int mask,first_bit_value;
FILE *ifile;
struct bitmap_header iff_bitmap_header;

// Parse ilbm style data at my_bh->raw_data.
BITMAP15 * IFF_To_15BPP(char * ifilename)
{
	struct bitmap_header * my_bh;
	int Process_width,Process_height;
	unsigned char  *p;
	struct pal_entry *palptr;
	int newptr = 0;
	int i;
	BITMAP15 * new;

	my_bh = &iff_bitmap_header;
	palptr=my_bh->palette;
	p=my_bh->raw_data;

	Process_width = 32767;  // say to process full width of bitmap
	Process_height = 32767; // say to process full height of bitmap

	if ((ifile = fopen(ifilename,"rb")) == NULL) {
		printf("Unable to open bitmap file %s.\n", ifilename);
		exit(1);
	}

	parse_iff(ifile,&iff_bitmap_header);
	if (Process_width > iff_bitmap_header.w)
		Process_width = iff_bitmap_header.w;

	if (Process_height > iff_bitmap_header.h)
		Process_height = iff_bitmap_header.h;

	//printf( "%d, %d\n", Process_width, Process_height );

	new = (BITMAP15 *)malloc( sizeof(BITMAP15)+ (Process_width * Process_height * 2 ));
	if (new==NULL) exit(1);

	new->Width = Process_width;
	new->Height = Process_height;

	//printf("Process_width = %i, Process_height = %i\n",Process_width,Process_height);
	first_bit_value = 1 << (my_bh->nplanes-1);
	bytes_per_row = 2*((my_bh->w+15)/16);
	for (y=0; y<Process_height; y++) {
		bc = Process_width;
		p = &my_bh->raw_data[y*bytes_per_row*my_bh->nplanes];

		switch (my_bh->type) {
			case PBM_TYPE:
				for (x=0; x<my_bh->w; x++) {
					new->Data[newptr++] = INDEX_TO_15BPP(my_bh->raw_data[y*my_bh->w+x]);
				}
				break;
			case ILBM_TYPE:
				for (x=0; x<bytes_per_row; x++) {
					for (mask=128; mask; mask /=2) {
						color = 0;
						for (pl=0; pl<my_bh->nplanes; pl++) {
							color /= 2;
							if ( p[pl*bytes_per_row+x] & mask)
								color += first_bit_value;
						}
						new->Data[newptr++] = INDEX_TO_15BPP(color);
						bc--;
						if (!bc)
							goto line_done;
					}
				}
line_done: ;
				break;
		}
	}
	free( my_bh->raw_data );
	return new;
}

ÿ