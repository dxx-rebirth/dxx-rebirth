/* $Id: */
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

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef RCS
static char rcsid[] = "$Id: gamemine.c,v 1.16 2003-01-15 02:47:19 btb Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "pstypes.h"
#include "mono.h"

#include "inferno.h"
#include "segment.h"
#include "textures.h"
#include "wall.h"
#include "object.h"
#include "gamemine.h"
#include "error.h"
#include "gameseg.h"
#include "switch.h"

#include "game.h"
#include "newmenu.h"

#ifdef EDITOR
#include "editor/editor.h"
#endif

#include "cfile.h"
#include "fuelcen.h"

#include "hash.h"
#include "key.h"
#include "piggy.h"

#include "byteswap.h"
#include "gamesave.h"

#define REMOVE_EXT(s)  (*(strchr( (s), '.' ))='\0')

fix Level_shake_frequency = 0, Level_shake_duration = 0;
int Secret_return_segment = 0;
vms_matrix Secret_return_orient;

struct mtfi mine_top_fileinfo;    // Should be same as first two fields below...
struct mfi mine_fileinfo;
struct mh mine_header;
struct me mine_editor;

typedef struct v16_segment {
	#ifdef EDITOR
	short   segnum;             // segment number, not sure what it means
	#endif
	side    sides[MAX_SIDES_PER_SEGMENT];       // 6 sides
	short   children[MAX_SIDES_PER_SEGMENT];    // indices of 6 children segments, front, left, top, right, bottom, back
	short   verts[MAX_VERTICES_PER_SEGMENT];    // vertex ids of 4 front and 4 back vertices
	#ifdef  EDITOR
	short   group;              // group number to which the segment belongs.
	#endif
	short   objects;            // pointer to objects in this segment
	ubyte   special;            // what type of center this is
	byte    matcen_num;         // which center segment is associated with.
	short   value;
	fix     static_light;       // average static light in segment
	#ifndef EDITOR
	short   pad;                // make structure longword aligned
	#endif
} v16_segment;

struct mfi_v19 {
	ushort  fileinfo_signature;
	ushort  fileinfo_version;
	int     fileinfo_sizeof;
	int     header_offset;      // Stuff common to game & editor
	int     header_size;
	int     editor_offset;      // Editor specific stuff
	int     editor_size;
	int     segment_offset;
	int     segment_howmany;
	int     segment_sizeof;
	int     newseg_verts_offset;
	int     newseg_verts_howmany;
	int     newseg_verts_sizeof;
	int     group_offset;
	int     group_howmany;
	int     group_sizeof;
	int     vertex_offset;
	int     vertex_howmany;
	int     vertex_sizeof;
	int     texture_offset;
	int     texture_howmany;
	int     texture_sizeof;
	int     walls_offset;
	int     walls_howmany;
	int     walls_sizeof;
	int     triggers_offset;
	int     triggers_howmany;
	int     triggers_sizeof;
	int     links_offset;
	int     links_howmany;
	int     links_sizeof;
	int     object_offset;      // Object info
	int     object_howmany;
	int     object_sizeof;
	int     unused_offset;      // was: doors_offset
	int     unused_howmamy;     // was: doors_howmany
	int     unused_sizeof;      // was: doors_sizeof
	short   level_shake_frequency;  // Shakes every level_shake_frequency seconds
	short   level_shake_duration;   // for level_shake_duration seconds (on average, random).  In 16ths second.
	int     secret_return_segment;
	vms_matrix  secret_return_orient;

	int     dl_indices_offset;
	int     dl_indices_howmany;
	int     dl_indices_sizeof;

	int     delta_light_offset;
	int     delta_light_howmany;
	int     delta_light_sizeof;

};

int CreateDefaultNewSegment();

#define TMAP_NUM_MASK 0x3FFF

/* converts descent 1 texture numbers to descent 2 texture numbers
 * textures whose names don't match between versions have extra spaces around "return"
 */
