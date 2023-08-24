#pragma once

// Copyright 2017 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "type_traits.h"

#include <cstdint> // for uintptr_t


namespace oel::_detail
{
	template< typename Ptr >
	struct DynarrBase
	{
		Ptr    data;
		size_t size;
		size_t capacity;
	};

////////////////////////////////////////////////////////////////////////////////

	struct DebugAllocationHeader
	{
		std::uintptr_t id;
		size_t   nObjects;
	};

	inline constexpr DebugAllocationHeader headerNoAllocation{};

	OEL_ALWAYS_INLINE inline auto DebugHeaderOf(void * data)
	{
		return static_cast<DebugAllocationHeader *>(data) - 1;
	}
	OEL_ALWAYS_INLINE inline auto DebugHeaderOf(const void * data)
	{
		return static_cast<const DebugAllocationHeader *>(data) - 1;
	}

	template< typename T >
	inline bool HasValidIndex(const T * arrayElem, const DebugAllocationHeader & h)
	{
		const void * p{&h + 1};
		size_t index = arrayElem - static_cast<const T *>(p);
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

			auto const h = _detail::DebugHeaderOf(p);
			// Take address, set highest and lowest bits for a hopefully unique bit pattern to compare later
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
				static_cast<volatile std::uintptr_t &>(_detail::DebugHeaderOf(p)->id) = 0;
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
			static_cast<volatile std::uintptr_t &>(_detail::DebugHeaderOf(p)->id) = 0;
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
				_detail::DebugHeaderOf(container.data)->nObjects = container.size;
		}
	#endif
	};
}