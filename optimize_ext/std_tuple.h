#pragma once

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


/** @file
* @brief Must be included manually in user code
*/

#include "../auxi/core_util.h"

#include <tuple>


template< typename... Ts >
struct oel::is_trivially_relocatable< std::tuple<Ts...> >
 :	std::conjunction< is_trivially_relocatable<Ts>... > {};
