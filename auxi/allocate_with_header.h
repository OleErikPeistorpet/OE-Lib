#pragma once

// Copyright 2017 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "contiguous_iterator_to_ptr.h"

#include <cstdint> // for uintptr_t

namespace oel
{
namespace _detail
{
	template<typename Alloc, typename Arg>
	decltype( std::declval<Alloc &>().construct((typename Alloc::value_type *)0, std::declval<Arg>()),
		true_type() )
		HasConstructTest(int);

	template<typename, typename>
	false_type HasConstructTest(long);

	template<typename Alloc, typename Arg>
	using AllocHasConstruct = decltype( HasConstructTest<Alloc, Arg>(0) );


	template<typename Alloc>
	decltype( std::declval<Alloc &>().reallocate((typename Alloc::value_type *)0, size_t{}),
		true_type() )
		HasReallocTest(int);

	template<typename>
	false_type HasReallocTest(long);

	template<typename Alloc>
	using AllocHasReallocate = decltype( HasReallocTest<Alloc>(0) );



	struct DebugAllocationHeader
	{
		std::uintptr_t id;
		size_t   nObjects;
	};

	constexpr DebugAllocationHeader headerNoAllocation{0, 0};

	#define OEL_DEBUG_HEADER_OF(ptr) ((DebugAllocationHeader *)static_cast<void *>(ptr) - 1)

	template<typename T>
	inline bool HasValidIndex(const T * arrayElem, const DebugAllocationHeader & h)
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

		static Ptr _addHeader(const Alloc & a, Ptr p)
		{
			p += sizeForHeader;

			auto const h = OEL_DEBUG_HEADER_OF(p);
			constexpr auto maxMinBits = ~(std::uintptr_t(-1) >> 1) | 1u;
			new(h) DebugAllocationHeader{reinterpret_cast<std::uintptr_t>(&a) | maxMinBits, 0};

			return p;
		}

		static Ptr Allocate(Alloc & a, size_t n)
		{
		#if OEL_MEM_BOUND_DEBUG_LVL
			n += sizeForHeader;
			Ptr p = a.allocate(n);
			return _addHeader(a, p);
		#else
			return a.allocate(n);
		#endif
		}

		static Ptr Realloc(Alloc & a, Ptr p, size_t n)
		{
		#if OEL_MEM_BOUND_DEBUG_LVL
			if (p)
			{	// header already present
				OEL_DEBUG_HEADER_OF(p)->id = 0;
				p -= sizeForHeader;
			}
			n += sizeForHeader;
			p = a.reallocate(p, n);
			return _addHeader(a, p);
		#else
			return a.reallocate(p, n);
		#endif
		}

		static void Dealloc(Alloc & a, Ptr p, size_t n) noexcept
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

////////////////////////////////////////////////////////////////////////////////

	template<typename Ptr>
	struct DynarrBase
	{
		Ptr data;
		Ptr end;
		Ptr reservEnd;
	};
}

} // namespace oel
