/*
 * This file is part of the DXX-Rebirth project <https://github.com/dxx-rebirth/dxx-rebirth/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */

#include "dxxsconf.h"
#include "args.h"
#include "console.h"
#include "dxxerror.h"
#include "u_mem.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

#undef main	/* Boost.Test supplies the test entry point. */
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Rebirth debug memory allocator
#include <boost/test/unit_test.hpp>

#if DXX_USE_DEBUG_MEMORY_ALLOCATOR
namespace {

class allocation_diagnostic : public std::runtime_error
{
public:
	using runtime_error::runtime_error;
};

static unsigned leaked_blocks;

static void observe_diagnostic(const std::string_view message)
{
	if (message.find("MEM_LEAKAGE") != message.npos)
		++leaked_blocks;
	/* Stop at the diagnostic, before the allocator's debug trap. */
	if (message.find("MEM_OVERWRITE") != message.npos ||
		message.find("_NOMALLOC") != message.npos ||
		message.find("MEM_FREE_NULL") != message.npos)
		throw allocation_diagnostic{std::string{message}};
}

struct free_allocation
{
	static void operator()(uint8_t *p) { ::dcx::mem_free(p); }
};
using allocation_ptr = std::unique_ptr<uint8_t, free_allocation>;

static allocation_ptr allocate(const std::size_t size)
{
	return allocation_ptr{static_cast<uint8_t *>(::dcx::mem_malloc(size, "test", __FILE__, __LINE__))};
}


}

/* Link the production allocator with observable diagnostics, without the
 * console or game startup/shutdown machinery. */
namespace dcx {

CArg CGameArg{};

void (con_printf)(con_priority_wrapper, const char *message, ...)
{
	observe_diagnostic(message);
}

void con_puts(con_priority_wrapper, const std::span<const char> message)
{
	observe_diagnostic({message.data(), message.size()});
}

void con_puts(con_priority_wrapper, const std::span<char> message)
{
	observe_diagnostic({message.data(), message.size()});
}

void Warning_puts(const char *) {}
void Warning(const char *, ...) {}

void (Error_puts)(const char *, unsigned, const char *, const char *message)
{
	throw std::runtime_error{message};
}

void (Error)(const char *, unsigned, const char *, const char *message, ...)
{
	throw std::runtime_error{message};
}

}

BOOST_AUTO_TEST_CASE(free_checks_the_allocation_guard)
{
	auto p = allocate(7);
	/* Keep deliberate guard access out of alloc_size constant checks. */
	volatile std::size_t guard_offset = 7;
	const auto offset = guard_offset;
	/* If free misses the corruption, it releases the block.  Stop at the
	 * failed assertion without touching or freeing that block again. */
	auto *const raw = p.release();
	const auto guard = raw[offset];
	raw[offset] = 0;
	BOOST_REQUIRE_THROW(::dcx::mem_free(raw), allocation_diagnostic);
	raw[offset] = guard;
	p.reset(raw);
}

BOOST_AUTO_TEST_CASE(calloc_and_realloc_preserve_the_payload)
{
	allocation_ptr p{static_cast<uint8_t *>((::dcx::mem_calloc)(4, 1, "test", __FILE__, __LINE__))};
	const std::array<uint8_t, 4> zero{};
	BOOST_CHECK_EQUAL_COLLECTIONS(p.get(), p.get() + 4, zero.begin(), zero.end());
	const std::array<uint8_t, 4> payload{{1, 127, 128, 255}};
	std::copy(payload.begin(), payload.end(), p.get());
	p.reset(static_cast<uint8_t *>(::dcx::mem_realloc(p.release(), 8, "test", __FILE__, __LINE__)));
	BOOST_CHECK_EQUAL_COLLECTIONS(p.get(), p.get() + 4, payload.begin(), payload.end());
	p.reset(static_cast<uint8_t *>(::dcx::mem_realloc(p.release(), 2, "test", __FILE__, __LINE__)));
	BOOST_CHECK_EQUAL_COLLECTIONS(p.get(), p.get() + 2, payload.begin(), payload.begin() + 2);
}

BOOST_AUTO_TEST_CASE(zero_sized_allocation_and_reallocation)
{
	auto empty = allocate(0);
	allocation_ptr p{static_cast<uint8_t *>(::dcx::mem_realloc(nullptr, 3, "test", __FILE__, __LINE__))};
	BOOST_CHECK(::dcx::mem_realloc(p.release(), 0, "test", __FILE__, __LINE__) == nullptr);
}

BOOST_AUTO_TEST_CASE(calloc_with_a_zero_dimension)
{
	/* Keep intentional boundary inputs out of alloc_size constant checks. */
	volatile std::size_t zero = 0, limit = std::numeric_limits<std::size_t>::max();
	const auto empty = zero, maximum = limit;
	allocation_ptr no_elements{static_cast<uint8_t *>((::dcx::mem_calloc)(empty, maximum, "test", __FILE__, __LINE__))};
	allocation_ptr no_size{static_cast<uint8_t *>((::dcx::mem_calloc)(maximum, empty, "test", __FILE__, __LINE__))};
}

BOOST_AUTO_TEST_CASE(freed_tracking_slots_are_reused)
{
	for (unsigned i = 0; i != 10001; ++i) {
		auto p = allocate(1);
		*p = 42;
	}
}

BOOST_AUTO_TEST_CASE(invalid_reallocation_does_not_allocate_or_use_a_missing_slot)
{
	uint8_t untracked{};
	BOOST_CHECK_THROW(::dcx::mem_realloc(&untracked, 1, "test", __FILE__, __LINE__), allocation_diagnostic);
	leaked_blocks = 0;
	::dcx::CGameArg.DbgShowMemInfo = true;
	::dcx::mem_display_blocks();
	::dcx::CGameArg.DbgShowMemInfo = false;
	BOOST_CHECK_EQUAL(leaked_blocks, 0);
}
#else
BOOST_AUTO_TEST_CASE(debug_memory_allocator_is_disabled)
{
	BOOST_TEST_MESSAGE("Debug allocator tests require memdebug=1.");
}
#endif
