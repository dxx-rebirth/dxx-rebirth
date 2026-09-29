/*
 * This file is part of the DXX-Rebirth project <https://github.com/dxx-rebirth/dxx-rebirth/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace dcx {

template <std::size_t player_count>
bool complete_player_slot_mapping(std::array<int8_t, player_count> &saved_to_current_player)
{
	std::array<bool, player_count> claimed{};
	for (const auto current_player : saved_to_current_player)
	{
		if (current_player == -1)
			continue;
		if (current_player < 0 || static_cast<std::size_t>(current_player) >= player_count || claimed[current_player])
			return false;
		claimed[current_player] = true;
	}
	std::size_t next_unclaimed_player = 0;
	for (auto &current_player : saved_to_current_player)
	{
		if (current_player != -1)
			continue;
		while (claimed[next_unclaimed_player])
			++next_unclaimed_player;
		current_player = static_cast<int8_t>(next_unclaimed_player);
		claimed[next_unclaimed_player] = true;
	}
	return true;
}

template <std::size_t player_count, std::size_t object_count, typename owned_remote_objnum_range, typename object_is_live>
bool validate_network_object_mappings(const owned_remote_objnum_range &mappings, const std::span<const int8_t, player_count> saved_to_current_player, const std::size_t local_player, const std::size_t active_player_count, object_is_live &&is_live)
{
	if (local_player >= active_player_count || active_player_count > player_count || mappings.size() > object_count)
		return false;
	std::array<std::array<bool, object_count>, player_count> claimed{};
	std::size_t local_object = 0;
	for (const auto &mapping : mappings)
	{
		if (mapping.owner == -1)
		{
			++local_object;
			continue;
		}
		if (!is_live(local_object) || mapping.owner < 0 || static_cast<std::size_t>(mapping.owner) >= player_count)
			return false;
		const auto owner = saved_to_current_player[mapping.owner];
		if (owner < 0 || owner >= active_player_count || mapping.objnum >= object_count || claimed[owner][mapping.objnum])
			return false;
		if (static_cast<std::size_t>(owner) == local_player && mapping.objnum != local_object)
			return false;
		claimed[owner][mapping.objnum] = true;
		++local_object;
	}
	return true;
}

}
