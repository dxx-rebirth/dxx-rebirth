#pragma once
#include "strictindex.h"

DEFINE_STRICT_INDEX_NUMBER(uint16_t, objnum_t);
DEFINE_STRICT_INDEX_CONSTANT_TYPE(object_magic_constant_t, objnum_t, uint16_t);
DEFINE_STRICT_INDEX_CONSTANT_NUMBER(object_magic_constant_t, 0xfffe, object_guidebot_cannot_reach);
DEFINE_STRICT_INDEX_CONSTANT_NUMBER(object_magic_constant_t, 0xffff, object_none);
DEFINE_STRICT_INDEX_CONSTANT_NUMBER(object_magic_constant_t, 0, object_first);

class object_signature_t
{
	uint16_t signature;
public:
	object_signature_t() = default;
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
