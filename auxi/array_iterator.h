#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "../util.h"


namespace oel
{
inline namespace debug
{

/** @brief Checked iterator, for container with contiguous and preferably static memory
*
* Note: a pair of value-initialized iterators count as an empty range (C++14 requirement)  */
template<typename Ptr, typename Container>
class array_iterator
{
#if OEL_MEM_BOUND_DEBUG_LVL >= 2
	// Test for iterator pair pointing to same container
	#define OEL_ITER_CHECK_COMPATIBLE(other)  \
		OEL_ASSERT(_container == other._container)
#else
	#define OEL_ITER_CHECK_COMPATIBLE(other)
#endif

	using _ptrTrait = std::pointer_traits<Ptr>;

public:
	using iterator_category = std::random_access_iterator_tag;

	using value_type      = typename std::remove_const<typename _ptrTrait::element_type>::type;
	using pointer         = Ptr;
	using reference       = decltype(*std::declval<Ptr>());
	using difference_type = typename _ptrTrait::difference_type;

	using const_iterator = array_iterator<typename _ptrTrait::template rebind<value_type const>, Container>;

	operator const_iterator() const noexcept  OEL_ALWAYS_INLINE
	{
		return {_pElem, _container};
	}

	reference operator*() const
	{
		OEL_ASSERT(_container->DerefValid(_pElem));
		return *_pElem;
	}

	pointer operator->() const
	{
		OEL_ASSERT(_container->DerefValid(_pElem));
		return _pElem;
	}

	array_iterator & operator++() &  OEL_ALWAYS_INLINE
	{	// preincrement
		++_pElem;
		return *this;
	}

	array_iterator operator++(int) &
	{	// postincrement
		auto tmp = *this;
		++_pElem;
		return tmp;
	}

	array_iterator & operator--() &  OEL_ALWAYS_INLINE
	{	// predecrement
		--_pElem;
		return *this;
	}

	array_iterator operator--(int) &
	{	// postdecrement
		auto tmp = *this;
		--_pElem;
		return tmp;
	}

	array_iterator & operator+=(difference_type offset) &  OEL_ALWAYS_INLINE
	{
		_pElem += offset;
		return *this;
	}

	array_iterator & operator-=(difference_type offset) &  OEL_ALWAYS_INLINE
	{
		_pElem -= offset;
		return *this;
	}

	friend array_iterator operator +(difference_type offset, array_iterator it)
	{
		return it += offset;
	}

	array_iterator operator +(difference_type offset) const
	{
		auto tmp = *this;
		return tmp += offset;
	}

	array_iterator operator -(difference_type offset) const
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
	bool operator==(const array_iterator<Ptr1, Container> & right) const
	{
		OEL_ITER_CHECK_COMPATIBLE(right);
		return _pElem == right._pElem;
	}

	template<typename Ptr1>
	bool operator!=(const array_iterator<Ptr1, Container> & right) const
	{
		OEL_ITER_CHECK_COMPATIBLE(right);
		return _pElem != right._pElem;
	}

	template<typename Ptr1>
	bool operator <(const array_iterator<Ptr1, Container> & right) const
	{
		OEL_ITER_CHECK_COMPATIBLE(right);
		return _pElem < right._pElem;
	}

	template<typename Ptr1>
	bool operator >(const array_iterator<Ptr1, Container> & right) const
	{
		OEL_ITER_CHECK_COMPATIBLE(right);
		return _pElem > right._pElem;
	}

	template<typename Ptr1>
	bool operator<=(const array_iterator<Ptr1, Container> & right) const
	{
		return !(*this > right);
	}

	template<typename Ptr1>
	bool operator>=(const array_iterator<Ptr1, Container> & right) const
	{
		return !(*this < right);
	}


#ifdef _CPPLIB_VER
	using _Unchecked_type = pointer;
#endif

	//! Wrapped pointer. Don't mess with the variables! Consider them private except for initialization
	pointer _pElem;
	typename _ptrTrait::template rebind<Container const> _container; //!< Parent container

#undef OEL_ITER_CHECK_COMPATIBLE
};

} // namespace debug

//! To raw pointer (unchecked)
template<typename Ptr, typename C>  inline
typename std::pointer_traits<Ptr>::element_type *
	to_pointer_contiguous(const array_iterator<Ptr, C> & it) noexcept
{
	return _detail::ToAddress(it._pElem);
}


////////////////////////////////////////////////////////////////////////////////


namespace _detail
{
	template<typename, typename> struct FcaProxy;


	template<typename Iterator>
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

} // namespace oel
