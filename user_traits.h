#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "error_handling.h"

#include <type_traits>


/** @file
* @brief specify_trivial_relocate to be overloaded for user classes
*
* Also provides a forward declaration of dynarray (and a few other minor things)
*/

#ifdef __has_include
	#if !__has_include(<boost/config.hpp>)
	#define OEL_NO_BOOST  1
	#endif
#endif


//! Obscure Efficient Library
namespace oel
{

template<typename T> struct allocator;  // forward declare

#ifdef OEL_DYNARRAY_IN_DEBUG
inline namespace debug
	#if __GNUC__ >= 5
		__attribute__((abi_tag))
	#endif
{
#endif

template<typename T, typename Alloc = allocator<T> >
class dynarray;

#ifdef OEL_DYNARRAY_IN_DEBUG
}
#endif


//! Functions marked with `noexcept(nodebug)` will only throw exceptions from OEL_ASSERT (none by default)
constexpr bool nodebug = OEL_MEM_BOUND_DEBUG_LVL == 0;



template<bool Val>
using bool_constant = std::integral_constant<bool, Val>;

using std::true_type; // equals bool_constant<true>
using std::false_type;


//! Equivalent to std::is_trivially_copyable, but may be specialized (for user types)
template<typename T>
struct is_trivially_copyable;

/**
* @brief Function declaration to specify that T objects can transparently be relocated in memory.
*
* This means that T cannot have a member that is a pointer to any of its non-static members
* (including inherited), and must not need to update external state during move construction.
*
* https://github.com/facebook/folly/blob/master/folly/docs/FBVector.md#object-relocation  <br>
* http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n4158.pdf
*
* Already true for trivially copyable types. For others, declare a function in the namespace of the type like this:
@code
oel::true_type specify_trivial_relocate(MyClass &&);

// Or if you are unsure if a member or base class is and will stay trivially relocatable:
class MyClass {
	std::string name;
};
oel::is_trivially_relocatable<std::string> specify_trivial_relocate(MyClass &&);

// With nested class, use friend keyword:
class Outer {
	class Inner {
		std::unique_ptr<whatever> a;
	};
	friend oel::true_type specify_trivial_relocate(Inner &&);
};
@endcode  */
template<typename T>
is_trivially_copyable<T> specify_trivial_relocate(T &&);

/** @brief Trait that tells if T can be trivially relocated. See specify_trivial_relocate(T &&)
*
* Many useful classes are declared trivially relocatable, see `trivial_relocate` folder. */
template<typename T>
struct is_trivially_relocatable;

} // namespace oel



////////////////////////////////////////////////////////////////////////////////
//
// Implementation


template<typename T>
struct oel::is_trivially_copyable :
	#if defined __GLIBCXX__ and __GNUC__ == 4
		bool_constant< (__has_trivial_copy(T) and __has_trivial_assign(T) and __has_trivial_destructor(T))
			#ifdef __INTEL_COMPILER
				or __is_pod(T)
			#endif
			> {};
	#else
		std::is_trivially_copyable<T> {};
	#endif
