#pragma once

template <typename T, typename iterator>
struct reverse_traversal_t
{
	typedef typename T::const_reverse_iterator const_iterator;
	T *container;
	reverse_traversal_t(T *c) : container(c) {}
	iterator begin() const { return container->rbegin(); }
	iterator end() const { return container->rend(); }
};

template <typename T>
static inline reverse_traversal_t<T, typename T::reverse_iterator> reverse_traversal(T &c)
{
	return &c;
}

template <typename T>
static inline reverse_traversal_t<const T, typename T::const_reverse_iterator> reverse_traversal(const T &c)
{
	return &c;
}
