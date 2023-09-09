#pragma once

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#ifdef _MSC_EXTENSIONS
#include <ciso646>
#endif
#include <type_traits>


/** @file
* @brief specify_trivial_relocate for user classes, error handling macros, forward declarations
*/

#ifndef OEL_MEM_BOUND_DEBUG_LVL
/** @brief 0: no iterator and precondition checks. 1: most checks. 2: all checks.
*
* Be careful with compilers other than MSVC. A non-zero level should only be
* set in combination with `-O0` or `-fno-strict-aliasing`. Also note that
* level 0 is not binary compatible with any other. Mixing 1 and 2 should work. */
	#ifdef _DEBUG
	#define OEL_MEM_BOUND_DEBUG_LVL  2
	#else
	#define OEL_MEM_BOUND_DEBUG_LVL  0
	#endif
#endif


#ifndef OEL_ABORT
/** @brief If exceptions are disabled, used anywhere that would normally throw. Also used by OEL_ASSERT
*
* Can be defined to something else, but note that it must never return.
* Moreover, don't expect to catch what it throws, because it's used in noexcept functions through OEL_ASSERT. */
#define OEL_ABORT(message) (std::abort(), (void) message)
#endif

#if OEL_MEM_BOUND_DEBUG_LVL == 0
	#undef OEL_ASSERT
	#define OEL_ASSERT(cond) ((void) 0)
#elif !defined OEL_ASSERT
	/** @brief Used for checking preconditions. Can be defined to your own
	*
	* Used in noexcept functions, so don't expect to catch anything thrown.
	* OEL_ASSERT itself should probably be noexcept for optimal performance. */
	#define OEL_ASSERT(cond)  \
		((cond) or (OEL_ABORT("Failed precond: " #cond), false))
#endif


//! Obscure Efficient Library
namespace oel
{

template< typename T > class allocator;

#if OEL_MEM_BOUND_DEBUG_LVL
inline namespace debug
	#ifdef __GNUC__
		__attribute__((abi_tag))
	#endif
{
#endif

template< typename T, typename Alloc = allocator<T> >
class dynarray;

#if OEL_MEM_BOUND_DEBUG_LVL
}
#endif



using std::bool_constant;
using std::false_type;
using std::true_type;


/**
* @brief Function declaration to specify that T objects can transparently be relocated in memory.
*
* This means that T cannot have a member that is a pointer to any of its non-static members, and
* must not need to update external state during move construction. (The same recursively for sub-objects)
*
* https://github.com/facebook/folly/blob/master/folly/docs/FBVector.md#object-relocation  <br>
* http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p1144r4.html
*
* Already true for trivially copyable types. For others, declare a function in the namespace of the type like this:
@code
oel::true_type specify_trivial_relocate(MyClass);

// Or if you are unsure if a member or base class is and will stay trivially relocatable:
class MyClass {
	std::string name;
};
oel::is_trivially_relocatable<std::string> specify_trivial_relocate(MyClass);

// With nested class, use friend keyword:
class Outer {
	class Inner {
		std::unique_ptr<whatever> a;
	};
	friend oel::true_type specify_trivial_relocate(Inner);
};
@endcode  */
template< typename T >
bool_constant< std::is_trivially_move_constructible_v<T> and std::is_trivially_destructible_v<T> >
	specify_trivial_relocate(T &&);

/** @brief Trait that tells if T can be trivially relocated. See specify_trivial_relocate(T &&)
*
* Many external classes are declared trivially relocatable, see `optimize_ext` folder. */
template< typename T >
struct is_trivially_relocatable;

} // namespace oel
