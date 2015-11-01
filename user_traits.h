#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include <type_traits>


/** @file
* @brief is_trivially_relocatable to be specialized for user classes
*
* Also provides a forward declaration of dynarray.
*/

#if _MSC_VER && _MSC_VER < 1900
	#ifndef _ALLOW_KEYWORD_MACROS
	#define _ALLOW_KEYWORD_MACROS 1
	#endif

	#ifndef noexcept
	#define noexcept throw()
	#endif

	#undef constexpr
	#define constexpr

	#define OEL_ALIGNOF __alignof
#else
	#define OEL_ALIGNOF alignof
#endif


#if defined(__has_include)
	#if !__has_include(<boost/config.hpp>)
	#define OEL_NO_BOOST 1
	#endif
#endif


/// Obscure Efficient Library
namespace oel
{

template<typename T> struct allocator;  // forward declare

template<typename T, typename Alloc = allocator<T> >
class dynarray;



template<bool Val>
using bool_constant = std::integral_constant<bool, Val>;


using std::true_type; // for specify_trivial_relocate and is_trivially_copyable

/// Equivalent to std::is_trivially_copyable, but can be specialized for a type if you are sure memcpy is safe to copy it
template<typename T>
struct is_trivially_copyable :
	#if __GLIBCXX__ && __GNUC__ == 4
		bool_constant< (__has_trivial_copy(T) && __has_trivial_assign(T))
			#if __INTEL_COMPILER
				|| __is_pod(T)
			#endif
			> {};
	#else
		std::is_trivially_copyable<T> {};
	#endif

/**
* @brief Function declaration to specify that T does not have a pointer member to any of its data members
*	(including inherited), and does not notify any observers in its copy/move constructor.
*
* https://github.com/facebook/folly/blob/master/folly/docs/FBVector.md#object-relocation  <br>
* http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n4158.pdf
*
* Declare a function in the namespace of the type or in \c oel like this:
@code
oel::true_type specify_trivial_relocate(MyClass &&);
// Or if you are unsure if a member or base class is and will stay trivially relocatable:
oel::is_trivially_relocatable<MemberTypeOfU> specify_trivial_relocate(U &&);
@endcode  */
template<typename T>
#if OEL_TRIVIAL_RELOCATE_DEFAULT
	true_type
#else
	is_trivially_copyable<T>
#endif
	specify_trivial_relocate(T &&);

/// Trait that tells if T can be trivially relocated. See specify_trivial_relocate(T &&)
template<typename T>
struct is_trivially_relocatable;

// Many useful classes are declared trivially relocatable, see compat folder


template<bool...> struct bool_pack_t;

/** @brief If all of Vs is true, all_true is-a true_type, else false_type
*
* Example: @code
template<typename... Ts>
void ProcessNumbers(Ts...) {
	static_assert(oel::all_true< std::is_arithmetic<Ts>::value... >::value, "Only arithmetic types, please");
@endcode  */
template<bool... Vs>
using all_true = std::is_same< bool_pack_t<true, Vs...>, bool_pack_t<Vs..., true> >;



template<typename T>
using make_signed_t   = typename std::make_signed<T>::type;  ///< std with C++14
template<typename T>
using make_unsigned_t = typename std::make_unsigned<T>::type; ///< std with C++14

} // namespace oel
