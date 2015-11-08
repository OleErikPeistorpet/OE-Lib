#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "../basic_util.h"


namespace oel
{

/** @brief Debug iterator for container with contiguous memory
*
* Wraps a pointer with error checks. The class has significant overhead. */
template<typename Pointer, typename Container>
class contiguous_ctnr_iterator
{
#if OEL_MEM_BOUND_DEBUG_LVL >= 2
	// Test for iterator pair pointing to same container
	#define OEL_ARRITER_CHECK_COMPAT(right)  \
		OEL_ASSERT_MEM_BOUND(_container && right._container == _container)
#else
	#define OEL_ARRITER_CHECK_COMPAT(right)
#endif

public:
	using iterator_category = std::random_access_iterator_tag;

	using value_type      = typename Container::value_type;
	using pointer         = Pointer;
	using reference       = decltype(*std::declval<Pointer>());
	using difference_type = typename Container::difference_type;

	using const_iterator = typename Container::const_iterator;

	contiguous_ctnr_iterator() noexcept : _container(nullptr) {}

	/// Construct with position in data and pointer to container
	contiguous_ctnr_iterator(pointer pos, const Container * container)
	 :	_pElem(pos), _container(container) {
	}

	operator const_iterator() const
	{
		return const_iterator(_pElem, _container);
	}

	reference operator*() const
	{
		OEL_ASSERT_MEM_BOUND(OEL_ARRITER_DEREF_VALID(_container, _pElem));
		return *_pElem;
	}

	pointer operator->() const
	{
		OEL_ASSERT_MEM_BOUND(OEL_ARRITER_DEREF_VALID(_container, _pElem));
		return _pElem;
	}

	contiguous_ctnr_iterator & operator++()
	{	// preincrement
	#if OEL_MEM_BOUND_DEBUG_LVL >= 2
		OEL_ASSERT_MEM_BOUND(_pElem < _container->end()._pElem);
	#endif
		++_pElem;
		return *this;
	}

	contiguous_ctnr_iterator operator++(int)
	{	// postincrement
		auto tmp = *this;
		++(*this);
		return tmp;
	}

	contiguous_ctnr_iterator & operator--()
	{	// predecrement
	#if OEL_MEM_BOUND_DEBUG_LVL >= 2
		OEL_ASSERT_MEM_BOUND(_container->data() < _pElem);
	#endif
		--_pElem;
		return *this;
	}

	contiguous_ctnr_iterator operator--(int)
	{	// postdecrement
		auto tmp = *this;
		--(*this);
		return tmp;
	}

	contiguous_ctnr_iterator & operator+=(difference_type offset)
	{
	#if OEL_MEM_BOUND_DEBUG_LVL >= 2
		// Check that adding offset keeps this in range [begin, end]
		OEL_ASSERT_MEM_BOUND( offset >= _container->data() - _pElem
						   && offset <= _container->end()._pElem - _pElem );
	#endif
		_pElem += offset;
		return *this;
	}

	contiguous_ctnr_iterator & operator-=(difference_type offset)
	{
	#if OEL_MEM_BOUND_DEBUG_LVL >= 2
		// Check that subtracting offset keeps this in range [begin, end]
		OEL_ASSERT_MEM_BOUND( offset <= _pElem - _container->data()
						   && offset >= _pElem - _container->end()._pElem );
	#endif
		_pElem -= offset;
		return *this;
	}

	friend contiguous_ctnr_iterator operator +(difference_type offset, contiguous_ctnr_iterator it)
	{
		return it += offset;
	}

	contiguous_ctnr_iterator operator +(difference_type offset) const
	{
		auto tmp = *this;
		return tmp += offset;
	}

	contiguous_ctnr_iterator operator -(difference_type offset) const
	{	// return this - integer
		auto tmp = *this;
		return tmp -= offset;
	}

	template<typename Ptr1>
	difference_type operator -(const contiguous_ctnr_iterator<Ptr1, Container> & right) const
	{	// return difference of iterators
		OEL_ARRITER_CHECK_COMPAT(right);
		return _pElem - right._pElem;
	}

	reference operator[](difference_type offset) const
	{
		return *(operator +(offset));
	}

	template<typename Ptr1>
	bool operator==(const contiguous_ctnr_iterator<Ptr1, Container> & right) const
	{
		OEL_ARRITER_CHECK_COMPAT(right);
		return _pElem == right._pElem;
	}

	template<typename Ptr1>
	bool operator!=(const contiguous_ctnr_iterator<Ptr1, Container> & right) const
	{
		OEL_ARRITER_CHECK_COMPAT(right);
		return _pElem != right._pElem;
	}

	template<typename Ptr1>
	bool operator <(const contiguous_ctnr_iterator<Ptr1, Container> & right) const
	{
		OEL_ARRITER_CHECK_COMPAT(right);
		return _pElem < right._pElem;
	}

	template<typename Ptr1>
	bool operator >(const contiguous_ctnr_iterator<Ptr1, Container> & right) const
	{
		OEL_ARRITER_CHECK_COMPAT(right);
		return _pElem > right._pElem;
	}

	template<typename Ptr1>
	bool operator<=(const contiguous_ctnr_iterator<Ptr1, Container> & right) const
	{
		return !(*this > right);
	}

	template<typename Ptr1>
	bool operator>=(const contiguous_ctnr_iterator<Ptr1, Container> & right) const
	{
		return !(*this < right);
	}

	/// Return pointer (unchecked)
	friend typename std::remove_reference<reference>::type *
		to_pointer_contiguous(const contiguous_ctnr_iterator & it) noexcept { return it._pElem; }

protected:
	pointer           _pElem;  // Wrapped pointer
	const Container * _container;

	template<typename, typename>
	friend class contiguous_ctnr_iterator;

#undef OEL_ARRITER_CHECK_COMPAT
#undef OEL_ARRITER_CHECK_DEREFABLE
};

} // namespace oel

#if _MSC_VER
	/// Mark contiguous_ctnr_iterator as checked
	template<typename P, typename C>
	struct std::_Is_checked_helper< oel::contiguous_ctnr_iterator<P, C> >
	 :	public std::true_type {};
#endif
