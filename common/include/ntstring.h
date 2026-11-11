#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include "dxxsconf.h"
#include "pack.h"
#include <array>

template <std::size_t L>
class ntstring :
	public std::array<char, L + 1>,
	public prohibit_void_ptr<>
{
public:
	using array_t = std::array<char, L + 1>;
	typedef char elements_t[L + 1];
	using array_t::operator[];
	dxx_compiler_attribute_nonnull()
	std::size_t _copy_n(std::size_t offset, const char *ib, std::size_t N)
	{
		if (offset >= this->size())
			return 0;
		const auto r = N <= L - offset ? N : 0;
		const auto dst = std::span{*this}.subspan(offset);
		const auto oc = std::copy_n(ib, r, dst.begin());
		std::fill(oc, dst.end(), 0);
		return r;
	}
	template <std::size_t N>
		dxx_compiler_attribute_nonnull()
		std::size_t _copy(std::size_t offset, const char *i)
		{
			static_assert(N <= L, "string too long");
			return _copy_n(offset, i, N);
		}
	template <typename I>
		std::size_t copy_if(I &&i)
		{
			return copy_if(std::size_t{0}, static_cast<I &&>(i));
		}
	template <typename I>
		std::size_t copy_if(I &&i, std::size_t N)
		{
			return copy_if(std::size_t{0}, static_cast<I &&>(i), N);
		}
	template <typename I>
		std::size_t copy_if(std::size_t out_offset, I &&i)
		{
			return copy_if(out_offset, static_cast<I &&>(i), this->size());
		}
	template <std::size_t N>
		std::size_t copy_if(std::size_t out_offset, const std::array<char, N> &i, std::size_t n = N)
		{
#ifdef DXX_CONSTANT_TRUE
			if (DXX_CONSTANT_TRUE(n > N))
				DXX_ALWAYS_ERROR_FUNCTION("read size exceeds array size");
#endif
			return copy_if(out_offset, i.data(), n);
		}
	template <std::size_t N>
		std::size_t copy_if(std::size_t out_offset, const char (&i)[N])
		{
			static_assert(N <= L + 1, "string too long");
			return copy_if(out_offset, i, N);
		}
	dxx_compiler_attribute_nonnull()
	std::size_t copy_if(std::size_t offset, const char *i, std::size_t N)
	{
		const auto end =
#ifdef DXX_CONSTANT_TRUE
			(N && DXX_CONSTANT_TRUE(!i[N - 1])) ? N - 1 :
#endif
			std::string_view{i, N}.find('\0');
		return _copy_n(offset, i, end == std::string_view::npos ? N : end);
	}
	[[nodiscard]]
	std::size_t copy_out(const std::span<uint8_t> dst) const
	{
		if (dst.empty())
			return 0;
		const auto end = std::string_view{this->data(), L}.find('\0');
		/* Only copy null if string is short. */
		const auto r = end == std::string_view::npos ? L : end + 1;
		if (r > dst.size())
		{
			/* This output does not fit */
			dst[0] = 0;
			return 1;
		}
		std::copy_n(this->data(), r, dst.begin());
		return r;
	}
	operator bool() const = delete;
	operator char *() const = delete;
	operator elements_t &() const = delete;
	operator const elements_t &() const
	{
		return *reinterpret_cast<const elements_t *>(this->data());
	}
	[[nodiscard]]
	bool operator==(const ntstring &r) const
	{
		return static_cast<const array_t &>(*this) == static_cast<const array_t &>(r);
	}
	[[nodiscard]]
	bool operator!=(const ntstring &r) const = default;
	template <std::size_t N>
		ntstring &operator=(const char (&i)[N])
		{
			copy_if(i);
			return *this;
		}
};
