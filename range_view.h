#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "util.h"

#ifndef OEL_NO_BOOST
	#include <boost/iterator/transform_iterator.hpp>
#endif


/** @file
*/

namespace oel
{

//! A minimal substitute for boost::iterator_range
template<typename Iterator>
class iterator_range
{
public:
	iterator_range(Iterator f, Iterator l)  : _begin(f), _end(l) {}

	Iterator begin() const   OEL_ALWAYS_INLINE { return _begin; }
	Iterator end() const     OEL_ALWAYS_INLINE { return _end; }

protected:
	Iterator _begin;
	Iterator _end;
};

template<typename Iterator, enable_if< iterator_is_random_access<Iterator>::value > = 0>
iterator_difference_t<Iterator> ssize(const iterator_range<Iterator> & r)   { return r.end() - r.begin(); }

//! Create an iterator_range from two iterators, with type deduced from arguments
template<typename Iterator> inline
iterator_range<Iterator> make_iterator_range(Iterator first, Iterator last)  { return {first, last}; }


//! Wrapper for iterator and size. Similar to gsl::span, less safe, but not just for arrays
template<typename Iterator, bool = iterator_is_random_access<Iterator>::value>
class counted_view
{
public:
	using iterator        = Iterator;
	using value_type      = typename std::iterator_traits<Iterator>::value_type;
	using difference_type = iterator_difference_t<Iterator>;
#ifndef OEL_VIEW_SIGNED_SIZE
	using size_type       = size_t;
#else
	using size_type       = difference_type;
#endif

	//! Initialize to empty
	constexpr counted_view() noexcept                      : _size() {}
	constexpr counted_view(Iterator f, difference_type n);
	//! Construct from array or container with matching iterator type
	template<typename SizedRange, enable_if<!std::is_same<SizedRange, counted_view>::value> = 0>
	constexpr counted_view(SizedRange & r)   : _begin(::adl_begin(r)), _size(oel::ssize(r)) {}

	constexpr iterator  begin() const   OEL_ALWAYS_INLINE { return _begin; }

	constexpr size_type size() const noexcept    OEL_ALWAYS_INLINE { return _size; }

	constexpr bool      empty() const noexcept   OEL_ALWAYS_INLINE { return 0 == _size; }

	//! Increment begin, decrementing size
	void      drop_front();
	//! Decrement size (and end)
	void      drop_back();

protected:
	Iterator  _begin;
	size_type _size;
};

//! Specialization for random-access Iterator
template<typename Iterator>
class counted_view<Iterator, true> : public counted_view<Iterator, false>
{
	using _base = counted_view<Iterator, false>;

public:
	using typename _base::iterator;
	using typename _base::value_type;
	using typename _base::difference_type;
	using typename _base::size_type;
	using reference = typename std::iterator_traits<Iterator>::reference;

	OEL_ALWAYS_INLINE
	constexpr counted_view() noexcept                      {}
	OEL_ALWAYS_INLINE
	constexpr counted_view(Iterator f, difference_type n)  : _base(f, n) {}
	template<typename SizedRange, enable_if<!std::is_same<SizedRange, counted_view>::value> = 0>  OEL_ALWAYS_INLINE
	constexpr counted_view(SizedRange & r)                 : _base(r) {}

	constexpr iterator  end() const   OEL_ALWAYS_INLINE { return this->_begin + this->_size; }

	constexpr reference back() const   OEL_ALWAYS_INLINE { return end()[-1]; }

	constexpr reference operator[](difference_type index) const   OEL_ALWAYS_INLINE { return this->_begin[index]; }

	//! Return raw pointer to underlying array. Will only be found with contiguous Iterator (see to_pointer_contiguous(T *))
	template<typename It = Iterator>
	auto data() const noexcept
	 -> decltype( to_pointer_contiguous(std::declval<It>()) )  { return to_pointer_contiguous(this->_begin); }
};

//! Make views. View is a concept described in the documentation for the Range v3 library
namespace view
{

//! Create a counted_view from iterator and count, with type deduced from first
template<typename Iterator>
constexpr counted_view<Iterator> counted(Iterator first, iterator_difference_t<Iterator> count)  { return {first, count}; }


//! Create an iterator_range of std::move_iterator from two iterators
template<typename InputIterator>
iterator_range< std::move_iterator<InputIterator> >
	move(InputIterator first, InputIterator last)  { using MovIt = std::move_iterator<InputIterator>;
	                                                 return {MovIt{first}, MovIt{last}}; }

//! Create a counted_view with move_iterator from iterator and count
template<typename InputIterator>
counted_view< std::move_iterator<InputIterator> >
	move_n(InputIterator first, iterator_difference_t<InputIterator> count)
	{
		return {std::make_move_iterator(first), count};
	}

#include "auxi/view_detail_move.inc"

/** @brief Wrap a range such that the elements can be moved from when passed to a container or algorithm
* @return counted_view<std::move_iterator> if r.size() exists or r is an array, else iterator_range<std::move_iterator>  */
template<typename InputRange> inline
auto move(InputRange & r)
 -> decltype( _detail::Move(r, int{}) )
     { return _detail::Move(r, int{}); }


#ifndef OEL_NO_BOOST
	/** @brief Create a view with boost::transform_iterator from a range with size() member or an array
	*
	* Similar to boost::adaptors::transform, but more efficient with typical use.
	* Note that passing an rvalue range should result in a compile error. Use a named variable.  */
	template<typename UnaryFunc, typename SizedRange>
	auto transform(SizedRange & r, UnaryFunc f)
	 ->	counted_view< boost::transform_iterator<UnaryFunc, decltype(begin(r))> >
		{
			return {boost::make_transform_iterator(begin(r), std::move(f)), oel::ssize(r)};
		}
#endif
}



////////////////////////////////////////////////////////////////////////////////
//
// Implementation only in rest of the file


template<typename Iterator, bool B>
constexpr counted_view<Iterator, B>::counted_view(Iterator f, difference_type n)
 :	_begin(f), _size(n)
{
#if __cplusplus >= 201402L || (defined _MSC_VER && _MSC_VER >= 1910)
	OEL_ASSERT(n >= 0);
#endif
}

template<typename Iterator, bool B>
void counted_view<Iterator, B>::drop_front()
{
#if OEL_MEM_BOUND_DEBUG_LVL >= 2
	OEL_ASSERT(_size > 0);
#endif
	++_begin; --_size;
}

template<typename Iterator, bool B>
void counted_view<Iterator, B>::drop_back()
{
#if OEL_MEM_BOUND_DEBUG_LVL >= 2
	OEL_ASSERT(_size > 0);
#endif
	--_size;
}

} // namespace oel
