#pragma once

// Copyright 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "auxi/algo_detail.h" // for oel_max

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

	static constexpr bool   can_reallocate()  { return is_trivially_relocatable<T>::value; }

	static constexpr size_t max_size();

	//! count greater than max_size() causes overflow and undefined behavior
	T *  allocate(size_t count);
	//! newCount greater than max_size() causes overflow and undefined behavior
	T *  reallocate(T * ptr, size_t newCount);
	void deallocate(T * ptr, size_t) noexcept;

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
	constexpr size_t defaultAlign =
		#if defined __STDCPP_DEFAULT_NEW_ALIGNMENT__
			__STDCPP_DEFAULT_NEW_ALIGNMENT__;
		#elif _WIN64 or defined __x86_64__ // then assuming 16 byte aligned from malloc
			16;
		#else
			alignof(::max_align_t);
		#endif

	constexpr auto * allocFailMsg = "No memory oel::allocator";

	struct BadAlloc
	{
		[[noreturn]] static void Throw()
		{
			OEL_THROW(std::bad_alloc{}, allocFailMsg);
		}
	};

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
	struct Malloc
	{
		void * operator()(size_t const nBytes) const
		{
			OEL_CONST_COND if (Align > defaultAlign)
			{
				void * p = std::malloc(nBytes + Align);
				return AlignAndStore<Align>(p);
			}
			else
			{	return std::malloc(nBytes);
			}
		}
	};

	template<size_t Align>
	struct Realloc
	{
		void * old;

		void * operator()(size_t const nBytes) const
		{
			OEL_CONST_COND if (Align > defaultAlign)
			{
				void * p = old ?
						static_cast<void **>(old)[-1] :
						old;
				p = std::realloc(p, nBytes + Align);
				return AlignAndStore<Align>(p);
			}
			else
			{	return std::realloc(old, nBytes);
			}
		}
	};

	inline void Free(void * p, false_type) noexcept
	{
		if (p) // replace with assert for malloc known to return non-null for 0 size?
		{
			p = static_cast<void **>(p)[-1];
			std::free(p);
		}
	}

	inline void Free(void * p, true_type) noexcept
	{
		std::free(p);
	}

	template<typename AllocFunc>
	void * AllocAndHandleFail(size_t const nBytes, AllocFunc const doAlloc)
	{
		if (nBytes > 0) // could be removed for implementations known not to return null
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
T * allocator<T>::allocate(size_t count)
{
#if OEL_MEM_BOUND_DEBUG_LVL >= 2
	OEL_ASSERT(count <= max_size());
#endif
	_detail::Malloc<oel_max( alignof(T), _detail::defaultAlign )> fn; // max used to reduce template instantiations
	return static_cast<T *>( _detail::AllocAndHandleFail(sizeof(T) * count, fn) );
}

template<typename T>
T * allocator<T>::reallocate(T * ptr, size_t count)
{
#if OEL_MEM_BOUND_DEBUG_LVL >= 2
	OEL_ASSERT(count <= max_size());
#endif
	_detail::Realloc<oel_max( alignof(T), _detail::defaultAlign )> fn{ptr};
	return static_cast<T *>( _detail::AllocAndHandleFail(sizeof(T) * count, fn) );
}

template<typename T>
OEL_ALWAYS_INLINE inline void allocator<T>::deallocate(T * ptr, size_t) noexcept
{
	_detail::Free(ptr, bool_constant<alignof(T) <= _detail::defaultAlign>());
}

template<typename T>
constexpr size_t allocator<T>::max_size()
{
	return ( size_t(-1)
	       - (alignof(T) > _detail::defaultAlign ? alignof(T) : 0) )
	     / sizeof(T);
}

} // namespace oel
