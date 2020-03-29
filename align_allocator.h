#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "auxi/algo_detail.h"

#if __cpp_aligned_new < 201606
#include <cstdint> // for uintptr_t
#endif
#include <stddef.h> // for max_align_t


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
	void deallocate(T * ptr, size_t) noexcept;

	static constexpr size_t max_size()   { return (size_t)-1 / sizeof(T); }

	allocator() = default;
	template< typename U >  OEL_ALWAYS_INLINE
	constexpr allocator(const allocator<U> &) noexcept {}

	template< typename U >
	friend bool operator==(allocator, allocator<U>) noexcept { return true; }
	template< typename U >
	friend bool operator!=(allocator, allocator<U>) noexcept { return false; }
};



//! Similar to std::aligned_storage_t, but supports any alignment the compiler can provide
template< size_t Size, size_t Align >
struct
#ifdef __GNUC__
	__attribute__(( aligned(Align), may_alias ))
#else
	alignas(Align)
#endif
	aligned_storage_t
{
	unsigned char as_bytes[Size];
};
//! A trivial type of same size and alignment as type T, suitable for use as uninitialized storage for an object
template< typename T >
using aligned_union_t = aligned_storage_t<sizeof(T), alignof(T)>;



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
			Align <= alignof(::max_align_t)
		#endif
		>;

	template< size_t >
	inline void * OpNew(size_t size, true_type)
	{
		return ::operator new(size);
	}

	OEL_ALWAYS_INLINE inline void OpDelete(void * p, size_t, true_type) noexcept
	{
		::operator delete(p);
	}

#if __cpp_aligned_new >= 201606
	template< size_t Align >
	inline void * OpNew(size_t size, false_type)
	{
		return ::operator new(size, std::align_val_t{Align});
	}

	inline void OpDelete(void * p, size_t const aligned, false_type) noexcept
	{
		::operator delete(p, std::align_val_t{aligned});
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
	void * OpNew(size_t const size, false_type)
	{
		if (size <= (size_t)-1 - Align) // then size + Align doesn't overflow
		{
			void * p = ::operator new(size + Align);
			return AlignAndStore<Align>(p);
		}
		Throw::LengthError("Too large size in oel::allocator");
	}

	inline void OpDelete(void * p, size_t, false_type)
	{
		OEL_ASSERT(p); // OpNew never returns null, and the standard mandates
		               // a pointer previously obtained from an equal allocator
		p = static_cast<void **>(p)[-1];
		::operator delete(p);
	}
#endif
}

template< typename T >
T * allocator<T>::allocate(size_t nElems)
{
	void * p = _detail::OpNew<alignof(T)>
		(sizeof(T) * nElems, _detail::CanDefaultNew<alignof(T)>());
	return static_cast<T *>(p);
}

template< typename T >
inline void allocator<T>::deallocate(T * ptr, size_t) noexcept
{
	_detail::OpDelete(ptr, alignof(T), _detail::CanDefaultNew<alignof(T)>());
}

} // namespace oel
