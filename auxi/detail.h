#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "../util.h"

#include <cstdint> // for uintptr_t

namespace oel
{
namespace _detail
{
	template<typename, typename> struct DynarrBase;


#if defined __GLIBCXX__ && __GNUC__ == 4
	template<typename T>
	using is_trivially_default_constructible = bool_constant< __has_trivial_constructor(T)
		#ifdef __INTEL_COMPILER
			|| __is_pod(T)
		#endif
		>;

	template<typename T>
	using is_trivially_destructible = bool_constant< __has_trivial_destructor(T)
		#ifdef __INTEL_COMPILER
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


	template<typename ContainerPtr>
	struct DebugAllocationHeader
	{
		ContainerPtr container;
		std::uintptr_t id;
	};

	// Extra dereference and address-of in case of fancy pointer
	#define OEL_ALLOCATION_HEADER(headerT, ptr)  (&reinterpret_cast<headerT &>(*ptr) - 1)

	template<typename ContainerBase, typename Alloc, typename Ptr>
	struct DebugAllocateWrapper
	{
		using CtnrConstPtr = typename std::pointer_traits<Ptr>::template rebind<ContainerBase const>;
		using Header = DebugAllocationHeader<CtnrConstPtr>;

		enum {
		#if OEL_MEM_BOUND_DEBUG_LVL >= 2
			_valSz = sizeof(typename Alloc::value_type),
			sizeForHeader = (sizeof(Header) + (_valSz - 1)) / _valSz
		#else
			sizeForHeader = 0
		#endif
		};

		static void UpdateAfterMove(const ContainerBase & c)
		{
		#if OEL_MEM_BOUND_DEBUG_LVL >= 2
			if (c.data)
				OEL_ALLOCATION_HEADER(Header, c.data)->container = &c;
		#else
			(void) c;
		#endif
		}

		template<typename Owner>
		static Ptr Allocate(Owner & a, size_t n)
		{
		#if OEL_MEM_BOUND_DEBUG_LVL >= 2
			n += sizeForHeader;
			Ptr p = a.allocate(n);
			p += sizeForHeader;

			Header *const h = OEL_ALLOCATION_HEADER(Header, p);
			h->container = &a;
			h->id = reinterpret_cast<std::uintptr_t>(&a);

			return p;
		#else
			return a.allocate(n);
		#endif
		}

		static void Deallocate(Alloc & a, Ptr p, size_t n)
		{
			if (p)
			{
			#if OEL_MEM_BOUND_DEBUG_LVL >= 2
				OEL_ALLOCATION_HEADER(Header, p)->id = 0;
				p -= sizeForHeader;
				n += sizeForHeader;
			#endif
				a.deallocate(p, n);
			}
		}
	};


	template< typename Alloc,
		bool = std::is_empty<Alloc>::value && std::is_default_constructible<Alloc>::value >
	struct AllocRefOptimized
	{
		Alloc & alloc;
	#ifdef _MSC_VER
		void operator =(AllocRefOptimized) = delete;
	#endif
		AllocRefOptimized(Alloc & a) : alloc(a) {}

		Alloc & Get() { return alloc; }
	};

	template<typename Alloc>
	struct AllocRefOptimized<Alloc, true>
	{
		AllocRefOptimized(Alloc &) {}

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
		_detail::UninitFillImpl(bool_constant<std::is_integral<T>::value && sizeof(T) == 1>(),
		                        first, last, alloc, arg...);
	}

	template<typename Alloc, typename T> inline
	void UninitDefaultConstruct(T *const first, T *const last, Alloc & alloc)
	{
		OEL_CONST_COND if (!is_trivially_default_constructible<T>::value)
			_detail::UninitFillImpl(false_type{}, first, last, alloc);
	}
}

} // namespace oel
