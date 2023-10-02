#pragma once

// Copyright 2021 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "../util.h"

/** @file
*/

namespace oel::view
{

//! Very similar to std::ranges::owning_view, but base() guards more against silent copies
template< typename Range >
class owning
{
	Range _r;

public:
	static_assert( std::is_move_assignable_v<Range> );

	using difference_type = iter_difference_t< iterator_t<Range> >;

	constexpr explicit owning(Range && r)   : _r{std::move(r)} {}

	owning(owning &&)      = default;
	owning(const owning &) = delete;
	owning & operator =(owning &&)      = default;
	owning & operator =(const owning &) = delete;

	OEL_ALWAYS_INLINE
	constexpr auto begin()   { return adl_begin(_r); }
	OEL_ALWAYS_INLINE
	constexpr auto end()     { return adl_end(_r); }

	template
	<	typename R = Range,
		typename SizeT = decltype( as_unsigned( _detail::Size(std::declval<R &>()) ) )
	>	OEL_ALWAYS_INLINE
	constexpr SizeT size()    { return static_cast<SizeT>(_detail::Size(_r)); }

	constexpr bool  empty()   { return _r.empty(); }

	OEL_ALWAYS_INLINE
	constexpr decltype(auto) operator[](difference_type index)
		OEL_REQUIRES(requires{ _r[index]; })   { return _r[index]; }

	constexpr Range base() &&   { return std::move(_r); }
};

}


template< typename R >
inline constexpr bool oel::enable_view< oel::view::owning<R> > = true;
