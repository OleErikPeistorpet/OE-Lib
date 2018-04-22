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
	inline void MemcpyMaybeNull(void * dest, const void * src, size_t nBytes)
	{	// memcpy(nullptr, nullptr, 0) is UB. The trouble is that checking can have significant performance hit.
		// GCC 4.9 and up known to need the check in some cases
	#if (!defined __GNUC__ and !defined _MSC_VER) or OEL_GCC_VERSION >= 409 or _MSC_VER >= 2000
		if (nBytes > 0)
	#endif
			std::memcpy(dest, src, nBytes);
	}


	template<typename T, typename Alloc, bool B, typename... Args>
	inline auto ConstructImpl(bool_constant<B>, Alloc & a, T * p, Args &&... args)
	 -> decltype( a.construct(p, std::forward<Args>(args)...) )
	            { a.construct(p, std::forward<Args>(args)...); }
	// void * worse match than T *
	template<typename T, typename Alloc, typename... Args>
	inline void ConstructImpl(std::true_type, Alloc &, void * p, Args &&... args)
	{	// T constructible from Args
		::new(p) T(std::forward<Args>(args)...);
	}
	// list-initialization
	template<typename T, typename Alloc, typename... Args>
	inline void ConstructImpl(std::false_type, Alloc &, void * p, Args &&... args)
	{
		::new(p) T{std::forward<Args>(args)...};
	}

	template<typename Alloc, typename T, typename... Args>
	OEL_ALWAYS_INLINE inline void Construct(Alloc & a, T * p, Args &&... args)
	{
		ConstructImpl<T>(std::is_constructible<T, Args...>(),
		                 a, p, std::forward<Args>(args)...);
	}


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

	template<typename Alloc, typename CntigusIter, typename T,
	         enable_if< can_memmove_with<T *, CntigusIter>::value > = 0>
	void UninitCopy(CntigusIter src, T * dFirst, T * dLast, Alloc &)
	{
		_detail::MemcpyMaybeNull(dFirst, to_pointer_contiguous(src), sizeof(T) * (dLast - dFirst));
	}

	template<typename Alloc, typename InputIter, typename T, typename FuncTakingLast = NoOp,
	         enable_if< !can_memmove_with<T *, InputIter>::value > = 0>
	InputIter UninitCopy(InputIter src, T * dest, T *const dLast, Alloc & alloc, FuncTakingLast extraCleanup = {})
	{
		T *const dFirst = dest;
		OEL_TRY_
		{
			while (dest != dLast)
			{
				_detail::Construct(alloc, dest, *src);
				++src; ++dest;
			}
		}
		OEL_CATCH_ALL
		{
			_detail::Destroy(dFirst, dest);
			extraCleanup(dLast);
			OEL_WHEN_EXCEPTIONS_ON(throw);
		}
		return src;
	}


	template<typename Alloc>
	struct UninitFill
	{
		template<typename T, typename... Arg>
		void operator()(T * first, T *const last, Alloc & alloc, const Arg &... arg)
		{
			T *const init = first;
			OEL_TRY_
			{
				for (; first != last; ++first)
					_detail::Construct(alloc, first, arg...);
			}
			OEL_CATCH_ALL
			{
				_detail::Destroy(init, first);
				OEL_WHEN_EXCEPTIONS_ON(throw);
			}
		}

		template<typename T, enable_if< sizeof(T) == 1 and std::is_integral<T>::value > = 0>
		void operator()(T * first, T * last, Alloc &, T val)
		{
			std::memset(first, val, last - first);
		}

		template<typename T, enable_if< std::is_trivial<T>::value > = 0>
		void operator()(T * first, T * last, Alloc &)
		{
			std::memset(first, 0, sizeof(T) * (last - first));
		}
	};

	template<typename Alloc, typename T>
	inline void UninitDefaultConstruct(T *const first, T *const last, Alloc & a)
	{
		OEL_CONST_COND if (!is_trivially_default_constructible<T>::value)
			UninitFill<Alloc>{}(first, last, a);
	}
}

} // namespace oel
