#pragma once

// Copyright 2021 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "core_util.h"

namespace oel
{
namespace _detail
{
	template< typename T, typename SansRef >
	constexpr bool ShouldPassByValue()
	{
		constexpr auto isConst = std::is_const_v<SansRef>;
		// Forwarding by value: a mutable lvalue reference is wrong, a function won't compile
		if constexpr (std::is_lvalue_reference_v<T> and !isConst)
			return false;
		else if constexpr (std::is_function_v<SansRef>)
			return false;
		// Note: arrays aren't copy/move constructible
	#if defined _MSC_VER and (_M_IX86 or _M_X64)
		else if constexpr (std::is_trivially_copy_constructible_v<SansRef> and sizeof(SansRef) <= 2 * sizeof(void *))
			return true;
		else // Small objects assumed cheap to move, so that unique_ptr can be passed in register
			return std::is_move_constructible_v<SansRef> and !isConst and sizeof(SansRef) <= sizeof(void *);
	#else
		else
			return std::is_trivially_copy_constructible_v<SansRef> and std::is_trivially_destructible_v<SansRef>
			       and sizeof(SansRef) <= 2 * sizeof(void *);
	#endif
	}
}


template
<	typename T,
	typename SansRef = std::remove_reference_t<T>
>
using forward_t =
	std::conditional_t
	<	_detail::ShouldPassByValue<T, SansRef>(),
		std::remove_cv_t<SansRef>,
		T &&
	>;

} // oel
