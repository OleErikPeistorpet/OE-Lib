#pragma once

// Copyright 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "detail/std_range.h"
#include "../util.h"

/** @file
*/

namespace oel
{

//! Wrapper for iterator and size
template< typename Iterator, bool = iter_is_random_access<Iterator>::value >
class counted_view
	#if OEL_HAS_STD_RANGES
	:	public std::ranges::view_base
	#endif
{
public:
	using value_type      = iter_value_t<Iterator>;
	using difference_type = iter_difference_t<Iterator>;

	counted_view() = default;
	constexpr counted_view(Iterator f, difference_type n)  : _begin(f), _size(n)  { OEL_ASSERT(n >= 0); }
	//! Construct from lvalue sized_range (C++20 concept) with matching iterator type
	template< typename SizedRange,
		enable_if< !std::is_base_of<counted_view, SizedRange>::value > = 0 // avoid being selected for copy
	>
	constexpr counted_view(SizedRange & r)   : _begin(oel::adl_begin(r)), _size(oel::ssize(r)) {}

	constexpr Iterator  begin() const   OEL_ALWAYS_INLINE { return _begin; }

	constexpr auto      end() const     { return _detail::SentinelAt<Iterator>::call(_begin, _size); }

	constexpr auto      size() const noexcept   OEL_ALWAYS_INLINE { return as_unsigned(_size); }

	constexpr bool      empty() const noexcept  OEL_ALWAYS_INLINE { return 0 == _size; }

	//! Modify this view to exclude last element
	constexpr void      drop_back() noexcept
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

	using _base::_base;

	constexpr decltype(auto) back() const        { return this->_begin[this->_size - 1]; }

	constexpr decltype(auto) operator[](difference_type index) const   OEL_ALWAYS_INLINE { return this->_begin[index]; }
};

namespace view
{

//! Create a counted_view from iterator and count, with type deduced from first
template< typename Iterator >
constexpr counted_view<Iterator> counted(Iterator first, iter_difference_t<Iterator> count)  { return {first, count}; }

}

} // namespace oel

#if OEL_HAS_STD_RANGES
	template< typename I, bool R >
	inline constexpr bool std::ranges::enable_borrowed_range< oel::counted_view<I, R> > = true;
#endif