short convert_d1_tmap_num(short d1_tmap_num) {
	switch (d1_tmap_num) {
	case   0:  return  43; // grey
	case   1: return 0;
	case   2:  return  43; // grey
	case   3: return 1;
	case   4:  return  43; // grey
	case   5:  return  43; // grey
	case   6:  return  270; // blue
	case   7:  return  271; // yellow
	case   8: return 2;
	case   9:  return  62; // purple
	case  10:  return  272; // red
	case  11:  return  117;
	case  12:  return  12;
	case  13: return 3;
	case  14: return 4;
	case  15: return 5;
	case  16: return 6;
	case  17:  return  17;
	case  18:  return  18;
	case  19: return 7;
	case  20:  return  22;
	case  21:  return  9;
	case  22: return 8;
	case  23: return 9;
	case  24: return 10;
	case  25:  return  12;
	case  26: return 11;
	case  27: return 12;
	case  28:  return  11;
	case  29: return 13;
	case  30: return 14;
	case  31: return 15;
	case  32: return 16;
	case  33: return 17;
	case  34: return 18;
	case  35: return 19;
	case  36: return 20;
	case  37: return 21;
	case  38:  return  163;
	case  39:  return  31;
	case  40: return 22;
	case  41:  return  266;
	case  42: return 23;
	case  43: return 24;
	case  44:  return  136;
	case  45: return 25;
	case  46: return 26;
	case  47: return 27;
	case  48: return 28;
	case  49:  return  43;
	case  50:  return  131;
	case  51: return 29;
	case  52: return 30;
	case  53: return 31;
	case  54: return 32;
	case  55:  return  165;
	case  56: return 33;
	case  57:  return  132;
	case  58: return 34;
	case  59: return 35;
	case  60: return 36;
	case  61: return 37;
	case  62: return 38;
	case  63: return 39;
	case  64: return 40;
	case  65: return 41;
	case  66: return 42;
	case  67: return 43;
	case  68: return 44;
	case  69: return 45;
	case  70: return 46;
	case  71: return 47;
	case  72: return 48;
	case  73: return 49;
	case  74: return 50;
	case  75: return 51;
	case  76: return 52;
	case  77: return 53;
	case  78: return 54;
	case  79: return 55;
	case  80: return 56;
	case  81: return 57;
	case  82: return 58;
	case  83: return 59;
	case  84: return 60;
	case  85: return 61;
	case  86: return 62;
	case  87: return 63;
	case  88:  return  66;
	case  89: return 64;
	case  90: return 65;
	case  91: return 66;
	case  92: return 67;
	case  93: return 68;
	case  94: return 69;
	case  95: return 70;
	case  96: return 71;
	case  97: return 72;
	case  98: return 73;
	case  99: return 74;
	case 100: return 75;
	case 101: return 76;
	case 102: return 77;
	case 103: return 78;
	case 104: return 79;
	case 105: return 80;
	case 106: return 81;
	case 107: return 82;
	case 108: return 83;
	case 109: return 84;
	case 110: return 85;
	case 111: return 86;
	case 112: return 87;
	case 113: return 88;
	case 114: return 89;
	case 115: return 90;
	case 116: return 91;
	case 117: return 92;
	case 118: return 93;
	case 119: return 94;
	case 120: return 95;
	case 121: return 96;
	case 122: return 97;
	case 123: return 98;
	case 124: return 99;
	case 125: return 100;
	case 126: return 101;
	case 127: return 102;
	case 128: return 103;
	case 129: return 104;
	case 130: return 105;
	case 131: return 106;
	case 132:  return  108;
	case 133: return 107;
	case 134: return 108;
	case 135: return 109;
	case 136: return 110;
	case 137: return 111;
	case 138: return 112;
	case 139: return 113;
	case 140: return 114;
	case 141:  return  110;
	case 142: return 115;
	case 143: return 116;
	case 144: return 117;
	case 145: return 118;
	case 146: return 119;
	case 147:  return  111;
	case 148: return 120;
	case 149: return 121;
	case 150: return 122;
	case 151: return 123;
	case 152: return 124;
	case 153: return 125;
	case 154:  return  82;
	case 155:  return  66;
	case 156: return 200;
	case 157: return 201;
	case 158:  return  186;
	case 159:  return  190;
	case 160:  return  206;
	case 161:  return  114;
	case 162: return 202;
	case 163: return 203;
	case 164: return 204;
	case 165: return 205;
	case 166: return 206;
	case 167:  return  206;
	case 168:  return  206;
	case 169:  return  206;
	case 170:  return  206;
	case 171:  return  227;
	case 172: return 207;
	case 173: return 208;
	case 174:  return  202;
	case 175:  return  206;
	case 176: return 209;
	case 177: return 210;
	case 178: return 211;
	case 179: return 212;
	case 180: return 213;
	case 181: return 214;
	case 182: return 215;
	case 183: return 216;
	case 184: return 217;
	case 185:  return  325;
	case 186: return 218;
	case 187: return 219;
	case 188: return 220;
	case 189: return 221;
	case 190: return 222;
	case 191: return 223;
	case 192: return 224;
	case 193:  return  206;
	case 194:  return  206;
	case 195:  return  234;
	case 196: return 225;
	case 197: return 226;
	case 198:  return  225;
	case 199:  return  206;
	case 200:  return  206;
	case 201: return 227;
	case 202:  return  206;
	case 203: return 228;
	case 204: return 229;
	case 205: return 230;
	case 206: return 231;
	case 207: return 232;
	case 208: return 233;
	case 209: return 234;
	case 210:  return  234;
	case 211:  return  206;
	case 212: return 235;
	case 213: return 236;
	case 214: return 237;
	case 215: return 238;
	case 216: return 239;
	case 217: return 240;
	case 218: return 241;
	case 219: return 242;
	case 220:  return  242;
	case 221: return 243;
	case 222: return 244;
	case 223: return  313;
	case 224: return 245;
	case 225: return 246;
	case 226: return 247;
	case 227:  return  179;
	case 228: return 248;
	case 229:  return  15;
	case 230:  return  15;
	case 231: return 249;
	case 232: return 250;
	case 233: return 251;
	case 234: return 252;
	case 235: return 253;
	case 236: return 254;
	case 237: return 255;
	case 238: return 256;
	case 239: return 257;
	case 240:  return  6;
	case 241:  return  130;
	case 242:  return  78;
	case 243:  return  33;
	case 244: return 258;
	case 245: return 259;
	case 246:  return  321;
	case 247: return 260;
	case 248: return 261;
	case 249: return 262;
	case 250: return  340; // white entrance
	case 251: return  412; // red entrance
	case 252: return  410; // blue entrance
	case 253: return  411; // yellow entrance
	case 254: return 263;
	case 255: return 264;
	case 256: return 265;
	case 257:  return  246;
	case 258:  return  246;
	case 259:  return  246;
	case 260:  return  246;
	case 261: return 273;
	case 262: return 274;
	case 263:  return  281;
	case 264: return 275;
	case 265: return 276;
	case 266:  return  279;
	case 267: return 277;
	case 268: return 278;
	case 269: return 279;
	case 270: return 280;
	case 271: return 281;
	case 272: return 282;
	case 273: return 283;
	case 274: return 284;
	case 275: return 285;
	case 276: return 286;
	case 277: return 287;
	case 278: return 288;
	case 279: return 289;
	case 280: return 290;
	case 281: return 291;
	case 282: return 293;
	case 283:  return  295;
	case 284: return 295;
	case 285: return 296;
	case 286: return 298;
	case 287: return 300;
	case 288: return 301;
	case 289: return 302;
	case 290: return 303;
	case 291: return 304;
	case 292: return 305;
	case 293: return 306;
	case 294: return 307;
	case 295: return 308;
	case 296: return 309;
	case 297: return 310;
	case 298:  return  364;
	case 299: return 311;
	case 300: return 312;
	case 301: return 313;
	case 302: return 314;
	case 303: return 315;
	case 304: return 316;
	case 305: return 317;
	case 306: return 318;
	case 307: return 319;
	case 308: return 320;
	case 309: return 321;
	case 310: return 322;
	case 311: return 323;
	case 312: return 324;
	case 313: return 325;
	case 314: return 326;
	case 315:  return  361; // broken producer
	case 316: return 327;
	case 317: return 328;
	case 318: return 329;
	case 319: return 330;
	case 320: return 331;
	case 321: return 332;
	case 322: return 333;
	case 323: return 334;
	case 324: return 335;
	case 325: return 336;
	case 326: return 337;
	case 327: return 352;
	case 328: return 353;
	case 329: return 354;
	case 330:  return  366;
	case 331: return 373;
	case 332: return 344;
 	case 333: return   409; // lava
	case 334: return 356;
	case 335: return 357;
	case 336: return 358;
	case 337: return 359;
	case 338: return 360;
	case 339: return 361;
	case 340: return 362;
	case 341: return 364;
	case 342: return 363;
	case 343: return 366;
	case 344: return 365;
	case 345: return 368;
	case 346: return 376;
	case 347: return 370;
	case 348: return 367;
	case 349:  return  370;
	case 350: return 369;
	case 351: return 429;
	case 352: return 387;
	case 353: return 371;
	case 354: return 425;
	case 355:  return  408;
	case 356: return 378; // lava02
	case 357: return 384;
	case 358: return 385;
	case 359: return 386;
	case 360: return 386;
	case 361: return 387;
	case 362:  return  388; // mntr04a
	case 363: return 388;
	case 364: return 391;
	case 365: return 392;
	case 366: return 393;
	case 367: return 394;
	case 368: return 395;
	case 369: return 396;
	case 370:  return  392; // mntr04b
	default:
		if (d1_tmap_num > 370 && d1_tmap_num < 584) // wall01 and door frames
			return d1_tmap_num + 64;
		{ // the following case (orientation != 0) occurs rarely
			short tmap_num = d1_tmap_num &  TMAP_NUM_MASK;
			short orient = d1_tmap_num & ~TMAP_NUM_MASK;
			if (orient == 0) {
				Warning("convert_d1_tmap_num doesn't know tmap #%d.\n", tmap_num);
				return d1_tmap_num;
			} else {
				return orient | convert_d1_tmap_num(tmap_num);
			}
		}
	}
}

