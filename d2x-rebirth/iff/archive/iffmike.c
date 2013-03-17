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

#include <stdio.h>
#include <stdlib.h>
#include <alloc.h>

#include "iff.h"

#define MIN(a,b) ((a<b)?a:b)

#define MAKE_SIG(a,b,c,d) (((long)(a)<<24)+((long)(b)<<16)+((c)<<8)+(d))

#define form_sig MAKE_SIG('F','O','R','M')
#define ilbm_sig MAKE_SIG('I','L','B','M')
#define body_sig MAKE_SIG('B','O','D','Y')
#define pbm_sig MAKE_SIG('P','B','M',' ')
#define bmhd_sig MAKE_SIG('B','M','H','D')
#define cmap_sig MAKE_SIG('C','M','A','P')

void printsig(long s)
{
	char *t=(char *) &s;

/*	printf("%c%c%c%c",*(&s+3),*(&s+2),*(&s+1),s);*/
	printf("%c%c%c%c",t[3],t[2],t[1],t[0]);
}

long get_sig(FILE *f)
{
	char s[4];

	if ((s[3]=getc(f))==EOF) return(EOF);
	if ((s[2]=getc(f))==EOF) return(EOF);
	if ((s[1]=getc(f))==EOF) return(EOF);
	if ((s[0]=getc(f))==EOF) return(EOF);

	return(*((long *) s));
}

int put_sig(long sig,FILE *f)
{
	char *s = (char *) &sig;

	putc(s[3],f);
	putc(s[2],f);
	putc(s[1],f);
	return putc(s[0],f);

}
	
int get_word(FILE *f)
{
	unsigned char c0,c1;

	c1=getc(f);
	c0=getc(f);

	if (c0==0xff) return(EOF);

	return(((int)c1<<8) + c0);

}

char get_byte(FILE *f)
{
	return getc(f);
}

char put_byte(unsigned char c,FILE *f)
{
	return putc(c,f);
}

int put_word(int n,FILE *f)
{
	unsigned char c0,c1;

	c0 = (n & 0xff00) >> 8;
	c1 = n & 0xff;

	put_byte(c0,f);
	return put_byte(c1,f);
}

int put_long(long n,FILE *f)
{
	int n0,n1;

	n0 = (int) ((n & 0xffff0000l) >> 16);
	n1 = (int) (n & 0xffff);

	put_word(n0,f);
	return put_word(n1,f);

}

long get_long(FILE *f)
{
	unsigned char c0,c1,c2,c3;

	c3=getc(f);
	c2=getc(f);
	c1=getc(f);
	c0=getc(f);

//printf("get_long %x %x %x %x\n",c3,c2,c1,c0);

//	if (c0==0xff) return(EOF);

	return(((long)c3<<24) + ((long)c2<<16) + ((long)c1<<8) + c0);

}

void parse_bmhd(FILE *ifile,long len,struct bitmap_header *bitmap_header)
{
	len++;	/* so no "parm not used" warning */

//	debug("parsing bmhd len=%ld\n",len);

	bitmap_header->w = get_word(ifile);
	bitmap_header->h = get_word(ifile);
	bitmap_header->x = get_word(ifile);
	bitmap_header->y = get_word(ifile);

	bitmap_header->nplanes = get_byte(ifile);
	bitmap_header->masking = get_byte(ifile);
	bitmap_header->compression = get_byte(ifile);
	get_byte(ifile);		/* skip pad */

	bitmap_header->transparentcolor = get_word(ifile);
	bitmap_header->xaspect = get_byte(ifile);
	bitmap_header->yaspect = get_byte(ifile);

	bitmap_header->pagewidth = get_word(ifile);
	bitmap_header->pageheight = get_word(ifile);

//	debug("w,h=%d,%d x,y=%d,%d\n",w,h,x,y);
//	debug("nplanes=%d, masking=%d ,compression=%d, transcolor=%d\n",nplanes,masking,compression,transparentcolor);

}


