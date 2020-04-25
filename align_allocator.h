#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


//! Users can override (in case of incompatible standard library)
#ifndef OEL_ALIGNED_NEW
	#if __cpp_aligned_new && __cpp_sized_deallocation
	#define OEL_ALIGNED_NEW  1
	#else
	#define OEL_ALIGNED_NEW  0
	#endif
#endif

#include "auxi/impl_algo.h"

#if !OEL_ALIGNED_NEW
#include <cstdint> // for uintptr_t
#endif
#include <cstddef> // for max_align_t

/** @file
*/

namespace oel
{

/** @brief An allocator which aligns the memory to alignof(T)
*
* Wraps operator new without overhead when possible. */
template< typename T >
struct allocator
{
	using value_type = T;

	using propagate_on_container_move_assignment = std::true_type;

	T *  allocate(size_t nElems);
	void deallocate(T * ptr, size_t nElems) noexcept;

	static constexpr size_t max_size() noexcept   { return (size_t)-1 / sizeof(T); }

	allocator() = default;
	template< typename U >  OEL_ALWAYS_INLINE
	constexpr allocator(const allocator<U> &) noexcept {}

	friend constexpr bool operator==(allocator, allocator) noexcept  { return true; }
	friend constexpr bool operator!=(allocator, allocator) noexcept  { return false; }
};



////////////////////////////////////////////////////////////////////////////////
//
// Implementation only in rest of the file


namespace _detail
{
	template< size_t Align >
	using CanDefaultNew = bool_constant<
		#if defined __STDCPP_DEFAULT_NEW_ALIGNMENT__
			Align <= __STDCPP_DEFAULT_NEW_ALIGNMENT__
		#elif _WIN64 or defined __x86_64__ // then assuming 16 byte aligned from operator new
			Align <= 16
		#else
			Align <= alignof(std::max_align_t)
		#endif
		>;

	inline void OpDelete(void * p, size_t const size, size_t, true_type /*regularAlignment*/) noexcept
	{
	#if __cpp_sized_deallocation
		::operator delete(p, size);
	#else
		::operator delete(p);
		(void) size;
	#endif
	}

#if OEL_ALIGNED_NEW
	template< size_t Align >
	inline void * OpNewAlign(size_t size)
	{
		return ::operator new(size, std::align_val_t{Align});
	}

	inline void OpDelete(void * p, size_t const size, size_t const align, false_type) noexcept
	{
		::operator delete(p, size, std::align_val_t{align});
	}
#else
	template< size_t Align >
	void * AlignAndStore(void *const orig) noexcept
	{
		auto i = reinterpret_cast<std::uintptr_t>(orig) + Align;
		i &= ~(Align - 1);
		auto const p = reinterpret_cast<void *>(i);

		static_cast<void **>(p)[-1] = orig;
		return p;
	}

	template< size_t Align >
	#ifdef _MSC_VER
	__declspec(restrict)
	#elif __GNUC__
	__attribute__(( assume_aligned(Align), malloc, returns_nonnull ))
	#endif
	void * OpNewAlign(size_t const size)
	{
		if (size <= (size_t)-1 - Align) // then size + Align doesn't overflow
		{
			void * p = ::operator new(size + Align);
			return AlignAndStore<Align>(p);
		}
		Throw::lengthError("Too large size in oel::allocator");
	}

	inline void OpDelete(void *const p, size_t const size, size_t const align, false_type)
	{
		OEL_ASSERT(p); // OpNewAlign never returns null, and the standard mandates
		               // a pointer previously obtained from an equal allocator
		::operator delete(
			static_cast<void **>(p)[-1]
	#if __cpp_sized_deallocation
			, size + align
	#endif
		);
		(void) size; (void) align;
	}
#endif
}

template< typename T >
T * allocator<T>::allocate(size_t nElems)
{
	size_t const size = sizeof(T) * nElems;
	void * p;
	if (_detail::CanDefaultNew<alignof(T)>::value)
		p = ::operator new(size);
	else
		p = _detail::OpNewAlign<alignof(T)>(size);

	return static_cast<T *>(p);
}

template< typename T >
inline void allocator<T>::deallocate(T * ptr, size_t nElems) noexcept
{
	_detail::OpDelete(
		ptr, sizeof(T) * nElems, alignof(T),
		_detail::CanDefaultNew<alignof(T)>() );
}

} // namespace oel
