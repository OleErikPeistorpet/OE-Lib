#pragma once

// Copyright 2023 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "range_traits.h"


namespace oel
{

#if OEL_STD_RANGES

	using std::ranges::view_interface;
	using std::ranges::enable_view;
#else

	struct _viewBase {};


	template< typename Derived >
	struct view_interface : _viewBase
	{
		template< typename D = Derived >
			OEL_REQUIRES(iter_is_random_access< iterator_t<D> >)
		constexpr decltype(auto) operator[](iter_difference_t< iterator_t<D> > index)
			{
				return static_cast<D &>(*this).begin()[index];
			}

		constexpr explicit operator bool()
			{
				return !static_cast<Derived &>(*this).empty();
			}
	};


	template< typename T >
	inline constexpr bool enable_view = std::is_base_of_v<_viewBase, T>;
#endif

}