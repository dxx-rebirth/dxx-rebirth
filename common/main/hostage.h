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

/*
 *
 * Header for hostage.c
 *
 */


#ifndef _HOSTAGE_H
#define _HOSTAGE_H

#ifdef __cplusplus
extern "C" {
#endif

struct object;

#define HOSTAGE_SIZE        i2f(5)  // 3d size of a hostage

#define MAX_HOSTAGE_TYPES   1       //only one hostage bitmap
#if defined(DXX_BUILD_DESCENT_I)
#define MAX_HOSTAGES				10		//max per any one level
#define HOSTAGE_MESSAGE_LEN	30

// 1 per hostage
typedef struct hostage_data {
	short		objnum;
	int		objsig;
} hostage_data;

extern hostage_data Hostages[MAX_HOSTAGES];

//returns true if something drew
int do_hostage_effects();

#ifdef EDITOR
void hostage_init_all();
void hostage_compress_all();
int hostage_is_valid( int hostage_num );
int hostage_object_is_valid( int objnum  );
void hostage_init_info( int objnum );
#endif
#elif defined(DXX_BUILD_DESCENT_II)
#ifdef EDITOR
static inline void hostage_init_all() {}
static inline void hostage_init_info( int objnum ) {(void)objnum;}
#endif
#endif

extern int N_hostage_types;

extern int Hostage_vclip_num[MAX_HOSTAGE_TYPES];    // for each type of hostage

void draw_hostage(object *obj);
void hostage_rescue( int hostage_number );

#ifdef __cplusplus
}
#endif

#endif /* _HOSTAGE_H */
