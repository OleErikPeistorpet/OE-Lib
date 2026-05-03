#pragma once

// Copyright 2021 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "../util.h" // for as_unsigned

/** @file
*/

namespace oel
{

//! Very similar to std::ranges::owning_view, but base() guards more against silent copies
template< typename Range >
class _owningView
{
	Range _r;

public:
	static_assert( std::is_move_assignable_v<Range> );

	using difference_type = iter_difference_t< iterator_t<Range> >;

	constexpr explicit _owningView(Range && r)   : _r{std::move(r)} {}

	_owningView(_owningView &&)      = default;
	_owningView(const _owningView &) = delete;
	_owningView & operator =(_owningView &&)      = default;
	_owningView & operator =(const _owningView &) = delete;

	constexpr auto begin()   { return oel::begin_(_r); }

	constexpr auto end()     { return oel::end_(_r); }

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

namespace view
{

struct _owningFn
{
	template< typename Range >
	constexpr auto operator()(Range && r) const              { return _owningView{static_cast<Range &&>(r)}; }

	template< typename Range >
	friend constexpr auto operator |(Range && r, _owningFn)  { return _owningView{static_cast<Range &&>(r)}; }
};
//! Note this is a function object that can be chained with `|`
inline constexpr _owningFn owning;

}

} // oel


template< typename R >
inline constexpr bool oel::enable_view< oel::_owningView<R> > = true;
