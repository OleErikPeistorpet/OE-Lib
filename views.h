#pragma once

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


/** @file
* @brief A small subset of the functionality of views in standard <ranges> (C++ 20/23)
*
* The goal is not to be a general range library,
* but to provide input for dynarray and the oel::copy functions in a way that optimizes really well.
*/

#include "view/counted.h"
#include "view/move.h"
#include "view/subrange.h"
#include "view/transform.h"
