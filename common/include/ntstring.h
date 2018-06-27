#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include "dxxsconf.h"
#include "compiler-array.h"
#include "pack.h"

template <std::size_t L>
class ntstring :
	public prohibit_void_ptr<ntstring<L>>,
	public array<char, L + 1>
{
	static char terminator() { return 0; }
public:
	typedef array<char, L + 1> array_t;
	typedef char elements_t[L + 1];
	using array_t::operator[];
	typename array_t::reference operator[](int i) { return array_t::operator[](i); }
	typename array_t::reference operator[](unsigned i) { return array_t::operator[](i); }
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
		std::fill(oc, this->end(), terminator());
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
		std::size_t copy_if(std::size_t out_offset, const array<char, N> &i, std::size_t n = N)
		{
#ifdef DXX_CONSTANT_TRUE
			if (DXX_CONSTANT_TRUE(n > N))
				DXX_ALWAYS_ERROR_FUNCTION(dxx_trap_overread, "read size exceeds array size");
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
			std::distance(i, std::find(i, i + N, terminator()));
		return _copy_n(offset, i, d);
	}
	__attribute_nonnull()
	std::size_t _copy_out(const std::size_t src_offset, char *const dst_base, const std::size_t dst_offset, const std::size_t max_copy) const
	{
		char *const dst = dst_base + dst_offset;
		if (L > max_copy || src_offset >= this->size())
		{
			/* Some outputs might not fit or nothing to copy */
			*dst = terminator();
			return 1;
		}
		const auto ie = std::prev(this->end());
		const auto ib = std::next(this->begin(), src_offset);
		auto ii = std::find(ib, ie, terminator());
		if (ii != ie)
			/* Only copy null if string is short  */
			++ ii;
		const std::size_t r = std::distance(ib, ii);
		if (r > max_copy)
		{
			/* This output does not fit */
			*dst = terminator();
			return 1;
		}
		std::copy(ib, ii, dst);
		return r;
	}
	__attribute_nonnull()
	std::size_t copy_out(std::size_t src_offset, char *dst, std::size_t dst_offset, std::size_t dst_size) const
	{
		if (dst_size <= dst_offset)
			return 0;
		return _copy_out(src_offset, dst, dst_offset, dst_size - dst_offset);
	}
	__attribute_nonnull()
	std::size_t copy_out(std::size_t src_offset, char *dst, std::size_t dst_size) const
	{
		return copy_out(src_offset, dst, 0, dst_size);
	}
	template <std::size_t N>
		std::size_t copy_out(std::size_t src_offset, array<char, N> &dst, std::size_t dst_offset) const
		{
			return copy_out(src_offset, dst.data(), dst_offset, N);
		}
	template <std::size_t N>
		std::size_t copy_out(std::size_t src_offset, array<uint8_t, N> &dst, std::size_t dst_offset) const
		{
			return copy_out(src_offset, reinterpret_cast<char *>(dst.data()), dst_offset, N);
		}
	operator bool() const = delete;
	operator char *() const = delete;
	operator elements_t &() const = delete;
	operator const elements_t &() const
	{
		return *reinterpret_cast<const elements_t *>(this->data());
	}
	bool operator==(const ntstring &r) const __attribute_warn_unused_result
	{
		return static_cast<const array_t &>(*this) == static_cast<const array_t &>(r);
	}
	bool operator!=(const ntstring &r) const __attribute_warn_unused_result
	{
		return !(*this == r);
	}
	template <std::size_t N>
		ntstring &operator=(const char (&i)[N])
		{
			copy_if(i);
			return *this;
		}
};
