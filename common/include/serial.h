#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <tuple>

#include "dxxsconf.h"
#include "compiler-addressof.h"
#include "compiler-array.h"
#include "compiler-range_for.h"
#include "compiler-type_traits.h"

namespace serial {

template <typename A1, typename... Args>
class message;

	/* Classifiers to identify whether a type is a message<...> */
template <typename>
class is_message : public tt::false_type
{
};

template <typename A1, typename... Args>
class is_message<message<A1, Args...>> : public tt::true_type
{
};

template <typename T>
class integral_type
{
	static_assert(tt::is_integral<T>::value, "integral_type used on non-integral type");
public:
	static const std::size_t maximum_size = sizeof(T);
};

template <typename T>
class enum_type
{
	static_assert(tt::is_enum<T>::value, "enum_type used on non-enum type");
public:
	static const std::size_t maximum_size = sizeof(T);
};

template <typename>
class is_cxx_array : public tt::false_type
{
};

template <typename T, std::size_t N>
class is_cxx_array<array<T, N>> : public tt::true_type
{
};

template <typename T>
class is_cxx_array<const T> : public is_cxx_array<T>
{
};

template <typename T>
class is_generic_class : public tt::conditional<is_cxx_array<T>::value, tt::false_type, tt::is_class<T>>::type
{
};

template <typename Accessor, typename A1>
static inline typename tt::enable_if<tt::is_integral<A1>::value, void>::type process_buffer(Accessor &, A1 &);

template <typename Accessor, typename A1>
static inline typename tt::enable_if<tt::is_enum<A1>::value, void>::type process_buffer(Accessor &, A1 &);

template <typename Accessor, typename A1>
static inline typename tt::enable_if<is_generic_class<A1>::value, void>::type process_buffer(Accessor &, A1 &);

template <typename Accessor, typename A1>
typename tt::enable_if<is_cxx_array<A1>::value, void>::type process_buffer(Accessor &, A1 &);

template <std::size_t offset, typename Accessor, typename... Args>
static inline typename tt::enable_if<offset == sizeof...(Args), void>::type process_message_tuple(Accessor &, const std::tuple<Args...> &);

template <std::size_t offset, typename Accessor, typename... Args>
static inline typename tt::enable_if<offset != sizeof...(Args), void>::type process_message_tuple(Accessor &, const std::tuple<Args...> &);

template <typename Accessor, typename A1, typename... Args>
void process_buffer(Accessor &, const message<A1, Args...> &);

template <typename>
class class_type;

template <typename>
class array_type;

template <typename>
class unhandled_type;

