#pragma once

// Copyright 2019 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "type_traits.h"


namespace oel
{
namespace _detail
{
	template< typename T, typename U,
		bool = std::is_empty<U>::value >
	struct TightPair
	{
		T first;
		U _2nd;

		OEL_ALWAYS_INLINE U &       second() noexcept       { return _2nd; }
		OEL_ALWAYS_INLINE const U & second() const noexcept { return _2nd; }
	};

	template<typename First_type_MSVC_needs_unique_name, typename Second_type_unique_name_for_MSVC>
	struct TightPair<First_type_MSVC_needs_unique_name, Second_type_unique_name_for_MSVC, true>
	 :	private Second_type_unique_name_for_MSVC
	{
		First_type_MSVC_needs_unique_name first;

		using _t = Second_type_unique_name_for_MSVC;

		constexpr TightPair(const First_type_MSVC_needs_unique_name & a, const _t & b)
		 :	_t{b}, first{a} {
		}

		OEL_ALWAYS_INLINE _t &       second() noexcept       { return *this; }
		OEL_ALWAYS_INLINE const _t & second() const noexcept { return *this; }
	};


	template< typename T, bool = std::is_empty<T>::value >
	struct RefOptimizeEmpty
	{
		using Ty = T;

		T & _ref;

		T & get() noexcept { return _ref; }
	};

	template<typename Type_needs_unique_name_for_MSVC>
	struct RefOptimizeEmpty<Type_needs_unique_name_for_MSVC, true>
	 :	protected Type_needs_unique_name_for_MSVC
	{
		using Ty = Type_needs_unique_name_for_MSVC;

		RefOptimizeEmpty(Ty & o) : Ty{o} {}

		OEL_ALWAYS_INLINE Ty & get() noexcept { return *this; }
	};
}

} // namespace oel
