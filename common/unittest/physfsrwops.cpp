/*
 * This file is part of the DXX-Rebirth project <https://github.com/dxx-rebirth/dxx-rebirth/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */

#include "dxxsconf.h"

#include "physfsrwops.h"

#include <SDL.h>
#include <physfs.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Rebirth PhysicsFS RWops
#include <boost/test/unit_test.hpp>

namespace {

class physfsrwops_test_fixture
{
	static constexpr std::array<uint8_t, 7> payload{{1, 2, 3, 4, 5, 6, 7}};
	static constexpr char filename[]{"physfsrwops-test-data"};
	std::filesystem::path directory;
	bool physfs_initialized{};
	bool directory_mounted{};

	void cleanup()
	{
		if (directory_mounted)
			PHYSFS_unmount(directory.string().c_str());
		if (physfs_initialized)
			PHYSFS_deinit();
		std::filesystem::remove_all(directory);
	}

public:
	physfsrwops_test_fixture()
	{
		try {
			const auto unique{std::chrono::steady_clock::now().time_since_epoch().count()};
			directory = std::filesystem::temp_directory_path() / ("dxx-rebirth-physfsrwops-" + std::to_string(unique));
			if (!std::filesystem::create_directory(directory))
				throw std::runtime_error("Failed to create temporary directory");
			{
				std::ofstream output{directory / filename, std::ios::binary};
				output.write(reinterpret_cast<const char *>(payload.data()), payload.size());
				if (!output)
					throw std::runtime_error("Failed to create test input");
			}
			const auto argv0{boost::unit_test::framework::master_test_suite().argv[0]};
			if (!PHYSFS_init(argv0))
				throw std::runtime_error(PHYSFS_getLastError());
			physfs_initialized = true;
			if (!PHYSFS_mount(directory.string().c_str(), nullptr, 1))
				throw std::runtime_error(PHYSFS_getLastError());
			directory_mounted = true;
		} catch (...) {
			cleanup();
			throw;
		}
	}

	~physfsrwops_test_fixture()
	{
		cleanup();
	}

	static RWops_ptr open()
	{
		auto &&[rwops, error]{PHYSFSRWOPS_openRead(filename)};
		if (!rwops)
			throw std::runtime_error(PHYSFS_getErrorByCode(error));
		return std::move(rwops);
	}
};

}

#if SDL_MAJOR_VERSION == 2
BOOST_FIXTURE_TEST_CASE(size_preserves_position, physfsrwops_test_fixture)
{
	auto rwops{open()};
	BOOST_REQUIRE_EQUAL(SDL_RWseek(rwops.get(), 2, RW_SEEK_SET), 2);
	BOOST_TEST(SDL_RWsize(rwops.get()) == 7);
	BOOST_TEST(SDL_RWtell(rwops.get()) == 2);
}
#endif

BOOST_FIXTURE_TEST_CASE(read_returns_complete_object_count, physfsrwops_test_fixture)
{
	auto rwops{open()};
	std::array<uint8_t, 6> buffer{};
	constexpr std::array<uint8_t, 6> expected{{1, 2, 3, 4, 5, 6}};
	BOOST_TEST(SDL_RWread(rwops.get(), buffer.data(), 2, 3) == 3u);
	BOOST_TEST(buffer == expected);
	BOOST_TEST(SDL_RWtell(rwops.get()) == 6);
}

BOOST_FIXTURE_TEST_CASE(read_does_not_count_partial_object, physfsrwops_test_fixture)
{
	auto rwops{open()};
	std::array<uint8_t, 8> buffer{};
	BOOST_TEST(SDL_RWread(rwops.get(), buffer.data(), 4, 2) == 1u);
	BOOST_TEST(buffer[6] == 7u);
	BOOST_TEST(SDL_RWtell(rwops.get()) == 7);
}

BOOST_FIXTURE_TEST_CASE(zero_length_read_does_not_move_position, physfsrwops_test_fixture)
{
	auto rwops{open()};
	uint8_t buffer{};
	BOOST_TEST(SDL_RWread(rwops.get(), &buffer, 0, 1) == 0u);
	BOOST_TEST(SDL_RWread(rwops.get(), &buffer, 1, 0) == 0u);
	BOOST_TEST(SDL_RWtell(rwops.get()) == 0);
}

#if SDL_MAJOR_VERSION == 2
BOOST_FIXTURE_TEST_CASE(read_rejects_size_product_overflow, physfsrwops_test_fixture)
{
	auto rwops{open()};
	uint8_t buffer{};
	SDL_ClearError();
	constexpr auto maximum{std::numeric_limits<std::size_t>::max()};
	BOOST_TEST(SDL_RWread(rwops.get(), &buffer, maximum, maximum) == 0u);
	BOOST_TEST(SDL_GetError()[0] != '\0');
	BOOST_TEST(SDL_RWtell(rwops.get()) == 0);
}
#endif

BOOST_FIXTURE_TEST_CASE(write_error_returns_zero_objects, physfsrwops_test_fixture)
{
	auto rwops{open()};
	std::array<uint8_t, 6> buffer{};
	SDL_ClearError();
	BOOST_TEST(SDL_RWwrite(rwops.get(), buffer.data(), 2, 3) == 0u);
	BOOST_TEST(SDL_GetError()[0] != '\0');
}
