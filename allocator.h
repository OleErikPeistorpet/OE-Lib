#pragma once

// Copyright 2016 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "auxi/allocator_detail.h"

#include <algorithm> // for max

/** @file
*/

namespace oel
{

//! Aligns memory to the greater of alignof(T) and MinAlign, and has `reallocate` function
/**
* Either throws std::bad_alloc or calls standard new_handler on failure, depending on value of OEL_NEW_HANDLER. */
template< unsigned MinAlign, typename T >
struct allocator
{
	using value_type = T;

	using propagate_on_container_move_assignment = std::true_type;

	static constexpr bool   can_reallocate() noexcept { return is_trivially_relocatable<T>::value; }

	static constexpr size_t align_value() noexcept
		{
			constexpr auto n = std::max({ alignof(T), size_t{MinAlign}, size_t{OEL_MALLOC_ALIGNMENT} });
			return n;
		}

	static constexpr size_t max_size() noexcept
		{
			constexpr auto n = SIZE_MAX - (align_value() > OEL_MALLOC_ALIGNMENT ? align_value() : 0);
			return n / sizeof(T);
		}

	//! `count` greater than max_size() causes overflow and undefined behavior
#ifdef __GNUC__
	[[gnu::malloc]]
#endif
	[[nodiscard]] static T * allocate(size_t count)
		{
			OEL_ASSERT(count <= max_size());

			using F = _detail::Malloc<align_value()>;
			return static_cast<T*>( _detail::AllocAndHandleFail<F>(sizeof(T) * count) );
		}

	//! Like C23 `realloc` except for failure handling (same as allocate, throws bad_alloc or calls new_handler)
	/** @pre If count is zero or greater than max_size(), the behavior is undefined  */
	[[nodiscard]] static T * reallocate(T * ptr, size_t count)
		{
			OEL_ASSERT(0 < count and count <= max_size());

			using F = _detail::Realloc<align_value()>;
			void * vp{ptr};
			return static_cast<T*>( _detail::AllocAndHandleFail<F, /*CheckZero*/ false>(sizeof(T) * count, vp) );
		}

	static void deallocate(T * ptr, size_t count) noexcept
		{
			_detail::Free<align_value()>(ptr, sizeof(T) * count);
		}

	allocator() = default;

	template< typename U >  OEL_ALWAYS_INLINE
	constexpr allocator(allocator<MinAlign, U>) noexcept {}

	template< typename U >
	struct rebind
	{
		using other = allocator<MinAlign, U>;
	};

	friend constexpr bool operator==(allocator, allocator) noexcept  { return true; }
	friend constexpr bool operator!=(allocator, allocator) noexcept  { return false; }
};

}