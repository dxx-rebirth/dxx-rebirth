//THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
//SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
//END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
//ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
//IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
//SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
//FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
//CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
//AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
//COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
//
// $Source: /cvsroot/dxx-rebirth/d1x-rebirth/main/snddecom.c,v $
// $Revision: 1.1.1.1 $
// $Author: zicodxx $
// $Date: 2006/03/17 19:43:28 $
//
// Routines for compressing digital sounds.
//
// $Log: snddecom.c,v $
// Revision 1.1.1.1  2006/03/17 19:43:28  zicodxx
// initial import
//
// Revision 1.1.1.1  1999/06/14 22:11:34  donut
// Import of d1x 1.37 source.
//
// Revision 2.0  1995/02/27  11:29:36  john
// New version 2.0, which has no anonymous unions, builds with
// Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
//
// Revision 1.2  1994/11/30  14:08:46  john
// First version.
// Revision 1.1  1994/11/29  14:33:51  john
// Initial revision
//

int index_table[16] = { -1, -1, -1, -1, 2, 4, 6, 8,
			-1, -1, -1, -1, 2, 4, 6, 8 };
int step_table[89] = { 7,   8,	 9,  10,  11,  12,  13, 14,
		      16,  17,	19,  21,  23,  25,  28,
		      31,  34,	37,  41,  45,  50,  55,
		      60,  66,	73,  80,  88,  97, 107,
		      118, 130, 143, 157, 173, 190, 209,
		      230, 253, 279, 307, 337, 371, 408,
		      449, 494, 544, 598, 658, 724, 796,
		      876, 963,1060,1166,1282,1411,1552,
		      1707,1878,
		      2066,2272,2499,2749,3024,3327,3660,4026,
		      4428,4871,5358,5894,6484,7132,7845,8630,
		      9493,10442,11487,12635,13899,15289,16818,
		      18500,20350,22385,24623,27086,29794,32767 };

void sound_decompress(unsigned char *data, int size, unsigned char *outp) {
    int newtoken = 1;
    int predicted = 0, index = 0, step = 7;
    int code, diff, out;
    while (size) {
	if (newtoken)
	    code = (*data) & 15;
	else {
	    code = (*(data++)) >> 4;
	    size--;
	}
	newtoken ^= 1;
	diff = 0;
	if (code & 4)
	    diff += step;
	if (code & 2)
	    diff += (step >> 1);
	if (code & 1)
	    diff += (step >> 2);
	diff += (step >> 3);
	if (code & 8)
	    diff = -diff;
	out = predicted + diff;
	if (out > 32767)
	    out = 32767;
	if (out < -32768)
	    out = -32768;
	predicted = out;
	*(outp++) = ((out >> 8) & 255) ^ 0x80;
	index += index_table[code];
	if (index < 0)
	    index = 0;
	if (index > 88)
	    index = 88;
	step = step_table[index];
    }
}
