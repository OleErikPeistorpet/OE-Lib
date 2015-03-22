#pragma once

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "../container_core.h"

#include <array>


namespace oel
{
template<typename T, size_t S>
struct is_trivially_relocatable< std::array<T, S> >
 :	is_trivially_relocatable<T> {};
}