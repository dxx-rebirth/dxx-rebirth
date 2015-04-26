#pragma once

#include <iterator>

template <typename I>
class null_sentinel_iterator : public std::iterator<std::forward_iterator_tag, I>
{
	I *p;
public:
	null_sentinel_iterator() :
		p(nullptr)
	{
	}
	null_sentinel_iterator(I *i) :
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

