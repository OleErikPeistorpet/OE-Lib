#pragma once

// Copyright 2023 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "range_traits.h"


namespace oel::_detail
{
	template< bool CanCallConst, typename Func, typename... Iterators >
	constexpr auto TransformIterCat()
	{
		if constexpr (std::is_copy_constructible_v<Func> and CanCallConst)
		{
			if constexpr ((... and iter_is_random_access<Iterators>))
				return std::random_access_iterator_tag{};
			else if constexpr ((... and iter_is_bidirectional<Iterators>))
				return std::bidirectional_iterator_tag{};
			else if constexpr ((... and iter_is_forward<Iterators>))
				return std::forward_iterator_tag{};
			else
				return std::input_iterator_tag{};
		}
		else
		{	return std::input_iterator_tag{};
		}
	}
}