#ifdef EDITOR

static char old_tmap_list[MAX_TEXTURES][FILENAME_LEN];
short tmap_xlate_table[MAX_TEXTURES];
static short tmap_times_used[MAX_TEXTURES];

// -----------------------------------------------------------------------------
//loads from an already-open file
// returns 0=everything ok, 1=old version, -1=error
int load_mine_data(CFILE *LoadFile)
{
	int   i, j,oldsizeadjust;
	short tmap_xlate;
	int 	translate;
	char 	*temptr;
	int	mine_start = cftell(LoadFile);

	oldsizeadjust=(sizeof(int)*2)+sizeof (vms_matrix);
	fuelcen_reset();

	for (i=0; i<MAX_TEXTURES; i++ )
		tmap_times_used[i] = 0;

	#ifdef EDITOR
	// Create a new mine to initialize things.
	//texpage_goto_first();
	create_new_mine();
	#endif

	//===================== READ FILE INFO ========================

	// These are the default values... version and fileinfo_sizeof
	// don't have defaults.
	mine_fileinfo.header_offset     =   -1;
	mine_fileinfo.header_size       =   sizeof(mine_header);
	mine_fileinfo.editor_offset     =   -1;
	mine_fileinfo.editor_size       =   sizeof(mine_editor);
	mine_fileinfo.vertex_offset     =   -1;
	mine_fileinfo.vertex_howmany    =   0;
	mine_fileinfo.vertex_sizeof     =   sizeof(vms_vector);
	mine_fileinfo.segment_offset    =   -1;
	mine_fileinfo.segment_howmany   =   0;
	mine_fileinfo.segment_sizeof    =   sizeof(segment);
	mine_fileinfo.newseg_verts_offset     =   -1;
	mine_fileinfo.newseg_verts_howmany    =   0;
	mine_fileinfo.newseg_verts_sizeof     =   sizeof(vms_vector);
	mine_fileinfo.group_offset		  =	-1;
	mine_fileinfo.group_howmany	  =	0;
	mine_fileinfo.group_sizeof		  =	sizeof(group);
	mine_fileinfo.texture_offset    =   -1;
	mine_fileinfo.texture_howmany   =   0;
 	mine_fileinfo.texture_sizeof    =   FILENAME_LEN;  // num characters in a name
 	mine_fileinfo.walls_offset		  =	-1;
	mine_fileinfo.walls_howmany	  =	0;
	mine_fileinfo.walls_sizeof		  =	sizeof(wall);  
 	mine_fileinfo.triggers_offset	  =	-1;
	mine_fileinfo.triggers_howmany  =	0;
	mine_fileinfo.triggers_sizeof	  =	sizeof(trigger);  
	mine_fileinfo.object_offset		=	-1;
	mine_fileinfo.object_howmany		=	1;
	mine_fileinfo.object_sizeof		=	sizeof(object);  

	mine_fileinfo.level_shake_frequency		=	0;
	mine_fileinfo.level_shake_duration		=	0;

	//	Delta light stuff for blowing out light sources.
//	if (mine_top_fileinfo.fileinfo_version >= 19) {
		mine_fileinfo.dl_indices_offset		=	-1;
		mine_fileinfo.dl_indices_howmany		=	0;
		mine_fileinfo.dl_indices_sizeof		=	sizeof(dl_index);  

		mine_fileinfo.delta_light_offset		=	-1;
		mine_fileinfo.delta_light_howmany		=	0;
		mine_fileinfo.delta_light_sizeof		=	sizeof(delta_light);  

//	}

	mine_fileinfo.segment2_offset		= -1;
	mine_fileinfo.segment2_howmany	= 0;
	mine_fileinfo.segment2_sizeof    = sizeof(segment2);

	// Read in mine_top_fileinfo to get size of saved fileinfo.
	
	memset( &mine_top_fileinfo, 0, sizeof(mine_top_fileinfo) );

	if (cfseek( LoadFile, mine_start, SEEK_SET ))
		Error( "Error moving to top of file in gamemine.c" );

	if (cfread( &mine_top_fileinfo, sizeof(mine_top_fileinfo), 1, LoadFile )!=1)
		Error( "Error reading mine_top_fileinfo in gamemine.c" );

	if (mine_top_fileinfo.fileinfo_signature != 0x2884)
		return -1;

	// Check version number
	if (mine_top_fileinfo.fileinfo_version < COMPATIBLE_VERSION )
		return -1;

	// Now, Read in the fileinfo
	if (cfseek( LoadFile, mine_start, SEEK_SET ))
		Error( "Error seeking to top of file in gamemine.c" );

	if (cfread( &mine_fileinfo, mine_top_fileinfo.fileinfo_sizeof, 1, LoadFile )!=1)
		Error( "Error reading mine_fileinfo in gamemine.c" );

	if (mine_top_fileinfo.fileinfo_version < 18) {
		mprintf((1, "Old version, setting shake intensity to 0.\n"));
		Level_shake_frequency = 0;
		Level_shake_duration = 0;
		Secret_return_segment = 0;
		Secret_return_orient = vmd_identity_matrix;
	} else {
		Level_shake_frequency = mine_fileinfo.level_shake_frequency << 12;
		Level_shake_duration = mine_fileinfo.level_shake_duration << 12;
		Secret_return_segment = mine_fileinfo.secret_return_segment;
		Secret_return_orient = mine_fileinfo.secret_return_orient;
	}

	//===================== READ HEADER INFO ========================

	// Set default values.
	mine_header.num_vertices        =   0;
	mine_header.num_segments        =   0;

	if (mine_fileinfo.header_offset > -1 )
	{
		if (cfseek( LoadFile, mine_fileinfo.header_offset, SEEK_SET ))
			Error( "Error seeking to header_offset in gamemine.c" );
	
		if (cfread( &mine_header, mine_fileinfo.header_size, 1, LoadFile )!=1)
			Error( "Error reading mine_header in gamemine.c" );
	}

	//===================== READ EDITOR INFO ==========================

	// Set default values
	mine_editor.current_seg         =   0;
	mine_editor.newsegment_offset   =   -1; // To be written
	mine_editor.newsegment_size     =   sizeof(segment);
	mine_editor.Curside             =   0;
	mine_editor.Markedsegp          =   -1;
	mine_editor.Markedside          =   0;

	if (mine_fileinfo.editor_offset > -1 )
	{
		if (cfseek( LoadFile, mine_fileinfo.editor_offset, SEEK_SET ))
			Error( "Error seeking to editor_offset in gamemine.c" );
	
		if (cfread( &mine_editor, mine_fileinfo.editor_size, 1, LoadFile )!=1)
			Error( "Error reading mine_editor in gamemine.c" );
	}

	//===================== READ TEXTURE INFO ==========================

	if ( (mine_fileinfo.texture_offset > -1) && (mine_fileinfo.texture_howmany > 0))
	{
		if (cfseek( LoadFile, mine_fileinfo.texture_offset, SEEK_SET ))
			Error( "Error seeking to texture_offset in gamemine.c" );

		for (i=0; i< mine_fileinfo.texture_howmany; i++ )
		{
			if (cfread( &old_tmap_list[i], mine_fileinfo.texture_sizeof, 1, LoadFile )!=1)
				Error( "Error reading old_tmap_list[i] in gamemine.c" );
		}
	}

	//=============== GENERATE TEXTURE TRANSLATION TABLE ===============

	translate = 0;
	
	Assert (NumTextures < MAX_TEXTURES);

	{
		hashtable ht;
	
		hashtable_init( &ht, NumTextures );
	
		// Remove all the file extensions in the textures list
	
		for (i=0;i<NumTextures;i++)	{
			temptr = strchr(TmapInfo[i].filename, '.');
			if (temptr) *temptr = '\0';
			hashtable_insert( &ht, TmapInfo[i].filename, i );
		}
	
		// For every texture, search through the texture list
		// to find a matching name.
		for (j=0;j<mine_fileinfo.texture_howmany;j++) 	{
			// Remove this texture name's extension
			temptr = strchr(old_tmap_list[j], '.');
			if (temptr) *temptr = '\0';
	
			tmap_xlate_table[j] = hashtable_search( &ht,old_tmap_list[j]);
			if (tmap_xlate_table[j]	< 0 )	{
				//tmap_xlate_table[j] = 0;
				// mprintf( (0, "Couldn't find texture '%s'\n", old_tmap_list[j] ));
				;
			}
			if (tmap_xlate_table[j] != j ) translate = 1;
			if (tmap_xlate_table[j] >= 0)
				tmap_times_used[tmap_xlate_table[j]]++;
		}
	
		{
			int count = 0;
			for (i=0; i<MAX_TEXTURES; i++ )
				if (tmap_times_used[i])
					count++;
			mprintf( (0, "This mine has %d unique textures in it (~%d KB)\n", count, (count*4096) /1024 ));
		}
	
		// -- mprintf( (0, "Translate=%d\n", translate ));
	
		hashtable_free( &ht );
	}

	//====================== READ VERTEX INFO ==========================

	// New check added to make sure we don't read in too many vertices.
	if ( mine_fileinfo.vertex_howmany > MAX_VERTICES )
		{
		mprintf((0, "Num vertices exceeds maximum.  Loading MAX %d vertices\n", MAX_VERTICES));
		mine_fileinfo.vertex_howmany = MAX_VERTICES;
		}

	if ( (mine_fileinfo.vertex_offset > -1) && (mine_fileinfo.vertex_howmany > 0))
	{
		if (cfseek( LoadFile, mine_fileinfo.vertex_offset, SEEK_SET ))
			Error( "Error seeking to vertex_offset in gamemine.c" );

		for (i=0; i< mine_fileinfo.vertex_howmany; i++ )
		{
			// Set the default values for this vertex
			Vertices[i].x = 1;
			Vertices[i].y = 1;
			Vertices[i].z = 1;

			if (cfread( &Vertices[i], mine_fileinfo.vertex_sizeof, 1, LoadFile )!=1)
				Error( "Error reading Vertices[i] in gamemine.c" );
		}
	}

	//==================== READ SEGMENT INFO ===========================

	// New check added to make sure we don't read in too many segments.
	if ( mine_fileinfo.segment_howmany > MAX_SEGMENTS ) {
		mprintf((0, "Num segments exceeds maximum.  Loading MAX %d segments\n", MAX_SEGMENTS));
		mine_fileinfo.segment_howmany = MAX_SEGMENTS;
		mine_fileinfo.segment2_howmany = MAX_SEGMENTS;
	}

	// [commented out by mk on 11/20/94 (weren't we supposed to hit final in October?) because it looks redundant.  I think I'll test it now...]  fuelcen_reset();

	if ( (mine_fileinfo.segment_offset > -1) && (mine_fileinfo.segment_howmany > 0))	{

		if (cfseek( LoadFile, mine_fileinfo.segment_offset,SEEK_SET ))

			Error( "Error seeking to segment_offset in gamemine.c" );

		Highest_segment_index = mine_fileinfo.segment_howmany-1;

		for (i=0; i< mine_fileinfo.segment_howmany; i++ ) {

			// Set the default values for this segment (clear to zero )
			//memset( &Segments[i], 0, sizeof(segment) );

			if (mine_top_fileinfo.fileinfo_version < 20) {
				v16_segment v16_seg;

				Assert(mine_fileinfo.segment_sizeof == sizeof(v16_seg));

				if (cfread( &v16_seg, mine_fileinfo.segment_sizeof, 1, LoadFile )!=1)
					Error( "Error reading segments in gamemine.c" );

				#ifdef EDITOR
				Segments[i].segnum = v16_seg.segnum;
				// -- Segments[i].pad = v16_seg.pad;
				#endif

				for (j=0; j<MAX_SIDES_PER_SEGMENT; j++)
					Segments[i].sides[j] = v16_seg.sides[j];

				for (j=0; j<MAX_SIDES_PER_SEGMENT; j++)
					Segments[i].children[j] = v16_seg.children[j];

				for (j=0; j<MAX_VERTICES_PER_SEGMENT; j++)
					Segments[i].verts[j] = v16_seg.verts[j];

				Segment2s[i].special = v16_seg.special;
				Segment2s[i].value = v16_seg.value;
				Segment2s[i].s2_flags = 0;
				Segment2s[i].matcen_num = v16_seg.matcen_num;
				Segment2s[i].static_light = v16_seg.static_light;
				fuelcen_activate( &Segments[i], Segment2s[i].special );

			} else  {
				if (cfread( &Segments[i], mine_fileinfo.segment_sizeof, 1, LoadFile )!=1)
					Error("Unable to read segment %i\n", i);
			}

			Segments[i].objects = -1;
			#ifdef EDITOR
			Segments[i].group = -1;
			#endif

			if (mine_top_fileinfo.fileinfo_version < 15) {	//used old uvl ranges
				int sn,uvln;

				for (sn=0;sn<MAX_SIDES_PER_SEGMENT;sn++)
					for (uvln=0;uvln<4;uvln++) {
						Segments[i].sides[sn].uvls[uvln].u /= 64;
						Segments[i].sides[sn].uvls[uvln].v /= 64;
						Segments[i].sides[sn].uvls[uvln].l /= 32;
					}
			}

			if (translate == 1)
				for (j=0;j<MAX_SIDES_PER_SEGMENT;j++) {
					unsigned short orient;
					tmap_xlate = Segments[i].sides[j].tmap_num;
					Segments[i].sides[j].tmap_num = tmap_xlate_table[tmap_xlate];
					if ((WALL_IS_DOORWAY(&Segments[i],j) & WID_RENDER_FLAG))
						if (Segments[i].sides[j].tmap_num < 0)	{
							mprintf( (0, "Couldn't find texture '%s' for Segment %d, side %d\n", old_tmap_list[tmap_xlate],i,j));
							Int3();
							Segments[i].sides[j].tmap_num = NumTextures-1;
						}
					tmap_xlate = Segments[i].sides[j].tmap_num2 & TMAP_NUM_MASK;
					orient = Segments[i].sides[j].tmap_num2 & (~TMAP_NUM_MASK);
					if (tmap_xlate != 0) {
						int xlated_tmap = tmap_xlate_table[tmap_xlate];

						if ((WALL_IS_DOORWAY(&Segments[i],j) & WID_RENDER_FLAG))
							if (xlated_tmap <= 0)	{
								mprintf( (0, "Couldn't find texture '%s' for Segment %d, side %d\n", old_tmap_list[tmap_xlate],i,j));
								Int3();
								Segments[i].sides[j].tmap_num2 = NumTextures-1;
							}
						Segments[i].sides[j].tmap_num2 = xlated_tmap | orient;
					}
				}
		}


		if (mine_top_fileinfo.fileinfo_version >= 20)
			for (i=0; i<=Highest_segment_index; i++) {
				cfread(&Segment2s[i], sizeof(segment2), 1, LoadFile);
				fuelcen_activate( &Segments[i], Segment2s[i].special );
			}
	}

	//===================== READ NEWSEGMENT INFO =====================

	#ifdef EDITOR

	{		// Default segment created.
		vms_vector	sizevec;
		med_create_new_segment(vm_vec_make(&sizevec,DEFAULT_X_SIZE,DEFAULT_Y_SIZE,DEFAULT_Z_SIZE));		// New_segment = Segments[0];
		//memset( &New_segment, 0, sizeof(segment) );
	}

	if (mine_editor.newsegment_offset > -1)
	{
		if (cfseek( LoadFile, mine_editor.newsegment_offset,SEEK_SET ))
			Error( "Error seeking to newsegment_offset in gamemine.c" );
		if (cfread( &New_segment, mine_editor.newsegment_size,1,LoadFile )!=1)
			Error( "Error reading new_segment in gamemine.c" );
	}

	if ( (mine_fileinfo.newseg_verts_offset > -1) && (mine_fileinfo.newseg_verts_howmany > 0))
	{
		if (cfseek( LoadFile, mine_fileinfo.newseg_verts_offset, SEEK_SET ))
			Error( "Error seeking to newseg_verts_offset in gamemine.c" );
		for (i=0; i< mine_fileinfo.newseg_verts_howmany; i++ )
		{
			// Set the default values for this vertex
			Vertices[NEW_SEGMENT_VERTICES+i].x = 1;
			Vertices[NEW_SEGMENT_VERTICES+i].y = 1;
			Vertices[NEW_SEGMENT_VERTICES+i].z = 1;
			
			if (cfread( &Vertices[NEW_SEGMENT_VERTICES+i], mine_fileinfo.newseg_verts_sizeof,1,LoadFile )!=1)
				Error( "Error reading Vertices[NEW_SEGMENT_VERTICES+i] in gamemine.c" );

			New_segment.verts[i] = NEW_SEGMENT_VERTICES+i;
		}
	}

	#endif
															
	//========================= UPDATE VARIABLES ======================

	#ifdef EDITOR

	// Setting to Markedsegp to NULL ignores Curside and Markedside, which
	// we want to do when reading in an old file.
	
 	Markedside = mine_editor.Markedside;
	Curside = mine_editor.Curside;
	for (i=0;i<10;i++)
		Groupside[i] = mine_editor.Groupside[i];

	if ( mine_editor.current_seg != -1 )
		Cursegp = mine_editor.current_seg + Segments;
	else
 		Cursegp = NULL;

	if (mine_editor.Markedsegp != -1 ) 
		Markedsegp = mine_editor.Markedsegp + Segments;
	else
		Markedsegp = NULL;

	num_groups = 0;
	current_group = -1;

	#endif

	Num_vertices = mine_fileinfo.vertex_howmany;
	Num_segments = mine_fileinfo.segment_howmany;
	Highest_vertex_index = Num_vertices-1;
	Highest_segment_index = Num_segments-1;

	reset_objects(1);		//one object, the player

	#ifdef EDITOR
	Highest_vertex_index = MAX_SEGMENT_VERTICES-1;
	Highest_segment_index = MAX_SEGMENTS-1;
	set_vertex_counts();
	Highest_vertex_index = Num_vertices-1;
	Highest_segment_index = Num_segments-1;

	warn_if_concave_segments();
	#endif

	#ifdef EDITOR
		validate_segment_all();
	#endif

	//create_local_segment_data();

	//gamemine_find_textures();

	if (mine_top_fileinfo.fileinfo_version < MINE_VERSION )
		return 1;		//old version
	else
		return 0;

}
#endif

