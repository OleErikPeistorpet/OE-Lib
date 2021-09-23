#pragma once

// Copyright 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "allocate_with_header.h"
#include "contiguous_iterator_to_ptr.h"


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
	#define OEL_ITER_CHECK_COMPATIBLE(a, b)  OEL_ASSERT((a)._allocationId == (b)._allocationId)
#else
	#define OEL_ITER_CHECK_COMPATIBLE(a, b)
#endif

public:
	using iterator_category = std::random_access_iterator_tag;
#if __cpp_lib_concepts
	using iterator_concept  = std::contiguous_iterator_tag;
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
	{
		++_pElem;
		return *this;
	}

	dynarray_iterator operator++(int) &
	{	// post-increment
		auto tmp = *this;
		++_pElem;
		return tmp;
	}

	dynarray_iterator & operator--() &  OEL_ALWAYS_INLINE
	{
		--_pElem;
		return *this;
	}

	dynarray_iterator operator--(int) &
	{	// post-decrement
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

	[[nodiscard]] friend dynarray_iterator operator +(difference_type offset, dynarray_iterator it)
	{
		it._pElem += offset;
		return it;
	}

	[[nodiscard]] friend dynarray_iterator operator +(dynarray_iterator it, difference_type offset)
	{
		it._pElem += offset;
		return it;
	}

	[[nodiscard]] friend dynarray_iterator operator -(dynarray_iterator it, difference_type offset)
	{
		it._pElem -= offset;
		return it;
	}

	friend difference_type operator -(const dynarray_iterator & left, const dynarray_iterator & right)
	{
		OEL_ITER_CHECK_COMPATIBLE(left, right);
		return left._pElem - right._pElem;
	}

	reference operator[](difference_type offset) const
	{
		return *(*this + offset);
	}

	template< typename Ptr1 >
	bool operator==(const dynarray_iterator<Ptr1> & right) const
	{
		OEL_ITER_CHECK_COMPATIBLE(*this, right);
		return _pElem == right._pElem;
	}

	template< typename Ptr1 >
	bool operator!=(const dynarray_iterator<Ptr1> & right) const
	{
		OEL_ITER_CHECK_COMPATIBLE(*this, right);
		return _pElem != right._pElem;
	}

	template< typename Ptr1 >
	bool operator <(const dynarray_iterator<Ptr1> & right) const
	{
		OEL_ITER_CHECK_COMPATIBLE(*this, right);
		return _pElem < right._pElem;
	}

	template< typename Ptr1 >
	bool operator >(const dynarray_iterator<Ptr1> & right) const
	{
		OEL_ITER_CHECK_COMPATIBLE(*this, right);
		return _pElem > right._pElem;
	}

	template< typename Ptr1 >
	bool operator<=(const dynarray_iterator<Ptr1> & right) const
	{
		return !(right < *this);
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

template< typename Ptr >
struct std::pointer_traits< oel::dynarray_iterator<Ptr> >
{
    using pointer         = oel::dynarray_iterator<Ptr>;
    using difference_type = typename pointer::difference_type;
    using element_type    = typename std::pointer_traits<Ptr>::element_type;

    static element_type * to_address(const pointer & it) noexcept  { return it._pElem; }
};



////////////////////////////////////////////////////////////////////////////////



namespace oel::_detail
{
	template< typename Ptr, typename P2 >
	auto MakeDynarrIter(const DynarrBase<P2> & parent, Ptr const pos) noexcept
	{
	#if OEL_MEM_BOUND_DEBUG_LVL
		if (parent.data)
		{
			auto const h = _detail::DebugHeaderOf(parent.data);
			return dynarray_iterator<Ptr>{pos, h, h->id};
		}
		else
		{	auto id = reinterpret_cast<std::uintptr_t>(&parent);
			return dynarray_iterator<Ptr>{pos, &_detail::headerNoAllocation, id};
		}
	#else
		(void) parent;
		return pos;
	#endif
	}
}