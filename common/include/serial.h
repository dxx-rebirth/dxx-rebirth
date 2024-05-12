/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once
#include <algorithm>
#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <tuple>
#include <type_traits>

#include "dxxsconf.h"
#include "compiler-range_for.h"
#include <array>
#include <memory>
#include <utility>

namespace serial {

template <std::size_t Expected, std::size_t Actual>
concept check_equal_value = (Expected == Actual);

template <typename... Args>
class message;

template <typename>
class message_type;

	/* Classifiers to identify whether a type is a message<...> */
template <typename>
class is_message : public std::false_type
{
};

template <typename... Args>
class is_message<message<Args...>> : public std::true_type
{
};

namespace detail {

template <std::size_t maximum, std::size_t minimum = maximum>
struct size_base
{
	static constexpr std::integral_constant<std::size_t, maximum> maximum_size{};
	static constexpr std::integral_constant<std::size_t, minimum> minimum_size{};
};

}

template <typename>
class is_cxx_array : public std::false_type
{
};

template <typename T, std::size_t N>
class is_cxx_array<std::array<T, N>> : public std::true_type
{
public:
	using array_type = std::array<T, N>;
};

template <typename T>
class is_cxx_array<const T> : public is_cxx_array<T>
{
};

template <typename T>
using is_generic_class = typename std::conditional<is_cxx_array<T>::value, std::false_type, std::is_class<T>>::type;

template <typename Accessor, typename A1>
requires(std::is_integral<typename std::remove_reference<A1>::type>::value)
static inline void process_buffer(Accessor &&accessor, A1 &&a1)
{
	process_integer(std::forward<Accessor &&>(accessor), a1);
}

template <typename Accessor, typename A1, typename A1rr = typename std::remove_reference<A1>::type>
requires(std::is_enum<A1rr>::value)
static inline void process_buffer(Accessor &, A1 &&);

template <typename Accessor, typename A1, typename A1rr = typename std::remove_reference<A1>::type>
requires(is_generic_class<A1rr>::value)
static inline void process_buffer(Accessor &, A1 &&);

template <typename Accessor, typename A1>
requires(is_cxx_array<A1>::value)
static void process_buffer(Accessor &&, A1 &);

template <typename Accessor, typename... Args>
static void process_buffer(Accessor &, const message<Args...> &);

class endian_access
{
public:
	/*
	 * Endian access modes:
	 * - little_endian: assume buffered data is little endian
	 *   Copy on little endian host, byte swap on big endian host
	 * - big_endian: assume buffered data is big endian
	 *   Copy on big endian host, byte swap on little endian host
	 * - native_endian: assume buffered data is native endian
	 *   Copy regardless of host byte order
	 */
	using little_endian_type = std::integral_constant<std::endian, std::endian::little>;
	using big_endian_type = std::integral_constant<std::endian, std::endian::big>;
	using native_endian_type = std::integral_constant<std::endian, std::endian::native>;
};

