#pragma once

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


/** @file
* @brief Variant2 was introduced in Boost 1.71
*/

#include "../auxi/type_traits.h"

#include <boost/variant2/variant.hpp>

namespace oel
{

template< typename... Ts >
struct is_trivially_relocatable< boost::variant2::variant<Ts...> >
 :	std::conjunction< is_trivially_relocatable<Ts>... > {};

}