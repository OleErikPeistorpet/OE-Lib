#pragma once

// Copyright 2017 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "core_util.h"

#include <cstdint>  // for uintptr_t
#include <stdexcept>


namespace oel::_detail
{
	struct LengthError
	{
		[[noreturn]] static void raise()
		{
			constexpr auto what = "Going over dynarray max_size";
			OEL_THROW(std::length_error(what), what);
		}
	};

////////////////////////////////////////////////////////////////////////////////

	struct DebugAllocationHeader
	{
		std::uintptr_t id;
		ptrdiff_t      nObjects;
	};

	inline constexpr DebugAllocationHeader headerNoAllocation{};

	inline DebugAllocationHeader * DebugHeaderOf(void * p)
	{
		return static_cast<DebugAllocationHeader *>(p) - 1;
	}

	template< typename T >
	inline bool HasValidIndex(const T * arrayElem, const DebugAllocationHeader & h)
	{
		auto index = arrayElem - reinterpret_cast<const T *>(&h + 1);
		return static_cast<size_t>(index) < static_cast<size_t>(h.nObjects);
	}

	template< typename Alloc, typename Ptr >
	struct DebugAllocateWrapper
	{
	#if OEL_MEM_BOUND_DEBUG_LVL == 0
		static constexpr size_t sizeForHeader{};
	#else
		static constexpr auto _valSize      = sizeof(typename Alloc::value_type);
		static constexpr auto sizeForHeader = ( sizeof(DebugAllocationHeader) + (_valSize - 1) ) / _valSize;

		static Ptr _addHeader(const Alloc & a, Ptr p)
		{
			p += sizeForHeader;

			auto const h = _detail::DebugHeaderOf(p);
			// Take address, set highest and lowest bits for a hopefully unique bit pattern to compare later
			constexpr auto maxMinBits = ~(~std::uintptr_t{} >> 1) | 1u;
			::new(h) DebugAllocationHeader{reinterpret_cast<std::uintptr_t>(&a) | maxMinBits, 0};

			return p;
		}
	#endif

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
		OEL_ALWAYS_INLINE DebugSizeInHeaderUpdater(ContainerBase &) {}
	#else
		ContainerBase & container;

		~DebugSizeInHeaderUpdater()
		{
			if (container.data)
			{
				auto h = _detail::DebugHeaderOf(container.data);
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



	template< typename Alloc >
	struct ToDynarrPartial
	{
		Alloc _a;

		template< typename Range >
		friend auto operator |(Range && r, ToDynarrPartial t)
		{
			return dynarray(static_cast<Range &&>(r), std::move(t)._a);
		}
	};
}