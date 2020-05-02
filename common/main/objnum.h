#pragma once
#include <cstdint>

typedef uint16_t objnum_t;

class object_signature_t
{
	uint16_t signature = 0;
public:
	constexpr object_signature_t() = default;
	constexpr explicit object_signature_t(uint16_t s) :
		signature(s)
	{
	}
	bool operator==(const object_signature_t &rhs) const
	{
		return signature == rhs.signature;
	}
	bool operator!=(const object_signature_t &rhs) const
	{
		return !(*this == rhs);
	}
	uint16_t get() const { return signature; }
};