#define COMPILED_MINE_VERSION 0

void read_children(int segnum,ubyte bit_mask,CFILE *LoadFile)
{
	int bit;

	for (bit=0; bit<MAX_SIDES_PER_SEGMENT; bit++) {
		if (bit_mask & (1 << bit)) {
			Segments[segnum].children[bit] = cfile_read_short(LoadFile);
		} else
			Segments[segnum].children[bit] = -1;
	}
}

void read_verts(int segnum,CFILE *LoadFile)
{
	int i;
	// Read short Segments[segnum].verts[MAX_VERTICES_PER_SEGMENT]
	for (i = 0; i < MAX_VERTICES_PER_SEGMENT; i++)
		Segments[segnum].verts[i] = cfile_read_short(LoadFile);
}

void read_special(int segnum,ubyte bit_mask,CFILE *LoadFile)
{
	if (bit_mask & (1 << MAX_SIDES_PER_SEGMENT)) {
		// Read ubyte	Segment2s[segnum].special
		Segment2s[segnum].special = cfile_read_byte(LoadFile);
		// Read byte	Segment2s[segnum].matcen_num
		Segment2s[segnum].matcen_num = cfile_read_byte(LoadFile);
		// Read short	Segment2s[segnum].value
		Segment2s[segnum].value = cfile_read_short(LoadFile);
	} else {
		Segment2s[segnum].special = 0;
		Segment2s[segnum].matcen_num = -1;
		Segment2s[segnum].value = 0;
	}
}

