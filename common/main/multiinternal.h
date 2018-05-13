/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once

#include <type_traits>
#include "dxxerror.h"
#include "object.h"
#include "powerup.h"
#include "serial.h"
#include "fwd-player.h"

#define _UNPACK_MULTIPLAYER_SERIAL_MESSAGE(A,...)	A, ## __VA_ARGS__
#define DEFINE_MULTIPLAYER_SERIAL_MESSAGE(C,T,V,A)	\
	DEFINE_SERIAL_UDT_TO_MESSAGE(T,V, (multiplayer_command<C>(), _UNPACK_MULTIPLAYER_SERIAL_MESSAGE A));	\
	ASSERT_SERIAL_UDT_MESSAGE_SIZE(T, command_length<C>::value)

#define define_multiplayer_command(NAME,SIZE)	NAME,

#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
#define for_each_multiplayer_command(VALUE)	\
	VALUE(MULTI_POSITION              , 1 + quaternionpos::packed_size::value)	\
	VALUE(MULTI_REAPPEAR              , 4)	\
	VALUE(MULTI_FIRE                  , 18)	\
	VALUE(MULTI_FIRE_TRACK            , 21)	\
	VALUE(MULTI_FIRE_BOMB             , 20)	\
	VALUE(MULTI_REMOVE_OBJECT         , 4)	\
	VALUE(MULTI_MESSAGE               , 37)	/* (MAX_MESSAGE_LENGTH = 40) */	\
	VALUE(MULTI_QUIT                  , 2)	\
	VALUE(MULTI_PLAY_SOUND           , 8)	\
	VALUE(MULTI_CONTROLCEN           , 4)	\
	VALUE(MULTI_ROBOT_CLAIM          , 5)	\
	VALUE(MULTI_CLOAK                , 2)	\
	VALUE(MULTI_ENDLEVEL_START       , DXX_MP_SIZE_ENDLEVEL_START)	\
	VALUE(MULTI_CREATE_EXPLOSION     , 2)	\
	VALUE(MULTI_CONTROLCEN_FIRE      , 16)	\
	VALUE(MULTI_CREATE_POWERUP       , 19)	\
	VALUE(MULTI_DECLOAK              , 2)	\
	VALUE(MULTI_ROBOT_POSITION       , 5 + quaternionpos::packed_size::value)	\
	VALUE(MULTI_PLAYER_DERES         , DXX_MP_SIZE_PLAYER_RELATED)	\
	VALUE(MULTI_DOOR_OPEN            , DXX_MP_SIZE_DOOR_OPEN)	\
	VALUE(MULTI_ROBOT_EXPLODE        , 7)	\
	VALUE(MULTI_ROBOT_RELEASE        , 5)	\
	VALUE(MULTI_ROBOT_FIRE           , 18)	\
	VALUE(MULTI_SCORE                , 6)	\
	VALUE(MULTI_CREATE_ROBOT         , 6)	\
	VALUE(MULTI_TRIGGER              , 3)	\
	VALUE(MULTI_BOSS_TELEPORT        , 5)	\
	VALUE(MULTI_BOSS_CLOAK           , 3)	\
	VALUE(MULTI_BOSS_START_GATE      , 3)	\
	VALUE(MULTI_BOSS_STOP_GATE       , 3)	\
	VALUE(MULTI_BOSS_CREATE_ROBOT    , 8)	\
	VALUE(MULTI_CREATE_ROBOT_POWERUPS, 27)	\
	VALUE(MULTI_HOSTAGE_DOOR         , 7)	\
	VALUE(MULTI_SAVE_GAME            , 2+24)	/* (ubyte slot, uint id, char name[20]) */	\
	VALUE(MULTI_RESTORE_GAME         , 2+4)	/* (ubyte slot, uint id) */	\
	VALUE(MULTI_HEARTBEAT            , 5)	\
	VALUE(MULTI_KILLGOALS            , 1 + MAX_PLAYERS)	\
	VALUE(MULTI_DO_BOUNTY            , 2)	\
	VALUE(MULTI_TYPING_STATE         , 3)	\
	VALUE(MULTI_GMODE_UPDATE         , 3)	\
	VALUE(MULTI_KILL_HOST            , 7)	\
	VALUE(MULTI_KILL_CLIENT          , 5)	\
	VALUE(MULTI_RANK                 , 3)	\
	VALUE(MULTI_DROP_WEAPON          , 10)	\
        VALUE(MULTI_PLAYER_INV           , DXX_MP_SIZE_PLAYER_INVENTORY)	\
	D2X_MP_COMMANDS(VALUE)	\

