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
	struct DebugAllocationHeader
	{
		std::uintptr_t id;
		size_t   nObjects;

		template<typename Ptr>
		OEL_ALWAYS_INLINE static DebugAllocationHeader * FromBody(Ptr p)
		{	// Extra dereference and address-of in case of fancy pointer
			return &reinterpret_cast<DebugAllocationHeader &>(*p) - 1;
		}
	};

	constexpr DebugAllocationHeader headerNoAllocation{0, 0};

	template<typename Alloc, typename Ptr>
	struct DebugAllocateWrapper
	{
	#if OEL_MEM_BOUND_DEBUG_LVL
		static constexpr size_t _valSz = sizeof(typename Alloc::value_type);
		static constexpr size_t sizeForHeader = (sizeof(DebugAllocationHeader) + (_valSz - 1)) / _valSz;
	#else
		static constexpr size_t sizeForHeader = 0;
	#endif

		static Ptr Allocate(Alloc & a, size_t n)
		{
		#if OEL_MEM_BOUND_DEBUG_LVL
			n += sizeForHeader;
			Ptr p = a.allocate(n);
			p += sizeForHeader;

			auto const h = DebugAllocationHeader::FromBody(p);
			constexpr auto maxMinBits = ~((std::uintptr_t)-1 >> 1) | 1U;
			h->id = reinterpret_cast<std::uintptr_t>(&a) | maxMinBits;
			h->nObjects = 0;

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
				DebugAllocationHeader::FromBody(p)->id = 0;
				p -= sizeForHeader;
				n += sizeForHeader;
			#endif
				a.deallocate(p, n);
			}
		}
	};

	template<typename ContainerBase>
	struct DebugSizeInHeaderUpdater
	{
	#if OEL_MEM_BOUND_DEBUG_LVL == 0
		DebugSizeInHeaderUpdater(ContainerBase &) {}
	#else
		ContainerBase & container;

		~DebugSizeInHeaderUpdater()
		{
			if (container.data)
			{
				auto h = DebugAllocationHeader::FromBody(container.data);
				h->nObjects = container.end - container.data;
			}
		}
	#endif
	};
}

} // namespace oel
