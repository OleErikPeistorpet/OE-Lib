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
	};

	constexpr DebugAllocationHeader headerNoAllocation{0, 0};

	#define OEL_DEBUG_HEADER_OF(ptr)  \
		(reinterpret_cast<_detail::DebugAllocationHeader *>(_detail::ToAddress(ptr)) - 1)

	template<typename T, typename Ptr>
	inline bool HasValidIndex(Ptr arrayElem, const DebugAllocationHeader & h)
	{
		size_t index = arrayElem - reinterpret_cast<const T *>(&h + 1);
		return index < h.nObjects;
	}

	template<typename Alloc, typename Ptr>
	struct DebugAllocateWrapper
	{
	#if OEL_MEM_BOUND_DEBUG_LVL
		static constexpr size_t _valSz = sizeof(typename Alloc::value_type);
		static constexpr size_t sizeForHeader = ( sizeof(DebugAllocationHeader) + (_valSz - 1) ) / _valSz;
	#else
		static constexpr size_t sizeForHeader = 0;
	#endif

		static Ptr Allocate(Alloc & a, size_t n)
		{
		#if OEL_MEM_BOUND_DEBUG_LVL
			n += sizeForHeader;
			Ptr p = a.allocate(n);
			p += sizeForHeader;

			auto const h = OEL_DEBUG_HEADER_OF(p);
			constexpr auto maxMinBits = ~((std::uintptr_t)-1 >> 1) | 1U;
			new(h) DebugAllocationHeader{reinterpret_cast<std::uintptr_t>(&a) | maxMinBits, 0};

			return p;
		#else
			return a.allocate(n);
		#endif
		}

		static void Deallocate(Alloc & a, Ptr p, size_t n)
		{
		#if OEL_MEM_BOUND_DEBUG_LVL
			OEL_DEBUG_HEADER_OF(p)->id = 0;
			p -= sizeForHeader;
			n += sizeForHeader;
		#endif
			a.deallocate(p, n);
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
				auto h = OEL_DEBUG_HEADER_OF(container.data);
				h->nObjects = container.end - container.data;
			}
		}
	#endif
	};
}

} // namespace oel
