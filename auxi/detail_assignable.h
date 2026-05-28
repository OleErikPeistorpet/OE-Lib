#pragma once

// Copyright 2020 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "core_util.h"

// Could this be inaccurate for some standard library implementations?
#if __cpp_lib_constexpr_dynamic_alloc < 201907
	#define OEL_HAS_STD_CONSTRUCT_AT  0
#else
	#define OEL_HAS_STD_CONSTRUCT_AT  1
#endif

namespace oel::_detail
{
	template< typename T,
	          bool = std::is_move_assignable_v<T> >
	struct AssignableImpl
	{
		static_assert( std::is_trivially_copy_constructible_v<T> and std::is_trivially_destructible_v<T>,
			"The user-supplied function must be move assignable, or trivially copy constructible and trivially destructible" );

		union Box
		{
			struct {} _none;
			T _val;

			OEL_ALWAYS_INLINE constexpr Box() noexcept : _none{} {}

			constexpr Box(const T & src) noexcept : _val(src) {}

			Box(const Box &) = default;

		#if OEL_HAS_STD_CONSTRUCT_AT
			constexpr
		#endif
			void operator =(const Box & other) & noexcept
			{
		#if OEL_HAS_STD_CONSTRUCT_AT
				std::construct_at(this, other);
		#else
				::new(this) Box(other);
		#endif
			}

			OEL_ALWAYS_INLINE constexpr operator const T &() const { return _val; }
			OEL_ALWAYS_INLINE constexpr operator       T &()       { return _val; }
		};

		using T_7KQw = T;

		struct BoxEmpty : public T
		{
			constexpr BoxEmpty(T_7KQw src) noexcept : T_7KQw(src) {}

			BoxEmpty() = default;
			BoxEmpty(const BoxEmpty &) = default;

			OEL_ALWAYS_INLINE constexpr void operator =(const BoxEmpty &) & noexcept {}
		};

		using Type = std::conditional_t< std::is_empty_v<T>, BoxEmpty, Box >;
	};

	template< typename T >
	struct AssignableImpl<T, true>
	{
		using Type = T;
	};

	template< typename T >
	using MakeAssignable = typename AssignableImpl<T>::Type;
}