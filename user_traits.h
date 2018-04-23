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
* Also provides all_ and a forward declaration of dynarray.
*/

#if defined __has_include
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


//! Equivalent to std::is_trivially_copyable, but may be specialized for some types
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

//! Trait that tells if T can be trivially relocated. See specify_trivial_relocate(T &&)
template<typename T>
struct is_trivially_relocatable;

// Many useful classes are declared trivially relocatable, see compat folder



template<bool...> struct bool_pack_t;

//! If all of Vs are equal to true, this is-a true_type, else false_type
template<bool... Vs>
using all_true   = std::is_same< bool_pack_t<true, Vs...>, bool_pack_t<Vs..., true> >;

/** @brief Similar to std::conjunction, but is not short-circuiting
*
* Example: @code
template<typename... Ts>
void ProcessNumbers(Ts... n) {
	static_assert(oel::all_< std::is_arithmetic<Ts>... >::value, "Only arithmetic types, please");
@endcode  */
template<typename... BoolConstants>
struct all_ : all_true<BoolConstants::value...> {};



////////////////////////////////////////////////////////////////////////////////
//
// Implementation only in rest of the file


namespace _detail
{
	template<typename Range>
	typename Range::difference_type DiffT(int);

	template<typename> std::ptrdiff_t DiffT(long);
}

} // namespace oel

template<typename T>
struct oel::is_trivially_copyable :
	#if defined __GLIBCXX__ and __GNUC__ == 4
		bool_constant< (__has_trivial_copy(T) and __has_trivial_assign(T))
			#ifdef __INTEL_COMPILER
				or __is_pod(T)
			#endif
			> {};
	#else
		std::is_trivially_copyable<T> {};
	#endif
