#pragma once

// Copyright 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "contiguous_iterator_to_ptr.h"
#include "../util.h"  // for as_unsigned

#include <cstring>
#include <memory> // for allocator_traits


namespace oel::_detail
{
	template< typename T >
	void Destroy([[maybe_unused]] T *const p, [[maybe_unused]] size_t const n) noexcept
	{
		if constexpr( !std::is_trivially_destructible_v<T> ) // for speed with non-optimized builds
		{
			for( size_t i{}; i != n; ++i )
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
		if( nElems != 0 )
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
	void Relocate(T *__restrict src, size_t const n, T *__restrict dest) noexcept
	{
		if constexpr( is_trivially_relocatable<T>::value )
		{
		#if OEL_CHECK_NULL_MEMCPY
			if( src )
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
			for( size_t i{}; i != n; ++i )
			{
				::new(static_cast<void *>(dest + i)) T( std::move(src[i]) );
				src[i].~T();
			}
		}
	}
	#undef OEL_CHECK_NULL_MEMCPY


	struct ValueInit
	{
		template< typename Alloc, typename T >
		static void call(T *__restrict first, size_t const n, [[maybe_unused]] Alloc a)
		{
			if constexpr( std::is_trivially_default_constructible_v<T> )
			{
				void * p{first};  // silence -Wclass-memaccess
				std::memset(p, 0, sizeof(T) * n);
			}
			else
			{	size_t i{};
				OEL_TRY_
				{
					for( ; i != n; ++i )
						std::allocator_traits<Alloc>::construct(a, first + i);
				}
				OEL_CATCH_ALL
				{
					_detail::Destroy(first, i);
					OEL_RETHROW;
				}
			}
		}
	};

	struct DefaultInit
	{
		template< typename Alloc, typename T >
		static void call(T *__restrict p, size_t const n, Alloc & a)
		{
			if constexpr( !std::is_trivially_default_constructible_v<T> )
			{
				ValueInit::call(p, n, a);
			}
			else
			{	(void) p; (void) n; (void) a;
			}
		}
	};



	template< typename Range >
	inline constexpr auto rangeIsForwardOrSized =
		iter_is< iterator_t<Range>, std::forward_iterator_tag >
		or range_is_sized<Range>;

	// Used only if rangeIsForwardOrSized
	template< typename Range >
	auto UDist(Range & r)
	{
		if constexpr( range_is_sized<Range> )
		{
			return as_unsigned(_detail::Size(r));
		}
		else
		{	auto    it = oel::begin_(r);
			auto const l = oel::end_(r);
			using D = iter_difference_t< decltype(it) >;
			std::make_unsigned_t<D> n{};
			while( it != l )
			{	++it; ++n;
			}
			return n;
		}
	}
}