#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "../user_traits.h"

#include <iterator>


namespace oel
{

using std::begin;
using std::end;

#if __cplusplus >= 201402 or defined _MSC_VER
	using std::cbegin;   using std::cend;
	using std::crbegin;  using std::crend;
#endif

/** @brief Argument-dependent lookup non-member begin, defaults to std::begin
*
* Note the global using-directive  @code
	auto it = container.begin();     // Fails with types that don't have begin member such as built-in arrays
	auto it = std::begin(container); // Fails with types that have only non-member begin outside of namespace std
	// Argument-dependent lookup, as generic as it gets
	using std::begin; auto it = begin(container);
	auto it = adl_begin(container);  // Equivalent to line above
@endcode  */
template<typename Range>   OEL_ALWAYS_INLINE
constexpr auto adl_begin(Range & r) -> decltype(begin(r))         { return begin(r); }
//! Const version of adl_begin(), analogous to std::cbegin
template<typename Range>   OEL_ALWAYS_INLINE
constexpr auto adl_cbegin(const Range & r) -> decltype(begin(r))  { return begin(r); }

//! Argument-dependent lookup non-member end, defaults to std::end
template<typename Range>   OEL_ALWAYS_INLINE
constexpr auto adl_end(Range & r) -> decltype(end(r))         { return end(r); }
//! Const version of adl_end()
template<typename Range>   OEL_ALWAYS_INLINE
constexpr auto adl_cend(const Range & r) -> decltype(end(r))  { return end(r); }

} // namespace oel

using oel::adl_begin;
using oel::adl_cbegin;
using oel::adl_end;
using oel::adl_cend;
