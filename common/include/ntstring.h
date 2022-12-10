#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include "dxxsconf.h"
#include "pack.h"
#include <array>

template <std::size_t L>
class ntstring :
	public prohibit_void_ptr<ntstring<L>>,
	public std::array<char, L + 1>
{
public:
	using array_t = std::array<char, L + 1>;
	typedef char elements_t[L + 1];
	using array_t::operator[];
	__attribute_nonnull()
	std::size_t _copy_n(std::size_t offset, const char *ib, std::size_t N)
	{
		if (offset >= this->size())
			return 0;
		auto eb = ib;
		std::size_t r = (offset + N) <= L ? N : 0;
		if (r)
			eb += N;
		auto ob = std::next(this->begin(), offset);
		auto oc = std::copy(ib, eb, ob);
		std::fill(oc, this->end(), 0);
		return r;
	}
	template <std::size_t N>
		__attribute_nonnull()
		std::size_t _copy(std::size_t offset, const char *i)
		{
			static_assert(N <= L, "string too long");
			return _copy_n(offset, i, N);
		}
	template <typename I>
		std::size_t copy_if(I &&i)
		{
			return copy_if(static_cast<std::size_t>(0), static_cast<I &&>(i));
		}
	template <typename I>
		std::size_t copy_if(I &&i, std::size_t N)
		{
			return copy_if(static_cast<std::size_t>(0), static_cast<I &&>(i), N);
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
	__attribute_nonnull()
	std::size_t copy_if(std::size_t offset, const char *i, std::size_t N)
	{
		const std::size_t d =
#ifdef DXX_CONSTANT_TRUE
			(DXX_CONSTANT_TRUE(!i[N - 1])) ? N - 1 :
#endif
			std::distance(i, std::find(i, i + N, 0));
		return _copy_n(offset, i, d);
	}
	[[nodiscard]]
	__attribute_nonnull()
	std::size_t copy_out(const std::span<uint8_t> dst) const
	{
		if (dst.empty())
			return 0;
		const auto ie = std::prev(this->end());
		const auto ib = this->begin();
		auto ii = std::find(ib, ie, 0);
		if (ii != ie)
			/* Only copy null if string is short  */
			++ ii;
		const std::size_t r = std::distance(ib, ii);
		if (r > dst.size())
		{
			/* This output does not fit */
			dst[0] = 0;
			return 1;
		}
		std::copy(ib, ii, dst.begin());
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
