#pragma once

// Copyright © 2014 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "core_util.h"


namespace oetl
{

/** @brief Iterator wrapping for pointer with error checking
*
* The class has significant overhead, is meant for debug builds only  */
template<typename ConstQualValT, typename Container>
class cntigus_ctr_dbg_iterator
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

	cntigus_ctr_dbg_iterator() : _myCont(nullptr) {
	}

	// construct with position in data and pointer to container
	cntigus_ctr_dbg_iterator(pointer pos, const Container * container)
	 :	_pElem(pos), _myCont(container) {
	}

	template< typename ValT2, typename = std::enable_if_t<std::is_same<value_type, ValT2>::value> >
	cntigus_ctr_dbg_iterator(const cntigus_ctr_dbg_iterator<ValT2, Container> & other)
	 :	_pElem(other._pElem), _myCont(other._myCont) {
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

	cntigus_ctr_dbg_iterator & operator++()
	{	// preincrement
#	if OETL_MEM_BOUND_DEBUG_LVL >= 3
		MEM_BOUND_ASSERT( _pElem < to_ptr(_myCont->end()) );
#	endif
		++_pElem;
		return *this;
	}

	cntigus_ctr_dbg_iterator operator++(int)
	{	// postincrement
		auto tmp = *this;
		++(*this);
		return tmp;
	}

	cntigus_ctr_dbg_iterator & operator--()
	{	// predecrement
#	if OETL_MEM_BOUND_DEBUG_LVL >= 3
		MEM_BOUND_ASSERT(_myCont->data() < _pElem);
#	endif
		--_pElem;
		return *this;
	}

	cntigus_ctr_dbg_iterator operator--(int)
	{	// postdecrement
		auto tmp = *this;
		--(*this);
		return tmp;
	}

	cntigus_ctr_dbg_iterator & operator+=(difference_type offset)
	{	// add integer to pointer
#	if OETL_MEM_BOUND_DEBUG_LVL >= 3
		MEM_BOUND_ASSERT( offset >= _myCont->data() - _pElem
					   && offset <= to_ptr(_myCont->end()) - _pElem );
#	endif
		_pElem += offset;
		return *this;
	}

	cntigus_ctr_dbg_iterator & operator-=(difference_type offset)
	{	// subtract integer from pointer
#	if OETL_MEM_BOUND_DEBUG_LVL >= 3
		MEM_BOUND_ASSERT( offset <= _pElem - _myCont->data()
					   && offset >= _pElem - to_ptr(_myCont->end()) );
#	endif
		_pElem -= offset;
		return *this;
	}

	cntigus_ctr_dbg_iterator operator +(difference_type offset) const
	{	// return this + integer
		auto tmp = *this;
		return tmp += offset;
	}

	cntigus_ctr_dbg_iterator operator -(difference_type offset) const
	{	// return this - integer
		auto tmp = *this;
		return tmp -= offset;
	}

	template<typename ValT2>
	difference_type operator -(const cntigus_ctr_dbg_iterator<ValT2, Container> & right) const
	{	// return difference of iterators
		OETL_ARRITER_CHECK_COMPAT(right);
		return _pElem - right._pElem;
	}

	reference operator[](difference_type offset) const
	{
		return *(operator +(offset));
	}

	template<typename ValT2>
	bool operator==(const cntigus_ctr_dbg_iterator<ValT2, Container> & right) const
	{
		return _pElem == right._pElem;
	}

	template<typename ValT2>
	bool operator!=(const cntigus_ctr_dbg_iterator<ValT2, Container> & right) const
	{
		return _pElem != right._pElem;
	}

	template<typename ValT2>
	bool operator <(const cntigus_ctr_dbg_iterator<ValT2, Container> & right) const
	{
		OETL_ARRITER_CHECK_COMPAT(right);
		return _pElem < right._pElem;
	}

	template<typename ValT2>
	bool operator >(const cntigus_ctr_dbg_iterator<ValT2, Container> & right) const
	{
		OETL_ARRITER_CHECK_COMPAT(right);
		return _pElem > right._pElem;
	}

	template<typename ValT2>
	bool operator<=(const cntigus_ctr_dbg_iterator<ValT2, Container> & right) const
	{
		return !(*this > right);
	}

	template<typename ValT2>
	bool operator>=(const cntigus_ctr_dbg_iterator<ValT2, Container> & right) const
	{
		return !(*this < right);
	}

	friend pointer to_ptr(cntigus_ctr_dbg_iterator it) NOEXCEPT
	{	// return pointer (unchecked)
		return it._pElem;
	}

protected:
	pointer           _pElem;  // wrapped pointer
	const Container * _myCont;

	template<typename, typename>
	friend class cntigus_ctr_dbg_iterator;

#undef OETL_ARRITER_CHECK_COMPAT
#undef OETL_ARRITER_CHECK_DEREFABLE
};

template<typename T, typename C> inline
cntigus_ctr_dbg_iterator<T, C> operator +(typename cntigus_ctr_dbg_iterator<T, C>::difference_type offset,
										  cntigus_ctr_dbg_iterator<T, C> iter)
{	// add offset to iterator
	return iter += offset;
}

#if OETL_MEM_BOUND_DEBUG_LVL >= 2

template<typename ConstQualValT, typename Container>
using contiguous_iterator = cntigus_ctr_dbg_iterator<ConstQualValT, Container>;

#else

template<typename ConstQualValT, typename>
using contiguous_iterator = ConstQualValT *;

#endif

} // namespace oetl