//	the buffer pointed to by raw_data is stuffed with a pointer to decompressed pixel data
int parse_body_pbm(FILE *ifile,long len,struct bitmap_header *bitmap_header)
{
	unsigned char huge *p=bitmap_header->raw_data;
	int width=bitmap_header->w;
	long cnt,old_cnt;
	char n;
	int nn,wid_cnt;
	char ignore;

	if (bitmap_header->compression == cmpNone) {		/* no compression */
		int x,y;

		for (y=bitmap_header->h;y;y--) {
			for (x=bitmap_header->w;x;x--) *p++=getc(ifile);
			if (bitmap_header->w & 1) ignore = getc(ifile);
		}

	}
	else if (bitmap_header->compression == cmpByteRun1)
		for (old_cnt=cnt=len,wid_cnt=width;cnt>0;) {
			unsigned char c;
	
			if (old_cnt-cnt > 2048) {
//				printf(".");
				old_cnt=cnt;
			}
	
			if (wid_cnt <= 0) wid_cnt = width;
	
			n=getc(ifile);
			if (n >= 0) {						// copy next n+1 bytes from source, they are not compressed
				nn = (int) n+1;
				cnt -= nn+1;
				wid_cnt -= nn;
				if (wid_cnt==-1) --nn;
				while (nn--) *p++=getc(ifile);
				if (wid_cnt==-1) ignore = getc(ifile);		/* extra char */
			}
			else if (n>=-127) {				// next -n + 1 bytes are following byte
				cnt -= 2;
				c=getc(ifile);
				nn = (int) -n+1;
				wid_cnt -= nn;
				if (wid_cnt==-1) --nn;
				while (nn--) *p++=c;
			}
	
		}
	
	if (len & 1) ignore = getc(ifile);

	if (ignore) ignore++;	// haha, suppress the evil warning message

	return IFF_NO_ERROR;
}

//	the buffer pointed to by raw_data is stuffed with a pointer to bitplane pixel data
int parse_body_ilbm(FILE *ifile,long len,struct bitmap_header *bitmap_header)
{
	unsigned char huge *p=bitmap_header->raw_data;
	int width=bitmap_header->w;
	long cnt,old_cnt;
	char n;
	int nn,wid_cnt;
	char	ignore;

	if (bitmap_header->compression == cmpNone) {		/* no compression */
		int x,y;

		for (y=bitmap_header->h;y;y--) {
			for (x=bitmap_header->w;x;x--) *p++=getc(ifile);
			if (bitmap_header->w & 1) ignore = getc(ifile);
		}

	}
	else if (bitmap_header->compression == cmpByteRun1)
		for (old_cnt=cnt=len,wid_cnt=width;cnt>0;) {
			unsigned char c;
	
			if (old_cnt-cnt > 2048) {
//				printf(".");
				old_cnt=cnt;
			}
	
			if (wid_cnt <= 0) wid_cnt = width;
	
			n=getc(ifile);
			if (n >= 0) {						// copy next n+1 bytes from source, they are not compressed
				nn = (int) n+1;
				cnt -= nn+1;
				wid_cnt -= nn;
				if (wid_cnt==-1) --nn;
				while (nn--) *p++=getc(ifile);
				if (wid_cnt==-1) ignore = getc(ifile);		/* extra char */
			}
			else if (n>=-127) {				// next -n + 1 bytes are following byte
				cnt -= 2;
				c=getc(ifile);
				nn = (int) -n+1;
				wid_cnt -= nn;
				if (wid_cnt==-1) --nn;
				while (nn--) *p++=c;
			}
	
		}
	
	if (len & 1) ignore = getc(ifile);

	if (ignore) ignore++;	// haha, suppress the evil warning message

	return IFF_NO_ERROR;
}

void skip_chunk(FILE *ifile,long len)
{
	len = len+1 & ~1;
	fseek(ifile,len,SEEK_CUR);
}

// Pass pointer to opened file, and to empty bitmap header.
int parse_iff(FILE *ifile,struct bitmap_header *bitmap_header)
{
	long sig,form_len,len,form_type;
	char	ignore;

	sig=get_sig(ifile);

//	printsig(sig);

	if (sig==form_sig) {

		form_len = get_long(ifile);
		form_len++;		/* get rid of never used message */

		form_type = get_sig(ifile);

//		printf(" %ld ",form_len);
//		printsig(form_type);
//		printf("\n");

		if ((form_type == pbm_sig) || (form_type == ilbm_sig)) {

			if (form_type == pbm_sig)
				bitmap_header->type = PBM_TYPE;
			else
				bitmap_header->type = ILBM_TYPE;

			while ((sig=get_sig(ifile)) != EOF) {

				len=get_long(ifile);

//				printf(" ");
//				printsig(sig);
//				printf(" %ld\n",len);

				switch (sig) {

					case bmhd_sig:

						parse_bmhd(ifile,len,bitmap_header);

						if (! (bitmap_header->raw_data = farmalloc((long) bitmap_header->w * bitmap_header->h))) return IFF_NO_MEM;
						
						break;

					case cmap_sig:
					{
						int ncolors=(int) (len/3),cnum;
						unsigned char r,g,b;
	
						for (cnum=0;cnum<ncolors;cnum++) {
							r=getc(ifile);
							g=getc(ifile);
							b=getc(ifile);
							r >>= 2; bitmap_header->palette[cnum].r = r;
							g >>= 2; bitmap_header->palette[cnum].g = g;
							b >>= 2; bitmap_header->palette[cnum].b = b;
						}
						if (len & 1) ignore = getc(ifile);

						break;
					}

					case body_sig:
					{
						int r;
						switch (form_type) {
							case pbm_sig:
								if (!(r=parse_body_pbm(ifile,len,bitmap_header))) return r;
								break;
							case ilbm_sig:
								if (!(r=parse_body_ilbm(ifile,len,bitmap_header))) return r;
								break;
						}
						break;
					}
					default:
						skip_chunk(ifile,len);
						break;
				}
			}
		}
		else return IFF_UNKNOWN_FORM;
	}
	else
		{printf("Not an IFF file\n"); return IFF_NOT_IFF;}

	if (ignore) ignore++;

	return IFF_NO_ERROR;	/* ok! */
}

