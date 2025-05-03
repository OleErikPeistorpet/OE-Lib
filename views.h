#pragma once

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


/** @file
* @brief Views designed as input for dynarray, largely with the same API as std::views
*
* The goal is not to be a general range library, but to provide views that optimize really well.
*/

#include "view/all.h"
#include "view/counted.h"
#include "view/generate.h"
#include "view/move.h"
#include "view/owning.h"
#include "view/repeat.h"
#include "view/subrange.h"
#include "view/transform.h"
#include "view/value_init.h"
#include "view/zip_transform.h"
