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

#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <type_traits>
#include "fwd-object.h"
#include "fwd-vclip.h"

#if DXX_USE_EDITOR
#include "fwd-d_array.h"
#endif

namespace dcx {

constexpr std::integral_constant<unsigned, 196> VULCAN_WEAPON_AMMO_AMOUNT{};
constexpr uint16_t VULCAN_AMMO_AMOUNT = 49*2;

constexpr std::integral_constant<unsigned, 16> POWERUP_NAME_LENGTH{};
struct powerup_type_info;

void powerup_type_info_read(NamedPHYSFS_File fp, powerup_type_info &pti);
void powerup_type_info_write(PHYSFS_File *fp, const powerup_type_info &pti);

extern uint8_t N_powerup_types;

}

#ifdef DXX_BUILD_DESCENT
namespace dsx {

enum class powerup_type_t : uint8_t;

#if DXX_BUILD_DESCENT == 1
constexpr std::integral_constant<unsigned, 392*2> VULCAN_AMMO_MAX{};
constexpr std::integral_constant<unsigned, 29> MAX_POWERUP_TYPES{};
#elif DXX_BUILD_DESCENT == 2
constexpr std::integral_constant<unsigned, 392*4> VULCAN_AMMO_MAX{};
constexpr std::integral_constant<unsigned, 392> GAUSS_WEAPON_AMMO_AMOUNT{};
constexpr std::integral_constant<unsigned, 50> MAX_POWERUP_TYPES{};
#endif

#if DXX_USE_EDITOR
using powerup_names_array = enumerated_array<std::array<char, POWERUP_NAME_LENGTH>, MAX_POWERUP_TYPES, powerup_type_t>;
extern powerup_names_array Powerup_names;
#endif

void draw_powerup(const d_vclip_array &Vclip, grs_canvas &, const object_base &obj);
int do_powerup(vmobjptridx_t obj);

//process (animate) a powerup for one frame
void do_powerup_frame(const d_vclip_array &Vclip, vmobjptridx_t obj);

void do_megawow_powerup(object &plrobj, int quantity);

void powerup_basic_str(int redadd, int greenadd, int blueadd, int score, std::span<const char> str);

// Interface to object_create_egg, puts count objects of type type, id
// = id in objp and then drops them.
imobjptridx_t call_object_create_egg(const object_base &objp, powerup_type_t id);
void call_object_create_egg(const object_base &objp, unsigned count, powerup_type_t id);
}
#endif
