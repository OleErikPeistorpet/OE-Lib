#pragma once

// Copyright 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "util.h"
#include "auxi/counted_iterator.h"
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

//! A minimal substitute for boost::iterator_range and std::ranges::subrange (C++20)
template< typename Iterator, typename Sentinel = Iterator,
          bool = iter_is_random_access<Iterator>::value >
class basic_view  : protected _detail::ViewBase<Iterator>
{
	using _base = _detail::ViewBase<Iterator>;

public:
	using value_type      = iter_value_t<Iterator>;
	using difference_type = iter_difference_t<Iterator>;
	using size_type       = typename std::make_unsigned<difference_type>::type;

	/** @code
	basic_view<int *> a;   // Unspecified value, must assign to before use (default-initialized)
	basic_view<int *> b{}; // Empty (value-initialized)
	@endcode  */
	constexpr basic_view() = default;
	explicit  basic_view(Iterator f)     : _base{std::move(f)}, _end{} {}
	basic_view(Iterator f, Sentinel l)   : _base{std::move(f)}, _end(l) {}
	//! Construct from array or container with matching iterator type
	template< typename Range,
		enable_if< !std::is_base_of<basic_view, Range>::value > = 0 // avoid being selected for copy
	>
	constexpr basic_view(Range & r)   : _base{oel::adl_begin(r)}, _end(oel::adl_end(r)) {}

	using _base::begin;

	constexpr Sentinel end() const   OEL_ALWAYS_INLINE { return _end; }

	constexpr bool     empty() const noexcept   { return this->first == _end; }

	//! Increment begin, decrementing size
	constexpr void     drop_front();
	//! Decrement end and size
	constexpr void     drop_back();

protected:
	Sentinel _end;
};

//! Specialization for random-access Iterator
template< typename Iterator >
class basic_view<Iterator, Iterator, true> : public basic_view<Iterator, Iterator, false>
{
	using _base = basic_view<Iterator, Iterator, false>;

public:
	using typename _base::value_type;
	using typename _base::difference_type;
	using typename _base::size_type;
	using reference = typename std::iterator_traits<Iterator>::reference;

	using _base::_base;

	constexpr size_type size() const noexcept   { return this->_end - this->first; }

	constexpr reference back() const            { return this->_end[-1]; }

	constexpr reference operator[](difference_type index) const   OEL_ALWAYS_INLINE { return this->first[index]; }

	//! Get raw pointer to underlying array. The function will exist only if `to_pointer_contiguous(begin())` is valid
	template< typename It = Iterator >
	auto data() const noexcept
	->	decltype( to_pointer_contiguous(std::declval<It>()) )  { return to_pointer_contiguous(this->first); }
};

template< typename Iterator >
class counted_view : public basic_view< counted_iterator<Iterator>, counted_sentinel >
{
	using _base = basic_view< counted_iterator<Iterator>, counted_sentinel >;

public:
	using typename _base::value_type;
	using typename _base::difference_type;
	using typename _base::size_type;

	constexpr counted_view() = default;
	//! Construct from array or container with matching iterator type
	template< typename Range >
	constexpr counted_view(Range & r)
	 :	basic_view< counted_iterator<Iterator>, counted_sentinel >({oel::adl_begin(r), oel::ssize(r)}, {}) {}
};


//! View creation functions. Trying to mimic subset of C++20 ranges
namespace view
{

//! Create a basic_view from two iterators, with type deduced from arguments
template< typename Iterator, typename Sentinel >
constexpr basic_view<Iterator, Sentinel>  subrange(Iterator first, Sentinel last)  { return {std::move(first), last}; }


//! Create a counted_view from iterator and count, with type deduced from first
template< typename Iterator >
constexpr basic_view< counted_iterator<Iterator>, counted_sentinel >
	counted(Iterator first, iter_difference_t<Iterator> n)   { return{ {std::move(first), n}, {} }; }


//! Create a basic_view of std::move_iterator from two iterators
template< typename InputIterator >
basic_view< std::move_iterator<InputIterator> >
	move(InputIterator first, InputIterator last)   { using MovI = std::move_iterator<InputIterator>;
	                                                  return {MovI{first}, MovI{last}}; }

#include "auxi/view_detail.inc"

/** @brief Wrap a range such that the elements can be moved from when passed to a container or algorithm
* @return type `counted_view<std::move_iterator>` if r.size() exists or r is an array,
*	else `basic_view<std::move_iterator>`
*
* Note that passing an rvalue range should result in a compile error. Use a named variable. */
template< typename InputRange >
auto move(InputRange & r)     { return _detail::Move(r, int{}); }


/** @brief Create a view with transform_iterator from a range
@code
std::bitset<8> arr[] { 3, 5, 7, 11 };
dynarray<std::string> result;
result.append( view::transform(arr, [](const auto & bs) { return bs.to_string(); }) );
@endcode
* Similar to boost::adaptors::transform, but supports lambdas directly. Also more efficient
* because it stores just one copy of f and has no size overhead for empty UnaryFunc. <br>
* Note that passing an rvalue range should result in a compile error. Use a named variable. */
template< typename UnaryFunc, typename Range >
auto transform(Range & r, UnaryFunc f)     { return _detail::Transform(r, std::move(f), int{}); }

}



////////////////////////////////////////////////////////////////////////////////
//
// Implementation only in rest of the file


template< typename Iterator, typename Sentinel, bool B >
constexpr void basic_view<Iterator, Sentinel, B>::drop_front()
{
#if OEL_MEM_BOUND_DEBUG_LVL >= 2
	OEL_ASSERT(this->first != _end);
#endif
	++this->first;
}

template< typename Iterator, typename Sentinel, bool B >
constexpr void basic_view<Iterator, Sentinel, B>::drop_back()
{
#if OEL_MEM_BOUND_DEBUG_LVL >= 2
	OEL_ASSERT(this->first != _end);
#endif
	--_end;
}

} // namespace oel
