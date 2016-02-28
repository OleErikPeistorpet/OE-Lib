#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "../core_util.h"


namespace oel
{

template<typename, typename> class contiguous_ctnr_iterator;

///@{
/// To raw pointer (unchecked)
template<typename Ptr, typename C> inline
typename std::pointer_traits<Ptr>::element_type *
	to_pointer_contiguous(const contiguous_ctnr_iterator<Ptr, C> & it)
{
	return it._pElem.operator->();
}
template<typename T, typename C> inline
T * to_pointer_contiguous(const contiguous_ctnr_iterator<T *, C> & it)
{
	return it._pElem;
}
///@}

/** @brief Debug iterator for container with contiguous memory
*
* Wraps a pointer with error checks. Note: a pair of value-initialized iterators count as an empty range  */
template<typename Ptr, typename Container>
class contiguous_ctnr_iterator
{
#if OEL_MEM_BOUND_DEBUG_LVL >= 2
	// Test for iterator pair pointing to same container
	#define OEL_ARRITER_CHECK_COMPAT(right)  \
		OEL_ASSERT_MEM_BOUND(_container == right._container)
#else
	#define OEL_ARRITER_CHECK_COMPAT(right)
#endif

	using _ptrTrait = std::pointer_traits<Ptr>;

public:
	using iterator_category = std::random_access_iterator_tag;

	using value_type      = typename Container::value_type;
	using pointer         = Ptr;
	using reference       = decltype(*std::declval<Ptr>());
	using difference_type = typename _ptrTrait::difference_type;

	using const_iterator = contiguous_ctnr_iterator<typename _ptrTrait::template rebind<value_type const>, Container>;

	friend typename _ptrTrait::element_type * to_pointer_contiguous<Ptr, Container>(const contiguous_ctnr_iterator &);

	operator const_iterator() const noexcept
	{
		return const_iterator{_pElem, _container};
	}

	reference operator*() const
	{
		OEL_ASSERT_MEM_BOUND(_container->DerefValid(_pElem));
		return *_pElem;
	}

	pointer operator->() const
	{
		OEL_ASSERT_MEM_BOUND(_container->DerefValid(_pElem));
		return _pElem;
	}

	contiguous_ctnr_iterator & operator++()
	{	// preincrement
	#if OEL_MEM_BOUND_DEBUG_LVL >= 2
		OEL_ASSERT_MEM_BOUND(_pElem < _container->End());
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
		OEL_ASSERT_MEM_BOUND(_container->Begin() < _pElem);
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
		OEL_ASSERT_MEM_BOUND( offset >= _container->Begin() - _pElem
						   && offset <= _container->End() - _pElem );
	#endif
		_pElem += offset;
		return *this;
	}

	contiguous_ctnr_iterator & operator-=(difference_type offset)
	{
	#if OEL_MEM_BOUND_DEBUG_LVL >= 2
		// Check that subtracting offset keeps this in range [begin, end]
		OEL_ASSERT_MEM_BOUND( offset <= _pElem - _container->Begin()
						   && offset >= _pElem - _container->End() );
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

	difference_type operator -(const const_iterator & right) const
	{	// return difference of iterators
		OEL_ARRITER_CHECK_COMPAT(right);
		return _pElem - right._pElem;
	}

	reference operator[](difference_type offset) const
	{
		return *contiguous_ctnr_iterator{_pElem + offset, _container};
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


	/// Wrapped pointer. Don't mess with the variables! Consider them private except for initialization
	pointer           _pElem;
	const Container * _container; ///< Parent container

#undef OEL_ARRITER_CHECK_COMPAT
};


namespace _detail
{
	template<typename Iterator>
#if OEL_MEM_BOUND_DEBUG_LVL
	using CtnrIteratorMaker = Iterator;
#else
	struct CtnrIteratorMaker
	{
		Iterator _pos;

		CtnrIteratorMaker(Iterator pos, const void *) : _pos(pos) {}

		operator Iterator() const { return _pos; }
	};
#endif
}

} // namespace oel

#if _MSC_VER
	/// Mark contiguous_ctnr_iterator as checked
	template<typename P, typename C>
	struct std::_Is_checked_helper< oel::contiguous_ctnr_iterator<P, C> >
	 :	public std::true_type {};
#endif
