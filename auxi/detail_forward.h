#pragma once

// Copyright 2020 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "core_util.h"


namespace oel::_detail
{
	// Note: false for arrays, they aren't copy/move constructible
	template< typename SansRef, bool IsMutableRvalue >
	struct PassByValueLikelyFaster
	 :	bool_constant<
		#if defined _MSC_VER and (_M_IX86 or _M_X64)
			// Small objects assumed cheap to move, so that unique_ptr can be passed in register
			(std::is_move_constructible_v<SansRef> and IsMutableRvalue and sizeof(SansRef) <= sizeof(void *))
			or (std::is_trivially_copy_constructible_v<SansRef> and sizeof(SansRef) <= 2 * sizeof(void *))
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
				// Forwarding by value: a mutable lvalue reference is wrong, a function won't compile
				bool_constant< !std::is_lvalue_reference_v<T> or IsConst >,
				std::negation< std::is_function<SansRef> >,
				PassByValueLikelyFaster<SansRef, !IsConst> // !IsConst implies rvalue here
			>,
			std::remove_cv_t<SansRef>,
			T &&
		>;
}