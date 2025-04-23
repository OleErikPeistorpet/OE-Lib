#pragma once

// Copyright 2020 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "core_util.h"


namespace oel::_detail
{
	template< typename T,
	          bool = std::is_move_assignable_v<T> >
	struct AssignableImpl
	{
		static_assert( std::is_trivially_copy_constructible_v<T> and std::is_trivially_destructible_v<T>,
			"The user-supplied function must be move assignable, or trivially copy constructible and trivially destructible" );
		static_assert( !std::is_empty_v<T> );

		union Box
		{
			struct {} _none{};
			T _val;

			constexpr Box(const T & src) noexcept : _val(src) {}

			Box() = default;
			Box(const Box &) = default;

			constexpr void operator =(const Box & other) & noexcept
			{
				std::construct_at(this, other);
			}

			OEL_ALWAYS_INLINE constexpr operator const T &() const { return _val; }
			OEL_ALWAYS_INLINE constexpr operator       T &()       { return _val; }
		};
	};

	template< typename T >
	struct AssignableImpl<T, true>
	{
		using Box = T;
	};

	template< typename T >
	using MakeAssignable = typename AssignableImpl<T>::Box;
}