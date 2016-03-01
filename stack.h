#pragma once

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "dynarray.h"

#include <stack>


namespace oel
{

template<typename T, typename Alloc = allocator<T> >
using stack = std::stack< T, dynarray<T, Alloc> >;

template<typename T, typename A>
true_type specify_trivial_relocate(stack<T, A>);

}