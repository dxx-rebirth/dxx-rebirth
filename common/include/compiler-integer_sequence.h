#pragma once
#include <cstddef>
#include "dxxsconf.h"

#ifdef DXX_HAVE_CXX14_INTEGER_SEQUENCE
#include <utility>
using std::index_sequence;
#else
template <std::size_t...>
struct index_sequence {};
#endif

template <typename, std::size_t, typename>
struct cat_index_sequence;

template <std::size_t... N1, std::size_t A2, std::size_t... N2>
struct cat_index_sequence<index_sequence<N1...>, A2, index_sequence<N2...>>
{
	typedef index_sequence<N1..., A2 + N2...> type;
};

/* Fan out to reduce template depth */
template <std::size_t N>
struct tree_index_sequence :
	cat_index_sequence<typename tree_index_sequence<N / 2>::type, N / 2, typename tree_index_sequence<(N + 1) / 2>::type>
{
};

template <>
struct tree_index_sequence<0>
{
	typedef index_sequence<> type;
};

template <>
struct tree_index_sequence<1>
{
	typedef index_sequence<0> type;
};

template <std::size_t N>
using make_tree_index_sequence = typename tree_index_sequence<N>::type;
