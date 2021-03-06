#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "util.h"
#include "auxi/transform_iterator.h"


/** @file
* @brief A view is a lightweight wrapper of a sequence of elements. Views do not mutate or
*	copy the underlying sequence on construction, and have non-owning reference semantics.
*
* These are mostly intended as input for dynarray and the oel::copy functions. They are useful for
* other things however, such as passing around a subrange of a container without expensive copying
*/

namespace oel
{

//! A minimal substitute for boost::iterator_range
template< typename Iterator >
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

template< typename Iterator,
          enable_if< iter_is_random_access<Iterator>::value > = 0
>
iter_difference_t<Iterator> ssize(const iterator_range<Iterator> & r)   { return r.end() - r.begin(); }


//! Wrapper for iterator and size. Similar to gsl::span, less safe, but not just for arrays
template< typename Iterator, bool = iter_is_random_access<Iterator>::value >
class counted_view
{
public:
	using iterator        = Iterator;
	using value_type      = iter_value_t<Iterator>;
	using difference_type = iter_difference_t<Iterator>;
	using size_type       = typename std::make_unsigned<difference_type>::type;

	//! Initialize to empty
	constexpr counted_view() noexcept                      : _size() {}
	constexpr counted_view(Iterator f, difference_type n);
	//! Construct from array or container with matching iterator type
	template< typename SizedRange,
	          enable_if< !std::is_base_of<counted_view, SizedRange>::value > = 0 // avoid being selected for copy
	>
	constexpr counted_view(SizedRange & r)   : _begin(oel::adl_begin(r)), _size(oel::ssize(r)) {}

	constexpr iterator  begin() const   OEL_ALWAYS_INLINE { return _begin; }

	constexpr size_type size() const noexcept   OEL_ALWAYS_INLINE { return _size; }

	constexpr bool      empty() const noexcept   { return 0 == _size; }

	//! Increment begin, decrementing size
	void      drop_front();
	//! Decrement size (and end)
	void      drop_back();

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
	using typename _base::iterator;
	using typename _base::value_type;
	using typename _base::difference_type;
	using typename _base::size_type;
	using reference = typename std::iterator_traits<Iterator>::reference;

	using _base::_base;

	constexpr iterator  end() const   OEL_ALWAYS_INLINE { return this->_begin + this->_size; }

	constexpr reference back() const                 { return this->_begin[this->_size - 1]; }

	constexpr reference operator[](difference_type index) const   OEL_ALWAYS_INLINE { return this->_begin[index]; }

	//! Get raw pointer to underlying array. The function will exist only if `to_pointer_contiguous(begin())` is valid
	template< typename It = Iterator >
	auto data() const noexcept
	->	decltype( to_pointer_contiguous(std::declval<It>()) )  { return to_pointer_contiguous(this->_begin); }
};


//! View creation functions. Trying to mimic subset of C++20 ranges
namespace view
{

//! Create an iterator_range from two iterators, with type deduced from arguments
template< typename Iterator >  inline
iterator_range<Iterator> subrange(Iterator first, Iterator last)  { return {first, last}; }


//! Create a counted_view from iterator and count, with type deduced from first
template< typename Iterator >
constexpr counted_view<Iterator> counted(Iterator first, iter_difference_t<Iterator> count)  { return {first, count}; }


//! Create an iterator_range of std::move_iterator from two iterators
template< typename InputIterator >
iterator_range< std::move_iterator<InputIterator> >
	move(InputIterator first, InputIterator last)   { using MovIt = std::move_iterator<InputIterator>;
	                                                  return {MovIt{first}, MovIt{last}}; }
#include "auxi/view_detail_move.inc"

/** @brief Wrap a range such that the elements can be moved from when passed to a container or algorithm
* @return type `counted_view<std::move_iterator>` if r.size() exists or r is an array,
*	else `iterator_range<std::move_iterator>`
*
* Note that passing an rvalue range should result in a compile error. Use a named variable. */
template< typename InputRange >  inline
auto move(InputRange & r)     { return _detail::Move(r, int{}); }


/** @brief Create a view with transform_iterator from a range with size() member or an array
@code
std::bitset<8> arr[] { 3, 5, 7, 11 };
dynarray<std::string> result;
result.append( view::transform(arr, [](const auto & bs) { return bs.to_string(); }) );
@endcode
* Similar to boost::adaptors::transform, but supports lambdas directly. Also more efficient because
* it stores just one iterator and has no size overhead for empty UnaryFunc. <br>
* Note that passing an rvalue range should result in a compile error. Use a named variable. */
template< typename UnaryFunc, typename SizedRange >
auto transform(SizedRange & r, UnaryFunc f)
->	counted_view< transform_iterator<UnaryFunc, decltype(begin(r))> >
	{
		return { {f, begin(r)}, oel::ssize(r) };
	}
}



////////////////////////////////////////////////////////////////////////////////
//
// Implementation only in rest of the file


template< typename Iterator, bool B >
constexpr counted_view<Iterator, B>::counted_view(Iterator f, difference_type n)
 :	_begin(f), _size(n)
{
#if __cplusplus >= 201402 or defined _MSC_VER
	OEL_ASSERT(n >= 0);
#endif
}

template< typename Iterator, bool B >
void counted_view<Iterator, B>::drop_front()
{
#if OEL_MEM_BOUND_DEBUG_LVL >= 2
	OEL_ASSERT(_size > 0);
#endif
	++_begin; --_size;
}

template< typename Iterator, bool B >
void counted_view<Iterator, B>::drop_back()
{
#if OEL_MEM_BOUND_DEBUG_LVL >= 2
	OEL_ASSERT(_size > 0);
#endif
	--_size;
}

} // namespace oel