	/* Implementation details - avoid namespace pollution */
namespace detail {

template <std::size_t amount>
class pad_type
{
};

template <std::size_t amount>
message<array<uint8_t, amount>> udt_to_message(const pad_type<amount> &);

/*
 * This can never be instantiated, but will be requested if a UDT
 * specialization is missing.
 */
template <typename T>
struct missing_udt_specialization
{
#ifndef DXX_HAVE_CXX11_EXPLICIT_DELETE
protected:
#endif
	missing_udt_specialization() DXX_CXX11_EXPLICIT_DELETE;
};

template <typename T>
void udt_to_message(T &, missing_udt_specialization<T> = missing_udt_specialization<T>());

template <typename Accessor, typename UDT>
void preprocess_udt(Accessor &, UDT &) {}

template <typename Accessor, typename UDT>
void postprocess_udt(Accessor &, UDT &) {}

template <typename Accessor, typename UDT>
static inline void process_udt(Accessor &accessor, UDT &udt)
{
	process_buffer(accessor, udt_to_message(udt));
}

template <typename T>
class class_type_indirection
{
public:
	typedef typename tt::enable_if<is_generic_class<T>::value, decltype(udt_to_message(*(const T*)0))>::type type;
};

template <typename Accessor, typename E>
void check_enum(Accessor &, E) {}

template <typename T, typename D>
class base_bytebuffer_t : public std::iterator<std::random_access_iterator_tag, T>
{
public:
	typedef typename std::iterator<std::random_access_iterator_tag, T>::pointer pointer;
	typedef typename std::iterator<std::random_access_iterator_tag, T>::difference_type difference_type;
	base_bytebuffer_t(pointer u) : p(u) {}
	operator pointer() const { return p; }
	D &operator+=(difference_type d)
	{
		p += d;
		return *static_cast<D *>(this);
	}
protected:
	pointer p;
};


template <typename Accessor, std::size_t amount>
static inline void process_udt(Accessor &accessor, const pad_type<amount> &udt)
{
#define SERIAL_UDT_ROUND_MULTIPLIER	(sizeof(void *))
#define SERIAL_UDT_ROUND_UP(X,M)	(((X) + (M) - 1) & ~((M) - 1))
#define SERIAL_UDT_ROUND_UP_AMOUNT SERIAL_UDT_ROUND_UP(amount, SERIAL_UDT_ROUND_MULTIPLIER)
	static_assert(amount % SERIAL_UDT_ROUND_MULTIPLIER ? SERIAL_UDT_ROUND_UP_AMOUNT > amount && SERIAL_UDT_ROUND_UP_AMOUNT < amount + SERIAL_UDT_ROUND_MULTIPLIER : SERIAL_UDT_ROUND_UP_AMOUNT == amount, "round up error");
	static_assert(SERIAL_UDT_ROUND_UP_AMOUNT % SERIAL_UDT_ROUND_MULTIPLIER == 0, "round modulus error");
	union {
		array<uint8_t, amount / 64 ? 64 : SERIAL_UDT_ROUND_UP_AMOUNT> f;
		array<uint8_t, amount % 64> p;
	};
#undef SERIAL_UDT_ROUND_UP_AMOUNT
#undef SERIAL_UDT_ROUND_UP
#undef SERIAL_UDT_ROUND_MULTIPLIER
	f.fill(0xcc);
	for (std::size_t count = amount; count; count -= f.size())
	{
		if (count < f.size())
		{
			assert(count == p.size());
			process_buffer(accessor, p);
			break;
		}
		process_buffer(accessor, f);
	}
}

}

template <std::size_t amount>
static inline const detail::pad_type<amount> pad()
{
	static const detail::pad_type<amount> p;
	return p;
}

#define DEFINE_SERIAL_UDT_TO_MESSAGE(TYPE, NAME, MEMBERLIST)	\
	DEFINE_SERIAL_CONST_UDT_TO_MESSAGE(TYPE, NAME, MEMBERLIST)	\
	DEFINE_SERIAL_MUTABLE_UDT_TO_MESSAGE(TYPE, NAME, MEMBERLIST)	\

#define _DEFINE_SERIAL_UDT_TO_MESSAGE(TYPE, NAME, MEMBERLIST)	\
	static inline auto udt_to_message(TYPE &NAME) -> decltype(serial::make_message MEMBERLIST) { \
		return serial::make_message MEMBERLIST;	\
	}

#define DEFINE_SERIAL_CONST_UDT_TO_MESSAGE(TYPE, NAME, MEMBERLIST)	\
	_DEFINE_SERIAL_UDT_TO_MESSAGE(const TYPE, NAME, MEMBERLIST)
#define DEFINE_SERIAL_MUTABLE_UDT_TO_MESSAGE(TYPE, NAME, MEMBERLIST)	\
	_DEFINE_SERIAL_UDT_TO_MESSAGE(TYPE, NAME, MEMBERLIST)

#define ASSERT_SERIAL_UDT_MESSAGE_SIZE(T, SIZE)	\
	static_assert(serial::class_type<T>::maximum_size == SIZE, "sizeof(" #T ") is not " #SIZE)

template <typename M1, typename T1>
class udt_message_compatible_same_type : public tt::integral_constant<bool, tt::is_same<typename tt::remove_const<M1>::type, T1>::value>
{
	static_assert(tt::is_same<typename tt::remove_const<M1>::type, T1>::value, "parameter type mismatch");
};

template <bool, typename M, typename T>
class assert_udt_message_compatible2;

template <typename M, typename T>
class assert_udt_message_compatible2<false, M, T> : public tt::integral_constant<bool, false>
{
};

template <typename M1, typename T1>
class assert_udt_message_compatible2<true, message<M1>, std::tuple<T1>> : public udt_message_compatible_same_type<M1, T1>
{
};

template <typename M1, typename M2, typename... Mn, typename T1, typename T2, typename... Tn>
class assert_udt_message_compatible2<true, message<M1, M2, Mn...>, std::tuple<T1, T2, Tn...>> :
	public tt::integral_constant<bool, assert_udt_message_compatible2<udt_message_compatible_same_type<M1, T1>::value, message<M2, Mn...>, std::tuple<T2, Tn...>>::value>
{
};

template <typename M, typename T>
class assert_udt_message_compatible1;

template <typename M1, typename... Mn, typename T1, typename... Tn>
class assert_udt_message_compatible1<message<M1, Mn...>, std::tuple<T1, Tn...>> : public tt::integral_constant<bool, assert_udt_message_compatible2<sizeof...(Mn) == sizeof...(Tn), message<M1, Mn...>, std::tuple<T1, Tn...>>::value>
{
	static_assert(sizeof...(Mn) <= sizeof...(Tn), "too few types in tuple");
	static_assert(sizeof...(Mn) >= sizeof...(Tn), "too few types in message");
};

template <typename, typename>
class assert_udt_message_compatible
{
};

template <typename C, typename T1, typename... Tn>
class assert_udt_message_compatible<C, std::tuple<T1, Tn...>> : public tt::integral_constant<bool, assert_udt_message_compatible1<typename class_type<C>::message, std::tuple<T1, Tn...>>::value>
{
};

#define _SERIAL_UDT_UNWRAP_LIST(A1,...)	A1, ## __VA_ARGS__

#define ASSERT_SERIAL_UDT_MESSAGE_TYPE(T, TYPELIST)	\
	ASSERT_SERIAL_UDT_MESSAGE_CONST_TYPE(T, TYPELIST);	\
	ASSERT_SERIAL_UDT_MESSAGE_MUTABLE_TYPE(T, TYPELIST);	\

#define _ASSERT_SERIAL_UDT_MESSAGE_TYPE(T, TYPELIST)	\
	static_assert(serial::assert_udt_message_compatible<T, std::tuple<_SERIAL_UDT_UNWRAP_LIST TYPELIST>>::value, "udt/message mismatch")

#define ASSERT_SERIAL_UDT_MESSAGE_CONST_TYPE(T, TYPELIST)	\
	_ASSERT_SERIAL_UDT_MESSAGE_TYPE(const T, TYPELIST)
#define ASSERT_SERIAL_UDT_MESSAGE_MUTABLE_TYPE(T, TYPELIST)	\
	_ASSERT_SERIAL_UDT_MESSAGE_TYPE(T, TYPELIST)

/*
 * Copy bytes from src to dst.  If running on a little endian system,
 * behave like memcpy.  If running on a big endian system, copy
 * backwards so that the destination is endian-swapped from the source.
 */
static inline void little_endian_copy(const uint8_t *src, uint8_t *dst, std::size_t len)
{
	const uint8_t *srcend = src + len;
	union {
		uint8_t c;
		uint16_t s;
	};
	s = 1;
	if (c)
		std::copy(src, srcend, dst);
	else
		std::reverse_copy(src, srcend, dst);
}

template <typename T>
class message_type
{
	typedef
		typename tt::conditional<tt::is_integral<T>::value, integral_type<T>,
			typename tt::conditional<tt::is_enum<T>::value, enum_type<T>,
				typename tt::conditional<is_cxx_array<T>::value, array_type<T>,
					typename tt::conditional<is_generic_class<T>::value, class_type<T>,
							unhandled_type<T>
						>::type
					>::type
				>::type
			>::type effective_type;
public:
	static const std::size_t maximum_size = effective_type::maximum_size;
};

template <typename T>
class class_type : public message_type<typename detail::class_type_indirection<T>::type>
{
public:
	typedef typename detail::class_type_indirection<T>::type message;
};

template <typename T, std::size_t N>
class array_type<const array<T, N>>
{
public:
	static const std::size_t maximum_size = message_type<T>::maximum_size * N;
};

template <typename T, std::size_t N>
class array_type<array<T, N>> : public array_type<const array<T, N>>
{
};

template <typename A1>
class message_type<message<A1>>
{
public:
	static const std::size_t maximum_size = message_type<A1>::maximum_size;
};

template <typename A1, typename A2, typename... Args>
class message_type<message<A1, A2, Args...>>
{
public:
	static const std::size_t maximum_size = message_type<A1>::maximum_size + message_type<message<A2, Args...>>::maximum_size;
};

template <typename A1, typename... Args>
class message
{
	typedef std::tuple<typename tt::add_pointer<A1>::type, typename tt::add_pointer<Args>::type...> tuple_type;
	template <typename T1>
		static void check_type()
		{
			static_assert(message_type<T1>::maximum_size > 0, "empty field in message");
		}
	template <typename T1, typename T2, typename... Tn>
		static void check_type()
		{
			check_type<T1>();
			check_type<T2, Tn...>();
		}
	tuple_type t;
public:
	typedef A1 head_type;
	message(A1 &a1, Args&... args) :
		t(addressof(a1), addressof(args)...)
	{
		check_type<A1, Args...>();
	}
	const tuple_type &get_tuple() const
	{
		return t;
	}
};

template <typename A1, typename... Args>
static inline message<A1, Args...> make_message(A1 &a1, Args&... args)
{
	return message<A1, Args...>(a1, std::forward<Args&>(args)...);
}

namespace reader {

class bytebuffer_t : public detail::base_bytebuffer_t<const uint8_t, bytebuffer_t>
{
public:
	bytebuffer_t(pointer u) : base_bytebuffer_t(u) {}
};

template <typename Accessor, typename A1>
static inline void process_integer(Accessor &buffer, A1 &a1)
{
	union {
		A1 a;
		uint8_t u[message_type<A1>::maximum_size];
	};
	static_assert(sizeof(a) == sizeof(u), "message_type<A1>::maximum_size is wrong");
	little_endian_copy(buffer, u, sizeof(u));
	std::advance(buffer, sizeof(u));
	a1 = a;
}

}

namespace writer {

class bytebuffer_t : public detail::base_bytebuffer_t<uint8_t, bytebuffer_t>
{
public:
	bytebuffer_t(pointer u) : base_bytebuffer_t(u) {}
};

template <typename Accessor, typename A1>
static inline void process_integer(Accessor &buffer, const A1 &a1)
{
	union {
		A1 a;
		uint8_t u[message_type<A1>::maximum_size];
	};
	static_assert(sizeof(a) == sizeof(u), "message_type<A1>::maximum_size is wrong");
	a = a1;
	little_endian_copy(u, buffer, sizeof(u));
	std::advance(buffer, sizeof(u));
}

}

template <typename Accessor, typename A1>
static inline typename tt::enable_if<tt::is_integral<A1>::value, void>::type process_buffer(Accessor &accessor, A1 &a1)
{
	process_integer(accessor, a1);
}

template <typename Accessor, typename A1>
static inline typename tt::enable_if<tt::is_enum<A1>::value, void>::type process_buffer(Accessor &accessor, A1 &a1)
{
	using detail::check_enum;
	process_integer(accessor, a1);
	/* Hook for enum types to check that the given value is legal */
	check_enum(accessor, a1);
}

template <typename Accessor, typename A1>
static inline typename tt::enable_if<is_generic_class<A1>::value, void>::type process_buffer(Accessor &accessor, A1 &a1)
{
	using detail::preprocess_udt;
	using detail::process_udt;
	using detail::postprocess_udt;
	preprocess_udt(accessor, a1);
	process_udt(accessor, a1);
	postprocess_udt(accessor, a1);
}

template <typename Accessor, typename A1>
typename tt::enable_if<is_cxx_array<A1>::value, void>::type process_buffer(Accessor &accessor, A1 &a1)
{
	range_for (auto &i, a1)
		process_buffer(accessor, i);
}

template <std::size_t offset, typename Accessor, typename... Args>
static inline typename tt::enable_if<offset == sizeof...(Args), void>::type process_message_tuple(Accessor &, const std::tuple<Args...> &)
{
}

template <std::size_t offset, typename Accessor, typename... Args>
static inline typename tt::enable_if<offset != sizeof...(Args), void>::type process_message_tuple(Accessor &accessor, const std::tuple<Args...> &t)
{
	process_buffer(accessor, *std::get<offset>(t));
	process_message_tuple<offset + 1>(accessor, t);
}

template <typename Accessor, typename A1, typename... Args>
void process_buffer(Accessor &accessor, const message<A1, Args...> &m)
{
	process_message_tuple<0>(accessor, m.get_tuple());
}

}
