#pragma once

// Copyright 2020 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "../../util.h"

namespace oel::_detail
{
	template< typename Range >
	class OwningView
	{
		using _iter = iterator_t<Range>;

		Range _r;

	public:
		using difference_type = iter_difference_t<_iter>;

		OwningView() = default;
		OwningView(OwningView &&) = default;
		OwningView(const OwningView &) = delete;
		OwningView & operator =(OwningView &&) = default;
		OwningView & operator =(const OwningView &) = delete;

		constexpr explicit OwningView(Range && r) : _r{std::move(r)} {}

		OEL_ALWAYS_INLINE constexpr _iter begin() { return adl_begin(_r); }

		template< typename R = Range >
		OEL_ALWAYS_INLINE constexpr auto  end()
		->	decltype( adl_end(std::declval<R &>()) ) { return adl_end(_r); }

		template< typename R = Range >
		OEL_ALWAYS_INLINE constexpr auto size()
		->	decltype(as_unsigned( _detail::Size(std::declval<R &>()) ))
		{
			return _detail::Size(_r);
		}

		// Might want to be more generic here, don't know if _r.empty exists
		constexpr bool empty() { return _r.empty(); }

		constexpr decltype(auto) operator[](difference_type index)
			OEL_REQUIRES(iter_is_random_access<_iter>)
		{
			return adl_begin(_r)[index];
		}

		constexpr Range &&      base() && noexcept      { return std::move(_r); }
		constexpr const Range & base() const & noexcept { return _r; }
	};
}

#if OEL_STD_RANGES

template< typename R >
inline constexpr bool std::ranges::enable_borrowed_range< oel::_detail::OwningView<R> >
	= std::ranges::enable_borrowed_range< std::remove_cv_t<R> >;

template< typename R >
inline constexpr bool std::ranges::enable_view< oel::_detail::OwningView<R> > = true;

#endif
