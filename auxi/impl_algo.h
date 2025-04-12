#pragma once

// Copyright 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "contiguous_iterator_to_ptr.h"
#include "../util.h"  // for as_unsigned

#include <cstring>


namespace oel::_detail
{
	template< typename Range >
	inline constexpr auto rangeIsForwardOrSized = iter_is_forward< iterator_t<Range> > or rangeIsSized<Range>;

	// Used only if rangeIsForwardOrSized
	template< typename Range >
	auto UDist(Range & r)
	{
		if constexpr (rangeIsSized<Range>)
		{
			return as_unsigned(_detail::Size(r));
		}
		else
		{	auto    it = begin(r);
			auto const l = end(r);
			using D = iter_difference_t<decltype(it)>;
			std::make_unsigned_t<D> n{};
			while (it != l) { ++it; ++n; }

			return n;
		}
	}


	template< typename Range >
	struct AssertForwardOrSizedRange
	{
	#if OEL_HAS_EXCEPTIONS
		static_assert(rangeIsForwardOrSized<Range>,
			"insert_range requires that source models std::ranges::forward_range or that source.size() is valid" );
	#endif
	};


	template< typename T >
	struct AssertNothrowMoveConstruct
	{
	#if OEL_HAS_EXCEPTIONS
		static_assert( std::is_nothrow_move_constructible_v<T>,
			"Containers in oel require that T is noexcept move constructible or trivially relocatable" );
	#endif
	};

	template< typename T >
	struct AssertTrivialRelocate
	{
		static_assert(is_trivially_relocatable<T>::value,
			"insert, emplace require trivially relocatable T, see declaration of is_trivially_relocatable");
	};

////////////////////////////////////////////////////////////////////////////////

	template< typename T >
	void Destroy([[maybe_unused]] T * first, [[maybe_unused]] const T * last) noexcept
	{
		if constexpr (!std::is_trivially_destructible_v<T>) // for speed with non-optimized builds
		{
			for (; first != last; ++first)
				first-> ~T();
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
	T * Relocate(T *__restrict src, size_t const n, T *__restrict dest) noexcept
	{
		if constexpr (is_trivially_relocatable<T>::value)
		{
			T *const dLast = dest + n;
		#if OEL_CHECK_NULL_MEMCPY
			if (src)
		#endif
			{	std::memcpy(
					static_cast<void *>(dest),
					static_cast<const void *>(src),
					sizeof(T) * n );
			}
			return dLast;
		}
		else
		{	(void) AssertNothrowMoveConstruct<T>{};

			for (size_t i{}; i < n; ++i)
			{
				::new(static_cast<void *>(dest + i)) T( std::move(src[i]) );
				src[i].~T();
			}
			return dest + n;
		}
	}
	#undef OEL_CHECK_NULL_MEMCPY


	template< typename Alloc >
	struct UninitFill
	{
		template< typename... Args, typename T >
		static void call(T *__restrict first, T *const last, [[maybe_unused]] Alloc allo, Args const... args)
		{
			[[maybe_unused]] constexpr auto isByte = sizeof(T) == 1 and (std::is_integral_v<T> or std::is_enum_v<T>);

			if constexpr (std::is_trivial_v<T> and sizeof...(Args) == 0)
			{
				std::memset(first, 0, sizeof(T) * (last - first));
			}
			else if constexpr (isByte)
			{
				std::memset(first, static_cast<int>(args)..., last - first);
			}
			else
			{	T *const init = first;
				OEL_TRY_
				{
					for (; first != last; ++first)
						std::allocator_traits<Alloc>::construct(allo, first, args...);
				}
				OEL_CATCH_ALL
				{
					_detail::Destroy(init, first);
					OEL_RETHROW;
				}
			}
		}
	};

	template< typename Alloc >
	struct DefaultInit
	{
		template< typename T >
		static void call(T *__restrict first, T *const last, Alloc & a)
		{
			if constexpr (!std::is_trivially_default_constructible_v<T>)
			{
				UninitFill<Alloc>::call(first, last, a);
			}
			else
			{	(void) first; (void) last; (void) a; // avoid VC++ 2017 warning C4100
			}
		}
	};
}