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

	#if OEL_MEM_BOUND_DEBUG_LVL
		static constexpr size_t _valSz = sizeof(typename Alloc::value_type);
		static constexpr size_t sizeForHeader = (sizeof(Header) + (_valSz - 1)) / _valSz;
	#else
		static constexpr size_t sizeForHeader = 0;
	#endif

		OEL_ALWAYS_INLINE static Header * HeaderOf(Ptr data)
		{	// Extra dereference and address-of in case of fancy pointer
			return &reinterpret_cast<Header &>(*data) - 1;
		}

		template<typename Iterator>
		static Iterator MakeIterator(Ptr const pos, OEL_MAYBE_UNUSED const ContainerBase & container)
		{
		#if OEL_MEM_BOUND_DEBUG_LVL
			const Header * h = nullptr;
			OEL_MAYBE_UNUSED std::uintptr_t id;
			if (container.data)
			{
				h = HeaderOf(container.data);
				id = h->id;
			}
			else
			{	id = reinterpret_cast<std::uintptr_t>(&container);
			}
			return {pos, h
				#if OEL_MEM_BOUND_DEBUG_LVL >= 2
					, id
				#endif
				};
		#else
			return pos;
		#endif
		}

		static void UpdateAfterMove(OEL_MAYBE_UNUSED const ContainerBase & c)
		{
		#if OEL_MEM_BOUND_DEBUG_LVL
			if (c.data)
				HeaderOf(c.data)->container = &c;
		#endif
		}

		template<typename Owner>
		static Ptr Allocate(Owner & a, size_t n)
		{
	#if OEL_MEM_BOUND_DEBUG_LVL
			n += sizeForHeader;
			Ptr p = a.allocate(n);
			p += sizeForHeader;

			Header *const h = HeaderOf(p);
			h->container = &a;
		#if OEL_MEM_BOUND_DEBUG_LVL >= 2
			constexpr auto maxMinBits = ~((std::uintptr_t)-1 >> 1) | 1U;
			h->id = reinterpret_cast<std::uintptr_t>(&a) | maxMinBits;
		#else
			h->id = reinterpret_cast<std::uintptr_t>(h);
		#endif

			return p;
	#else
			return a.allocate(n);
	#endif
		}

		static void Deallocate(Alloc & a, Ptr p, size_t n)
		{
			if (p)
			{
			#if OEL_MEM_BOUND_DEBUG_LVL
				HeaderOf(p)->id = 0;
				p -= sizeForHeader;
				n += sizeForHeader;
			#endif
				a.deallocate(p, n);
			}
		}
	};


	template< typename Alloc,
		bool = std::is_empty<Alloc>::value and std::is_default_constructible<Alloc>::value >
	struct AllocRefOptimized
	{
		Alloc & alloc;

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
