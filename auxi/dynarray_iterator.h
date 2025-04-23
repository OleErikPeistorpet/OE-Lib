#pragma once

// Copyright 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "dynarray_detail.h"
#include "iterator_facade.h"


#ifdef _MSC_VER
	#if OEL_MEM_BOUND_DEBUG_LVL
	#pragma detect_mismatch("OEL_MEM_BOUND_DEBUG_LVL", "1or2")
	#else
	#pragma detect_mismatch("OEL_MEM_BOUND_DEBUG_LVL", "0")
	#endif
#endif


namespace oel::iter
{

template< typename Ptr >
struct _dynarrayChecked
 :	public _iteratorFacade< _dynarrayChecked<Ptr>, ptrdiff_t >
{
#define OEL_ITER_VALIDATE_DEREF  \
	OEL_ASSERT( _header->id == _allocationId and oel::_detail::HasValidIndex(_pElem, *_header) )


	using iterator_category = std::random_access_iterator_tag;
#if __cpp_lib_concepts
	using iterator_concept  = std::contiguous_iterator_tag;
#endif

	using difference_type = ptrdiff_t;
	using value_type      = iter_value_t<Ptr>;
	using pointer         = Ptr;
	using reference       = decltype(*Ptr{});

	using const_iterator = _dynarrayChecked<const value_type *>;

	operator const_iterator() const noexcept  OEL_ALWAYS_INLINE
		{
			return {{}, _pElem, _header, _allocationId};
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

	_dynarrayChecked & operator++() &  OEL_ALWAYS_INLINE
		{
			++_pElem;
			return *this;
		}

	_dynarrayChecked operator++(int) &
		{	// post-increment
			auto tmp = *this;
			++_pElem;
			return tmp;
		}

	_dynarrayChecked & operator--() &  OEL_ALWAYS_INLINE
		{
			--_pElem;
			return *this;
		}

	_dynarrayChecked operator--(int) &
		{	// post-decrement
			auto tmp = *this;
			--_pElem;
			return tmp;
		}

	_dynarrayChecked & operator+=(difference_type offset) &
		{
			_pElem += offset;
			return *this;
		}

	friend difference_type operator -(const _dynarrayChecked & left, const _dynarrayChecked & right)
		{
			return left._pElem - right._pElem;
		}

	friend bool operator!=(const _dynarrayChecked & left, const _dynarrayChecked & right)
		{
			return left._pElem != right._pElem;
		}

	friend bool operator <(const _dynarrayChecked & left, const _dynarrayChecked & right)
		{
			return left._pElem < right._pElem;
		}


	pointer _pElem; //!< Wrapped pointer. Treat the member variables as private!
	//! Pointer to struct storing allocation ID and container size
	const oel::_detail::DebugAllocationHeader * _header;
	std::uintptr_t _allocationId;  //!< Used to check if this iterator has been invalidated by deallocation

#undef OEL_ITER_VALIDATE_DEREF
};

//! To raw pointer (unchecked)
template< typename Ptr >  inline
Ptr as_contiguous_address(const iter::_dynarrayChecked<Ptr> & it) noexcept  { return it._pElem; }

} // oel::iter

template< typename Ptr >
struct std::pointer_traits< oel::iter::_dynarrayChecked<Ptr> >
{
    using pointer         = oel::iter::_dynarrayChecked<Ptr>;
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
			return iter::_dynarrayChecked<Ptr>{{}, pos, h, h->id};
		}
		else
		{	auto id = reinterpret_cast<std::uintptr_t>(&parent);
			return iter::_dynarrayChecked<Ptr>{{}, pos, &_detail::headerNoAllocation, id};
		}
	#else
		(void) parent;
		return pos;
	#endif
	}
}