#pragma once

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


/** @file
* @brief Error handling macros and forward declarations, including is_trivially_relocatable (for user classes)
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


//! Used anywhere that would normally throw if exceptions are off (compiler switch)
/**
* Also used instead of throwing std::bad_alloc if OEL_NEW_HANDLER has been defined to non-zero,
* regardless of exceptions being enabled.
*
* Feel free to define it to something else, but note that it must never return. */
#ifndef OEL_ABORT
#define OEL_ABORT(message) (std::abort(), (void) message)
#endif


//! Used for checking preconditions. Can be defined to your own
/**
* Used in noexcept functions, so don't expect to catch anything thrown.
* OEL_ASSERT itself should probably be noexcept to avoid bloat. */
#if OEL_MEM_BOUND_DEBUG_LVL == 0
	#undef  OEL_ASSERT
	#define OEL_ASSERT(cond) void(0)
#elif !defined OEL_ASSERT
	#if defined _MSC_VER
	#define OEL_ASSERT(cond) ((cond) or (__debugbreak(), false))
	#else
	#define OEL_ASSERT(cond) ((cond) or (__builtin_trap(), false))
	#endif
#endif


//! Obscure Efficient Library
namespace oel
{

template< typename T = unsigned char >
class allocator;

#if OEL_MEM_BOUND_DEBUG_LVL
inline namespace debug
	#ifdef __GNUC__
		__attribute__((abi_tag))
	#endif
{
#endif

template< typename T, typename Alloc = allocator<> >
class dynarray;

#if OEL_MEM_BOUND_DEBUG_LVL
}
#endif



//! Trait that tells if T objects can transparently be relocated in memory
/**
* This means that T cannot have a member that is a pointer to any of its non-static members, and
* must not need to update external state during move construction. (The same recursively for sub-objects)
*
* https://github.com/facebook/folly/blob/master/folly/docs/FBVector.md#object-relocation  <br>
* https://isocpp.org/files/papers/P1144R8.html
*
* True by default for types that are trivially move constructible and trivially destructible.
* For others, declare a function in the namespace of the type like this:
@code
oel::true_type specify_trivial_relocate(MyClass);

// Or if you are unsure if a member or base class is and will stay trivially relocatable:
class MyClass {
	std::string name;
};
oel::is_trivially_relocatable<std::string> specify_trivial_relocate(MyClass);

// With nested class, you can use friend keyword:
class Outer {
	class Inner {
		std::unique_ptr<whatever> a;
	};
	friend oel::true_type specify_trivial_relocate(Inner);
};
@endcode

* Many external classes are declared trivially relocatable, see `optimize_ext` folder. */
template< typename T >
struct is_trivially_relocatable;


template< bool Val >
struct bool_constant
{
	static constexpr auto value = Val;
};

using false_type = bool_constant<false>;
using true_type  = bool_constant<true>;

} // namespace oel
