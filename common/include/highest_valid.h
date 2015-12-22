#pragma once
#include <cstddef>
#include <iterator>

template <typename T>
struct highest_valid_factory
{
	using I = typename T::result_type;
	struct iterator :
		std::iterator<std::forward_iterator_tag, I>,
		I
	{
		iterator(I &&i) :
			I(static_cast<I &&>(i))
		{
		}
		iterator &operator++()
		{
			I::operator++();
			return *this;
		}
		I operator*() const
		{
			return *this;
		}
	};
	const iterator m_begin, m_end;
	highest_valid_factory(T &factory, const typename T::index_type start) :
		m_begin(factory(start)), m_end(++iterator(factory(factory.highest())))
	{
	}
	iterator begin() const { return m_begin; }
	iterator end() const { return m_end; }
};

template <typename T>
highest_valid_factory<T> highest_valid(T &t, const typename T::index_type start = 0)
{
	return {t, start};
}
