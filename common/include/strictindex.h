#pragma once

template <typename, typename I, I N>
struct strict_index_magic_number_t
{
	operator I() const { return N; }
};

#define DEFINE_STRICT_INDEX_NUMBER(contained_type,name)	\
	typedef contained_type name

#define DEFINE_STRICT_INDEX_CONSTANT_TYPE(name,tag,I)	\
	template <I N>	\
	struct name : public strict_index_magic_number_t<tag,I,N> {}

#define DEFINE_STRICT_INDEX_CONSTANT_NUMBER(type,N,name)	\
	static const type<N> name = {}
