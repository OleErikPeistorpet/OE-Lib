#pragma once

// Copyright © 2014 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "basic_util.h"


namespace oetl
{

template<typename, typename> class array_debug_iterator;

/** @brief Iterator wrapping for pointer with error checking
*
* The class has significant overhead, is meant for debug builds only  */
template<typename ConstQualValT, typename Container>
class array_debug_iterator
{
#define OETL_ARRITER_CHECK_DEREFABLE  \
	MEM_BOUND_ASSERT( static_cast<typename Container::size_type>(_pElem - _myCont->data()) < _myCont->size() )

#if OETL_MEM_BOUND_DEBUG_LVL >= 3
	// test for iterator pair pointing to same container
#	define OETL_ARRITER_CHECK_COMPAT(right)  \
		MEM_BOUND_ASSERT(_myCont && right._myCont == _myCont)
#else
#	define OETL_ARRITER_CHECK_COMPAT(right)
#endif

public:
	using iterator_category = std::random_access_iterator_tag;

	using value_type = typename Container::value_type;
	using pointer    = ConstQualValT *;
	using reference  = ConstQualValT &;
	using difference_type = std::ptrdiff_t;

	array_debug_iterator() : _myCont(nullptr) {
	}

	// construct with position in data and pointer to container
	array_debug_iterator(pointer pos, const Container * container)
	 :	_pElem(pos), _myCont(container) {
	}

	operator array_debug_iterator<value_type const, Container>() const
	{
		return {_pElem, _myCont};
	}

	reference operator*() const
	{
		OETL_ARRITER_CHECK_DEREFABLE;
		return *_pElem;
	}

	pointer operator->() const
	{
		OETL_ARRITER_CHECK_DEREFABLE;
		return _pElem;
	}

	array_debug_iterator & operator++()
	{	// preincrement
#	if OETL_MEM_BOUND_DEBUG_LVL >= 3
		MEM_BOUND_ASSERT( _pElem < to_ptr(_myCont->end()) );
#	endif
		++_pElem;
		return *this;
	}

	array_debug_iterator operator++(int)
	{	// postincrement
		auto tmp = *this;
		++(*this);
		return tmp;
	}

	array_debug_iterator & operator--()
	{	// predecrement
#	if OETL_MEM_BOUND_DEBUG_LVL >= 3
		MEM_BOUND_ASSERT(_myCont->data() < _pElem);
#	endif
		--_pElem;
		return *this;
	}

	array_debug_iterator operator--(int)
	{	// postdecrement
		auto tmp = *this;
		--(*this);
		return tmp;
	}

	array_debug_iterator & operator+=(difference_type offset)
	{	// add integer to pointer
#	if OETL_MEM_BOUND_DEBUG_LVL >= 3
		MEM_BOUND_ASSERT( offset >= _myCont->data() - _pElem
					   && offset <= to_ptr(_myCont->end()) - _pElem );
#	endif
		_pElem += offset;
		return *this;
	}

	array_debug_iterator & operator-=(difference_type offset)
	{	// subtract integer from pointer
#	if OETL_MEM_BOUND_DEBUG_LVL >= 3
		MEM_BOUND_ASSERT( offset <= _pElem - _myCont->data()
					   && offset >= _pElem - to_ptr(_myCont->end()) );
#	endif
		_pElem -= offset;
		return *this;
	}

	array_debug_iterator operator +(difference_type offset) const
	{	// return this + integer
		auto tmp = *this;
		return tmp += offset;
	}

	array_debug_iterator operator -(difference_type offset) const
	{	// return this - integer
		auto tmp = *this;
		return tmp -= offset;
	}

	template<typename ValT2>
	difference_type operator -(const array_debug_iterator<ValT2, Container> & right) const
	{	// return difference of iterators
		OETL_ARRITER_CHECK_COMPAT(right);
		return _pElem - right._pElem;
	}

	reference operator[](difference_type offset) const
	{
		return *(operator +(offset));
	}

	template<typename ValT2>
	bool operator==(const array_debug_iterator<ValT2, Container> & right) const
	{
#	if OETL_MEM_BOUND_DEBUG_LVL >= 3
		MEM_BOUND_ASSERT(_myCont || right._myCont);
#	endif
		return _pElem == right._pElem;
	}

	template<typename ValT2>
	bool operator!=(const array_debug_iterator<ValT2, Container> & right) const
	{
#	if OETL_MEM_BOUND_DEBUG_LVL >= 3
		MEM_BOUND_ASSERT(_myCont || right._myCont);
#	endif
		return _pElem != right._pElem;
	}

	template<typename ValT2>
	bool operator <(const array_debug_iterator<ValT2, Container> & right) const
	{
		OETL_ARRITER_CHECK_COMPAT(right);
		return _pElem < right._pElem;
	}

	template<typename ValT2>
	bool operator >(const array_debug_iterator<ValT2, Container> & right) const
	{
		OETL_ARRITER_CHECK_COMPAT(right);
		return _pElem > right._pElem;
	}

	template<typename ValT2>
	bool operator<=(const array_debug_iterator<ValT2, Container> & right) const
	{
		return !(*this > right);
	}

	template<typename ValT2>
	bool operator>=(const array_debug_iterator<ValT2, Container> & right) const
	{
		return !(*this < right);
	}

	friend pointer to_ptr(array_debug_iterator it) NOEXCEPT
	{	// return pointer (unchecked)
		return it._pElem;
	}

protected:
	pointer           _pElem;  // wrapped pointer
	const Container * _myCont;

	friend class array_debug_iterator;

#undef OETL_ARRITER_CHECK_COMPAT
#undef OETL_ARRITER_CHECK_DEREFABLE
};

template<typename T, typename C> inline
array_debug_iterator<T, C> operator +(typename array_debug_iterator<T, C>::difference_type offset,
									  array_debug_iterator<T, C> iter)
{	// add offset to iterator
	return iter += offset;
}

#if OETL_MEM_BOUND_DEBUG_LVL >= 2

template<typename Container>
using array_iterator       = array_debug_iterator<typename Container::value_type, Container>;
template<typename Container>
using array_const_iterator = array_debug_iterator< std::add_const_t<typename Container::value_type>, Container >;

#else

template<typename Container>
using array_iterator       = typename Container::value_type *;
template<typename Container>
using array_const_iterator = const typename Container::value_type *;

#endif

} // namespace oetl
