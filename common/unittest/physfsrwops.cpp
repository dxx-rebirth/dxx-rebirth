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

struct RAIItemporary_filesystem_directory
{
	std::filesystem::path directory;
	RAIItemporary_filesystem_directory(std::string &&directory_name_prefix) :
		directory{
			std::filesystem::temp_directory_path().append(
				(directory_name_prefix.append(std::to_string(/* unique time-based value to minimize chance of accidental collision */ std::chrono::steady_clock::now().time_since_epoch().count())), std::move(directory_name_prefix))
			)
		}
	{
		/* There are three possible results from calling `create_directory`:
		 * - true: directory created
		 * - false: directory already existed
		 * - exception: error trying to create directory
		 */
		if (!std::filesystem::create_directory(directory))
			/* No exception was thrown, but the directory was not created
			 * because it already existed.  Existence is very unlikely here
			 * due to the timestamp included in the path.
			 *
			 * Throw so that the directory will not be destroyed.
			 */
			throw std::runtime_error("Failed to create temporary directory " + directory.string());
	}
	~RAIItemporary_filesystem_directory()
	{
		std::filesystem::remove_all(directory);
	}
};

struct RAIIphysfs_init
{
	RAIIphysfs_init()
	{
		const auto argv0{boost::unit_test::framework::master_test_suite().argv[0]};
		if (!PHYSFS_init(argv0))
			throw std::runtime_error(PHYSFS_getLastError());
	}
	~RAIIphysfs_init()
	{
		PHYSFS_deinit();
	}
};

struct RAIItemporary_physfs_mounted_directory : RAIItemporary_filesystem_directory
{
	RAIItemporary_physfs_mounted_directory(std::string &&directory_name_prefix) :
		RAIItemporary_filesystem_directory{std::move(directory_name_prefix)}
	{
		if (!PHYSFS_mount(directory.string().c_str(), nullptr, 1))
			throw std::runtime_error(PHYSFS_getLastError());
	}

	~RAIItemporary_physfs_mounted_directory()
	{
		PHYSFS_unmount(directory.string().c_str());
	}
};

class physfsrwops_test_fixture : RAIIphysfs_init, RAIItemporary_physfs_mounted_directory
{
	static constexpr std::array<uint8_t, 7> payload{{1, 2, 3, 4, 5, 6, 7}};
	static constexpr char filename[]{"physfsrwops-test-data"};
public:
	physfsrwops_test_fixture() :
		RAIItemporary_physfs_mounted_directory{"dxx-rebirth-physfsrwops-"}
	{
		std::string pathname{directory / filename};
		std::ofstream output{pathname, std::ios::binary};
		output.write(reinterpret_cast<const char *>(payload.data()), payload.size());
		if (!output)
			throw std::runtime_error("Failed to create test input " + pathname);
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
