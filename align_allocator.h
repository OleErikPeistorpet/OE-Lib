#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "util.h"

#if __cpp_aligned_new < 201606
#include <cstdint> // for uintptr_t
#endif
#include <stddef.h> // for max_align_t
#include <limits>


/** @file
*/

namespace oel
{

//! An automatic alignment allocator. If the alignment of T is not supported, allocate does not compile.
template<typename T>
struct allocator
{
	using value_type = T;

	using propagate_on_container_move_assignment = std::true_type;

	T *  allocate(size_t nElems);
	void deallocate(T * ptr, size_t) noexcept;

	static constexpr size_t max_size()  { return std::numeric_limits<size_t>::max() / sizeof(T); }

	allocator() = default;
	template<typename U>  OEL_ALWAYS_INLINE
	constexpr allocator(const allocator<U> &) noexcept {}

	template<typename U>
	friend bool operator==(allocator, allocator<U>) noexcept { return true; }
	template<typename U>
	friend bool operator!=(allocator, allocator<U>) noexcept { return false; }
};



//! Similar to std::aligned_storage_t, but supports any alignment the compiler can provide
template<size_t Size, size_t Align>
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
template<typename T>
using aligned_union_t = aligned_storage_t<sizeof(T), alignof(T)>;



////////////////////////////////////////////////////////////////////////////////
//
// Implementation only in rest of the file


namespace _detail
{
	template<size_t Align>
	using CanDefaultAlloc = bool_constant<
		#if defined __STDCPP_DEFAULT_NEW_ALIGNMENT__
			Align <= __STDCPP_DEFAULT_NEW_ALIGNMENT__
		#elif _WIN64 or defined __x86_64__ // then assuming 16 byte aligned from operator new
			Align <= 16
		#else
			Align <= alignof(::max_align_t)
		#endif
		>;

	template<size_t>
	void * OpNew(size_t size, true_type)
	{
		return ::operator new(size);
	}

	inline void OpDelete(void * p, size_t, true_type) noexcept
	{
		::operator delete(p);
	}

#if __cpp_aligned_new >= 201606
	template<size_t Align>
	void * OpNew(size_t size, false_type)
	{
		return ::operator new(size, std::align_val_t{Align});
	}

	inline void OpDelete(void * p, size_t const aligned, false_type) noexcept
	{
		::operator delete(p, std::align_val_t{aligned});
	}
#else
	template<size_t Align>
	void * AlignAndStore(void *const orig) noexcept
	{
		auto i = reinterpret_cast<std::uintptr_t>(orig) + Align;
		i &= ~(Align - 1);
		auto const p = reinterpret_cast<void *>(i);

		static_cast<void **>(p)[-1] = orig;
		return p;
	}

	template<size_t Align>
	void * OpNew(size_t const size, false_type)
	{
		if (size <= std::numeric_limits<size_t>::max() - Align)
		{
			for (;;)
			{
				void * p = std::malloc(size + Align);
				if (p)
					return AlignAndStore<Align>(p);

				auto handler = std::get_new_handler();
				if (!handler)
					break;

				(*handler)();
			}
		}
		OEL_THROW(std::bad_alloc{}, "No memory oel::allocator");
	}

	inline void OpDelete(void * p, size_t, false_type) noexcept(nodebug)
	{
		OEL_ASSERT(p);

		p = static_cast<void **>(p)[-1];
		std::free(p);
	}
#endif
}

template<typename T>
inline T * allocator<T>::allocate(size_t nElems)
{
	void * p = _detail::OpNew<alignof(T)>
		(sizeof(T) * nElems, _detail::CanDefaultAlloc<alignof(T)>());
	return static_cast<T *>(p);
}

template<typename T>
OEL_ALWAYS_INLINE inline void allocator<T>::deallocate(T * ptr, size_t) noexcept
{
	_detail::OpDelete(ptr, alignof(T), _detail::CanDefaultAlloc<alignof(T)>());
}

} // namespace oel
