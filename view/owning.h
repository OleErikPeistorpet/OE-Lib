#pragma once

// Copyright 2021 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "../util.h"

/** @file
*/

namespace oel
{

template< typename Range >
class _owningView
{
	Range _r;

public:
	using difference_type = iter_difference_t< iterator_t<Range> >;

	constexpr explicit _owningView(Range && r)   : _r{std::move(r)} {}

	_owningView() = default;
	_owningView(_owningView &&)      = default;
	_owningView(const _owningView &) = delete;
	_owningView & operator =(_owningView &&)      = default;
	_owningView & operator =(const _owningView &) = delete;

	constexpr auto begin()    OEL_ALWAYS_INLINE { return adl_begin(_r); }

	template< typename R = Range >  OEL_ALWAYS_INLINE
	constexpr auto end()
	->	decltype( adl_end(std::declval<R &>()) )  { return adl_end(_r); }

	template
	<	typename R = Range,
		typename SizeT = decltype( as_unsigned( _detail::Size(std::declval<R &>()) ) )
	>	OEL_ALWAYS_INLINE
	constexpr SizeT size()    { return static_cast<SizeT>(_detail::Size(_r)); }

	constexpr bool  empty()   { return _r.empty(); }

	OEL_ALWAYS_INLINE
	constexpr decltype(auto) operator[](difference_type index)
		OEL_REQUIRES(requires{ _r[index]; })   { return _r[index]; }

	constexpr Range         base() &&                 { return std::move(_r); }
	constexpr const Range & base() const & noexcept   { return _r; }
	void                    base() const && = delete;
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
//! Very similar to std::ranges::owning_view
inline constexpr _owningFn owning;

}

} // oel


#if OEL_STD_RANGES

template< typename R >
inline constexpr bool std::ranges::enable_borrowed_range< oel::_owningView<R> >
	= std::ranges::enable_borrowed_range< std::remove_cv_t<R> >;
#endif

template< typename R >
inline constexpr bool OEL_NS_OF_ENABLE_VIEW::enable_view< oel::_owningView<R> > = true;
