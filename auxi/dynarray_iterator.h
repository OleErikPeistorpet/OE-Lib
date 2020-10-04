#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "allocate_with_header.h"


#ifdef _MSC_VER
	#if OEL_MEM_BOUND_DEBUG_LVL
	#pragma detect_mismatch("OEL_MEM_BOUND_DEBUG_LVL", "1or2")
	#else
	#pragma detect_mismatch("OEL_MEM_BOUND_DEBUG_LVL", "0")
	#endif
#endif


namespace oel
{
inline namespace debug
{

/** @brief Checked iterator, for container with contiguous, dynamically allocated memory
*
* Note: a pair of value-initialized iterators count as an empty range (C++14 requirement)  */
template< typename Ptr >
class dynarray_iterator
{
#define OEL_ITER_VALIDATE_DEREF  \
	OEL_ASSERT( _header->id == _allocationId and _detail::HasValidIndex(_pElem, *_header) )

#if OEL_MEM_BOUND_DEBUG_LVL >= 2
	// Test for iterator pair pointing to same container
	#define OEL_ITER_CHECK_COMPATIBLE(other)  OEL_ASSERT(_allocationId == other._allocationId)
#else
	#define OEL_ITER_CHECK_COMPATIBLE(other)
#endif

public:
	using iterator_category = std::random_access_iterator_tag;
#if __cpp_lib_concepts
	using iterator_concept  = std::contiguous_iterator_tag;

#elif _CPPLIB_VER
	using _Unchecked_type = Ptr; // to silence MS checked iterator warnings
#endif

	using difference_type = ptrdiff_t;
	using value_type      = iter_value_t<Ptr>;
	using pointer         = Ptr;
	using reference       = decltype(*Ptr{});

	using const_iterator = dynarray_iterator<const value_type *>;

	operator const_iterator() const noexcept  OEL_ALWAYS_INLINE
	{
		return {_pElem, _header, _allocationId};
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

	dynarray_iterator & operator++() &  OEL_ALWAYS_INLINE
	{	// preincrement
		++_pElem;
		return *this;
	}

	dynarray_iterator operator++(int) &
	{	// postincrement
		auto tmp = *this;
		++_pElem;
		return tmp;
	}

	dynarray_iterator & operator--() &  OEL_ALWAYS_INLINE
	{	// predecrement
		--_pElem;
		return *this;
	}

	dynarray_iterator operator--(int) &
	{	// postdecrement
		auto tmp = *this;
		--_pElem;
		return tmp;
	}

	dynarray_iterator & operator+=(difference_type offset) &  OEL_ALWAYS_INLINE
	{
		_pElem += offset;
		return *this;
	}

	dynarray_iterator & operator-=(difference_type offset) &  OEL_ALWAYS_INLINE
	{
		_pElem -= offset;
		return *this;
	}

	friend dynarray_iterator operator +(difference_type offset, dynarray_iterator it)
	{
		return it += offset;
	}

	friend dynarray_iterator operator +(dynarray_iterator it, difference_type offset)
	{
		return it += offset;
	}

	friend dynarray_iterator operator -(dynarray_iterator it, difference_type offset)
	{
		return it -= offset;
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

	template< typename Ptr1 >
	bool operator==(const dynarray_iterator<Ptr1> & right) const
	{
		OEL_ITER_CHECK_COMPATIBLE(right);
		return _pElem == right._pElem;
	}

	template< typename Ptr1 >
	bool operator!=(const dynarray_iterator<Ptr1> & right) const
	{
		OEL_ITER_CHECK_COMPATIBLE(right);
		return _pElem != right._pElem;
	}

	template< typename Ptr1 >
	bool operator <(const dynarray_iterator<Ptr1> & right) const
	{
		OEL_ITER_CHECK_COMPATIBLE(right);
		return _pElem < right._pElem;
	}

	template< typename Ptr1 >
	bool operator >(const dynarray_iterator<Ptr1> & right) const
	{
		OEL_ITER_CHECK_COMPATIBLE(right);
		return _pElem > right._pElem;
	}

	template< typename Ptr1 >
	bool operator<=(const dynarray_iterator<Ptr1> & right) const
	{
		return !(*this > right);
	}

	template< typename Ptr1 >
	bool operator>=(const dynarray_iterator<Ptr1> & right) const
	{
		return !(*this < right);
	}


	pointer _pElem; //!< Wrapped pointer. Treat the member variables as private!
	//! Pointer to struct storing allocation ID and container size
	const _detail::DebugAllocationHeader * _header;
	std::uintptr_t _allocationId;  //!< Used to check if this iterator has been invalidated by deallocation

#undef OEL_ITER_CHECK_COMPATIBLE
#undef OEL_ITER_VALIDATE_DEREF
};

} // debug

//! To raw pointer (unchecked)
template< typename Ptr >  inline
Ptr to_pointer_contiguous(const dynarray_iterator<Ptr> & it) noexcept  { return it._pElem; }

} // namespace oel

namespace std
{

template< typename Ptr >
struct pointer_traits< oel::dynarray_iterator<Ptr> >
{
    using pointer         = oel::dynarray_iterator<Ptr>;
    using difference_type = typename pointer::difference_type;
    using element_type    = typename std::pointer_traits<Ptr>::element_type;

    static element_type * to_address(const pointer & it) noexcept  { return it._pElem; }
};

}



////////////////////////////////////////////////////////////////////////////////



namespace oel
{
namespace _detail
{
	template< typename Ptr >
	dynarray_iterator<Ptr> MakeDynarrayIter(Ptr const pos, Ptr const begin, const void * parent) noexcept
	{
		if (begin)
		{
			auto const h = OEL_DEBUG_HEADER_OF_C(begin);
			return {pos, h, h->id};
		}
		else
		{	return {pos, &_detail::headerNoAllocation, reinterpret_cast<std::uintptr_t>(parent)};
		}
	}
}

}