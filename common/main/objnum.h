#pragma once
#include "strictindex.h"

DEFINE_STRICT_INDEX_NUMBER(int16_t, objnum_t);
DEFINE_STRICT_INDEX_CONSTANT_TYPE(object_magic_constant_t, objnum_t, int16_t);
DEFINE_STRICT_INDEX_CONSTANT_NUMBER(object_magic_constant_t, -2, object_guidebot_cannot_reach);
DEFINE_STRICT_INDEX_CONSTANT_NUMBER(object_magic_constant_t, -1, object_none);
DEFINE_STRICT_INDEX_CONSTANT_NUMBER(object_magic_constant_t, 0, object_first);
