#pragma once

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#ifdef _MSC_EXTENSIONS
#include <ciso646>
#endif
#include <type_traits>


/** @file
* @brief specify_trivial_relocate for user classes, error handling macros, forward declarations
*
* Notably provides a forward declaration of dynarray
*/

#ifndef OEL_MEM_BOUND_DEBUG_LVL
/** @brief 0: no iterator and precondition checks. 1: most checks. 2: all checks.
*
* Level 0 is not binary compatible with any other. Mixing 1 and 2 should work. */
	#ifdef NDEBUG
	#define OEL_MEM_BOUND_DEBUG_LVL  0
	#else
	#define OEL_MEM_BOUND_DEBUG_LVL  2
	#endif
#endif


#ifndef OEL_ABORT
	/** @brief If exceptions are disabled, used anywhere that would normally throw. If predefined, used by OEL_ASSERT
	*
	* Users may define this to call a function that never returns or to throw an exception.
	* Example: @code
	#define OEL_ABORT(errorMessage)  throw std::logic_error(errorMessage "; in " __FILE__)
	@endcode  */
	#define OEL_ABORT(msg) (std::abort(), (void) msg)

	#if !defined OEL_ASSERT and !defined NDEBUG
	#include <cassert>

	//! Can be defined to your own or changed right here.
	#define OEL_ASSERT  assert
	#endif
#endif


#if OEL_MEM_BOUND_DEBUG_LVL and !defined _MSC_VER
	#define OEL_DYNARRAY_IN_DEBUG  1  // would not work with the .natvis
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



template<bool Val>
using bool_constant = std::integral_constant<bool, Val>;

using std::true_type; // equals bool_constant<true>
using std::false_type;


/**
* @brief Function declaration to specify that T objects can transparently be relocated in memory.
*
* This means that T cannot have a member that is a pointer to any of its non-static members, and
* must not need to update external state during move construction. (The same recursively for sub-objects)
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
bool_constant<
	#if defined __GLIBCXX__ and __GNUC__ == 4
		__has_trivial_copy(T) and __has_trivial_destructor(T)
	#else
		std::is_trivially_move_constructible<T>::value and std::is_trivially_destructible<T>::value
	#endif
>	specify_trivial_relocate(T &&);

/** @brief Trait that tells if T can be trivially relocated. See specify_trivial_relocate(T &&)
*
* Many external classes are declared trivially relocatable, see `optimize_ext` folder. */
template<typename T>
struct is_trivially_relocatable;

} // namespace oel



////////////////////////////////////////////////////////////////////////////////
//
// The rest of the file is not for users (implementation)


//! @cond INTERNAL

#if OEL_MEM_BOUND_DEBUG_LVL == 0
	#undef OEL_ASSERT
	#define OEL_ASSERT(expr) ((void) 0)
#elif !defined OEL_ASSERT
	#define OEL_ASSERT(expr)  \
		((expr) or (OEL_ABORT("Failed assert " #expr), false))
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


#if defined __cpp_deduction_guides or (_MSC_VER >= 1914 and _HAS_CXX17)
	#define OEL_HAS_DEDUCTION_GUIDES  1
#endif


#if defined _CPPUNWIND or defined __EXCEPTIONS
	#define OEL_THROW(exception, msg) throw exception
	#define OEL_TRY_                  try
	#define OEL_CATCH_ALL             catch (...)
	#define OEL_WHEN_EXCEPTIONS_ON(x) x
#else
	#define OEL_THROW(exc, message)   OEL_ABORT(message)
	#define OEL_TRY_
	#define OEL_CATCH_ALL             OEL_CONST_COND if (false)
	#define OEL_WHEN_EXCEPTIONS_ON(x)
#endif

//! @endcond
