#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "util.h"

#include <cstdint>  // for uintptr_t
#include <stddef.h> // for max_align_t


/** @file
*/

#ifndef OEL_NEW_HANDLER
#define OEL_NEW_HANDLER  !OEL_HAS_EXCEPTIONS
#endif


namespace oel
{

/** @brief Automatically handles over-aligned T. Has reallocate method in addition to standard functionality
*
* Either throws std::bad_alloc or calls standard new_handler on failure, depending on value of OEL_NEW_HANDLER  */
template<typename T>
struct allocator
{
	using value_type = T;

	using propagate_on_container_move_assignment = std::true_type;

	T *  allocate(size_t count);
	template< typename T_ = T, typename = enable_if<is_trivially_relocatable<T_>::value> >
	T *  reallocate(T * ptr, size_t newCount);
	void deallocate(T * ptr, size_t) noexcept;

	static constexpr size_t max_size()   { return (size_t)-1 / sizeof(T); }

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
		#elif _WIN64 or defined __x86_64__ // then assuming 16 byte aligned from malloc
			Align <= 16
		#else
			Align <= alignof(::max_align_t)
		#endif
		>;

	constexpr auto * allocFailMsg = "No memory oel::allocator";

	struct BadAlloc
	{
		[[noreturn]] static void Throw()
		{
			OEL_THROW(std::bad_alloc{}, allocFailMsg);
		}
	};

	template<size_t>
	inline void * Malloc(size_t nBytes, true_type) noexcept
	{
		return std::malloc(nBytes);
	}

	template<size_t>
	inline void * Realloc(void * p, size_t nBytes, true_type) noexcept
	{
		return std::realloc(p, nBytes);
	}

	inline void Free(void * p, true_type) noexcept
	{
		std::free(p);
	}

	template<size_t Align>
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

	template<size_t Align>
	void * Malloc(size_t nBytes, false_type) noexcept
	{
		void * p = std::malloc(nBytes + Align);
		return AlignAndStore<Align>(p);
	}

	template<size_t Align>
	inline void * Realloc(void * old, size_t nBytes, false_type) noexcept
	{
		if (old)
			old = static_cast<void **>(old)[-1];

		void * p = std::realloc(old, nBytes + Align);
		return AlignAndStore<Align>(p);
	}

	inline void Free(void * p, false_type) noexcept
	{
		if (p)
		{
			p = static_cast<void **>(p)[-1];
			std::free(p);
		}
	}

	template<typename AllocFunc>
	void * AllocAndHandleFail(size_t nBytes, AllocFunc doAlloc)
	{
		if (nBytes > 0) // could be removed for known malloc implementations?
		{
		#if OEL_NEW_HANDLER
			for (;;)
			{
				void * p = doAlloc(nBytes);
				if (p)
					return p;

				auto handler = std::get_new_handler();
				if (!handler)
					OEL_ABORT(allocFailMsg);

				(*handler)();
			}
		#else
			void * p = doAlloc(nBytes);
			if (p)
				return p;
			else
				BadAlloc::Throw();
		#endif
		}
		else
		{	return nullptr;
		}
	}
}

template<typename T>
inline T * allocator<T>::allocate(size_t count)
{
	auto f = [](size_t nBytes)
	{
		return _detail::Malloc<alignof(T)>
			(nBytes, _detail::CanDefaultAlloc<alignof(T)>());
	};
	void * p = _detail::AllocAndHandleFail(sizeof(T) * count, f);
	return static_cast<T *>(p);
}

template<typename T> template<typename, typename>
inline T * allocator<T>::reallocate(T * old, size_t count)
{
	auto f = [old](size_t nBytes)
	{
		return _detail::Realloc<alignof(T)>
			(old, nBytes, _detail::CanDefaultAlloc<alignof(T)>());
	};
	void * p = _detail::AllocAndHandleFail(sizeof(T) * count, f);
	return static_cast<T *>(p);
}

template<typename T>
OEL_ALWAYS_INLINE inline void allocator<T>::deallocate(T * ptr, size_t) noexcept
{
	_detail::Free(ptr, _detail::CanDefaultAlloc<alignof(T)>());
}

} // namespace oel
