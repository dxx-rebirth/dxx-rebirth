/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once

#include <algorithm>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include "dxxsconf.h"
#include "compiler-array.h"

template <typename T = unsigned>
class base_count_array_t
{
protected:
	typedef T size_type;
	void clear() { m_count = 0; }
	size_type m_count;
public:
	size_type size() const { return m_count; }
	bool empty() const { return !m_count; }
	base_count_array_t() : m_count(0) {}
};

template <typename T, std::size_t S>
class count_array_t : public base_count_array_t<>
{
public:
	typedef array<T, S> array_type;
	typedef typename array_type::value_type value_type;
	typedef typename array_type::iterator iterator;
	typedef typename array_type::const_iterator const_iterator;
	typedef typename array_type::const_reference const_reference;
	static typename array_type::size_type max_size() { return S; }
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
		T *uninitialized = static_cast<T *>(&arrayref()[m_count]);
		new(static_cast<void *>(uninitialized)) T(std::forward<Args>(args)...);
		++ m_count;
	}
	void clear()
	{
		shrink(begin());
	}
	// for std::back_insert_iterator
	void push_back(const T& t)
	{
		emplace_back(t);
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
	const_reference back() const { return (*this)[m_count - 1]; }
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
private:
	union U {
		array_type m_data;
		U() {}
	} u;
	array_type &arrayref() { return u.m_data; }
	const array_type &arrayref() const { return u.m_data; }
};
