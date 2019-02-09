#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "type_traits.h"

namespace oel
{
namespace _detail
{
	template<typename> struct DynarrBase;

	template<typename, typename> struct FcaProxy;


	template<typename T> struct AssertTrivialRelocate
	{
		static_assert(is_trivially_relocatable<T>::value,
			"The function requires trivially relocatable T, see declaration of is_trivially_relocatable");
	};


	template< typename T,
		bool = std::is_empty<T>::value and std::is_default_constructible<T>::value >
	struct RefOptimizeEmpty
	{
		T & _ref;

		T & Get() noexcept { return _ref; }
	};

	template<typename T>
	struct RefOptimizeEmpty<T, true>
	{
		OEL_ALWAYS_INLINE RefOptimizeEmpty(T &) {}

		T Get() noexcept { return T{}; }
	};


	template<typename Alloc, typename Arg>
	decltype( std::declval<Alloc &>().construct( (typename Alloc::value_type *)0, std::declval<Arg>() ),
		true_type() )
		HasConstructTest(int);

	template<typename, typename>
	false_type HasConstructTest(long);

	template<typename Alloc, typename Arg>
	using AllocHasConstruct = decltype( HasConstructTest<Alloc, Arg>(0) );
}

} // namespace oel
