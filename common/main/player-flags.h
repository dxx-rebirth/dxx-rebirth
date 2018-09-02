/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */

#pragma once

#include <cstdint>

#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
// Values for special flags
enum class PLAYER_FLAG : uint32_t
{
	None = 0,
	INVULNERABLE = 1,	// Player is invincible
	BLUE_KEY = 2,		// Player has blue key
	RED_KEY = 4,		// Player has red key
	GOLD_KEY = 8,		// Player has gold key
	MAP_ALL = 64,		// Player can see unvisited areas on map
	QUAD_LASERS = 1024,	// Player shoots 4 at once
	/* Renamed from CLOAKED due to name conflict with AI define
	 * `CLOAKED`.
	 */
	PLAYER_CLOAKED = 2048,		// Player is cloaked for awhile
#if defined(DXX_BUILD_DESCENT_II)
	HAS_TEAM_FLAG = 16,			// Player has his team's flag
	AMMO_RACK = 128,	// Player has ammo rack
	CONVERTER = 256,	// Player has energy->shield converter
	AFTERBURNER = 4096,	// Player's afterburner is engaged
	HEADLIGHT = 8192,	// Player has headlight boost
	HEADLIGHT_ON = 16384,	// is headlight on or off?
	HEADLIGHT_PRESENT_AND_ON = HEADLIGHT | HEADLIGHT_ON,	// required for thief
#endif
};

constexpr PLAYER_FLAG operator~(const PLAYER_FLAG a)
{
	return static_cast<PLAYER_FLAG>(~static_cast<uint32_t>(a));
}

constexpr PLAYER_FLAG operator|(const PLAYER_FLAG a, const PLAYER_FLAG b)
{
	return static_cast<PLAYER_FLAG>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

#define PLAYER_FLAGS_INVULNERABLE	PLAYER_FLAG::INVULNERABLE
#define PLAYER_FLAGS_BLUE_KEY	PLAYER_FLAG::BLUE_KEY
#define PLAYER_FLAGS_RED_KEY	PLAYER_FLAG::RED_KEY
#define PLAYER_FLAGS_GOLD_KEY	PLAYER_FLAG::GOLD_KEY
#define PLAYER_FLAGS_MAP_ALL	PLAYER_FLAG::MAP_ALL
#define PLAYER_FLAGS_QUAD_LASERS	PLAYER_FLAG::QUAD_LASERS
#define PLAYER_FLAGS_CLOAKED	PLAYER_FLAG::PLAYER_CLOAKED
#if defined(DXX_BUILD_DESCENT_II)
#define PLAYER_FLAGS_FLAG	PLAYER_FLAG::HAS_TEAM_FLAG
#define PLAYER_FLAGS_AMMO_RACK	PLAYER_FLAG::AMMO_RACK
#define PLAYER_FLAGS_CONVERTER	PLAYER_FLAG::CONVERTER
#define PLAYER_FLAGS_AFTERBURNER	PLAYER_FLAG::AFTERBURNER
#define PLAYER_FLAGS_HEADLIGHT	PLAYER_FLAG::HEADLIGHT
#define PLAYER_FLAGS_HEADLIGHT_ON	PLAYER_FLAG::HEADLIGHT_ON
#endif

class player_flags
{
	uint32_t flags;
public:
	void operator&() const = delete;
	player_flags() = default;
	explicit player_flags(const uint32_t &f) : flags(f)
	{
	}
	explicit player_flags(PLAYER_FLAG f) : flags(static_cast<uint32_t>(f))
	{
	}
	uint32_t get_player_flags() const
	{
		return flags;
	}
	player_flags &operator&=(const player_flags &rhs)
	{
		flags &= rhs.flags;
		return *this;
	}
	player_flags &operator|=(const player_flags &rhs)
	{
		flags |= rhs.flags;
		return *this;
	}
	player_flags &operator|=(const uint32_t &rhs)
	{
		flags |= rhs;
		return *this;
	}
	player_flags operator|(const player_flags &rhs) const
	{
		return player_flags(flags | rhs.flags);
	}
	player_flags operator~() const
	{
		return player_flags(~flags);
	}
	uint32_t operator&(const PLAYER_FLAG value) const
	{
		return flags & static_cast<uint32_t>(value);
	}
	player_flags &operator^=(const PLAYER_FLAG value)
	{
		flags ^= static_cast<uint32_t>(value);
		return *this;
	}
	player_flags &operator&=(const PLAYER_FLAG value)
	{
		flags &= static_cast<uint32_t>(value);
		return *this;
	}
	player_flags &operator|=(const PLAYER_FLAG value)
	{
		flags |= static_cast<uint32_t>(value);
		return *this;
	}
};
#endif
