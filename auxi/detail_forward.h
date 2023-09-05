#pragma once

// Copyright 2020 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "core_util.h"


namespace oel::_detail
{
	// Note: false for arrays, they aren't copy/move constructible
	template< typename T, typename SansRef, bool IsConst >
	struct PassByValueIsBetter : bool_constant<
		#if defined _MSC_VER and (_M_IX86 or _M_AMD64)
			// Small objects assumed cheap to move, so that unique_ptr can be passed in register
			(	std::is_trivially_copy_constructible_v<SansRef> or
				(std::is_move_constructible_v<SansRef> and !IsConst) ) // !IsConst implies rvalue due to check in ForwardT
			and sizeof(SansRef) <= 2 * sizeof(int)
		#else
			std::is_trivially_copy_constructible_v<SansRef> and std::is_trivially_destructible_v<SansRef>
			and sizeof(SansRef) <= 2 * sizeof(void *)
		#endif
		> {};

	template< typename T,
		typename SansRef = std::remove_reference_t<T>,
		bool IsConst = std::is_const_v<SansRef>
	>
	using ForwardT =
		std::conditional_t<
			std::conjunction_v<
				// Forwarding a function or mutable lvalue reference by value would break
				bool_constant< !std::is_lvalue_reference_v<T> or IsConst >,
				std::negation< std::is_function<SansRef> >,
				PassByValueIsBetter<T, SansRef, IsConst>
			>,
			std::remove_cv_t<SansRef>,
			T &&
		>;
}