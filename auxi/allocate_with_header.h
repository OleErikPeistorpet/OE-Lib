#pragma once

// Copyright 2017 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "type_traits.h"

#include <cstdint> // for uintptr_t


namespace oel::_detail
{
	struct DebugAllocationHeader
	{
		std::uintptr_t id;
		size_t   nObjects;
	};

	inline constexpr DebugAllocationHeader headerNoAllocation{};

	#define OEL_DEBUG_HEADER_OF(ptr)   (      (DebugAllocationHeader *)static_cast<void *>(ptr) - 1)
	#define OEL_DEBUG_HEADER_OF_C(ptr) ((const DebugAllocationHeader *)static_cast<const void *>(ptr) - 1)

	template< typename T >
	inline bool HasValidIndex(const T * arrayElem, const DebugAllocationHeader & h)
	{
		size_t index = arrayElem - reinterpret_cast<const T *>(&h + 1);
		return index < h.nObjects;
	}

	template< typename Alloc, typename Ptr >
	struct DebugAllocateWrapper
	{
	#if OEL_MEM_BOUND_DEBUG_LVL
		static constexpr size_t _valSz = sizeof(typename Alloc::value_type);
		static constexpr size_t sizeForHeader = ( sizeof(DebugAllocationHeader) + (_valSz - 1) ) / _valSz;
	#else
		static constexpr size_t sizeForHeader = 0;
	#endif

		static Ptr _addHeader(const Alloc & a, Ptr p)
		{
			p += sizeForHeader;

			auto const h = OEL_DEBUG_HEADER_OF(p);
			constexpr auto maxMinBits = ~(~std::uintptr_t{} >> 1) | 1u;
			new(h) DebugAllocationHeader{reinterpret_cast<std::uintptr_t>(&a) | maxMinBits, 0};

			return p;
		}

		static Ptr allocate(Alloc & a, size_t n)
		{
		#if OEL_MEM_BOUND_DEBUG_LVL
			n += sizeForHeader;
			Ptr p = a.allocate(n);
			return _addHeader(a, p);
		#else
			return a.allocate(n);
		#endif
		}

		static Ptr realloc(Alloc & a, Ptr p, size_t n)
		{
		#if OEL_MEM_BOUND_DEBUG_LVL
			if (p)
			{	// volatile to make sure the write isn't optimized away
				static_cast<volatile std::uintptr_t &>(OEL_DEBUG_HEADER_OF(p)->id) = 0;
				p -= sizeForHeader;
			}
			n += sizeForHeader;
			p = a.reallocate(p, n);
			return _addHeader(a, p);
		#else
			return a.reallocate(p, n);
		#endif
		}

		static void dealloc(Alloc & a, Ptr p, size_t n) noexcept(noexcept( a.deallocate(p, n) ))
		{
		#if OEL_MEM_BOUND_DEBUG_LVL
			static_cast<volatile std::uintptr_t &>(OEL_DEBUG_HEADER_OF(p)->id) = 0;
			p -= sizeForHeader;
			n += sizeForHeader;
		#endif
			a.deallocate(p, n);
		}
	};

	template< typename ContainerBase >
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

////////////////////////////////////////////////////////////////////////////////

	template< typename Ptr >
	struct DynarrBase
	{
		Ptr data;
		Ptr end;
		Ptr reservEnd;
	};
}