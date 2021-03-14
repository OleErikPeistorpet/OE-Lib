#pragma once

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


/** @file
* @brief Views that move, in addition to a small subset of the functionality of C++20 ranges
*
* The views are mostly intended as input for dynarray and the oel::copy functions.
*/

#include "algo/assign_append_insert.h"
#include "algo/copy.h"
#include "algo/erase.h"
#include "view/counted.h"
#include "view/move.h"
#include "view/subrange.h"
#include "view/transform.h"