	/* Implementation details - avoid namespace pollution */
namespace detail {

template <typename T, typename Trr = typename std::remove_reference<T>::type>
using capture_type = typename std::conditional<std::is_lvalue_reference<T>::value,
			std::reference_wrapper<Trr>,
			std::tuple<Trr>
		>;

template <typename T, typename Trr = typename std::remove_reference<T>::type>
static inline auto capture_value(Trr &t)
{
	return std::ref(t);
}

template <typename T, typename Trr = typename std::remove_reference<T>::type>
requires(std::is_rvalue_reference<T>::value)
static inline auto capture_value(Trr &&t)
{
	return std::tuple<Trr>{std::forward<T>(t)};
}

template <typename extended_signed_type, typename wrapped_type>
class sign_extend_type : std::reference_wrapper<wrapped_type>
{
	static_assert(sizeof(extended_signed_type) > sizeof(wrapped_type), "cannot sign-extend into a type of equal or smaller size");
	static_assert(std::is_signed<extended_signed_type>::value, "cannot sign-extend into an unsigned type");
	using base_type = std::reference_wrapper<wrapped_type>;
public:
	using base_type::base_type;
	using base_type::get;
};

template <typename extended_signed_type, typename wrapped_type>
message<std::array<uint8_t, sizeof(extended_signed_type)>> udt_to_message(const sign_extend_type<extended_signed_type, wrapped_type> &);

template <std::size_t amount, uint8_t value>
class pad_type
{
};

template <std::size_t amount, uint8_t value>
message<std::array<uint8_t, amount>> udt_to_message(const pad_type<amount, value> &);

/*
 * This can never be instantiated, but will be requested if a UDT
 * specialization is missing.
 */
template <typename T>
class missing_udt_specialization
{
public:
	missing_udt_specialization() = delete;
};

template <typename T>
void udt_to_message(T &, missing_udt_specialization<T> = missing_udt_specialization<T>());

static void preprocess_udt(auto && /* accessor */, auto & /* udt */) {}

static void postprocess_udt(auto && /* accessor */, auto & /* udt */) {}

template <typename Accessor, typename UDT>
static inline void process_udt(Accessor &&accessor, UDT &udt)
{
	process_buffer(std::forward<Accessor &&>(accessor), udt_to_message(udt));
}

template <typename E>
void check_enum(auto && /* accessor */, E) {}

template <typename buffer_value_type>
struct bytebuffer : endian_access
{
	using iterator_category = std::random_access_iterator_tag;
	using value_type = buffer_value_type;
	using difference_type = std::ptrdiff_t;
	using pointer = value_type *;
	using reference = value_type &;
	constexpr bytebuffer(const pointer u) : p{u} {}
	operator pointer() const { return p; }
	void operator++()
	{
		++p;
	}
	void operator--()
	{
		--p;
	}
	void operator+=(const difference_type d)
	{
		p += d;
	}
	operator const void *() const = delete;
protected:
	pointer p;
};

#define SERIAL_UDT_ROUND_UP(X,M)	(((X) + (M) - 1) & ~((M) - 1))
template <std::size_t amount,
	std::size_t SERIAL_UDT_ROUND_MULTIPLIER = sizeof(void *),
	std::size_t SERIAL_UDT_ROUND_UP_AMOUNT = SERIAL_UDT_ROUND_UP(amount, SERIAL_UDT_ROUND_MULTIPLIER),
	std::size_t FULL_SIZE = amount / 64 ? 64 : SERIAL_UDT_ROUND_UP_AMOUNT,
	std::size_t REMAINDER_SIZE = amount % 64>
union pad_storage
{
	static_assert(amount % SERIAL_UDT_ROUND_MULTIPLIER ? SERIAL_UDT_ROUND_UP_AMOUNT > amount && SERIAL_UDT_ROUND_UP_AMOUNT < amount + SERIAL_UDT_ROUND_MULTIPLIER : SERIAL_UDT_ROUND_UP_AMOUNT == amount, "round up error");
	static_assert(SERIAL_UDT_ROUND_UP_AMOUNT % SERIAL_UDT_ROUND_MULTIPLIER == 0, "round modulus error");
	static_assert(amount % FULL_SIZE == REMAINDER_SIZE || FULL_SIZE == REMAINDER_SIZE, "padding alignment error");
	std::array<uint8_t, FULL_SIZE> f;
	std::array<uint8_t, REMAINDER_SIZE> p;
#undef SERIAL_UDT_ROUND_UP
};

template <typename Accessor, std::size_t amount, uint8_t value>
static inline void process_udt(Accessor &&accessor, const pad_type<amount, value> &)
{
	/* If reading from accessor, accessor data is const and buffer is
	 * overwritten by read.
	 * If writing to accessor, accessor data is non-const, so initialize
	 * buffer to be written.
	 */
	pad_storage<amount> s;
	if constexpr (!std::is_const<
		typename std::remove_pointer<
		/* rvalue reference `Accessor &&` causes `Accessor` to be `T &`
		 * for some type T.  Use std::remove_reference to get T.  Then
		 * take the type `pointer` from type T to use as input to
		 * std::remove_pointer.
		 */
			typename std::remove_reference<Accessor>::type
			::pointer
		>::type
	>::value)
		s.f.fill(value);
	for (std::size_t count{amount}; count; count -= s.f.size())
	{
		if (count < s.f.size())
		{
			assert(count == s.p.size());
			process_buffer(accessor, s.p);
			break;
		}
		process_buffer(accessor, s.f);
	}
}

template <typename T>
static inline T &extract_value(std::reference_wrapper<T> t)
{
	return t;
}

template <typename T>
static inline T &extract_value(std::tuple<T> &t)
{
	return std::get<0>(t);
}

template <typename T>
static inline const T &extract_value(const std::tuple<T> &t)
{
	return std::get<0>(t);
}

template <typename T>
struct message_dispatch_base
{
	using effective_type = T;
};

}

template <std::size_t amount, uint8_t value = 0xcc>
using pad = detail::pad_type<amount, value>;

template <typename extended_signed_type, typename wrapped_type>
static inline detail::sign_extend_type<extended_signed_type, wrapped_type> sign_extend(wrapped_type &t)
{
	return {t};
}

#define DEFINE_SERIAL_UDT_TO_MESSAGE(TYPE, NAME, MEMBERLIST)	\
	DEFINE_SERIAL_CONST_UDT_TO_MESSAGE(TYPE, NAME, MEMBERLIST)	\
	DEFINE_SERIAL_MUTABLE_UDT_TO_MESSAGE(TYPE, NAME, MEMBERLIST)	\

#define _DEFINE_SERIAL_UDT_TO_MESSAGE(TYPE, NAME, MEMBERLIST)	\
	template <typename Accessor>	\
	static inline void process_udt(Accessor &&accessor, TYPE &NAME)	\
	{	\
		using serial::process_buffer;	\
		process_buffer(std::forward<Accessor &&>(accessor), _SERIAL_UDT_UNWRAP_LIST MEMBERLIST);	\
	}	\
	\
	__attribute_unused	\
	static inline auto udt_to_message(TYPE &NAME) { \
		return serial::message MEMBERLIST;	\
	}

#define DEFINE_SERIAL_CONST_UDT_TO_MESSAGE(TYPE, NAME, MEMBERLIST)	\
	_DEFINE_SERIAL_UDT_TO_MESSAGE(const TYPE, NAME, MEMBERLIST)
#define DEFINE_SERIAL_MUTABLE_UDT_TO_MESSAGE(TYPE, NAME, MEMBERLIST)	\
	_DEFINE_SERIAL_UDT_TO_MESSAGE(TYPE, NAME, MEMBERLIST)

template <std::size_t Expected, typename T>
concept check_serial_udt_maximum_message_size = check_equal_value<Expected, T::maximum_size.value>;

#define ASSERT_SERIAL_UDT_MESSAGE_SIZE(T, SIZE)	\
	static_assert(serial::check_serial_udt_maximum_message_size<SIZE, serial::class_type<T>>)

template <typename M1, typename T1, typename base_type = std::is_same<typename std::remove_cv<typename std::remove_reference<M1>::type>::type, T1>>
struct udt_message_compatible_same_type : base_type
{
	static_assert(base_type::value, "parameter type mismatch");
};

template <bool, typename M, typename T>
class assert_udt_message_compatible2;

template <typename M, typename T>
class assert_udt_message_compatible2<false, M, T> : public std::false_type
{
};

template <typename... Mn, typename... Tn>
class assert_udt_message_compatible2<true, message<Mn...>, std::tuple<Tn...>> :
	public std::integral_constant<bool, (udt_message_compatible_same_type<Mn, Tn>::value && ...)>
{
};

template <typename M, typename T>
class assert_udt_message_compatible;

template <typename... Mn, typename... Tn>
class assert_udt_message_compatible<message<Mn...>, std::tuple<Tn...>> : public assert_udt_message_compatible2<sizeof...(Mn) == sizeof...(Tn), message<Mn...>, std::tuple<Tn...>>
{
	static_assert(sizeof...(Mn) <= sizeof...(Tn), "too few types in tuple");
	static_assert(sizeof...(Mn) >= sizeof...(Tn), "too few types in message");
};

template <typename T>
using class_type = message_type<decltype(udt_to_message(std::declval<T>()))>;

#define _SERIAL_UDT_UNWRAP_LIST(A1,...)	A1, ## __VA_ARGS__

#define ASSERT_SERIAL_UDT_MESSAGE_TYPE(T, TYPELIST)	\
	ASSERT_SERIAL_UDT_MESSAGE_CONST_TYPE(T, TYPELIST);	\
	ASSERT_SERIAL_UDT_MESSAGE_MUTABLE_TYPE(T, TYPELIST);	\

#define _ASSERT_SERIAL_UDT_MESSAGE_TYPE(T, TYPELIST)	\
	static_assert(serial::assert_udt_message_compatible<typename class_type<T>::as_message, std::tuple<_SERIAL_UDT_UNWRAP_LIST TYPELIST>>::value, "udt/message mismatch")

#define ASSERT_SERIAL_UDT_MESSAGE_CONST_TYPE(T, TYPELIST)	\
	_ASSERT_SERIAL_UDT_MESSAGE_TYPE(const T, TYPELIST)
#define ASSERT_SERIAL_UDT_MESSAGE_MUTABLE_TYPE(T, TYPELIST)	\
	_ASSERT_SERIAL_UDT_MESSAGE_TYPE(T, TYPELIST)

static constexpr uint8_t endian_skip_byteswap(std::endian E)
{
	/* If this static_assert fails, then the comparison may return the wrong
	 * result.
	 */
	static_assert(std::endian::little == std::endian::native || std::endian::big == std::endian::native, "host byte order must be little endian or big endian");
	return E == std::endian::native;
}

template <typename T, std::size_t N>
union unaligned_storage
{
	T a;
	typename std::conditional<N < 4,
		typename std::conditional<N == 1, uint8_t, uint16_t>,
		typename std::conditional<N == 4, uint32_t, uint64_t>>::type::type i;
	uint8_t u[N];
	static_assert(check_equal_value<N, sizeof(i)>);
	static_assert(check_equal_value<sizeof(u), sizeof(a)>);
};

template <typename T, typename = void>
class message_dispatch_type;

template <typename T>
class message_dispatch_type<T, typename std::enable_if<std::is_integral<T>::value or std::is_enum<T>::value, void>::type> :
	public detail::message_dispatch_base<detail::size_base<sizeof(T)>>
{
};

template <typename T>
class message_dispatch_type<T, typename std::enable_if<is_cxx_array<T>::value, void>::type> :
	public detail::message_dispatch_base<
		detail::size_base<
			message_type<typename T::value_type>::maximum_size * std::tuple_size<typename is_cxx_array<T>::array_type>::value,
			message_type<typename T::value_type>::minimum_size * std::tuple_size<typename is_cxx_array<T>::array_type>::value
		>
	>
{
};

template <typename T>
class message_dispatch_type<T, typename std::enable_if<is_generic_class<T>::value && !is_message<T>::value, void>::type> :
	public detail::message_dispatch_base<class_type<T>>
{
};

template <typename T>
class message_type :
	message_dispatch_type<typename std::remove_reference<T>::type>::effective_type
{
	using effective_type = typename message_dispatch_type<typename std::remove_reference<T>::type>::effective_type;
public:
	using effective_type::maximum_size;
	using effective_type::minimum_size;
};

template <typename A1>
class message_dispatch_type<message<A1>, void> :
	public detail::message_dispatch_base<message_type<A1>>
{
public:
	typedef message<A1> as_message;
};

template <typename... Args>
class message_type<message<Args...>> :
	public detail::size_base<
		(0 + ... + message_dispatch_type<message<Args>>::effective_type::maximum_size),
		(0 + ... + message_dispatch_type<message<Args>>::effective_type::minimum_size)
	>
{
public:
	using as_message = message<Args...>;
};

template <typename... Args>
class message
{
	static_assert(sizeof...(Args) > 0, "message must have at least one template argument");
	using tuple_type = std::tuple<typename detail::capture_type<Args &&>::type...>;
	template <typename T1>
		static void check_type()
		{
			static_assert(message_type<T1>::maximum_size > 0, "empty field in message");
		}
	tuple_type t;
public:
	message(Args &&... args) :
		t{detail::capture_value<Args>(std::forward<Args>(args))...}
	{
		(check_type<Args>(), ...);
	}
	const tuple_type &get_tuple() const
	{
		return t;
	}
};

template <typename... Args>
message(Args &&... args) -> message<Args && ...>;

template <typename T>
static constexpr T bswap(const T u)
{
	if constexpr (std::is_same<T, int8_t>::value || std::is_same<T, uint8_t>::value)
		/* Swapping a byte-sized value is a no-op.  This is permitted here so
		 * that callers can swap without checking the size of the value.
		 */
		return u;
	if constexpr (std::is_same<T, int16_t>::value || std::is_same<T, uint16_t>::value)
	{
#ifdef DXX_HAVE_BUILTIN_BSWAP
		return __builtin_bswap16(u);
#else
		return (static_cast<T>(static_cast<uint8_t>(u)) << 8) | static_cast<T>(static_cast<uint8_t>(u >> 8));
#endif
	}
	if constexpr (std::is_same<T, int32_t>::value || std::is_same<T, uint32_t>::value)
	{
#ifdef DXX_HAVE_BUILTIN_BSWAP
		return __builtin_bswap32(u);
#else
		return (static_cast<T>(bswap(static_cast<uint16_t>(u))) << 16) | static_cast<T>(bswap(static_cast<uint16_t>(u >> 16)));
#endif
	}
	if constexpr (std::is_same<T, int64_t>::value || std::is_same<T, uint64_t>::value)
	{
#ifdef DXX_HAVE_BUILTIN_BSWAP
		return __builtin_bswap64(u);
#else
		return (static_cast<T>(bswap(static_cast<uint32_t>(u))) << 32) | static_cast<T>(bswap(static_cast<uint32_t>(u >> 32)));
#endif
	}
	/* Unsupported type.  Fall off the end of the function and trigger a
	 * compile error due to the missing `return` statement.
	 */
}

template <typename T, T Expected, T input>
concept check_bswap = check_equal_value<Expected, bswap(input)>;

static_assert(check_bswap<uint8_t, 1, 1>);

static_assert(check_bswap<uint16_t, 0x1200, 0x12>);
static_assert(check_bswap<uint16_t, 0x9200, 0x92>);
static_assert(check_bswap<uint16_t, 0x92, 0x9200>);
static_assert(check_bswap<uint16_t, 0x201, 0x102>);

static_assert(check_bswap<uint32_t, 0x2010000, 0x102>);

static_assert(check_bswap<uint64_t, 0x201000000000000ull, 0x102>);

namespace reader {

template <std::endian endian_value>
struct bytebuffer : detail::bytebuffer<const uint8_t>
{
	using detail::bytebuffer<const uint8_t>::bytebuffer;
	explicit bytebuffer(const bytebuffer &) = default;
	bytebuffer(bytebuffer &&) = default;
	static constexpr std::integral_constant<std::endian, endian_value> endian{};
};
using be_bytebuffer = bytebuffer<std::endian::big>;
using le_bytebuffer = bytebuffer<std::endian::little>;
using ne_bytebuffer = bytebuffer<std::endian::native>;

template <typename A1, std::size_t BYTES>
static inline void unaligned_copy(const uint8_t *src, unaligned_storage<A1, BYTES> &dst)
{
	if constexpr (BYTES == 1)
		dst.u[0] = {*src};
	else
		std::copy_n(src, sizeof(dst.u), dst.u);
}

template <typename Accessor, typename A1>
static inline void process_integer(Accessor &buffer, A1 &a1)
{
	using std::advance;
	unaligned_storage<A1, message_type<A1>::maximum_size> u;
	unaligned_copy(buffer, u);
	if (!endian_skip_byteswap(buffer.endian()))
		u.i = {bswap(u.i)};
	a1 = u.a;
	advance(buffer, sizeof(u.u));
}

template <typename Accessor, typename A, typename T = typename A::value_type>
/* When the type is an array of byte-size integers (typically, `char` or
 * `uint8_t`), enable this overload for more efficient copying.
 */
requires(sizeof(T) == 1 && std::is_integral<T>::value)
static inline void process_array(Accessor &accessor, A &a)
{
	using std::advance;
	std::copy_n(static_cast<typename Accessor::pointer>(accessor), a.size(), a.data());
	advance(accessor, a.size());
}

template <typename Accessor, typename extended_signed_type, typename wrapped_type>
static inline void process_udt(Accessor &&accessor, const detail::sign_extend_type<extended_signed_type, wrapped_type> &v)
{
	extended_signed_type est;
	process_integer(static_cast<Accessor &&>(accessor), est);
	v.get() = static_cast<wrapped_type>(est);
}

}

namespace writer {

template <std::endian endian_value>
struct bytebuffer : detail::bytebuffer<uint8_t>
{
	using detail::bytebuffer<uint8_t>::bytebuffer;
	explicit bytebuffer(const bytebuffer &) = default;
	bytebuffer(bytebuffer &&) = default;
	static constexpr std::integral_constant<std::endian, endian_value> endian{};
};
using be_bytebuffer = bytebuffer<std::endian::big>;
using le_bytebuffer = bytebuffer<std::endian::little>;
using ne_bytebuffer = bytebuffer<std::endian::native>;

/* If unaligned_copy is manually inlined into the caller, then gcc
 * inlining of copy_n creates a loop instead of a store.
 */
template <typename A1, std::size_t BYTES>
static inline void unaligned_copy(const unaligned_storage<A1, BYTES> &src, uint8_t *dst)
{
	if constexpr (BYTES == 1)
		*dst = {src.u[0]};
	else
		std::copy_n(src.u, sizeof(src.u), dst);
}

template <typename Accessor, typename A1>
static inline void process_integer(Accessor &buffer, const A1 &a1)
{
	using std::advance;
	unaligned_storage<A1, message_type<A1>::maximum_size> u{a1};
	if (!endian_skip_byteswap(buffer.endian()))
		u.i = {bswap(u.i)};
	unaligned_copy(u, buffer);
	advance(buffer, sizeof(u.u));
}

template <typename Accessor, typename A, typename T = typename A::value_type>
requires(sizeof(T) == 1 && std::is_integral<T>::value)
static inline void process_array(Accessor &accessor, const A &a)
{
	using std::advance;
	std::copy_n(a.data(), a.size(), static_cast<typename Accessor::pointer>(accessor));
	advance(accessor, a.size());
}

template <typename Accessor, typename extended_signed_type, typename wrapped_type>
static inline void process_udt(Accessor &&accessor, const detail::sign_extend_type<extended_signed_type, const wrapped_type> &v)
{
	const typename std::make_signed<wrapped_type>::type swt = v.get();
	process_integer(static_cast<Accessor &&>(accessor), extended_signed_type{swt});
}

}

template <typename Accessor, typename A1, typename A1rr>
requires(std::is_enum<A1rr>::value)
static inline void process_buffer(Accessor &accessor, A1 &&a1)
{
	using detail::check_enum;
	process_integer(accessor, a1);
	/* Hook for enum types to check that the given value is legal */
	check_enum(accessor, a1);
}

template <typename Accessor, typename A1, typename A1rr>
requires(is_generic_class<A1rr>::value)
static inline void process_buffer(Accessor &accessor, A1 &&a1)
{
	using detail::preprocess_udt;
	using detail::process_udt;
	using detail::postprocess_udt;
	preprocess_udt(accessor, a1);
	process_udt(accessor, std::forward<A1>(a1));
	postprocess_udt(accessor, a1);
}

template <typename Accessor, typename A, typename T = typename A::value_type>
requires(!(sizeof(T) == 1 && std::is_integral<T>::value))
static void process_array(Accessor &accessor, A &a)
{
	range_for (auto &i, a)
		process_buffer(accessor, i);
}

template <typename Accessor, typename A1>
requires(is_cxx_array<A1>::value)
static void process_buffer(Accessor &&accessor, A1 &a1)
{
	process_array(std::forward<Accessor &&>(accessor), a1);
}

template <typename Accessor, typename... Args, std::size_t... N>
static inline void process_message_tuple(Accessor &&accessor, const std::tuple<Args...> &t, std::index_sequence<N...>)
{
	(process_buffer(accessor, detail::extract_value(std::get<N>(t))), ...);
}

template <typename Accessor, typename... Args>
static void process_buffer(Accessor &&accessor, const message<Args...> &m)
{
	process_message_tuple(std::forward<Accessor &&>(accessor), m.get_tuple(), std::make_index_sequence<sizeof...(Args)>());
}

/* Require at least two arguments to prevent self-selection */
template <typename Accessor, typename... An>
requires(sizeof...(An) > 1)
static void process_buffer(Accessor &&accessor, An &&... an)
{
	(process_buffer(accessor, std::forward<An>(an)), ...);
}

}
