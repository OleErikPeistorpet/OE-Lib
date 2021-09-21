#pragma once

// Copyright 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "detail/std_range.h"
#include "../auxi/type_traits.h"

/** @file
*/

namespace oel
{

//! A minimal substitute for boost::iterator_range and std::ranges::subrange (C++20)
template< typename Iterator, typename Sentinel = Iterator >
class basic_view
	#if OEL_HAS_STD_RANGES
	:	public std::ranges::view_base
	#endif
{
public:
	using value_type      = iter_value_t<Iterator>;
	using difference_type = iter_difference_t<Iterator>;

	basic_view() = default;
	constexpr basic_view(Iterator f, Sentinel l)  : _begin(f), _end(l) {}

	constexpr Iterator begin() const   OEL_ALWAYS_INLINE { return _begin; }
	constexpr Sentinel end() const     OEL_ALWAYS_INLINE { return _end; }

	constexpr bool     empty() const   { return _begin == _end; }

	//! Is present only if an Iterator can be subtracted from a Sentinel
	template< typename I = Iterator,
	          enable_if< maybe_sized_sentinel_for<Sentinel, I>::value > = 0
	>
	constexpr auto size() const
	->	decltype( as_unsigned(end() - std::declval<I>()) )  { return _end - _begin); }

	//! Requires that Sentinel is convertible to Iterator and Iterator is bidirectional (else compile error)
	constexpr decltype(auto) back() const   { return *std::prev(static_cast<Iterator>(_end)); }

	//! Decrement end. Requires that Sentinel is bidirectional (else compile error)
	constexpr void           drop_back()
		{
		#if OEL_MEM_BOUND_DEBUG_LVL >= 2
			OEL_ASSERT(_begin != _end);
		#endif
			--_end;
		}

protected:
	Iterator _begin;
	Sentinel _end;
};

//! View creation functions. Trying to mimic subset of C++20 ranges
namespace view
{

//! Create a basic_view from iterator pair, or iterator and sentinel
template< typename Iterator, typename Sentinel >
constexpr basic_view<Iterator, Sentinel> subrange(Iterator first, Sentinel last)  { return {first, last}; }

}

} // oel

#if OEL_HAS_STD_RANGES
	template< typename I, typename S >
	inline constexpr bool std::ranges::enable_borrowed_range< oel::basic_view<I, S> > = true;
#endif
