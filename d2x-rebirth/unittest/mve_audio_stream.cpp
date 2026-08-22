/*
 * This file is part of the DXX-Rebirth project <https://github.com/dxx-rebirth/dxx-rebirth/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */

#include "dxxsconf.h"

#include <SDL.h>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Rebirth MVE audio stream
#include <boost/test/unit_test.hpp>

#if DXX_USE_SDLMIXER && SDL_MAJOR_VERSION == 2

namespace {

struct audio_stream_deleter
{
	static void operator()(SDL_AudioStream *const stream)
	{
		SDL_FreeAudioStream(stream);
	}
};

using audio_stream_ptr = std::unique_ptr<SDL_AudioStream, audio_stream_deleter>;

std::vector<uint8_t> convert(const std::span<const int16_t> input, const std::span<const std::size_t> partitions)
{
	audio_stream_ptr stream{SDL_NewAudioStream(AUDIO_S16SYS, 1, 22050, AUDIO_S16SYS, 2, 44100)};
	BOOST_REQUIRE(stream);
	std::vector<uint8_t> output;
	std::size_t offset{};
	const auto drain = [&] {
		const auto available = SDL_AudioStreamAvailable(stream.get());
		BOOST_REQUIRE(available >= 0);
		const auto old_size = output.size();
		output.resize(old_size + available);
		if (available)
			BOOST_REQUIRE_EQUAL(SDL_AudioStreamGet(stream.get(), output.data() + old_size, available), available);
	};
	for (const auto count : partitions)
	{
		BOOST_REQUIRE(count <= input.size() - offset);
		BOOST_REQUIRE_EQUAL(SDL_AudioStreamPut(stream.get(), input.data() + offset,
			static_cast<int>(count * sizeof(input.front()))), 0);
		offset += count;
		drain();
	}
	BOOST_REQUIRE_EQUAL(offset, input.size());
	BOOST_REQUIRE_EQUAL(SDL_AudioStreamFlush(stream.get()), 0);
	drain();
	return output;
}

}

BOOST_AUTO_TEST_CASE(conversion_is_independent_of_input_partitioning)
{
	std::vector<int16_t> input(22050);
	for (std::size_t i = 0; i != input.size(); ++i)
		input[i] = static_cast<int16_t>(((i * 977u) % 60001u) - 30000);

	std::vector<std::size_t> regular;
	for (std::size_t left = input.size(); left;)
	{
		const auto count = std::min<std::size_t>(left, 733);
		regular.push_back(count);
		left -= count;
	}

	std::vector<std::size_t> irregular;
	for (std::size_t left = input.size(), count = 1; left; count = (count * 17 + 31) % 509 + 1)
	{
		const auto actual = std::min(left, count);
		irregular.push_back(actual);
		left -= actual;
	}

	const auto regularly_chunked = convert(input, regular);
	const auto irregularly_chunked = convert(input, irregular);
	BOOST_TEST(regularly_chunked == irregularly_chunked, boost::test_tools::per_element());
	BOOST_TEST(regularly_chunked.size() == input.size() * 2u * 2u * 2u);
}

#else

BOOST_AUTO_TEST_CASE(requires_sdl2_mixer)
{
	BOOST_TEST(true);
}

#endif
