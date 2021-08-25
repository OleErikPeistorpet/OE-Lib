#pragma once

// Copyright 2020 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "dynarray.h"

#include <memory_resource>

//! Mirroring std::pmr and boost::container::pmr
namespace oel::pmr
{

using std::pmr::polymorphic_allocator;

template< typename T >
using dynarray = oel::dynarray< T, polymorphic_allocator<> >;

//! Same as oel::to_dynarray, except always with polymorphic_allocator
constexpr auto to_dynarray(polymorphic_allocator<> a = {})  { return oel::to_dynarray(a); }

}