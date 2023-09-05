#pragma once

// Copyright 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "../fwd.h"
#include <utility> // for declval

//! @cond INTERNAL

#if __cpp_lib_concepts < 201907
	#define OEL_REQUIRES(...)
#else
	#define OEL_REQUIRES(...) requires(__VA_ARGS__)

	#if !defined _LIBCPP_VERSION or _LIBCPP_VERSION >= 15000
	#define OEL_HAS_STD_MOVE_SENTINEL  1
	#endif
#endif

#ifndef OEL_HAS_STD_MOVE_SENTINEL
#define OEL_HAS_STD_MOVE_SENTINEL  0
#endif


#ifdef __GNUC__
	#define OEL_ALWAYS_INLINE __attribute__((always_inline))
#else
	#define OEL_ALWAYS_INLINE
#endif

#ifdef _MSC_VER
	#define OEL_CONST_COND __pragma(warning(suppress : 4127 6326))
#else
	#define OEL_CONST_COND
#endif


#if defined _CPPUNWIND or defined __EXCEPTIONS
	#define OEL_HAS_EXCEPTIONS        1
	#define OEL_THROW(exception, msg) throw exception
	#define OEL_RETHROW               throw
	#define OEL_TRY_                  try
	#define OEL_CATCH_ALL             catch (...)
#else
	#define OEL_HAS_EXCEPTIONS        0
	#define OEL_THROW(exc, message)   OEL_ABORT(message)
	#define OEL_RETHROW
	#define OEL_TRY_
	#define OEL_CATCH_ALL             OEL_CONST_COND if (false)
#endif

//! @endcond

namespace oel
{

//! Same as std::enable_if_t<Condition, int>. Type int is intended as unused dummy
template< bool Condition >
using enable_if = typename std::enable_if<Condition, int>::type;



////////////////////////////////////////////////////////////////////////////////



namespace _detail
{
	template< typename >
	inline constexpr bool isUnboundedArray = false;

	template< typename T >
	inline constexpr bool isUnboundedArray<T[]> = true;



	template< typename T >
	constexpr T MoveIfNotCopyable(T & ob)
	{
		if constexpr (std::is_copy_constructible_v<T>)
			return ob;
		else
			return static_cast<T &&>(ob);
	}



	template< typename Alloc >  // pass dummy int to prefer this overload
	constexpr auto CanRealloc(int)
	->	decltype(Alloc::can_reallocate())
	{	return   Alloc::can_reallocate(); }

	template< typename >
	constexpr bool CanRealloc(long) { return false; }
}

} // oel


// declared in fwd.h
template< typename T >
struct oel::is_trivially_relocatable : decltype( specify_trivial_relocate(std::declval<T>()) ) {};
