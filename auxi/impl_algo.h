#pragma once

// Copyright 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "../util.h"

#include <cstring>
#include <stdexcept>


namespace oel::_detail
{
	struct Throw
	{	// Exception throwing has been split out from templates to avoid bloat
		[[noreturn]] static void outOfRange(const char * what)
		{
			OEL_THROW(std::out_of_range(what), what);
		}

		[[noreturn]] static void lengthError(const char * what)
		{
			OEL_THROW(std::length_error(what), what);
		}
	};

////////////////////////////////////////////////////////////////////////////////

	template< typename T >
	void Destroy([[maybe_unused]] T *const p, [[maybe_unused]] size_t const n) noexcept
	{
		if constexpr (!std::is_trivially_destructible_v<T>) // for speed with non-optimized builds
		{
			for (size_t i{}; i < n; ++i)
				p[i].~T();
		}
	}


#if !defined _MSC_VER or _MSC_VER >= 2000
	#define OEL_CHECK_NULL_MEMCPY  1
#endif
	template< typename ContiguousIter >
	void MemcpyCheck(ContiguousIter const src, size_t const nElems, void *const dest)
	{	// memcpy(nullptr, nullptr, 0) is UB. Unfortunately, checking can have significant performance hit,
		// probably due to functions no longer being inlined. GCC known to need the check in some cases
	#if OEL_CHECK_NULL_MEMCPY or OEL_MEM_BOUND_DEBUG_LVL
		if (nElems != 0)
	#endif
		{	// Dereference to detect out of range errors if the iterator has internal check
		#if OEL_MEM_BOUND_DEBUG_LVL
			(void) *src;
			(void) *(src + (nElems - 1));
		#endif
			std::memcpy(dest, to_pointer_contiguous(src), sizeof(*src) * nElems);
		}
	}


	template< typename T >
	void Relocate(T *__restrict src, size_t const n, T *__restrict dest)
	{
		if constexpr (is_trivially_relocatable<T>::value)
		{
		#if OEL_CHECK_NULL_MEMCPY
			if (src)
		#endif
			{	std::memcpy(
					static_cast<void *>(dest),
					static_cast<const void *>(src),
					sizeof(T) * n );
			}
		}
		else
		{
		#if OEL_HAS_EXCEPTIONS
			static_assert( std::is_nothrow_move_constructible_v<T>,
				"dynarray requires that T is noexcept move constructible or trivially relocatable" );
		#endif
			for (size_t i{}; i < n; ++i)
			{
				::new(static_cast<void *>(dest + i)) T( std::move(src[i]) );
				src[i].~T();
			}
		}
	}
	#undef OEL_CHECK_NULL_MEMCPY


	template< typename Alloc, typename InputIter, typename T >
	InputIter UninitCopy(InputIter src, size_t const n, T *__restrict dest, [[maybe_unused]] Alloc & allo)
	{
		if constexpr (can_memmove_with<T *, InputIter>)
		{
			_detail::MemcpyCheck(src, n, dest);
			return src + n;
		}
		else
		{	size_t i{};
			OEL_TRY_
			{
				while (i < n)
				{
					std::allocator_traits<Alloc>::construct(allo, dest + i, *src);
					++i; ++src;
				}
			}
			OEL_CATCH_ALL
			{
				_detail::Destroy(dest, i);
				OEL_RETHROW;
			}
			return src;
		}
	}


	template< typename Alloc >
	struct UninitFill
	{
		template< typename T >
		static constexpr auto isByte =
			sizeof(T) == 1 and (std::is_integral_v<T> or std::is_enum_v<T>);

		template< typename T, typename... Args,
		          enable_if< !isByte<T> > = 0
		>
		static void call(T *__restrict p, size_t const n, Alloc & allo, const Args &... args)
		{
			size_t i{};
			OEL_TRY_
			{
				for (; i < n; ++i)
					std::allocator_traits<Alloc>::construct(allo, p + i, args...);
			}
			OEL_CATCH_ALL
			{
				_detail::Destroy(p, i);
				OEL_RETHROW;
			}
		}

		template< typename T,
		          enable_if< isByte<T> > = 0
		>
		static void call(T * p, size_t n, Alloc &, T val)
		{
			std::memset(p, static_cast<int>(val), n);
		}

		template< typename T,
		          enable_if< std::is_trivial_v<T> > = 0
		>
		static void call(T * p, size_t n, Alloc &)
		{
			std::memset(p, 0, sizeof(T) * n);
		}
	};

	struct UninitDefaultConstruct
	{
		template< typename Alloc, typename T >
		static void call(T *__restrict p, size_t const n, Alloc & a)
		{
			if constexpr (!std::is_trivially_default_constructible_v<T>)
			{
				UninitFill<Alloc>::call(p, n, a);
			}
			else
			{	(void) p; (void) n; (void) a; // avoid VC++ 2017 warning C4100
			}
		}
	};



	// If r is sized or multi-pass, returns element count as size_t, else end(r)
	template< typename Range, typename... None >
	inline auto CountOrEnd(Range & r, None...)
	{
		if constexpr (iter_is_forward< iterator_t<Range> >)
		{
			size_t n{};
			auto it = begin(r);
			auto const l = end(r);
			while (it != l) { ++it; ++n; }

			return n;
		}
		else
		{	return end(r);
		}
	}

	template< typename Range >
	auto CountOrEnd(Range & r)
	->	decltype( static_cast<size_t>(_detail::Size(r)) )
	{	return    static_cast<size_t>(_detail::Size(r)); }
}