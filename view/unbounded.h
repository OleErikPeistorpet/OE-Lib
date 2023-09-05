#pragma once

// Copyright 2021 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "subrange.h"

/** @file
*/

namespace oel::view
{
//! TODO
inline constexpr auto unbounded =
	[](auto iterator)   { return subrange(std::move(iterator), unreachable_sentinel); };
}