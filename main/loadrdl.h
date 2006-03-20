#ifndef _LOADRDL_H
#define _LOADRDL_H

#include "types.h"

#define COMPILED_MINE_VERSION 0

#define GAME_VERSION                                    25
#define GAME_COMPATIBLE_VERSION	22

#define MENU_CURSOR_X_MIN			MENU_X
#define MENU_CURSOR_X_MAX			MENU_X+6

#define HOSTAGE_DATA_VERSION	0

//Start old wall structures

typedef struct v16_wall {
	byte  type; 			  	// What kind of special wall.
	byte	flags;				// Flags for the wall.		
	fix   hps;				  	// "Hit points" of the wall. 
	byte	trigger;				// Which trigger is associated with the wall.
	byte	clip_num;			// Which	animation associated with the wall. 
	byte	keys;
	} __pack__ v16_wall;

typedef struct v19_wall {
	int	segnum,sidenum;	// Seg & side for this wall
	byte	type; 			  	// What kind of special wall.
	byte	flags;				// Flags for the wall.		
	fix   hps;				  	// "Hit points" of the wall. 
	byte	trigger;				// Which trigger is associated with the wall.
	byte	clip_num;			// Which	animation associated with the wall. 
	byte	keys;
	int	linked_wall;		// number of linked wall
	} __pack__ v19_wall;

typedef struct v19_door {
	int		n_parts;					// for linked walls
	short 	seg[2]; 					// Segment pointer of door.
	short 	side[2];					// Side number of door.
	short 	type[2];					// What kind of door animation.
	fix 		open;						//	How long it has been open.
} __pack__ v19_door;

//End old wall structures

struct {
	ushort 	fileinfo_signature;
	ushort	fileinfo_version;
	int		fileinfo_sizeof;
} __pack__ game_top_fileinfo;	 // Should be same as first two fields below...

struct {
	ushort 	fileinfo_signature;
	ushort	fileinfo_version;
	int		fileinfo_sizeof;
	char		mine_filename[15];
	int		level;
	int		player_offset;				// Player info
	int		player_sizeof;
	int		object_offset;				// Object info
	int		object_howmany;    	
	int		object_sizeof;  
	int		walls_offset;
	int		walls_howmany;
	int		walls_sizeof;
	int		doors_offset;
	int		doors_howmany;
	int		doors_sizeof;
	int		triggers_offset;
	int		triggers_howmany;
	int		triggers_sizeof;
	int		links_offset;
	int		links_howmany;
	int		links_sizeof;
	int		control_offset;
	int		control_howmany;
	int		control_sizeof;
	int		matcen_offset;
	int		matcen_howmany;
	int		matcen_sizeof;
} __pack__ game_fileinfo;

#endif
