#pragma once

#define _DECLARE_VALPTR_SUBTYPE(prefix)	\
	struct prefix##ptr_t;	\
	struct prefix##ptridx_t;	\
	struct v##prefix##ptr_t;	\
	struct v##prefix##ptridx_t	\

#define DECLARE_VALPTR_SUBTYPE(prefix, type)	\
	struct type;	\
	_DECLARE_VALPTR_SUBTYPE(prefix);	\
	_DECLARE_VALPTR_SUBTYPE(c##prefix);	\

DECLARE_VALPTR_SUBTYPE(obj, object);
DECLARE_VALPTR_SUBTYPE(seg, segment);
