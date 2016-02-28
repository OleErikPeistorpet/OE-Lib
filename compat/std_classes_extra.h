#pragma once

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "../user_traits.h"

#include <array>
#include <tuple>

// is_trivially_relocatable<std::array>, is_trivially_copyable<std::tuple>

namespace oel
{

template<typename T, std::size_t S>
struct is_trivially_relocatable< std::array<T, S> >
 :	is_trivially_relocatable<T> {};

template<typename... Ts>
struct is_trivially_copyable< std::tuple<Ts...> >
 :	all_true< is_trivially_copyable<Ts>::value... > {};

}