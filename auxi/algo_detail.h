#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "../util.h"

#include <cstring>

namespace oel
{

// std::max not constexpr for GCC 4
template<typename T>
constexpr T max(const T & a, const T & b)
{
	return a < b ? b : a;
}


#if defined __GLIBCXX__ and __GNUC__ == 4
	template<typename T>
	using is_trivially_default_constructible = bool_constant< __has_trivial_constructor(T)
		#ifdef __INTEL_COMPILER
			or __is_pod(T)
		#endif
		>;
#else
	using std::is_trivially_default_constructible;
#endif


namespace _detail
{
	template<typename T>
	void Destroy(T * first, T *const last) noexcept
	{	// first > last is OK, does nothing
		OEL_CONST_COND if (!std::is_trivially_destructible<T>::value) // for speed with non-optimized builds
		{
			for (; first < last; ++first)
				first-> ~T();
		}
	}


	struct NoOp
	{	OEL_ALWAYS_INLINE void operator()(...) const {}
	};

	template<typename Alloc, typename InputIter, typename T, typename FuncTakingLast = NoOp>
	InputIter UninitCopy(InputIter src, T * dest, T *const dLast, Alloc & alloc, FuncTakingLast extraCleanup = {})
	{
		T *const destBegin = dest;
		OEL_TRY_
		{
			while (dest != dLast)
			{
				std::allocator_traits<Alloc>::construct(alloc, dest, *src);
				++src; ++dest;
			}
		}
		OEL_CATCH_ALL
		{
			_detail::Destroy(destBegin, dest);
			extraCleanup(dLast);
			OEL_WHEN_EXCEPTIONS_ON(throw);
		}
		return src;
	}


	template<typename Alloc, typename T, typename... Arg>
	void UninitFillImpl(false_type, T * first, T *const last, Alloc & alloc, const Arg &... arg)
	{
		T *const init = first;
		OEL_TRY_
		{
			for (; first != last; ++first)
				std::allocator_traits<Alloc>::construct(alloc, first, arg...);
		}
		OEL_CATCH_ALL
		{
			_detail::Destroy(init, first);
			OEL_WHEN_EXCEPTIONS_ON(throw);
		}
	}

	template<typename Alloc, typename T> inline
	void UninitFillImpl(true_type, T * first, T * last, Alloc &, int val = 0)
	{
		std::memset(first, val, last - first);
	}

	template<typename Alloc, typename T, typename... Arg> inline
	void UninitFill(T *const first, T *const last, Alloc & alloc, const Arg &... arg)
	{
		// TODO: investigate libstdc++ std::fill
		_detail::UninitFillImpl(bool_constant<std::is_integral<T>::value and sizeof(T) == 1>(),
		                        first, last, alloc, arg...);
	}

	template<typename Alloc, typename T> inline
	void UninitDefaultConstruct(T *const first, T *const last, Alloc & alloc)
	{
		OEL_CONST_COND if (!is_trivially_default_constructible<T>::value)
			_detail::UninitFillImpl(false_type{}, first, last, alloc);
	}


	inline void MemcpyMaybeNull(void * dest, const void * src, size_t nBytes)
	{	// memcpy(nullptr, nullptr, 0) is UB. The trouble is that checking can have significant performance hit.
		// GCC 4.9 and up known to need the check in some cases
	#if (!defined __GNUC__ and !defined _MSC_VER) or OEL_GCC_VERSION >= 409 or _MSC_VER >= 2000
		if (nBytes > 0)
	#endif
			std::memcpy(dest, src, nBytes);
	}
}

} // namespace oel
