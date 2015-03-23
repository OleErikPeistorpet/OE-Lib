#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "basic_util.h"


namespace oel
{

/** @brief Debug iterator for container with contiguous memory
*
* Wraps a pointer with error checks. The class has significant overhead. */
template<typename ConstQualValT, typename Container>
class cntigus_ctr_dbg_iterator
{
#define OEL_ARRITER_CHECK_DEREFABLE  \
	MEM_BOUND_ASSERT( static_cast<typename Container::size_type>(_pElem - _myCont->data()) < _myCont->size() )

#if OEL_MEM_BOUND_DEBUG_LVL >= 3
	// test for iterator pair pointing to same container
	#define OEL_ARRITER_CHECK_COMPAT(right)  \
		MEM_BOUND_ASSERT(_myCont && right._myCont == _myCont)
#else
	#define OEL_ARRITER_CHECK_COMPAT(right)
#endif

public:
	using iterator_category = std::random_access_iterator_tag;

	using value_type      = typename Container::value_type;
	using pointer         = ConstQualValT *;
	using reference       = ConstQualValT &;
	using difference_type = std::ptrdiff_t;

	using const_iterator = cntigus_ctr_dbg_iterator<value_type const, Container>;

	cntigus_ctr_dbg_iterator() : _myCont(nullptr) {
	}

	// construct with position in data and pointer to container
	cntigus_ctr_dbg_iterator(pointer pos, const Container * container)
	 :	_pElem(pos), _myCont(container) {
	}

	operator const_iterator() const
	{
		return const_iterator(_pElem, _myCont);
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

	cntigus_ctr_dbg_iterator & operator++()
	{	// preincrement
	#if OEL_MEM_BOUND_DEBUG_LVL >= 3
		MEM_BOUND_ASSERT( _pElem < to_ptr(_myCont->end()) );
	#endif
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
	#if OEL_MEM_BOUND_DEBUG_LVL >= 3
		MEM_BOUND_ASSERT(_myCont->data() < _pElem);
	#endif
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
	#if OEL_MEM_BOUND_DEBUG_LVL >= 3
		MEM_BOUND_ASSERT( offset >= _myCont->data() - _pElem
					   && offset <= to_ptr(_myCont->end()) - _pElem );
	#endif
		_pElem += offset;
		return *this;
	}

	cntigus_ctr_dbg_iterator & operator-=(difference_type offset)
	{	// subtract integer from pointer
	#if OEL_MEM_BOUND_DEBUG_LVL >= 3
		MEM_BOUND_ASSERT( offset <= _pElem - _myCont->data()
					   && offset >= _pElem - to_ptr(_myCont->end()) );
	#endif
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
		OEL_ARRITER_CHECK_COMPAT(right);
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
		OEL_ARRITER_CHECK_COMPAT(right);
		return _pElem < right._pElem;
	}

	template<typename ValT2>
	bool operator >(const cntigus_ctr_dbg_iterator<ValT2, Container> & right) const
	{
		OEL_ARRITER_CHECK_COMPAT(right);
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

#undef OEL_ARRITER_CHECK_COMPAT
#undef OEL_ARRITER_CHECK_DEREFABLE
};

template<typename T, typename C> inline
cntigus_ctr_dbg_iterator<T, C> operator +(typename cntigus_ctr_dbg_iterator<T, C>::difference_type offset,
										  cntigus_ctr_dbg_iterator<T, C> iter)
{	// add offset to iterator
	return iter += offset;
}

} // namespace oel