#define BMHD_SIZE 20

int write_bmhd(FILE *ofile,struct bitmap_header *bitmap_header)
{

	put_sig(bmhd_sig,ofile);
	put_long((long) BMHD_SIZE,ofile);

	put_word(bitmap_header->w,ofile);
	put_word(bitmap_header->h,ofile);
	put_word(bitmap_header->x,ofile);
	put_word(bitmap_header->y,ofile);

	put_byte(bitmap_header->nplanes,ofile);
	put_byte(bitmap_header->masking,ofile);
	put_byte(bitmap_header->compression,ofile);
	put_byte(0,ofile);	/* pad */

	put_word(bitmap_header->transparentcolor,ofile);
	put_byte(bitmap_header->xaspect,ofile);
	put_byte(bitmap_header->yaspect,ofile);

	put_word(bitmap_header->pagewidth,ofile);
	put_word(bitmap_header->pageheight,ofile);

	return 1;

}

int write_huge(unsigned char huge *huge_ptr,long len,FILE *f)
{
	unsigned char temp_buffer[256],*t;

//printf("write_huge %ld\n",len);

	while (len) {
		int n,wsize = (int) MIN(len,256);

//printf("len,wsize=%ld,%d\n",len,wsize);
		for (t=temp_buffer,n=wsize;n--;) *t++ = *huge_ptr++;

		fwrite(temp_buffer,wsize,1,f);

		len -= wsize;

	}

	return 1;
}

int write_pal(FILE *ofile,struct bitmap_header *bitmap_header)
{
	int	i;

	int n_colors = 1<<bitmap_header->nplanes;

	put_sig(cmap_sig,ofile);
//	put_long(sizeof(struct pal_entry) * n_colors,ofile);
	put_long(3 * n_colors,ofile);

//printf("new write pal %d %d\n",3,n_colors);

	for (i=0; i<256; i++) {
		unsigned char r,g,b;
		r = bitmap_header->palette[i].r * 4;
		g = bitmap_header->palette[i].g * 4;
		b = bitmap_header->palette[i].b * 4;
		fputc(r,ofile);
		fputc(g,ofile);
		fputc(b,ofile);
	}

//printf("write pal %d %d\n",sizeof(struct pal_entry),n_colors);
//	fwrite(bitmap_header->palette,sizeof(struct pal_entry),n_colors,ofile);

	return 1;
}

#define EVEN(a) ((a+1)&0xfffffffel)

int write_body(FILE *ofile,struct bitmap_header *bitmap_header)
{
	int w=bitmap_header->w,h=bitmap_header->h;
	int y,odd=w&1;
	long len = EVEN(w) * h;
	unsigned char huge *p=bitmap_header->raw_data;

	put_sig(body_sig,ofile);
	put_long(len,ofile);

	for (y=bitmap_header->h;y--;) {

		write_huge(p,bitmap_header->w,ofile);
		if (odd) putc(0,ofile);
		p+=bitmap_header->w;

	}

	return 1;
}

int write_pbm(FILE *ofile,struct bitmap_header *bitmap_header)			/* writes a pbm iff file */
{
	long raw_size = EVEN(bitmap_header->w) * bitmap_header->h;
	long pbm_size = 4 + BMHD_SIZE + 8 + EVEN(raw_size) + sizeof(struct pal_entry)*(1<<bitmap_header->nplanes)+8;

//printf("write_pbm\n");

	put_sig(form_sig,ofile);
	put_long(pbm_size+8,ofile);
	put_sig(pbm_sig,ofile);

	write_bmhd(ofile,bitmap_header);

	write_pal(ofile,bitmap_header);

	write_body(ofile,bitmap_header);

	return 1;

}

