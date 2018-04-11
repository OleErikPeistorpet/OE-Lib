#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "container_util.h"


#ifdef _MSC_VER
	#if OEL_MEM_BOUND_DEBUG_LVL
	#pragma detect_mismatch("OEL_MEM_BOUND_DEBUG_LVL", "1or2")
	#else
	#pragma detect_mismatch("OEL_MEM_BOUND_DEBUG_LVL", "0")
	#endif
#endif


namespace oel
{

#if OEL_MEM_BOUND_DEBUG_LVL >= 2
inline namespace dl2 OEL_ABI_TAG_NAMESPACE
{
#endif

/** @brief Checked iterator, for container with contiguous memory that can be reallocated
*
* Wraps a pointer with error checks. Note: a pair of value-initialized iterators count as an empty range  */
template<typename Ptr, typename Container>
class dynarray_debug_iterator
{
#if OEL_MEM_BOUND_DEBUG_LVL >= 2
	#define OEL_ITER_VALIDATE_DEREF  \
		OEL_ASSERT(_memInfo->id == _allocationId and _memInfo->container->DerefValid(_pElem))

	// Test for iterator pair pointing to same container
	#define OEL_ITER_CHECK_COMPATIBLE(other)  \
		OEL_ASSERT(_allocationId == other._allocationId)
#else
	#define OEL_ITER_VALIDATE_DEREF  \
		OEL_ASSERT( _memInfo->id == reinterpret_cast<std::uintptr_t>(_memInfo)  \
		        and _memInfo->container->DerefValid(_pElem) )

	#define OEL_ITER_CHECK_COMPATIBLE(other)
#endif

	using _ptrTrait = std::pointer_traits<Ptr>;
	using _ctnrConstPtr = typename _ptrTrait::template rebind<Container const>;

public:
	using iterator_category = std::random_access_iterator_tag;

	using value_type      = typename Container::value_type;
	using pointer         = Ptr;
	using reference       = decltype(*std::declval<Ptr>());
	using difference_type = typename _ptrTrait::difference_type;

	using const_iterator = dynarray_debug_iterator<typename _ptrTrait::template rebind<value_type const>, Container>;

	operator const_iterator() const noexcept  OEL_ALWAYS_INLINE
	{
		return {_pElem, _memInfo
			#if OEL_MEM_BOUND_DEBUG_LVL >= 2
				, _allocationId
			#endif
			};
	}

	reference operator*() const
	{
		OEL_ITER_VALIDATE_DEREF;
		return *_pElem;
	}

	pointer operator->() const
	{
		OEL_ITER_VALIDATE_DEREF;
		return _pElem;
	}

	dynarray_debug_iterator & operator++() &  OEL_ALWAYS_INLINE
	{	// preincrement
		++_pElem;
		return *this;
	}

	dynarray_debug_iterator operator++(int) &
	{	// postincrement
		auto tmp = *this;
		++_pElem;
		return tmp;
	}

	dynarray_debug_iterator & operator--() &  OEL_ALWAYS_INLINE
	{	// predecrement
		--_pElem;
		return *this;
	}

	dynarray_debug_iterator operator--(int) &
	{	// postdecrement
		auto tmp = *this;
		--_pElem;
		return tmp;
	}

	dynarray_debug_iterator & operator+=(difference_type offset) &  OEL_ALWAYS_INLINE
	{
		_pElem += offset;
		return *this;
	}

	dynarray_debug_iterator & operator-=(difference_type offset) &  OEL_ALWAYS_INLINE
	{
		_pElem -= offset;
		return *this;
	}

	friend dynarray_debug_iterator operator +(difference_type offset, dynarray_debug_iterator it)
	{
		return it += offset;
	}

	dynarray_debug_iterator operator +(difference_type offset) const
	{
		auto tmp = *this;
		return tmp += offset;
	}

	dynarray_debug_iterator operator -(difference_type offset) const
	{	// this - integer
		auto tmp = *this;
		return tmp -= offset;
	}

	difference_type operator -(const const_iterator & right) const
	{	// difference of iterators
		OEL_ITER_CHECK_COMPATIBLE(right);
		return _pElem - right._pElem;
	}

	reference operator[](difference_type offset) const
	{
		auto tmp = *this;
		tmp += offset; // not operator + to save a call in non-optimized builds
		return *tmp;
	}

	template<typename Ptr1>
	bool operator==(const dynarray_debug_iterator<Ptr1, Container> & right) const
	{
		OEL_ITER_CHECK_COMPATIBLE(right);
		return _pElem == right._pElem;
	}

	template<typename Ptr1>
	bool operator!=(const dynarray_debug_iterator<Ptr1, Container> & right) const
	{
		OEL_ITER_CHECK_COMPATIBLE(right);
		return _pElem != right._pElem;
	}

	template<typename Ptr1>
	bool operator <(const dynarray_debug_iterator<Ptr1, Container> & right) const
	{
		OEL_ITER_CHECK_COMPATIBLE(right);
		return _pElem < right._pElem;
	}

	template<typename Ptr1>
	bool operator >(const dynarray_debug_iterator<Ptr1, Container> & right) const
	{
		OEL_ITER_CHECK_COMPATIBLE(right);
		return _pElem > right._pElem;
	}

	template<typename Ptr1>
	bool operator<=(const dynarray_debug_iterator<Ptr1, Container> & right) const
	{
		return !(*this > right);
	}

	template<typename Ptr1>
	bool operator>=(const dynarray_debug_iterator<Ptr1, Container> & right) const
	{
		return !(*this < right);
	}


	pointer _pElem; //!< Wrapped pointer. Don't mess with the variables! Consider them private except for initialization
	//! Pointer to struct with pointer to parent container and allocation ID
	const _detail::DebugAllocationHeader<_ctnrConstPtr> * _memInfo;
#if OEL_MEM_BOUND_DEBUG_LVL >= 2
	std::uintptr_t _allocationId; //!< Used to check if this iterator has been invalidated by deallocation
#endif

#undef OEL_ITER_CHECK_COMPATIBLE
#undef OEL_ITER_VALIDATE_DEREF
};

#if OEL_MEM_BOUND_DEBUG_LVL >= 2
} // namespace dl2
#endif

//! To raw pointer (unchecked)
template<typename Ptr, typename C>  OEL_ALWAYS_INLINE inline
typename std::pointer_traits<Ptr>::element_type *
	to_pointer_contiguous(const dynarray_debug_iterator<Ptr, C> & it) noexcept
{
	return _detail::ToAddress(it._pElem);
}

} // namespace oel


#ifdef _MSC_VER
	//! Mark dynarray_debug_iterator as checked
	template<typename P, typename C>
	struct std::_Is_checked_helper< oel::dynarray_debug_iterator<P, C> >
	 :	public std::true_type {};
#endif
