#include "network-object-mapping.h"
#include <array>
#include <cstdint>
#include <span>

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Rebirth network object mapping
#include <boost/test/unit_test.hpp>

namespace {

constexpr std::size_t player_count = 4;
constexpr std::size_t object_count = 8;

struct mapping
{
	int8_t owner{-1};
	uint16_t objnum{};
};

using mapping_array = std::array<mapping, object_count>;
using player_mapping = std::array<int8_t, player_count>;

bool validate(const mapping_array &mappings, const player_mapping &players, const std::size_t local_player, const std::size_t active_players, const std::array<bool, object_count> &live)
{
	return dcx::validate_network_object_mappings<player_count, object_count>(mappings, std::span<const int8_t, player_count>{players}, local_player, active_players, [&live](const std::size_t object) {
		return live[object];
	});
}

}

BOOST_AUTO_TEST_CASE(accepts_preplaced_local_and_remote_objects)
{
	mapping_array mappings{};
	mappings[1] = {0, 1};
	mappings[5] = {1, 3};
	const player_mapping players{{0, 1, 2, 3}};
	std::array<bool, object_count> live{};
	live[1] = live[5] = true;
	BOOST_TEST(validate(mappings, players, 0, player_count, live));
}

BOOST_AUTO_TEST_CASE(rejects_invalid_or_duplicate_identities)
{
	mapping_array mappings{};
	const player_mapping players{{0, 1, 2, 3}};
	std::array<bool, object_count> live{};
	live[1] = live[2] = true;
	mappings[1] = {1, 3};
	mappings[2] = {1, 3};
	BOOST_TEST(!validate(mappings, players, 0, player_count, live));
	mappings[2] = {-2, 2};
	BOOST_TEST(!validate(mappings, players, 0, player_count, live));
	mappings[2] = {4, 2};
	BOOST_TEST(!validate(mappings, players, 0, player_count, live));
	mappings[2] = {1, static_cast<uint16_t>(object_count)};
	BOOST_TEST(!validate(mappings, players, 0, player_count, live));
}

BOOST_AUTO_TEST_CASE(rejects_self_owner_mismatch_and_dead_object_mapping)
{
	mapping_array mappings{};
	const player_mapping players{{0, 1, 2, 3}};
	std::array<bool, object_count> live{};
	live[2] = true;
	mappings[2] = {0, 3};
	BOOST_TEST(!validate(mappings, players, 0, player_count, live));
	mappings[2] = {-1, 0};
	mappings[3] = {1, 3};
	BOOST_TEST(!validate(mappings, players, 0, player_count, live));
}

BOOST_AUTO_TEST_CASE(completes_player_permutation_and_preserves_peer_identity)
{
	player_mapping players{{1, 0, -1, -1}};
	BOOST_REQUIRE(dcx::complete_player_slot_mapping(players));
	const player_mapping expected{{1, 0, 2, 3}};
	BOOST_TEST(players == expected);

	mapping_array creator_mappings{};
	mapping_array remote_mappings{};
	creator_mappings[1] = {0, 1};
	remote_mappings[5] = {0, 1};
	std::array<bool, object_count> creator_live{};
	std::array<bool, object_count> remote_live{};
	creator_live[1] = true;
	remote_live[5] = true;
	BOOST_TEST(validate(creator_mappings, players, 1, 2, creator_live));
	BOOST_TEST(validate(remote_mappings, players, 0, 2, remote_live));
	BOOST_TEST(players[creator_mappings[1].owner] == players[remote_mappings[5].owner]);
	BOOST_TEST(creator_mappings[1].objnum == remote_mappings[5].objnum);
	remote_mappings[5].owner = 3;
	BOOST_TEST(!validate(remote_mappings, players, 0, 2, remote_live));
}

BOOST_AUTO_TEST_CASE(rejects_duplicate_partial_player_mapping)
{
	player_mapping players{{0, 0, -1, -1}};
	BOOST_TEST(!dcx::complete_player_slot_mapping(players));
}
