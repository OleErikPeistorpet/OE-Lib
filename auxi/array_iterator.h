#pragma once

// Copyright 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "../util.h"


namespace oel
{
inline namespace debug
{

/** @brief Checked iterator, for container with static and contiguous memory
*
* Note: a pair of value-initialized iterators count as an empty range (C++14 requirement)  */
// TODO: constexpr for inplace_growarr?
template< typename Ptr, typename Container >
struct array_iterator
{
	using iterator_category = std::random_access_iterator_tag;
#if __cpp_lib_concepts
	using iterator_concept  = std::contiguous_iterator_tag;
#endif

	using difference_type = ptrdiff_t;
	using value_type      = iter_value_t<Ptr>;
	using pointer         = Ptr;
	using reference       = decltype(*Ptr{});

	using const_iterator = array_iterator<const value_type *, Container>;

	operator const_iterator() const noexcept  OEL_ALWAYS_INLINE
	{
		return {_pElem, _container};
	}

	reference operator*() const
	{
		OEL_ASSERT(_container->derefValid(_pElem));
		return *_pElem;
	}

	pointer operator->() const
	{
		OEL_ASSERT(_container->derefValid(_pElem));
		return _pElem;
	}

	array_iterator & operator++() &  OEL_ALWAYS_INLINE
	{
		++_pElem;
		return *this;
	}

	array_iterator operator++(int) &
	{	// post-increment
		auto tmp = *this;
		++_pElem;
		return tmp;
	}

	array_iterator & operator--() &  OEL_ALWAYS_INLINE
	{
		--_pElem;
		return *this;
	}

	array_iterator operator--(int) &
	{	// post-decrement
		auto tmp = *this;
		--_pElem;
		return tmp;
	}

	array_iterator & operator+=(difference_type offset) &
	{
		_pElem += offset;
		return *this;
	}

	array_iterator & operator-=(difference_type offset) &
	{
		_pElem -= offset;
		return *this;
	}

	[[nodiscard]] friend array_iterator operator +(difference_type offset, array_iterator it)  OEL_ALWAYS_INLINE
	{
		return it += offset;
	}

	[[nodiscard]] friend array_iterator operator +(array_iterator it, difference_type offset)  OEL_ALWAYS_INLINE
	{
		return it += offset;
	}

	[[nodiscard]] friend array_iterator operator -(array_iterator it, difference_type offset)  OEL_ALWAYS_INLINE
	{
		return it -= offset;
	}

	friend difference_type operator -(array_iterator left, array_iterator right)
	{
		return left._pElem - right._pElem;
	}

	reference operator[](difference_type offset) const
	{	// not `*(*this + offset)` to save function calls when inlining is disabled
		auto tmp = *this;
		tmp._pElem += offset;
		return *tmp;
	}

	friend bool operator==(array_iterator left, array_iterator right)
	{
		return left._pElem == right._pElem;
	}

	friend bool operator!=(array_iterator left, array_iterator right)
	{
		return left._pElem != right._pElem;
	}

	friend bool operator <(array_iterator left, array_iterator right)
	{
		return left._pElem < right._pElem;
	}

	friend bool operator >(array_iterator left, array_iterator right)
	{
		return left._pElem > right._pElem;
	}

	friend bool operator<=(array_iterator left, array_iterator right)
	{
		return left._pElem <= right._pElem;
	}

	friend bool operator>=(array_iterator left, array_iterator right)
	{
		return left._pElem >= right._pElem;
	}


	//! Wrapped pointer. Treat the member variables as private!
	pointer _pElem;
	const Container * _container; //!< Parent container

#undef OEL_ITER_CHECK_COMPATIBLE
};

} // namespace debug

//! To raw pointer (unchecked)
template< typename Ptr, typename C >  inline
auto to_pointer_contiguous(const array_iterator<Ptr, C> & it) noexcept
{
	return it._pElem;
}

} // oel

template< typename Ptr, typename C >
struct std::pointer_traits< oel::array_iterator<Ptr, C> >
{
    using pointer         = oel::array_iterator<Ptr, C>;
    using difference_type = typename pointer::difference_type;
    using element_type    = typename std::pointer_traits<Ptr>::element_type;

    static element_type * to_address(pointer it) noexcept  { return it._pElem; }
};



////////////////////////////////////////////////////////////////////////////////



namespace oel::_detail
{
	template< typename Iterator >
#if OEL_MEM_BOUND_DEBUG_LVL
	using ArrayIteratorMaker = Iterator;
#else
	struct ArrayIteratorMaker
	{
		Iterator _pos;

		ArrayIteratorMaker(Iterator pos, const void *) : _pos(pos) {}

		operator Iterator() const { return _pos; }
	};
#endif
}