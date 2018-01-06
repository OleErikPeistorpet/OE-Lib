#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "../util.h"

namespace oel
{
namespace _detail
{
	template<typename, typename> struct DynarrBase;


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

		OEL_ALWAYS_INLINE static Header * HeaderOf(Ptr data)
		{	// Extra dereference and address-of in case of fancy pointer
			return &reinterpret_cast<Header &>(*data) - 1;
		}

		static void UpdateAfterMove(const ContainerBase & c)
		{
		#if OEL_MEM_BOUND_DEBUG_LVL >= 2
			if (c.data)
				HeaderOf(c.data)->container = &c;
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

			Header *const h = HeaderOf(p);
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
				HeaderOf(p)->id = 0;
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
		OEL_ALWAYS_INLINE AllocRefOptimized(Alloc &) {}

		Alloc Get() { return Alloc{}; }
	};
}

} // namespace oel
