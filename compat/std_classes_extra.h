#pragma once

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "../user_traits.h"

#include <array>
#include <tuple>

// std:: array, tuple, pair

namespace oel
{

template<typename T, std::size_t S>
is_trivially_relocatable<T> specify_trivial_relocate(std::array<T, S> &&);

template<typename... Ts>
struct is_trivially_relocatable< std::tuple<Ts...> >
 :	all_true< is_trivially_relocatable<Ts>::value... > {};

template<typename... Ts>
struct is_trivially_copyable< std::tuple<Ts...> >
 :	all_true< is_trivially_copyable<Ts>::value... > {};

template<typename T, typename U>
struct is_trivially_relocatable< std::pair<T, U> > : bool_constant<
	is_trivially_relocatable<T>::value && is_trivially_relocatable<U>::value > {};

template<typename T, typename U>
struct is_trivially_copyable< std::pair<T, U> > : bool_constant<
	is_trivially_copyable<T>::value && is_trivially_copyable<U>::value > {};

}