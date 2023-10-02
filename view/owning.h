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

//! Same as std::ranges::owning_view, just with less functionality
template< typename Range >
class owning
{
	Range _r;

public:
	using difference_type = iter_difference_t< iterator_t<Range> >;

	owning() = default;
	owning(owning &&) = default;
	owning(const owning &) = delete;
	owning & operator =(owning &&) = default;
	owning & operator =(const owning &) = delete;

	constexpr explicit owning(Range && r)   : _r{std::move(r)} {}

	constexpr auto begin()   { return adl_begin(_r); }

	template< typename R = Range >  OEL_ALWAYS_INLINE
	constexpr auto end()
	->	decltype( adl_end(std::declval<R &>()) )  { return adl_end(_r); }

	template< typename R = Range,
	          typename SizeT = decltype(as_unsigned( _detail::Size(std::declval<R &>()) ))
	>	OEL_ALWAYS_INLINE
	constexpr SizeT size()    { return static_cast<SizeT>(_detail::Size(_r)); }

	constexpr bool  empty()   { return _r.empty(); }

	constexpr decltype(auto) operator[](difference_type index)
		OEL_REQUIRES(requires{ _r[index]; })   { return _r[index]; }

	constexpr Range         base() &&                 { return std::move(_r); }
	constexpr const Range & base() const & noexcept   { return _r; }
	void                    base() const && = delete;
};

}


#if OEL_STD_RANGES

template< typename R >
inline constexpr bool std::ranges::enable_borrowed_range< oel::view::owning<R> >
	= std::ranges::enable_borrowed_range< std::remove_cv_t<R> >;

template< typename R >
inline constexpr bool std::ranges::enable_view< oel::view::owning<R> > = true;

#endif
