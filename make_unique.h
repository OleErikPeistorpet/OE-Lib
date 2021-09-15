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

//! Equivalent to std::make_unique_for_overwrite (C++20), for array types with unknown bound
template< typename T,
          typename = enable_if< _detail::isUnboundedArray<T> >
>  inline
std::unique_ptr<T> make_unique_for_overwrite(size_t count)
	{
		return std::unique_ptr<T>(new std::remove_extent_t<T>[count]);
	}

template< typename T, typename... Args,
          typename = enable_if< !std::is_array<T>::value >
>
[[deprecated]] std::unique_ptr<T> make_unique(Args &&... args);

template< typename T,
          typename = enable_if< _detail::isUnboundedArray<T> >
>
[[deprecated]] inline std::unique_ptr<T> make_unique(size_t count)  { return std::make_unique<T>(count); }



////////////////////////////////////////////////////////////////////////////////
//
// Implementation only


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

} // oel

template< typename T, typename... Args, typename >
inline std::unique_ptr<T>  oel::make_unique(Args &&... args)
{
	T * p = _detail::New<T>(std::is_constructible<T, Args...>(), static_cast<Args &&>(args)...);
	return std::unique_ptr<T>(p);
}