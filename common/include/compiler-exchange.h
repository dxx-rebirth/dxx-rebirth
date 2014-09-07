#pragma once

#include <utility>	// for std::move or for std::exchange

#ifdef DXX_HAVE_CXX14_EXCHANGE
using std::exchange;
#else
template <typename T1, typename T2 = T1>
static inline T1 exchange(T1 &a, T2 &&b)
{
	T1 t = std::move(a);
	a = std::forward<T2>(b);
	return t;
}
#endif
