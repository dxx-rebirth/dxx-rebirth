#pragma once

#include <iterator>

template <typename I>
struct null_sentinel_sentinel
{
};

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
	reference operator*() const
	{
		return *p;
	}
	null_sentinel_iterator &operator++()
	{
		++p;
		return *this;
	}
	null_sentinel_iterator operator++(int)
	{
		auto r = *this;
		++ *this;
		return r;
	}
	constexpr bool operator==(const null_sentinel_iterator &) const = default;
	constexpr bool operator==(null_sentinel_sentinel<I>) const
	{
		return !**this;
	}
private:
	pointer p = nullptr;
};

template <typename I>
class null_sentinel_array
{
	I *b;
public:
	null_sentinel_array(I *i) :
		b(i)
	{
	}
	null_sentinel_iterator<I> begin() const
	{
		return b;
	}
	null_sentinel_sentinel<I> end() const
	{
		return {};
	}
};