int load_mine_data_compiled(CFILE *LoadFile)
{
	int     i, segnum, sidenum;
	ubyte   compiled_version;
	short   temp_short;
	ushort  temp_ushort;
	ubyte   bit_mask;

	//	For compiled levels, textures map to themselves, prevent tmap_override always being gray,
	//	bug which Matt and John refused to acknowledge, so here is Mike, fixing it.
#ifdef EDITOR
	for (i=0; i<MAX_TEXTURES; i++)
		tmap_xlate_table[i] = i;
#endif

//	memset( Segments, 0, sizeof(segment)*MAX_SEGMENTS );
	fuelcen_reset();

	//=============================== Reading part ==============================
	compiled_version = cfile_read_byte(LoadFile);
	//Assert( compiled_version==COMPILED_MINE_VERSION );
 	if (compiled_version!=COMPILED_MINE_VERSION)
		mprintf((0,"compiled mine version=%i\n", compiled_version)); //many levels have "wrong" versions.  Theres no point in aborting because of it, I think.

	Num_vertices = cfile_read_short(LoadFile);
	Assert( Num_vertices <= MAX_VERTICES );

	// skip two bytes for shareware D1 levels
	if (!strcmp(strchr(Gamesave_current_filename, '.'), ".sdl"))
		cfseek(LoadFile, 2, SEEK_CUR);

	Num_segments = cfile_read_short(LoadFile);
	Assert( Num_segments <= MAX_SEGMENTS );

	// skip two bytes for shareware D1 levels
	if (!strcmp(strchr(Gamesave_current_filename, '.'), ".sdl"))
		cfseek(LoadFile, 2, SEEK_CUR);

	for (i = 0; i < Num_vertices; i++)
		cfile_read_vector( &(Vertices[i]), LoadFile);

	for (segnum=0; segnum<Num_segments; segnum++ )	{

		#ifdef EDITOR
		Segments[segnum].segnum = segnum;
		Segments[segnum].group = 0;
		#endif

 		bit_mask = cfile_read_byte(LoadFile);

		if (Gamesave_current_version == 5) { // d2 SHAREWARE level
			read_special(segnum,bit_mask,LoadFile);
			read_verts(segnum,LoadFile);
			read_children(segnum,bit_mask,LoadFile);
		} else {
			read_children(segnum,bit_mask,LoadFile);
			read_verts(segnum,LoadFile);
			if (Gamesave_current_version <= 1) { // descent 1 level
				read_special(segnum,bit_mask,LoadFile);
			}
		}

		Segments[segnum].objects = -1;

		if (Gamesave_current_version <= 5) { // descent 1 thru d2 SHAREWARE level
			// Read fix	Segments[segnum].static_light (shift down 5 bits, write as short)
			temp_ushort = cfile_read_short(LoadFile);
			Segment2s[segnum].static_light	= ((fix)temp_ushort) << 4;
			//cfread( &Segments[segnum].static_light, sizeof(fix), 1, LoadFile );
		}

		// Read the walls as a 6 byte array
		for (sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++ )	{
			Segments[segnum].sides[sidenum].pad = 0;
		}

 		bit_mask = cfile_read_byte(LoadFile);
		for (sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++) {
			ubyte byte_wallnum;

			if (bit_mask & (1 << sidenum)) {
				byte_wallnum = cfile_read_byte(LoadFile);
				if ( byte_wallnum == 255 )
					Segments[segnum].sides[sidenum].wall_num = -1;
				else
					Segments[segnum].sides[sidenum].wall_num = byte_wallnum;
			} else
					Segments[segnum].sides[sidenum].wall_num = -1;
		}

		for (sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++ )	{

			if ( (Segments[segnum].children[sidenum]==-1) || (Segments[segnum].sides[sidenum].wall_num!=-1) )	{
				// Read short Segments[segnum].sides[sidenum].tmap_num;
				temp_ushort = cfile_read_short(LoadFile);
				Segments[segnum].sides[sidenum].tmap_num = temp_ushort & 0x7fff;


				if (Gamesave_current_version <= 1)
				  Segments[segnum].sides[sidenum].tmap_num = convert_d1_tmap_num(Segments[segnum].sides[sidenum].tmap_num);

				if (!(temp_ushort & 0x8000))
					Segments[segnum].sides[sidenum].tmap_num2 = 0;
				else {
					// Read short Segments[segnum].sides[sidenum].tmap_num2;
					Segments[segnum].sides[sidenum].tmap_num2 = cfile_read_short(LoadFile);
					if (Gamesave_current_version <= 1)
					  Segments[segnum].sides[sidenum].tmap_num2 = convert_d1_tmap_num(Segments[segnum].sides[sidenum].tmap_num2);
				}

				// Read uvl Segments[segnum].sides[sidenum].uvls[4] (u,v>>5, write as short, l>>1 write as short)
				for (i=0; i<4; i++ )	{
					temp_short = cfile_read_short(LoadFile);
					Segments[segnum].sides[sidenum].uvls[i].u = ((fix)temp_short) << 5;
					temp_short = cfile_read_short(LoadFile);
					Segments[segnum].sides[sidenum].uvls[i].v = ((fix)temp_short) << 5;
					temp_ushort = cfile_read_short(LoadFile);
					Segments[segnum].sides[sidenum].uvls[i].l = ((fix)temp_ushort) << 1;
					//cfread( &Segments[segnum].sides[sidenum].uvls[i].l, sizeof(fix), 1, LoadFile );
				}
			} else {
				Segments[segnum].sides[sidenum].tmap_num = 0;
				Segments[segnum].sides[sidenum].tmap_num2 = 0;
				for (i=0; i<4; i++ )	{
					Segments[segnum].sides[sidenum].uvls[i].u = 0;
					Segments[segnum].sides[sidenum].uvls[i].v = 0;
					Segments[segnum].sides[sidenum].uvls[i].l = 0;
				}
			}
		}
	}

#if 0
	{
		FILE *fp;

		fp = fopen("segments.out", "wt");
		for (i = 0; i <= Highest_segment_index; i++) {
			side	sides[MAX_SIDES_PER_SEGMENT];	// 6 sides
			short	children[MAX_SIDES_PER_SEGMENT];	// indices of 6 children segments, front, left, top, right, bottom, back
			short	verts[MAX_VERTICES_PER_SEGMENT];	// vertex ids of 4 front and 4 back vertices
			int		objects;								// pointer to objects in this segment

			for (j = 0; j < MAX_SIDES_PER_SEGMENT; j++) {
				byte	type;									// replaces num_faces and tri_edge, 1 = quad, 2 = 0:2 triangulation, 3 = 1:3 triangulation
				ubyte	pad;									//keep us longword alligned
				short	wall_num;
				short	tmap_num;
				short	tmap_num2;
				uvl		uvls[4];
				vms_vector	normals[2];						// 2 normals, if quadrilateral, both the same.
				fprintf(fp, "%d\n", Segments[i].sides[j].type);
				fprintf(fp, "%d\n", Segments[i].sides[j].pad);
				fprintf(fp, "%d\n", Segments[i].sides[j].wall_num);
				fprintf(fp, "%d\n", Segments[i].tmap_num);

			}
			fclose(fp);
		}
	}
#endif

	Highest_vertex_index = Num_vertices-1;
	Highest_segment_index = Num_segments-1;

	validate_segment_all();			// Fill in side type and normals.

	for (i=0; i<Num_segments; i++) {
		if (Gamesave_current_version > 5)
			segment2_read(&Segment2s[i], LoadFile);
		fuelcen_activate( &Segments[i], Segment2s[i].special );
	}

	reset_objects(1);		//one object, the player

	return 0;
}