#if defined(DXX_BUILD_DESCENT_I)
#define DXX_MP_SIZE_ENDLEVEL_START	3
#define DXX_MP_SIZE_PLAYER_RELATED	58
#define DXX_MP_SIZE_PLAYER_INVENTORY	15
#define DXX_MP_SIZE_BEGIN_SYNC	37
#define DXX_MP_SIZE_DOOR_OPEN	4
#define D2X_MP_COMMANDS(VALUE)
#elif defined(DXX_BUILD_DESCENT_II)
#define DXX_MP_SIZE_ENDLEVEL_START	2
#define DXX_MP_SIZE_PLAYER_RELATED	(97+10)
#define DXX_MP_SIZE_PLAYER_INVENTORY	21
#define DXX_MP_SIZE_BEGIN_SYNC	41
#define DXX_MP_SIZE_DOOR_OPEN	5
#define D2X_MP_COMMANDS(VALUE)	\
	VALUE(MULTI_MARKER               , 55)	\
	VALUE(MULTI_GUIDED               , 3+sizeof(shortpos))	\
	VALUE(MULTI_STOLEN_ITEMS         , 11)	\
	VALUE(MULTI_WALL_STATUS          , 6)	/* send to new players */	\
	VALUE(MULTI_SEISMIC              , 5)	\
	VALUE(MULTI_LIGHT                , 16)	\
	VALUE(MULTI_START_TRIGGER        , 2)	\
	VALUE(MULTI_FLAGS                , 6)	\
	VALUE(MULTI_DROP_BLOB            , 2)	\
	VALUE(MULTI_SOUND_FUNCTION       , 4)	\
	VALUE(MULTI_CAPTURE_BONUS        , 2)	\
	VALUE(MULTI_GOT_FLAG             , 2)	\
	VALUE(MULTI_DROP_FLAG            , 8)	\
	VALUE(MULTI_FINISH_GAME          , 2)	\
	VALUE(MULTI_ORB_BONUS            , 3)	\
	VALUE(MULTI_GOT_ORB              , 2)	\
	VALUE(MULTI_EFFECT_BLOWUP        , 17)	\
	VALUE(MULTI_VULWPN_AMMO_ADJ      , 6)	\

#endif

#include "dxxsconf.h"

enum multiplayer_command_t : uint8_t
{
	for_each_multiplayer_command(define_multiplayer_command)
};

template <multiplayer_command_t>
struct command_length;
#define define_command_length(NAME,SIZE)	\
	template <>	\
	struct command_length<NAME> : public std::integral_constant<unsigned, SIZE> {};
for_each_multiplayer_command(define_command_length);

template <multiplayer_command_t C>
struct multi_command : public std::array<uint8_t, command_length<C>::value>
{
};

void _multi_send_data(const uint8_t *buf, unsigned len, int priority);

template <multiplayer_command_t C>
static inline void multi_send_data(uint8_t *const buf, const unsigned len, const int priority)
{
	buf[0] = C;
	constexpr unsigned expected = command_length<C>::value;
#ifdef DXX_CONSTANT_TRUE
	if (DXX_CONSTANT_TRUE(len != expected))
		DXX_ALWAYS_ERROR_FUNCTION(dxx_trap_multi_send_data, "wrong packet size");
#endif
	if (len != expected)
	{
		Error("multi_send_data: Packet type %i length: %i, expected: %i\n", C, len, expected);
	}
	_multi_send_data(buf, len, priority);
}

template <multiplayer_command_t C>
static inline void multi_send_data(multi_command<C> &buf, const int priority)
{
	buf[0] = C;
	_multi_send_data(buf.data(), buf.size(), priority);
}

template <typename T>
static inline void multi_serialize_read(const uint8_t *const buf, T &t)
{
	serial::reader::bytebuffer_t b(buf);
	serial::process_buffer(b, t);
}

template <typename T>
static inline void multi_serialize_write(int priority, const T &t)
{
	constexpr size_t maximum_size = serial::message_type<T>::maximum_size;
	uint8_t buf[maximum_size];
	serial::writer::bytebuffer_t b(buf);
	serial::process_buffer(b, t);
	_multi_send_data(buf, maximum_size, priority);
}

template <multiplayer_command_t C>
using multiplayer_command = serial::pad<1, static_cast<uint8_t>(C)>;

#endif
