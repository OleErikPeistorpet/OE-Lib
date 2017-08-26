#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "core_util.h"

#include <memory>
#include <cstdint>


/** @file
* @brief Utilities: as_signed/as_unsigned, index_valid, make_unique, deref_args
*/

namespace oel
{

//! Given argument val of integral or enumeration type T, returns val cast to the signed integer type corresponding to T
template<typename T> inline
constexpr typename std::make_signed<T>::type   as_signed(T val) noexcept  { return (typename std::make_signed<T>::type)val; }
//! Given argument val of integral or enumeration type T, returns val cast to the unsigned integer type corresponding to T
template<typename T> inline
constexpr typename std::make_unsigned<T>::type as_unsigned(T val) noexcept
	{ return (typename std::make_unsigned<T>::type)val; }


///@{
//! Check if index is valid (can be used with operator[]) for array or other range.
template< typename UnsignedInt, typename SizedRange,
          enable_if<std::is_unsigned<UnsignedInt>::value> = 0 > inline
bool index_valid(const SizedRange & r, UnsignedInt index)   { return index < as_unsigned(oel::ssize(r)); }

template<typename SizedRange> inline
bool index_valid(const SizedRange & r, std::int32_t index)  { return 0 <= index && index < oel::ssize(r); }

template<typename SizedRange> inline
bool index_valid(const SizedRange & r, std::int64_t index)
{	// assumes that r.size() is never greater than INT64_MAX
	return static_cast<std::uint64_t>(index) < as_unsigned(oel::ssize(r));
}
///@}


/**
* @brief Same as std::make_unique, but performs direct-list-initialization if there is no matching constructor
*
* (Works for aggregates.) http://open-std.org/JTC1/SC22/WG21/docs/papers/2015/n4462.html  */
template< typename T, typename... Args, typename = enable_if<!std::is_array<T>::value> >
std::unique_ptr<T> make_unique(Args &&... args);

//! Equivalent to std::make_unique (array version).
template< typename T, typename = enable_if<std::is_array<T>::value> >
std::unique_ptr<T> make_unique(size_t arraySize);
/**
* @brief Array is default-initialized, can be significantly faster for non-class elements
*
* Non-class elements get indeterminate values. http://en.cppreference.com/w/cpp/language/default_initialization  */
template< typename T, typename = enable_if<std::is_array<T>::value> >
std::unique_ptr<T> make_unique_default(size_t arraySize);



/** @brief Calls operator * on arguments before passing them to Func
*
* Example, sort pointers by pointed-to values, not addresses:
@code
oel::dynarray< std::unique_ptr<double> > d;
std::sort(d.begin(), d.end(), deref_args<std::less<>>{}); // std::less<double> before C++14
@endcode  */
template<typename Func>
class deref_args
{
	Func _f;

public:
	deref_args(Func f = Func{})  : _f(std::move(f)) {}

	template<typename... Ts>
	auto operator()(Ts &&... args) const -> decltype( _f(*std::forward<Ts>(args)...) )
	                                         { return _f(*std::forward<Ts>(args)...); }

	using is_transparent = void;
};



////////////////////////////////////////////////////////////////////////////////
//
// Implementation only in rest of the file


namespace _detail
{
	template<typename T, typename... Args> inline
	T * New(std::true_type, Args &&... args)
	{
		return new T(std::forward<Args>(args)...);
	}

	template<typename T, typename... Args> inline
	T * New(std::false_type, Args &&... args)
	{
		return new T{std::forward<Args>(args)...};
	}
}

} // namespace oel

template<typename T, typename... Args, typename>
inline std::unique_ptr<T>  oel::make_unique(Args &&... args)
{
	T * p = _detail::New<T>(std::is_constructible<T, Args...>(), std::forward<Args>(args)...);
	return std::unique_ptr<T>(p);
}

#define OEL_MAKE_UNIQUE_A(newExpr)  \
	static_assert(std::extent<T>::value == 0, "make_unique forbids T[size]. Please use T[]");  \
	using Elem = typename std::remove_extent<T>::type;  \
	return std::unique_ptr<T>(newExpr)

template<typename T, typename>
inline std::unique_ptr<T>  oel::make_unique(size_t size)
{
	OEL_MAKE_UNIQUE_A( new Elem[size]() ); // value-initialize
}

template<typename T, typename>
inline std::unique_ptr<T>  oel::make_unique_default(size_t size)
{
	OEL_MAKE_UNIQUE_A(new Elem[size]);
}

#undef OEL_MAKE_UNIQUE_A
