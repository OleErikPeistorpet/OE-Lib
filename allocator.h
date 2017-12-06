#pragma once

// Copyright 2016 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "auxi/core_util.h"

#include <algorithm> // for max
#include <cstdint>  // for uintptr_t

#ifndef OEL_HAS_FREE_SIZED
	#if __STDC_VERSION_STDLIB_H__ < 202311
	#define OEL_HAS_FREE_SIZED  0
	#else
	#define OEL_HAS_FREE_SIZED  1
	#endif
#endif

#if !defined OEL_HAS_SDALLOCX and !OEL_HAS_FREE_SIZED
	#if __has_include("jemalloc/jemalloc.h")
	#include "jemalloc/jemalloc.h"

	#define OEL_HAS_SDALLOCX  1

	#elif __has_include("tcmalloc/malloc_extension.h")
	#include "tcmalloc/malloc_extension.h"

	#define OEL_HAS_SDALLOCX  1
	#else
	#define OEL_HAS_SDALLOCX  0
	#endif
#endif

#ifndef OEL_NEW_HANDLER
#define OEL_NEW_HANDLER  !OEL_HAS_EXCEPTIONS
#endif

/** @file
*/

namespace oel
{

//! Aligns memory to the greater of alignof(T) and MinAlign, and has `reallocate` function
/**
* Either throws std::bad_alloc or calls standard new_handler on failure, depending on value of OEL_NEW_HANDLER. */
template< typename T, size_t MinAlign >
struct allocator
{
	using value_type = T;

	using propagate_on_container_move_assignment = std::true_type;

	static constexpr bool   can_reallocate() noexcept { return is_trivially_relocatable<T>::value; }

	static constexpr size_t alignment_value() noexcept   { return std::max(alignof(T), MinAlign); }

	static constexpr size_t max_size() noexcept
		{
			constexpr auto n = SIZE_MAX - (alignment_value() > OEL_MALLOC_ALIGNMENT ? alignment_value() : 0);
			return n / sizeof(T);
		}

	//! `count` greater than max_size() causes overflow and undefined behavior
	[[nodiscard]] static T * allocate(size_t count);

	//! Like C23 `realloc` except for failure handling (same as allocate, throws bad_alloc or calls new_handler)
	/** @pre If newCount is zero or greater than max_size(), the behavior is undefined  */
	[[nodiscard]] static T * reallocate(T * ptr, size_t newCount);

	static void              deallocate(T * ptr, size_t count) noexcept;

	allocator() = default;

	template< typename U >  OEL_ALWAYS_INLINE
	constexpr allocator(allocator<U, MinAlign>) noexcept {}

	template< typename U >
	struct rebind
	{
		using other = allocator<U, MinAlign>;
	};

	friend constexpr bool operator==(allocator, allocator) noexcept  { return true; }
	friend constexpr bool operator!=(allocator, allocator) noexcept  { return false; }
};



////////////////////////////////////////////////////////////////////////////////
//
// Implementation only in rest of the file


namespace _detail
{
	inline constexpr auto allocFailMsg = "No memory oel::allocator";

	struct BadAlloc
	{
		[[noreturn]] static void raise()
		{
			OEL_THROW(std::bad_alloc{}, allocFailMsg);
		}
	};

	template< size_t Align >
	void * AlignAndStore(void *const orig) noexcept
	{
		if (orig)
		{
			auto i = reinterpret_cast<std::uintptr_t>(orig) + Align;
			i &= ~(Align - 1);
			auto const p = reinterpret_cast<void *>(i);

			static_cast<void **>(p)[-1] = orig;
			return p;
		}
		else
		{	return orig;
		}
	}

	template< size_t Align >
	struct Malloc
	{
	#ifdef __GNUC__
		__attribute__(( assume_aligned(Align) ))
	#endif
		static void * call(size_t const nBytes)
		{
			if constexpr (Align > OEL_MALLOC_ALIGNMENT)
			{
				auto p = ::malloc(nBytes + Align);
				return AlignAndStore<Align>(p);
			}
			else
			{	return ::malloc(nBytes);
			}
		}
	};

	template< size_t Align >
	struct Realloc
	{
	#ifdef __GNUC__
		__attribute__(( assume_aligned(Align) ))
	#endif
		static void * call(size_t const nBytes, void * old)
		{
			if constexpr (Align > OEL_MALLOC_ALIGNMENT)
			{
				if (old)
					old = static_cast<void **>(old)[-1];

				auto p = ::realloc(old, nBytes + Align);
				return AlignAndStore<Align>(p);
			}
			else
			{	return ::realloc(old, nBytes);
			}
		}
	};

	template< size_t Align >
	void Free(void * p, size_t const nBytes) noexcept
	{
		if constexpr (Align > OEL_MALLOC_ALIGNMENT)
		{
			if (p)
				p = static_cast<void **>(p)[-1];
			else
				return;
		}
	#if OEL_HAS_FREE_SIZED
		::free_sized(p, nBytes);
	#elif OEL_HAS_SDALLOCX
		::sdallocx(p, nBytes, {});
	#else
		::free(p);
		(void) nBytes;
	#endif
	}

	template< typename AllocFunc, typename... Ptr > // should pass void * for fewer template instantiations
#ifdef _MSC_VER
	__declspec(restrict)
#elif __GNUC__
	[[gnu::malloc]]
#endif
	void * AllocAndHandleFail(size_t const nBytes, Ptr const... old)
	{
		if (nBytes > 0) // could be removed for implementations known not to return null
		{
		#if OEL_NEW_HANDLER
			for (;;)
			{
				auto p = AllocFunc::call(nBytes, old...);
				if (p)
					return p;

				auto const handler = std::get_new_handler();
				if (!handler)
					OEL_ABORT(allocFailMsg);

				(*handler)();
	}
		#else
			auto p = AllocFunc::call(nBytes, old...);
			if (p)
				return p;
			else
				BadAlloc::raise();
#endif
}
		else
		{	return nullptr;
		}
	}
}

template< typename T, size_t MinAlign >
T * allocator<T, MinAlign>::allocate(size_t count)
{
#if OEL_MEM_BOUND_DEBUG_LVL >= 2
	OEL_ASSERT(count <= max_size());
#endif
	using F = _detail::Malloc<alignment_value()>;
	return static_cast<T *>( _detail::AllocAndHandleFail<F>(sizeof(T) * count) );
}

template< typename T, size_t MinAlign >
T * allocator<T, MinAlign>::reallocate(T * ptr, size_t count)
{
#if OEL_MEM_BOUND_DEBUG_LVL >= 2
	OEL_ASSERT(count <= max_size());
#endif
	using F = _detail::Realloc<alignment_value()>;
	void * vp{ptr};
	return static_cast<T *>( _detail::AllocAndHandleFail<F>(sizeof(T) * count, vp) );
}

template< typename T, size_t MinAlign >
inline void allocator<T, MinAlign>::deallocate(T * ptr, size_t count) noexcept
{
	_detail::Free<alignment_value()>(ptr, sizeof(T) * count);
}

} // namespace oel
