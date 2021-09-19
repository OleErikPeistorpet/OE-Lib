#pragma once

// Copyright 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "../util.h"
#include "detail/core.h"

/** @file
*/

namespace oel
{

/** @brief Wrapper for iterator and size
*
* Satisfies the std::ranges::range concept only if Iterator is random-access. */
template< typename Iterator >
class counted_view
{
public:
	using difference_type = iter_difference_t<Iterator>;

	counted_view() = default;
	constexpr counted_view(Iterator f, difference_type n)   : _begin{std::move(f)}, _size{n} {}
	//! Construct from range (lvalue) that knows its size, with matching iterator type
	template< typename SizedRange,
		enable_if< !std::is_base_of<counted_view, SizedRange>::value > = 0 // avoid being selected for copy
	>
	constexpr counted_view(SizedRange & r)   : _begin(oel::adl_begin(r)), _size(oel::ssize(r)) {}

	constexpr Iterator begin()         { return _detail::MoveIfNotCopyable(_begin); }
	constexpr Iterator begin() const   OEL_ALWAYS_INLINE { return _begin; }
	//! Provided only if Iterator is random-access
	template< typename I = Iterator,
	          enable_if< iter_is_random_access<I> > = 0
	>
	constexpr Iterator end() const   { return _begin + _size; }

	constexpr std::make_unsigned_t<difference_type>
	               size() const noexcept    OEL_ALWAYS_INLINE { return _size; }

	constexpr bool empty() const noexcept   { return 0 == _size; }

	constexpr decltype(auto) operator[](difference_type index) const   OEL_ALWAYS_INLINE
		OEL_REQUIRES(iter_is_random_access<Iterator>)          { return _begin[index]; }

protected:
	Iterator       _begin;
	difference_type _size;
};

namespace view
{

//! Create a counted_view from iterator and count, with type deduced from first
template< typename Iterator >
constexpr counted_view<Iterator> counted(Iterator first, iter_difference_t<Iterator> n)  { return {std::move(first), n}; }

}

} // namespace oel

#if OEL_STD_RANGES

template< typename I >
inline constexpr bool std::ranges::enable_borrowed_range< oel::counted_view<I> > = true;

template< typename I >
inline constexpr bool std::ranges::enable_view< oel::counted_view<I> > = true;

#endif
