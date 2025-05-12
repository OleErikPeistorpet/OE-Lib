#pragma once

// Copyright 2017 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "../util.h" // for from_range

#include <stdexcept>


namespace oel::_detail
{
	template< typename T >
	struct AssertTrivialRelocate
	{
		static_assert( is_trivially_relocatable<T>::value,
			"insert, emplace require trivially relocatable T, see declaration of is_trivially_relocatable" );
	};


	struct LengthError
	{
		[[noreturn]] static void raise()
		{
			constexpr auto what = "Going over dynarray max_size";
			OEL_THROW(std::length_error(what), what);
		}
	};


	template< typename Alloc >
	struct ToDynarrPartial
	{
		Alloc _a;

		template< typename Range >
		friend auto operator |(Range && r, ToDynarrPartial t)
		{
			return dynarray(from_range, static_cast<Range &&>(r), std::move(t)._a);
		}
	};
}