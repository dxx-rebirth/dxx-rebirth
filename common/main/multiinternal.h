/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once

#include "object.h"
#include "powerup.h"

#define define_multiplayer_command(NAME,SIZE)	NAME,

#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
#define for_each_multiplayer_command(VALUE)	\
	VALUE(MULTI_POSITION              , 25)	\
	VALUE(MULTI_REAPPEAR              , 4)	\
	VALUE(MULTI_FIRE                  , 18)	\
	VALUE(MULTI_FIRE_TRACK            , 21)	\
	VALUE(MULTI_FIRE_BOMB             , 20)	\
	VALUE(MULTI_KILL                  , 5)	\
	VALUE(MULTI_REMOVE_OBJECT         , 4)	\
	VALUE(MULTI_MESSAGE               , 37)	/* (MAX_MESSAGE_LENGTH = 40) */	\
	VALUE(MULTI_QUIT                  , 2)	\
	VALUE(MULTI_PLAY_SOUND            , 4)	\
	VALUE(MULTI_CONTROLCEN           , 4)	\
	VALUE(MULTI_ROBOT_CLAIM          , 5)	\
	VALUE(MULTI_CLOAK                , 2)	\
	VALUE(MULTI_ENDLEVEL_START       , 3)	\
	VALUE(MULTI_CREATE_EXPLOSION     , 2)	\
	VALUE(MULTI_CONTROLCEN_FIRE      , 16)	\
	VALUE(MULTI_CREATE_POWERUP       , 19)	\
	VALUE(MULTI_DECLOAK              , 2)	\
	VALUE(MULTI_ROBOT_POSITION       , 5+sizeof(shortpos))	\
	VALUE(MULTI_PLAYER_DERES         , DXX_MP_SIZE_PLAYER_RELATED)	\
	VALUE(MULTI_DOOR_OPEN            , DXX_MP_SIZE_DOOR_OPEN)	\
	VALUE(MULTI_ROBOT_EXPLODE        , DXX_MP_SIZE_ROBOT_EXPLODE)	\
	VALUE(MULTI_ROBOT_RELEASE        , 5)	\
	VALUE(MULTI_ROBOT_FIRE           , 18)	\
	VALUE(MULTI_SCORE                , 6)	\
	VALUE(MULTI_CREATE_ROBOT         , 6)	\
	VALUE(MULTI_TRIGGER              , 3)	\
	VALUE(MULTI_BOSS_ACTIONS         , 10)	\
	VALUE(MULTI_CREATE_ROBOT_POWERUPS, 27)	\
	VALUE(MULTI_HOSTAGE_DOOR         , 7)	\
	VALUE(MULTI_SAVE_GAME            , 2+24)	/* (ubyte slot, uint id, char name[20]) */	\
	VALUE(MULTI_RESTORE_GAME         , 2+4)	/* (ubyte slot, uint id) */	\
	VALUE(MULTI_HEARTBEAT            , 5)	\
	VALUE(MULTI_KILLGOALS            , 9)	\
	VALUE(MULTI_POWCAP_UPDATE        , MAX_POWERUP_TYPES+1)	\
	VALUE(MULTI_DO_BOUNTY            , 2)	\
	VALUE(MULTI_TYPING_STATE         , 3)	\
	VALUE(MULTI_GMODE_UPDATE         , 3)	\
	VALUE(MULTI_KILL_HOST            , 7)	\
	VALUE(MULTI_KILL_CLIENT          , 5)	\
	VALUE(MULTI_RANK                 , 3)	\
	D2X_MP_COMMANDS(VALUE)	\

#endif

#if defined(DXX_BUILD_DESCENT_I)
#define DXX_MP_SIZE_PLAYER_RELATED	58
#define DXX_MP_SIZE_BEGIN_SYNC	37
#define DXX_MP_SIZE_DOOR_OPEN	4
#define DXX_MP_SIZE_ROBOT_EXPLODE	8
#define D2X_MP_COMMANDS(VALUE)
#elif defined(DXX_BUILD_DESCENT_II)
#define DXX_MP_SIZE_PLAYER_RELATED	(97+10)
#define DXX_MP_SIZE_BEGIN_SYNC	41
#define DXX_MP_SIZE_DOOR_OPEN	5
#define DXX_MP_SIZE_ROBOT_EXPLODE	9
#define D2X_MP_COMMANDS(VALUE)	\
	VALUE(MULTI_MARKER               , 55)	\
	VALUE(MULTI_DROP_WEAPON          , 12)	\
	VALUE(MULTI_GUIDED               , 3+sizeof(shortpos))	\
	VALUE(MULTI_STOLEN_ITEMS         , 11)	\
	VALUE(MULTI_WALL_STATUS          , 6)	/* send to new players */	\
	VALUE(MULTI_SEISMIC              , 9)	\
	VALUE(MULTI_LIGHT                , 18)	\
	VALUE(MULTI_START_TRIGGER        , 2)	\
	VALUE(MULTI_FLAGS                , 6)	\
	VALUE(MULTI_DROP_BLOB            , 2)	\
	VALUE(MULTI_SOUND_FUNCTION       , 4)	\
	VALUE(MULTI_CAPTURE_BONUS        , 2)	\
	VALUE(MULTI_GOT_FLAG             , 2)	\
	VALUE(MULTI_DROP_FLAG            , 12)	\
	VALUE(MULTI_FINISH_GAME          , 2)	\
	VALUE(MULTI_ORB_BONUS            , 3)	\
	VALUE(MULTI_GOT_ORB              , 2)	\
	VALUE(MULTI_EFFECT_BLOWUP        , 17)	\

#endif

#include "dxxsconf.h"
#include "compiler-type_traits.h"

enum multiplayer_command_t {
	for_each_multiplayer_command(define_multiplayer_command)
};

template <multiplayer_command_t>
struct command_length;
#define define_command_length(NAME,SIZE)	\
	template <>	\
	struct command_length<NAME> : public tt::integral_constant<unsigned, SIZE> {};
for_each_multiplayer_command(define_command_length);

void _multi_send_data(const ubyte *buf, unsigned len, int priority);

template <multiplayer_command_t C>
static inline void multi_send_data(ubyte *buf, unsigned len, int priority)
{
	buf[0] = C;
	unsigned expected = command_length<C>::value;
	if (len != expected)
		Error("multi_send_data: Packet type %i length: %i, expected: %i\n", C, len, expected);
	_multi_send_data(buf, len, priority);
}
