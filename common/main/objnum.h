#pragma once
#include <cstdint>

typedef uint16_t objnum_t;

enum class object_signature_t : uint16_t
{
};

static constexpr object_signature_t next(object_signature_t o)
{
	return object_signature_t{static_cast<uint16_t>(static_cast<uint16_t>(o) + 1u)};
}
