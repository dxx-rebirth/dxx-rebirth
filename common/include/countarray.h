#pragma once

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include "dxxsconf.h"
#include "compiler-array.h"

template <typename T = unsigned>
class base_count_array_t
{
protected:
	typedef T size_type;
	typedef base_count_array_t<size_type> base_type;
	void clear() { m_count = 0; }
	size_type m_count;
public:
	size_type count() const { return m_count; }
	bool empty() const { return !m_count; }
};

template <typename T, std::size_t S>
class count_array_t : public base_count_array_t<>
{
//	typedef base_type::size_type size_type;
public:
	typedef array<T, S> array_type;
	typedef typename array_type::value_type value_type;
	typedef typename array_type::iterator iterator;
	typedef typename array_type::const_iterator const_iterator;
	typedef typename array_type::const_reference const_reference;
	~count_array_t() { clear(); }
	count_array_t &operator=(const count_array_t &rhs)
	{
		if (this != &rhs)
		{
			iterator lb = begin();
			iterator le = end();
			const_iterator rb = rhs.begin();
			const_iterator re = rhs.end();
			for (; lb != le && rb != re; ++lb, ++rb)
				*lb = *rb;
			shrink(lb);
			for (; rb != re; ++rb)
				emplace_back(*rb);
		}
		return *this;
	}
	template <typename... Args>
	void emplace_back(Args&&... args)
	{
		if (m_count >= S)
			throw std::length_error("too many elements");
		new(reinterpret_cast<void *>(&arrayref()[m_count])) T(std::forward<Args>(args)...);
		++ m_count;
	}
	void clear()
	{
		shrink(begin());
	}
	void pop_back()
	{
		shrink(end() - 1);
	}
	iterator begin() { return arrayref().begin(); }
	iterator end() { return arrayref().begin() + m_count; }
	iterator find(const T &t) { return std::find(begin(), end(), t); }
	const_iterator find(const T &t) const { return std::find(begin(), end(), t); }
	const_reference operator[](size_type i) const
	{
		if (i >= m_count)
			throw std::out_of_range("not enough elements");
		return arrayref()[i];
	}
	bool contains(const T &t) const { return find(t) != end(); }
	void erase(iterator i)
	{
		shrink(i);
	}
	void erase(const T &t)
	{
		shrink(std::remove(begin(), end(), t));
	}
	template <typename F>
		void erase_if(F f)
		{
			shrink(std::remove_if(begin(), end(), f));
		}
	void replace(const T &o, const T &n)
	{
		std::replace(begin(), end(), o, n);
	}
	const_iterator begin() const { return arrayref().begin(); }
	const_iterator end() const { return arrayref().begin() + m_count; }
private:
	void destroy(iterator b, iterator e)
	{
		for (; b != e; ++b)
			b->~T();
	}
	void shrink(iterator b)
	{
		destroy(b, end());
		m_count = std::distance(begin(), b);
	}
	array<unsigned char, sizeof(T[S])> m_bytes;
	array_type& arrayref() { return reinterpret_cast<array_type&>(m_bytes); }
	const array_type& arrayref() const { return reinterpret_cast<const array_type&>(m_bytes); }
};
