#pragma once
#include <cstddef>
#include <iterator>

template <typename T, typename I = std::size_t>
struct highest_valid_t
{
	struct iterator : std::iterator<std::forward_iterator_tag, I>
	{
		I i;
		iterator(I pos) : i(pos)
		{
		}
		iterator &operator++()
		{
			++ i;
			return *this;
		}
		I operator*() const
		{
			return i;
		}
		bool operator!=(const iterator &rhs) const
		{
			return i != rhs.i;
		}
	};
	T &m_container;
	I m_begin;
	highest_valid_t(T &t, I start) :
		m_container(t), m_begin(start)
	{
	}
	iterator begin() const { return m_begin; }
	iterator end() const { return m_container.highest + 1; }
};

template <typename T>
highest_valid_t<T> highest_valid(T &t, std::size_t start = 0)
{
	return {t, start};
}
