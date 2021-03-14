#pragma once

// Copyright 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "auxi/type_traits.h"

#include <memory>


/** @file
*/

namespace oel
{

//! Equivalent to std::make_unique_for_overwrite (C++20)
template< typename T,
          typename = enable_if< std::is_array<T>::value >
>
std::unique_ptr<T> make_unique_for_overwrite(size_t arraySize);

template< typename T, typename... Args,
          typename = enable_if< !std::is_array<T>::value >
>
[[deprecated]] std::unique_ptr<T> make_unique(Args &&... args);

template< typename T,
          typename = enable_if< std::is_array<T>::value >
>
[[deprecated]] std::unique_ptr<T> make_unique(size_t arraySize);



////////////////////////////////////////////////////////////////////////////////
//
// Implementation only in rest of the file


namespace _detail
{
	template< typename T, typename... Args >
	inline T * New(std::true_type, Args &&... args)
	{
		return new T(static_cast<Args &&>(args)...);
	}

	template< typename T, typename... Args >
	inline T * New(std::false_type, Args &&... args)
	{
		return new T{static_cast<Args &&>(args)...};
	}
}

} // namespace oel

template< typename T, typename... Args, typename >
inline std::unique_ptr<T>  oel::make_unique(Args &&... args)
{
	T * p = _detail::New<T>(std::is_constructible<T, Args...>(), static_cast<Args &&>(args)...);
	return std::unique_ptr<T>(p);
}

#define OEL_MAKE_UNIQUE_A(newExpr)  \
	static_assert(std::extent<T>::value == 0, "make_unique forbids T[size]. Please use T[]");  \
	using Elem = std::remove_extent_t<T>;  \
	return std::unique_ptr<T>(newExpr)

template< typename T, typename >
inline std::unique_ptr<T>  oel::make_unique(size_t size)
{
	OEL_MAKE_UNIQUE_A( new Elem[size]() ); // value-initialize
}

template< typename T, typename >
inline std::unique_ptr<T>  oel::make_unique_for_overwrite(size_t size)
{
	OEL_MAKE_UNIQUE_A(new Elem[size]);
}

#undef OEL_MAKE_UNIQUE_A
