#pragma once

// Copyright 2021 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "../auxi/view_interface.h"
#include "../util.h"  // for as_unsigned

/** @file
*/

namespace oel::view
{

//! Very similar to std::ranges::owning_view, but base() guards more against silent copies
template< typename Range >
class owning
 :	public view_interface< owning<Range> >
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

	constexpr auto begin()   { return oel::begin_(_r); }

	constexpr auto end()     { return oel::end_(_r); }

	template
	<	typename R = Range,
		typename SizeT = decltype( as_unsigned( _detail::Size(std::declval<R &>()) ) )
	>	OEL_ALWAYS_INLINE
	constexpr SizeT size()    { return static_cast<SizeT>(_detail::Size(_r)); }

	constexpr bool  empty()   { return _r.empty(); }

	constexpr Range base() &&   { return std::move(_r); }
};

}