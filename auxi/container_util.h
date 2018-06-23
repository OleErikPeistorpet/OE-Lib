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
}

} // namespace oel
