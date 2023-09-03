#pragma once

// Copyright 2021 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "transform.h"

/** @file
*/

namespace oel::view
{

//! Like std::views::elements
/**
* Use `elements<0>` or `transform(oel::key)` to mimic std::views::keys.
* Use `elements<1>` or `transform(oel::value)` to mimic std::views::values. */
template< size_t I >
inline constexpr auto elements = transform( _detail::Get<I>{} );

}