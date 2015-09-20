#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "../basic_util.h"


#if OEL_MEM_BOUND_DEBUG_LVL >= 2
	#define OEL_MEM_BOUND_ASSERT ASSERT_ALWAYS_NOEXCEPT
#else
	#define OEL_MEM_BOUND_ASSERT(expr) ((void) 0)
#endif
#if OEL_MEM_BOUND_DEBUG_LVL
	#define OEL_BOUND_ASSERT_CHEAP ASSERT_ALWAYS_NOEXCEPT
#else
	#define OEL_BOUND_ASSERT_CHEAP(expr) ((void) 0)
#endif


namespace oel
{

/** @brief Debug iterator for container with contiguous memory
*
* Wraps a pointer with error checks. The class has significant overhead. */
template<typename Pointer, typename Container>
class cntigus_ctr_dbg_iterator
{
#define OEL_ARRITER_CHECK_DEREFABLE  \
	using sizeT = make_unsigned_t<std::ptrdiff_t>;  \
	OEL_MEM_BOUND_ASSERT( static_cast<sizeT>(_pElem - _myCont->data()) < static_cast<sizeT>(_myCont->size()) )

#if OEL_MEM_BOUND_DEBUG_LVL >= 3
	// test for iterator pair pointing to same container
	#define OEL_ARRITER_CHECK_COMPAT(right)  \
		OEL_MEM_BOUND_ASSERT(_myCont && right._myCont == _myCont)
#else
	#define OEL_ARRITER_CHECK_COMPAT(right)
#endif

public:
	using iterator_category = std::random_access_iterator_tag;

	using value_type      = typename Container::value_type;
	using pointer         = Pointer;
	using reference       = decltype(*std::declval<Pointer>());
	using difference_type = typename Container::difference_type;

	using const_iterator = typename Container::const_iterator;

	cntigus_ctr_dbg_iterator() noexcept  : _myCont(nullptr) {}

	/// Construct with position in data and pointer to container
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
		OEL_MEM_BOUND_ASSERT(_pElem < _myCont->end()._pElem);
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
		OEL_MEM_BOUND_ASSERT(_myCont->data() < _pElem);
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
	{
	#if OEL_MEM_BOUND_DEBUG_LVL >= 3
		// Check that adding offset keeps this in range [begin, end]
		OEL_MEM_BOUND_ASSERT( offset >= _myCont->data() - _pElem
						   && offset <= _myCont->end()._pElem - _pElem );
	#endif
		_pElem += offset;
		return *this;
	}

	cntigus_ctr_dbg_iterator & operator-=(difference_type offset)
	{
	#if OEL_MEM_BOUND_DEBUG_LVL >= 3
		// Check that subtracting offset keeps this in range [begin, end]
		OEL_MEM_BOUND_ASSERT( offset <= _pElem - _myCont->data()
						   && offset >= _pElem - _myCont->end()._pElem );
	#endif
		_pElem -= offset;
		return *this;
	}

	cntigus_ctr_dbg_iterator operator +(difference_type offset) const
	{
		auto tmp = *this;
		return tmp += offset;
	}

	cntigus_ctr_dbg_iterator operator -(difference_type offset) const
	{	// return this - integer
		auto tmp = *this;
		return tmp -= offset;
	}

	template<typename Ptr1>
	difference_type operator -(const cntigus_ctr_dbg_iterator<Ptr1, Container> & right) const
	{	// return difference of iterators
		OEL_ARRITER_CHECK_COMPAT(right);
		return _pElem - right._pElem;
	}

	reference operator[](difference_type offset) const
	{
		return *(operator +(offset));
	}

	template<typename Ptr1>
	bool operator==(const cntigus_ctr_dbg_iterator<Ptr1, Container> & right) const
	{
		return _pElem == right._pElem;
	}

	template<typename Ptr1>
	bool operator!=(const cntigus_ctr_dbg_iterator<Ptr1, Container> & right) const
	{
		return _pElem != right._pElem;
	}

	template<typename Ptr1>
	bool operator <(const cntigus_ctr_dbg_iterator<Ptr1, Container> & right) const
	{
		OEL_ARRITER_CHECK_COMPAT(right);
		return _pElem < right._pElem;
	}

	template<typename Ptr1>
	bool operator >(const cntigus_ctr_dbg_iterator<Ptr1, Container> & right) const
	{
		OEL_ARRITER_CHECK_COMPAT(right);
		return _pElem > right._pElem;
	}

	template<typename Ptr1>
	bool operator<=(const cntigus_ctr_dbg_iterator<Ptr1, Container> & right) const
	{
		return !(*this > right);
	}

	template<typename Ptr1>
	bool operator>=(const cntigus_ctr_dbg_iterator<Ptr1, Container> & right) const
	{
		return !(*this < right);
	}

	/// Return pointer (unchecked)
	friend pointer to_pointer_contiguous(cntigus_ctr_dbg_iterator it) noexcept  { return it._pElem; }

protected:
	pointer           _pElem;  // Wrapped pointer
	const Container * _myCont;

	template<typename, typename>
	friend class cntigus_ctr_dbg_iterator;

#undef OEL_ARRITER_CHECK_COMPAT
#undef OEL_ARRITER_CHECK_DEREFABLE
};

template<typename P, typename C> inline
cntigus_ctr_dbg_iterator<P, C> operator +(typename cntigus_ctr_dbg_iterator<P, C>::difference_type offset,
										  cntigus_ctr_dbg_iterator<P, C> iter)
{
	return iter += offset;
}

} // namespace oel

#if _MSC_VER
	/// Mark cntigus_ctr_dbg_iterator as checked
	template<typename P, typename C>
	struct std::_Is_checked_helper< oel::cntigus_ctr_dbg_iterator<P, C> >
	 :	public std::true_type {};
#endif
