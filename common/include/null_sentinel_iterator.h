#pragma once

#include <iterator>

template <typename I>
class null_sentinel_iterator
{
public:
	using iterator_category = std::forward_iterator_tag;
	using value_type = I;
	using difference_type = std::ptrdiff_t;
	using pointer = I *;
	using reference = I &;
	null_sentinel_iterator() = default;
	null_sentinel_iterator(const pointer i) :
		p(i)
	{
	}
	pointer get() const
	{
		return p;
	}
	value_type operator*() const
	{
		return *p;
	}
	reference operator*()
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
private:
	pointer p = nullptr;
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
