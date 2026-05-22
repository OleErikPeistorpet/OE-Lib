#pragma once

// Copyright 2021 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "core_util.h"


namespace oel::_detail
{
	template< typename T >
	constexpr bool ShouldPassByValue()
	{	// Note: arrays aren't copy/move constructible
	#if defined _MSC_VER and (_M_IX86 or _M_X64)
		if constexpr( !std::is_trivially_copy_constructible_v<T> )
			return false;
		else
			return sizeof(T) <= sizeof(void *) or std::is_scalar_v<T>;
	#else
		if constexpr( sizeof(T) > 2 * sizeof(void *) )
			return false;
		else
			return std::is_trivially_copy_constructible_v<T> and std::is_trivially_destructible_v<T>;
	#endif
	}

	template< typename T >
	using ConstParam =
		std::conditional_t
		<	_detail::ShouldPassByValue<T>(),
			T const,
			const T &
		>;
}