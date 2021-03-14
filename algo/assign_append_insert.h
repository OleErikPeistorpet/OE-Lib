#pragma once

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "type_traits.h"

/** @file
* @brief Range-based wrapper functions for containers
*/

namespace oel
{

/** @name GenericContainerInsert
* @brief For generic code that may use either dynarray or std library container (overloaded in dynarray.h)  */
//!@{
template< typename Container, typename InputRange >
constexpr void assign(Container & dest, InputRange && source)  { dest.assign(begin(source), end(source)); }

template< typename Container, typename InputRange >
constexpr void append(Container & dest, InputRange && source)  { dest.insert(dest.end(), begin(source), end(source)); }

template< typename Container, typename T >
constexpr void append(Container & dest, typename Container::size_type count, const T & val)
{
	dest.resize(dest.size() + count, val);
}

template< typename Container, typename ContainerIterator, typename InputRange >
constexpr auto insert(Container & dest, ContainerIterator pos, InputRange && source)
{
	return dest.insert(pos, begin(source), end(source));
}
//!@}
} // oel
