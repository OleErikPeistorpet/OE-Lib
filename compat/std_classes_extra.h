#pragma once

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "../user_traits.h"

#include <array>
#include <tuple>

// std::array, tuple, pair

namespace oel
{

template<typename T, size_t S>
struct is_trivially_relocatable< std::array<T, S> >
 :	is_trivially_relocatable<T> {};

template<typename T, typename... Us>
struct is_trivially_relocatable< std::tuple<T, Us...> > : bool_constant<
	is_trivially_relocatable<T>::value && is_trivially_relocatable< std::tuple<Us...> >::value > {};

template<> struct is_trivially_relocatable< std::tuple<> > : true_type {};

template<typename T, typename U>
struct is_trivially_relocatable< std::pair<T, U> > : bool_constant<
	is_trivially_relocatable<T>::value && is_trivially_relocatable<U>::value > {};

}