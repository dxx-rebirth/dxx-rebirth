#pragma once

#include <iterator>

template <typename I>
class null_sentinel_iterator : public std::iterator<std::forward_iterator_tag, I>
{
	I *p = nullptr;
public:
	null_sentinel_iterator() = default;
	null_sentinel_iterator(I *const i) :
		p(i)
	{
	}
	I *get() const
	{
		return p;
	}
	I operator*() const
	{
		return *p;
	}
	I &operator*()
	{
		return *p;
	}
	null_sentinel_iterator &operator++()
	{
		++p;
		return *this;
	}
	bool operator==(null_sentinel_iterator rhs) const
	{
		if (rhs.p)
			return p == rhs.p;
		if (!p)
			/* Should never happen */
			return true;
		return !**this;
	}
	bool operator!=(null_sentinel_iterator rhs) const
	{
		return !(*this == rhs);
	}
};

template <typename I>
class null_sentinel_array
{
	typedef null_sentinel_iterator<I> iterator;
	I *b;
public:
	null_sentinel_array(I *i) :
		b(i)
	{
	}
	iterator begin() const
	{
		return b;
	}
	iterator end() const
	{
		return {};
	}
};

template <typename I>
static inline null_sentinel_array<I> make_null_sentinel_array(I *i)
{
	return i;
}
