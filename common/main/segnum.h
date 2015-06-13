#pragma once
#include "strictindex.h"

DEFINE_STRICT_INDEX_NUMBER(uint16_t, segnum_t);
DEFINE_STRICT_INDEX_CONSTANT_TYPE(segment_magic_constant_t, segnum_t, uint16_t);
DEFINE_STRICT_INDEX_CONSTANT_NUMBER(segment_magic_constant_t, 0xfffe, segment_exit);
DEFINE_STRICT_INDEX_CONSTANT_NUMBER(segment_magic_constant_t, 0xffff, segment_none);
DEFINE_STRICT_INDEX_CONSTANT_NUMBER(segment_magic_constant_t, 0, segment_first);
