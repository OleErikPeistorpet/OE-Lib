#pragma once

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


/** @file
* @brief Must be included manually in user code
*/

#include "../auxi/type_traits.h"

#include <variant>

namespace oel
{

template< typename... Ts >
struct is_trivially_relocatable< std::variant<Ts...> >
 :	all_< is_trivially_relocatable<Ts>... > {};

}