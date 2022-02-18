#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "util.h"
#include "auxi/transform_iterator.h"

//! Users may define (to override __cpp_lib_ranges or to not pay for include)
#ifndef OEL_STD_RANGES
	#if __cpp_lib_ranges < 201911
	#define OEL_STD_RANGES  0
	#else
	#define OEL_STD_RANGES  1
	#endif
#endif
#if OEL_STD_RANGES
#include <ranges>
#endif

/** @file
* @brief A view is a lightweight wrapper of a sequence of elements. Views do not mutate or
*	copy the underlying sequence on construction, and have non-owning reference semantics.
*
* These are mostly intended as input for dynarray and the oel::copy functions. They are useful for
* other things however, such as passing around a subrange of a container without expensive copying
*/

namespace oel
{

//! A minimal substitute for boost::iterator_range and std::ranges::subrange (C++20)
template< typename Iterator, typename Sentinel = Iterator >
class basic_view
{
public:
	basic_view()                        : _begin(), _end() {}
	basic_view(Iterator f, Sentinel l)  : _begin(f), _end(l) {}

	Iterator begin() const   OEL_ALWAYS_INLINE { return _begin; }
	Sentinel end() const     OEL_ALWAYS_INLINE { return _end; }

protected:
	Iterator _begin;
	Sentinel _end;
};


//! Wrapper for iterator and size
template< typename Iterator, bool = iter_is_random_access<Iterator>::value >
class counted_view
{
public:
	using value_type      = iter_value_t<Iterator>;
	using difference_type = iter_difference_t<Iterator>;
	using size_type       = std::make_unsigned_t<difference_type>;

	//! Initialize to empty
	constexpr counted_view() noexcept                      : _size() {}
	constexpr counted_view(Iterator f, difference_type n)  : _begin(f), _size(n)  { OEL_ASSERT(n >= 0); }
	//! Construct from range (lvalue) that knows its size, with matching iterator type
	template< typename SizedRange,
		enable_if< !std::is_base_of<counted_view, SizedRange>::value > = 0 // avoid being selected for copy
	>
	constexpr counted_view(SizedRange & r)   : _begin(oel::adl_begin(r)), _size(oel::ssize(r)) {}

	constexpr Iterator  begin() const   OEL_ALWAYS_INLINE { return _begin; }
	//! Provided only if Iterator is random-access or special case
	template< typename I = Iterator >
	constexpr auto      end() const
	->	decltype( _detail::SentinelAt(std::declval<I>(), difference_type{}) )  { return _detail::SentinelAt(_begin, _size); }

	constexpr size_type size() const noexcept   OEL_ALWAYS_INLINE { return _size; }

	constexpr bool      empty() const noexcept  OEL_ALWAYS_INLINE { return 0 == _size; }

	//! Modify this view to exclude first element
	constexpr void      drop_front()
		{
		#if OEL_MEM_BOUND_DEBUG_LVL >= 2
			OEL_ASSERT(_size > 0);
		#endif
			++_begin; --_size;
		}
	//! Modify this view to exclude last element
	constexpr void      drop_back()
		{
		#if OEL_MEM_BOUND_DEBUG_LVL >= 2
			OEL_ASSERT(_size > 0);
		#endif
			--_size;
		}

protected:
	Iterator       _begin;
	difference_type _size;
};

//! Specialization for random-access Iterator
template< typename Iterator >
class counted_view<Iterator, true> : public counted_view<Iterator, false>
{
	using _base = counted_view<Iterator, false>;

public:
	using typename _base::value_type;
	using typename _base::difference_type;
	using typename _base::size_type;
	using reference = typename std::iterator_traits<Iterator>::reference;

	using _base::_base;

	constexpr reference back() const        { return this->_begin[this->_size - 1]; }

	constexpr reference operator[](difference_type index) const   OEL_ALWAYS_INLINE { return this->_begin[index]; }
};


//! View creation functions. Trying to mimic subset of C++20 ranges
namespace view
{

//! Create a basic_view from two iterators, with type deduced from arguments
template< typename Iterator >  inline
basic_view<Iterator> subrange(Iterator first, Iterator last)  { return {first, last}; }


//! Create a counted_view from iterator and count, with type deduced from first
template< typename Iterator >
constexpr counted_view<Iterator> counted(Iterator first, iter_difference_t<Iterator> count)  { return {first, count}; }


#include "auxi/view_detail.inc"

//! Create a basic_view of std::move_iterator from two iterators
template< typename InputIterator >
basic_view< std::move_iterator<InputIterator> >
	move(InputIterator first, InputIterator last)   { using MovI = std::move_iterator<InputIterator>;
	                                                  return {MovI{first}, MovI{last}}; }
/**
* @brief Wrap a range such that the elements can be moved from when passed to a container or algorithm
* @return Type `counted_view<std::move_iterator>` if r.size() exists or r is an array,
*	else `basic_view<std::move_iterator>`
*
* Note that passing an rvalue range is meant to give a compile error. Use a named variable. */
template< typename InputRange >
auto move(InputRange & r)     { using MovI = std::move_iterator<decltype( begin(r) )>;
                                return _detail::all<MovI>(MovI{begin(r)}, r); }

/**
* @brief Create a view with transform_iterator from a range
@code
std::bitset<8> arr[] { 3, 5, 7, 11 };
dynarray<std::string> result;
result.append( view::transform(arr, [](const auto & bs) { return bs.to_string(); }) );
@endcode
* Similar to boost::adaptors::transform, but supports lambdas directly. Also more efficient
* because it stores just one copy of f and has no size overhead for empty UnaryFunc. <br>
* Note that passing an rvalue range is meant to give a compile error. Use a named variable. */
template< typename UnaryFunc, typename Range >
auto transform(Range & r, UnaryFunc f)
	{
		using It = decltype(begin(r));
		return _detail::all<It>(transform_iterator<UnaryFunc, It>{f, begin(r)}, r);
	}
} // view

}

#if OEL_STD_RANGES

template< typename I, typename S >
inline constexpr bool std::ranges::enable_borrowed_range< oel::basic_view<I, S> > = true;

template< typename I, typename S >
inline constexpr bool std::ranges::enable_view< oel::basic_view<I, S> > = true;

template< typename I, bool R >
inline constexpr bool std::ranges::enable_borrowed_range< oel::counted_view<I, R> > = true;

template< typename I, bool R >
inline constexpr bool std::ranges::enable_view< oel::counted_view<I, R> > = true;

#endif
