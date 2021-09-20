#pragma once

// Copyright 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "detail/may_sized.h"

/** @file
*/

namespace oel
{
namespace view
{

//! Create a basic_view of std::move_iterator from two iterators
template< typename InputIterator >
constexpr basic_view< std::move_iterator<InputIterator> >
	move(InputIterator first, InputIterator last)   { using MI = std::move_iterator<InputIterator>;
	                                                  return {MI{first}, MI{last}}; }
/**
* @brief Wrap a range such that the elements can be moved from when passed to a container or algorithm
* @return Type `counted_view<std::move_iterator>` if r.size() exists or r is an array,
*	else `basic_view<std::move_iterator>`
*
* Note that passing an rvalue range is meant to give a compile error. Use a named variable. */
template< typename InputRange >
constexpr auto move(InputRange & r)   { return _detail::move(r); }

}

} // oel
