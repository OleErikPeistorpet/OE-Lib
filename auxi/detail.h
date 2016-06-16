#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "core_util.h"

#include <string.h> // for memset

namespace oel
{
namespace _detail
{
	template<typename> struct DynarrBase;


#if __GLIBCXX__ && __GNUC__ == 4
	template<typename T>
	using is_trivially_default_constructible = bool_constant< __has_trivial_constructor(T)
		#if __INTEL_COMPILER
			|| __is_pod(T)
		#endif
		>;

	template<typename T>
	using is_trivially_destructible = bool_constant< __has_trivial_destructor(T)
		#if __INTEL_COMPILER
			|| __is_pod(T)
		#endif
		>;
#else
	using std::is_trivially_default_constructible;
	using std::is_trivially_destructible;
#endif


	template<typename T> struct AssertTrivialRelocate
	{
		static_assert(is_trivially_relocatable<T>::value,
			"The function requires trivially relocatable T, see declaration of is_trivially_relocatable");
	};


	template<typename Alloc, bool = std::is_empty<Alloc>::value>
	struct AllocRefOptimizeEmpty
	{
		Alloc & alloc;
	#if _MSC_VER
		void operator =(AllocRefOptimizeEmpty) = delete;
	#endif
		AllocRefOptimizeEmpty(Alloc & a) : alloc(a) {}

		Alloc & Get() { return alloc; }
	};

	template<typename Alloc>
	struct AllocRefOptimizeEmpty<Alloc, true>
	{
		void operator =(AllocRefOptimizeEmpty) = delete;

		AllocRefOptimizeEmpty(Alloc &) {}

		Alloc Get() { return Alloc{}; }
	};


	template<typename T>
	void Destroy(T * first, T *const last) noexcept
	{	// first > last is OK, does nothing
		OEL_CONST_COND if (!is_trivially_destructible<T>::value) // for speed with optimizations off (debug build)
		{
			for (; first < last; ++first)
				first-> ~T();
		}
	}


	struct NoOp
	{	void operator()(...) const {}
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

	template<typename InputIter, typename T>
	InputIter UninitCopyA(InputIter src, T * dest, T *const dLast)
	{
		allocator<T> a;
		return _detail::UninitCopy(src, dest, dLast, a);
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
		::memset(first, val, last - first);
	}

	template<typename Alloc, typename T, typename... Arg> inline
	void UninitFill(T *const first, T *const last, Alloc & alloc, const Arg &... arg)
	{
		// TODO: investigate libstdc++ std::fill
		_detail::UninitFillImpl(bool_constant<std::is_integral<T>::value && sizeof(T) == 1>(),
								first, last, alloc, arg...);
	}

	template<typename T, typename... Arg> inline
	void UninitFillA(T *const first, T *const last, const Arg &... arg)
	{
		allocator<T> a;
		UninitFill(first, last, a, arg...);
	}

	template<typename Alloc, typename T> inline
	void UninitDefaultConstruct(T *const first, T *const last, Alloc & alloc)
	{
		OEL_CONST_COND if (!is_trivially_default_constructible<T>::value)
			_detail::UninitFillImpl(false_type{}, first, last, alloc);
	}

	template<typename T> inline
	void UninitDefaultConstructA(T *const first, T *const last)
	{
		allocator<T> a;
		UninitDefaultConstruct(first, last, a);
	}
}

} // namespace oel
