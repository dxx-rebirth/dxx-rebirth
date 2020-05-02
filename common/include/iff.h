/*
 * Portions of this file are copyright Rebirth contributors and licensed as
 * described in COPYING.txt.
 * Portions of this file are copyright Parallax Software and licensed
 * according to the Parallax license below.
 * See COPYING.txt for license details.

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

/*
 *
 * Header for IFF routines
 *
 */

#ifndef _IFF_H
#define _IFF_H

#include "pstypes.h"
#include "fwd-gr.h"

#ifdef __cplusplus

constexpr std::integral_constant<std::size_t, 30> MAX_BITMAPS_PER_BRUSH{};

//Prototypes for IFF library functions

int iff_read_bitmap(const char *ifilename, grs_bitmap &bm, palette_array_t *palette);
	//reads an IFF file into a grs_bitmap structure. fills in palette if not null
	//returns error codes - see IFF.H.  see GR.H for bitmap_type
	//MEM DETAILS:  This routines assumes that you already have the grs_bitmap
	//structure allocated, but that you don't have the data for this bitmap
	//allocated. In other words, do this:
	//   grs_bitmap * MyPicture;
	//   MALLOC( MyPicture, grs_bitmap, 1);
	//   iff_read_bitmap( filename, MyPicture, bm_mode::linear, NULL );
	//   ...do whatever with your bitmap ...
	//   gr_free_bitmap( MyPicture );
	//   exit(0)

//read in animator brush (.abm) file
//fills in array of pointers, and n_bitmaps.
//returns iff error codes. max_bitmaps is size of array.
int iff_read_animbrush(const char *ifilename,std::array<std::unique_ptr<grs_main_bitmap>, MAX_BITMAPS_PER_BRUSH> &bm,unsigned *n_bitmaps,palette_array_t &palette);

// After a read
extern ubyte iff_transparent_color;
extern ubyte iff_has_transparency;	// 0=no transparency, 1=iff_transparent_color is valid

int iff_write_bitmap(const char *ofilename,grs_bitmap *bm,palette_array_t *palette);
	//writes an IFF file from a grs_bitmap structure. writes palette if not null
	//returns error codes - see IFF.H.

//function to return pointer to error message
const char *iff_errormsg(int error_number);


//Error codes for read & write routines

#define IFF_NO_ERROR        0   //everything is fine, have a nice day
#define IFF_NO_MEM          1   //not enough mem for loading or processing
#define IFF_UNKNOWN_FORM    2   //IFF file, but not a bitmap
#define IFF_NOT_IFF         3   //this isn't even an IFF file
#define IFF_NO_FILE         4   //cannot find or open file
#define IFF_BAD_BM_TYPE     5   //tried to save invalid type, like bm_mode::rgb15
#define IFF_CORRUPT         6   //bad data in file
#define IFF_FORM_ANIM       7   //this is an anim, with non-anim load rtn
#define IFF_FORM_BITMAP     8   //this is not an anim, with anim load rtn
#define IFF_TOO_MANY_BMS    9   //anim read had more bitmaps than room for
#define IFF_UNKNOWN_MASK    10  //unknown masking type
#define IFF_READ_ERROR      11  //error reading from file
#define IFF_BM_MISMATCH     12  //bm being loaded doesn't match bm loaded into

#endif

#endif

