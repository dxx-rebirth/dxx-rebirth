#pragma once

#include <cstdint>
#include "dxxsconf.h"
#include "compiler-array.h"
#include "pack.h"

template <std::size_t L>
class ntstring :
	public prohibit_void_ptr<ntstring<L>>,
	public array<char, L + 1>
{
public:
	typedef array<char, L + 1> array_t;
	typedef char elements_t[L + 1];
	bool _copy_n(std::size_t offset, const char *ib, std::size_t N)
	{
		auto eb = ib;
		bool r = (offset + N) <= L;
		if (r)
			eb += N;
		auto ob = std::next(this->begin(), offset);
		auto oc = std::copy(ib, eb, ob);
		std::fill(oc, this->end(), 0);
		return r;
	}
	template <std::size_t N>
		bool _copy(const char *i)
		{
			return _copy<N>(0, i);
		}
	template <std::size_t N>
		bool _copy(std::size_t offset, const char *i)
		{
			static_assert(N <= L, "string too long");
			return _copy_n(offset, i, N);
		}
	template <std::size_t N>
		void copy_if(const ntstring<N> &, std::size_t = 0) = delete;
	template <std::size_t N>
		void copy_if(std::size_t, const ntstring<N> &, std::size_t = 0) = delete;
	template <std::size_t N>
		bool copy_if(const array<char, N> &i)
		{
			return copy_if(i.data(), N);
		}
	template <std::size_t N>
		bool copy_if(const char (&i)[N])
		{
#ifdef DXX_HAVE_BUILTIN_CONSTANT_P
			if (__builtin_constant_p(i[N - 1]) && !i[N - 1])
				return _copy<N - 1>(i);
#endif
			return copy_if(i, N);
		}
	bool copy_if(const char *i, std::size_t N)
	{
		return copy_if(0, i, N);
	}
	bool copy_if(std::size_t offset, const char *i, std::size_t N)
	{
		const std::size_t d =
#ifdef DXX_HAVE_BUILTIN_CONSTANT_P
			(__builtin_constant_p(i[N - 1]) && !i[N - 1]) ? N - 1 :
#endif
			std::distance(i, std::find(i, i + N, 0));
		return _copy_n(offset, i, d);
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
};
