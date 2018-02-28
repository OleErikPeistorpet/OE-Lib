#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "container_util.h"


#ifdef _MSC_VER
	#ifdef OEL_USE_DEBUG_ITER_AFTER_SWAP
	#pragma detect_mismatch("OEL_MEM_BOUND_DEBUG_LVL", "2")
	#else
	#pragma detect_mismatch("OEL_MEM_BOUND_DEBUG_LVL", "0or1")
	#endif
#endif


namespace oel
{
#ifdef OEL_DEBUG_ABI
inline namespace debug
{
#endif

/** @brief Debug iterator for container with contiguous memory
*
* Wraps a pointer with error checks. Note: a pair of value-initialized iterators count as an empty range  */
template<typename Ptr, typename Container>
class contiguous_ctnr_iterator
{
#ifdef OEL_USE_DEBUG_ITER_AFTER_SWAP
	#define OEL_ARRITER_CHECK_DEREFABLE  \
		OEL_ASSERT(_memInfo->id == _allocationId && _memInfo->container->DerefValid(_pElem))
#else
	#define OEL_ARRITER_CHECK_DEREFABLE  \
		OEL_ASSERT(_container->DerefValid(_pElem))
#endif

#if OEL_MEM_BOUND_DEBUG_LVL >= 2
	// Test for iterator pair pointing to same container
	#define OEL_ARRITER_CHECK_COMPAT(right)  \
		OEL_ASSERT(_allocationId == right._allocationId)
#else
	#define OEL_ARRITER_CHECK_COMPAT(right)
#endif

	using _ptrTrait = std::pointer_traits<Ptr>;
	using _ctnrConstPtr = typename _ptrTrait::template rebind<Container const>;

public:
	using iterator_category = std::random_access_iterator_tag;

	using value_type      = typename Container::value_type;
	using pointer         = Ptr;
	using reference       = decltype(*std::declval<Ptr>());
	using difference_type = typename _ptrTrait::difference_type;

	using const_iterator = contiguous_ctnr_iterator<typename _ptrTrait::template rebind<value_type const>, Container>;

	operator const_iterator() const noexcept  OEL_ALWAYS_INLINE
	{
	#ifdef OEL_USE_DEBUG_ITER_AFTER_SWAP
		return {_pElem, _memInfo, _allocationId};
	#else
		return {_pElem, _container};
	#endif
	}

	reference operator*() const
	{
		OEL_ARRITER_CHECK_DEREFABLE;
		return *_pElem;
	}

	pointer operator->() const
	{
		OEL_ARRITER_CHECK_DEREFABLE;
		return _pElem;
	}

	contiguous_ctnr_iterator & operator++()  OEL_ALWAYS_INLINE
	{	// preincrement
		++_pElem;
		return *this;
	}

	contiguous_ctnr_iterator operator++(int)
	{	// postincrement
		auto tmp = *this;
		++_pElem;
		return tmp;
	}

	contiguous_ctnr_iterator & operator--()  OEL_ALWAYS_INLINE
	{	// predecrement
		--_pElem;
		return *this;
	}

	contiguous_ctnr_iterator operator--(int)
	{	// postdecrement
		auto tmp = *this;
		--_pElem;
		return tmp;
	}

	contiguous_ctnr_iterator & operator+=(difference_type offset)  OEL_ALWAYS_INLINE
	{
		_pElem += offset;
		return *this;
	}

	contiguous_ctnr_iterator & operator-=(difference_type offset)  OEL_ALWAYS_INLINE
	{
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
	{	// this - integer
		auto tmp = *this;
		return tmp -= offset;
	}

	difference_type operator -(const const_iterator & right) const
	{	// difference of iterators
		OEL_ARRITER_CHECK_COMPAT(right);
		return _pElem - right._pElem;
	}

	reference operator[](difference_type offset) const
	{
		auto tmp = *this;
		tmp += offset; // not operator + to save a call in non-optimized builds
		return *tmp;
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


	//! Wrapped pointer. Don't mess with the variables! Consider them private except for initialization
	pointer _pElem;
#ifdef OEL_USE_DEBUG_ITER_AFTER_SWAP
	const _detail::DebugAllocationHeader<_ctnrConstPtr> * _memInfo; //!< Pointer to parent container and allocation ID
	std::uintptr_t _allocationId;  //!< Used to check if this iterator has been invalidated by deallocation
#else
	const Container * _container; //!< Parent container
#endif

#undef OEL_ARRITER_CHECK_COMPAT
#undef OEL_ARRITER_CHECK_DEREFABLE
};

//! To raw pointer (unchecked)
template<typename Ptr, typename C>  OEL_ALWAYS_INLINE inline
typename std::pointer_traits<Ptr>::element_type *
	to_pointer_contiguous(const contiguous_ctnr_iterator<Ptr, C> & it) noexcept
{
	return _detail::ToAddress(it._pElem);
}

#ifdef OEL_DEBUG_ABI
	using oel::to_pointer_contiguous;
}
#endif
} // namespace oel


#ifdef _MSC_VER
	//! Mark contiguous_ctnr_iterator as checked
	template<typename P, typename C>
	struct std::_Is_checked_helper< oel::contiguous_ctnr_iterator<P, C> >
	 :	public std::true_type {};
#endif