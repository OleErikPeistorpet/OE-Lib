#pragma once

// Copyright © 2014 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "basic_util.h"


namespace oetl
{

template<class T>
struct value_type
{
	typedef typename T::value_type the;
};
template<typename T, size_t Size>
struct value_type<T[Size]>
{
	typedef T the;
};


/** @brief Iterator wrapping for pointer to const value
*
* The array iterators have significant overhead, are meant for debugging only  */
template<typename Container>
class array_const_iterator
{
	typedef typename value_type<Container>::the * _nonConstPtrT;

#if OETL_MEM_BOUND_DEBUG_LVL >= 2
	// test for iterator pair pointing to same container
#	define OETL_ARRITER_CHECKCOMPAT(right)  \
		MEM_BOUND_ASSERT(_myCont && right._myCont == _myCont)
#else
#	define OETL_ARRITER_CHECKCOMPAT(right)
#endif

public:
	typedef std::random_access_iterator_tag iterator_category;

	typedef typename value_type<Container>::the value_type;
	typedef std::ptrdiff_t                      difference_type;
	typedef const value_type *                  pointer;
	typedef const value_type &                  reference;

	array_const_iterator()
	 :	_myCont(nullptr)
	{}

	// construct with position in container (and pointer to container for debug)
	array_const_iterator(pointer pos, const Container * container)
	 :	_pElem(const_cast<_nonConstPtrT>(pos)),
		_myCont(container)
	{}

	reference operator*() const
	{	// return reference to element
		return *operator->();
	}

	pointer operator->() const
	{	// return pointer to element
		MEM_BOUND_ASSERT( to_ptr(begin(*_myCont)) <= _pElem && _pElem < to_ptr(end(*_myCont)) );
		return _pElem;
	}

	array_const_iterator & operator++()
	{	// preincrement
#	if OETL_MEM_BOUND_DEBUG_LVL >= 3
		MEM_BOUND_ASSERT(_pElem < to_ptr(end(*_myCont)));
#	endif
		++_pElem;
		return *this;
	}

	array_const_iterator operator++(int)
	{	// postincrement
		auto tmp = *this;
		++(*this);
		return tmp;
	}

	array_const_iterator & operator--()
	{	// predecrement
#	if OETL_MEM_BOUND_DEBUG_LVL >= 3
		MEM_BOUND_ASSERT(to_ptr(begin(*_myCont)) < _pElem);
#	endif
		--_pElem;
		return *this;
	}

	array_const_iterator operator--(int)
	{	// postdecrement
		auto tmp = *this;
		--(*this);
		return tmp;
	}

	array_const_iterator & operator+=(difference_type offset)
	{	// add integer to pointer
#	if OETL_MEM_BOUND_DEBUG_LVL >= 3
		MEM_BOUND_ASSERT( offset >= to_ptr(begin(*_myCont)) - _pElem
					   && offset <= to_ptr(end(*_myCont)) - _pElem );
#	endif
		_pElem += offset;
		return *this;
	}

	array_const_iterator & operator-=(difference_type offset)
	{	// subtract integer from pointer
#	if OETL_MEM_BOUND_DEBUG_LVL >= 3
		MEM_BOUND_ASSERT( offset <= _pElem - to_ptr(begin(*_myCont))
					   && offset >= _pElem - to_ptr(end(*_myCont)) );
#	endif
		_pElem -= offset;
		return *this;
	}

	array_const_iterator operator +(difference_type offset) const
	{	// return this + integer
		return array_const_iterator(*this) += offset;
	}

	array_const_iterator operator -(difference_type offset) const
	{	// return this - integer
		return array_const_iterator(*this) -= offset;
	}

	difference_type operator -(const array_const_iterator & right) const
	{	// return difference of iterators
		OETL_ARRITER_CHECKCOMPAT(right);
		return _pElem - right._pElem;
	}

	reference operator[](difference_type offset) const
	{	// subscript
		return *(operator +(offset));
	}

	bool operator==(const array_const_iterator & right) const
	{
		return _pElem == right._pElem;
	}

	bool operator!=(const array_const_iterator & right) const
	{
		return _pElem != right._pElem;
	}

	bool operator <(const array_const_iterator & right) const
	{
		OETL_ARRITER_CHECKCOMPAT(right);
		return _pElem < right._pElem;
	}

	bool operator >(const array_const_iterator & right) const
	{
		OETL_ARRITER_CHECKCOMPAT(right);
		return _pElem > right._pElem;
	}

	bool operator<=(const array_const_iterator & right) const
	{
		return !(*this > right);
	}

	bool operator>=(const array_const_iterator & right) const
	{
		return !(*this < right);
	}

	friend pointer to_ptr(array_const_iterator it) NOEXCEPT
	{	// return pointer (unchecked)
		return it._pElem;
	}

protected:
	_nonConstPtrT     _pElem;  // wrapped pointer
	const Container * _myCont;

#undef OETL_ARRITER_CHECKCOMPAT
};

template<typename C> inline
array_const_iterator<C> operator +(typename array_const_iterator<C>::difference_type offset,
                                   array_const_iterator<C> iter)
{	// add offset to iterator
	return iter += offset;
}


/// Iterator wrapping for pointer to non-const value
template<typename Container>
class array_iterator
	: public array_const_iterator<Container>
{
	typedef array_const_iterator<Container> _baseT;

public:
	typedef typename _baseT::iterator_category iterator_category;

	typedef typename _baseT::value_type      value_type;
	typedef typename _baseT::difference_type difference_type;
	typedef value_type *                     pointer;
	typedef value_type &                     reference;

	array_iterator() {}

	array_iterator(pointer pos, const Container * container)
	 :	_baseT(pos, container)
	{}

	reference operator*() const
	{	// using base class operator-> for debug checks
		return *const_cast<pointer>(_baseT::operator->());;
	}

	pointer operator->() const
	{	// using base class for debug checks
		return const_cast<pointer>(_baseT::operator->());
	}

	array_iterator & operator++()
	{	// preincrement
		_baseT::operator++();
		return *this;
	}

	array_iterator operator++(int)
	{	// postincrement
		auto tmp = *this;
		_baseT::operator++();
		return tmp;
	}

	array_iterator & operator--()
	{	// predecrement
		_baseT::operator--();
		return *this;
	}

	array_iterator operator--(int)
	{	// postdecrement
		auto tmp = *this;
		_baseT::operator--();
		return tmp;
	}

	array_iterator & operator+=(difference_type offset)
	{
		_baseT::operator+=(offset);
		return *this;
	}

	array_iterator & operator-=(difference_type offset)
	{
		_baseT::operator-=(offset);
		return *this;
	}

	array_iterator operator +(difference_type offset) const
	{
		return array_iterator(*this) += offset;
	}

	array_iterator operator -(difference_type offset) const
	{
		return array_iterator(*this) -= offset;
	}

	difference_type operator -(const _baseT & right) const
	{
		return _baseT::operator -(right);
	}

	reference operator[](difference_type offset) const
	{
		return const_cast<reference>(_baseT::operator[](offset));
	}

	friend pointer to_ptr(array_iterator it) NOEXCEPT
	{	// return pointer (unchecked)
		return it._baseT::_pElem;
	}
};

template<typename C> inline
array_iterator<C> operator +(typename array_iterator<C>::difference_type offset, array_iterator<C> iter)
{
	return iter += offset;
}

} // namespace oetl
