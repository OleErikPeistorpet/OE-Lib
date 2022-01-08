#pragma once

// Copyright 2020 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "type_traits.h"

namespace oel
{
namespace _detail
{
	// Note: false for arrays, they aren't copy/move constructible
	template< typename T, typename SansCVRef >
	struct ShouldPassByValue : bool_constant<
		#if defined _MSC_VER and (_M_IX86 or _M_AMD64)
			sizeof(SansCVRef) <= 2 * sizeof(int) and
			(	std::is_trivially_copy_constructible<SansCVRef>::value or
				(std::is_move_constructible<SansCVRef>::value
				 and !std::is_lvalue_reference<T>::value and !std::is_const<T>::value) )
			// Small objects assumed cheap to move, so that unique_ptr can be passed by value
		#else
			sizeof(SansCVRef) <= 2 * sizeof(void *)
			and std::is_trivially_copy_constructible<SansCVRef>::value and std::is_trivially_destructible<SansCVRef>::value
		#endif
		> {};

	template< typename T,
		typename SansCVRef = std::remove_cv_t< std::remove_reference_t<T> >
	>
	using ForwardT =
		std::conditional_t<
			conjunctionV<
				bool_constant< !std::is_function<SansCVRef>::value >,
				ShouldPassByValue<T, SansCVRef>
			>,
			SansCVRef,
			T &&
		>;
}

}