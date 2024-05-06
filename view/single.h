#pragma once

// Copyright 2022 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "owning.h"
#include "../auxi/detail_in_param.h" // for ShouldPassByRef

#if __cpp_lib_span
#include <span>
#else
#include "counted.h"
#endif

#include <array>

/** @file
*/

namespace oel::view
{

//! Like std::views::single, but may hold a pointer if passed an lvalue
/**
* The returned view is not copyable if it holds the object rather than a pointer to it. */
inline constexpr auto single =
	[](auto && object)
	{
		using Ref = decltype(object);
		using T = std::remove_reference_t<Ref>;
		if constexpr( std::is_lvalue_reference_v<Ref> and _detail::ShouldPassByRef<T>() )
		{
		#if __cpp_lib_span
			return std::span<T, 1>{&object, 1};
		#else
			return counted(&object, 1);
		#endif
		}
		else
		{	using A = std::array< std::remove_cv_t<T>, 1 >;
			return owning( A{static_cast<Ref>(object)} );
		}
	};

}