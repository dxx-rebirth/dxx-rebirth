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
#include <type_traits>
#include "dxxsconf.h"
#include <array>

namespace d_count_array {

namespace detail {

template <typename count_type>
class count_storage
{
protected:
	using size_type = count_type;
	size_type m_count{};
public:
	constexpr size_type size() const { return m_count; }
	constexpr bool empty() const { return !m_count; }
};

template <std::size_t size>
using base_count_value = count_storage<typename std::conditional_t<
	(size <= UINT8_MAX), uint8_t,
	typename std::conditional_t<(size <= UINT16_MAX), uint16_t, uint32_t>
>>;

}

}

template <typename T, std::size_t S>
class count_array_t : public d_count_array::detail::base_count_value<S>
{
	using base_count_value = d_count_array::detail::base_count_value<S>;
	using base_count_value::m_count;
public:
	using array_type = std::array<T, S>;
	using value_type = typename array_type::value_type;
	using iterator = typename array_type::iterator;
	using const_iterator = typename array_type::const_iterator;
	using const_reference = typename array_type::const_reference;
	static constexpr typename array_type::size_type max_size() { return {S}; }
	auto data() const { return arrayref().data(); }
	~count_array_t() { clear(); }
	count_array_t &operator=(const count_array_t &rhs)
	{
		if (this != &rhs)
		{
			iterator lb = begin();
			const_iterator rb = rhs.begin();
			const const_iterator re{rhs.end()};
			for (const auto le{end()}; lb != le && rb != re; ++lb, ++rb)
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
	iterator end() { return std::next(begin(), m_count); }
	iterator find(const T &t) { return std::find(begin(), end(), t); }
	const_iterator find(const T &t) const { return std::find(begin(), end(), t); }
	const_reference operator[](const typename base_count_value::size_type i) const
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
		void erase_if(F &&f)
		{
			shrink(std::remove_if(begin(), end(), std::forward<F>(f)));
		}
	void replace(const T &o, const T &n)
	{
		std::replace(begin(), end(), o, n);
	}
	const_iterator begin() const { return arrayref().begin(); }
	const_iterator end() const { return std::next(begin(), m_count); }
private:
	static void destroy(const iterator b, iterator e)
	{
		if constexpr (std::is_trivially_destructible_v<T>)
		{
			(void)b;
			(void)e;
		}
		else
		{
			for (; b != e;)
				(-- e)->~T();
		}
	}
	void shrink(const iterator b)
	{
		destroy(b, end());
		m_count = std::distance(begin(), b);
	}
	union U {
		array_type m_data;
		U() {}
	} u;
	array_type &arrayref() { return u.m_data; }
	const array_type &arrayref() const { return u.m_data; }
};
