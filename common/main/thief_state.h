/*
 * This file is part of the DXX-Rebirth project <https://github.com/dxx-rebirth/dxx-rebirth/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */

#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include "aistruct.h"
#include "serial.h"

#ifdef DXX_BUILD_DESCENT
#if DXX_BUILD_DESCENT == 2
namespace dsx {

struct thief_state
{
	ai_mode mode;
	fix next_action_time;
};

DEFINE_SERIAL_UDT_TO_MESSAGE(thief_state, s, (s.mode, s.next_action_time));
inline constexpr std::size_t thief_state_size{serial::message_type<thief_state>::maximum_size};

static inline void write_thief_state(const thief_state &state, const std::span<uint8_t, thief_state_size> buf)
{
	const thief_state normalized{state.mode, std::max(state.next_action_time, fix{})};
	serial::writer::le_bytebuffer b{buf.data()};
	serial::process_buffer(b, normalized);
}

static inline std::optional<thief_state> read_thief_state(const std::span<const uint8_t, thief_state_size> buf)
{
	thief_state state;
	serial::reader::le_bytebuffer b{buf.data()};
	serial::process_buffer(b, state);
	switch (state.mode) {
		case ai_mode::AIM_THIEF_ATTACK:
		case ai_mode::AIM_THIEF_RETREAT:
		case ai_mode::AIM_THIEF_WAIT:
			break;
		default:
			return {};
	}
	if (state.next_action_time < 0)
		return {};
	return state;
}

}
#endif
#endif
