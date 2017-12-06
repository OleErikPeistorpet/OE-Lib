#pragma once

// Copyright 2016 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "core_util.h"

#include <cstdint>  // for uintptr_t
#include <new>
#include <stdlib.h> // for malloc, free_sized, etc.

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


namespace oel::_detail
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
		if( orig )
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
		[[ gnu::assume_aligned(Align) ]]
	#endif
		static void * call(size_t const nBytes)
		{
			if constexpr( Align > OEL_MALLOC_ALIGNMENT )
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
		[[ gnu::assume_aligned(Align) ]]
	#endif
		static void * call(size_t const nBytes, void * old)
		{
			if constexpr( Align > OEL_MALLOC_ALIGNMENT )
			{
				if( old )
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
		if constexpr( Align > OEL_MALLOC_ALIGNMENT )
		{
			if( p )
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

	template< typename AllocFunc, bool CheckZero = true, typename... Ptr >
#ifdef _MSC_VER
	__declspec(restrict)
#endif
	void * AllocAndHandleFail(size_t const nBytes, Ptr const... old) // should pass void * for fewer template instantiations
	{
		auto const zeroSize = CheckZero ? (nBytes == 0) : false;
	#if OEL_NEW_HANDLER
		if( !zeroSize )
		{
			for( ;; )
			{
				auto p = AllocFunc::call(nBytes, old...);
				if( p )
					return p;

				auto const handler = std::get_new_handler();
				if( handler )
					handler();
				else
					OEL_ABORT(allocFailMsg);
			}
		}
		else
		{	return nullptr;
		}
	#else
		auto p = AllocFunc::call(nBytes, old...);
		if( p or zeroSize )
			return p;
		else
			BadAlloc::raise();
	#endif
	}
}