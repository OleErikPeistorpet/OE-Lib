#pragma once

// Copyright 2016 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "auxi/core_util.h"

#include <algorithm> // for max
#include <cstdint>  // for uintptr_t

/** @file
*/

#ifndef OEL_MALLOC_ALIGNMENT
#define OEL_MALLOC_ALIGNMENT  __STDCPP_DEFAULT_NEW_ALIGNMENT__
#endif

#ifndef OEL_NEW_HANDLER
#define OEL_NEW_HANDLER  !OEL_HAS_EXCEPTIONS
#endif

namespace oel
{

/** @brief Has reallocate method in addition to standard functionality
*
* Either throws std::bad_alloc or calls standard new_handler on failure, depending on value of OEL_NEW_HANDLER.
* (Automatically handles over-aligned T, like std::allocator does from C++17) */
template< typename T >
class allocator
{
public:
	using value_type = T;

	using propagate_on_container_move_assignment = std::true_type;

	static constexpr bool   can_reallocate() noexcept  { return is_trivially_relocatable<T>::value; }

	static constexpr size_t max_size() noexcept
		{
			constexpr auto n = SIZE_MAX - (alignof(T) > OEL_MALLOC_ALIGNMENT ? alignof(T) : 0);
			return n / sizeof(T);
		}

	//! count greater than max_size() causes overflow and undefined behavior
	[[nodiscard]] static T * allocate(size_t count);
	//! newCount greater than max_size() causes overflow and undefined behavior
	[[nodiscard]] static T * reallocate(T * ptr, size_t newCount);
	static void              deallocate(T * ptr, size_t) noexcept;

	allocator() = default;
	template< typename U >  OEL_ALWAYS_INLINE
	constexpr allocator(const allocator<U> &) noexcept {}

	friend constexpr bool operator==(allocator, allocator) noexcept  { return true; }
	friend constexpr bool operator!=(allocator, allocator) noexcept  { return false; }



////////////////////////////////////////////////////////////////////////////////
//
// Implementation only in rest of the file


private:
	static constexpr size_t _alignment()
	{
		return std::max(alignof(T), size_t(OEL_MALLOC_ALIGNMENT));
	}
};

namespace _detail
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

	template< size_t Align >
	struct Malloc
	{
		static void * call(size_t const nBytes)
		{
			if constexpr (Align > OEL_MALLOC_ALIGNMENT)
			{
				auto p = std::malloc(nBytes + Align);
				return AlignAndStore<Align>(p);
			}
			else
			{	return std::malloc(nBytes);
			}
		}
	};

	template< size_t Align >
	struct Realloc
	{
		static void * call(size_t const nBytes, void * old)
		{
			if constexpr (Align > OEL_MALLOC_ALIGNMENT)
			{
				if (old)
					old = static_cast<void **>(old)[-1];

				auto p = std::realloc(old, nBytes + Align);
				return AlignAndStore<Align>(p);
			}
			else
			{	return std::realloc(old, nBytes);
			}
		}
	};

	template< size_t Align >
	void Free(void * p) noexcept
	{
		if constexpr (Align > OEL_MALLOC_ALIGNMENT)
		{
			if (p)
			{
				p = static_cast<void **>(p)[-1];
				std::free(p);
			}
		}
		else
		{	std::free(p);
		}
	}

	template< typename AllocFunc, typename... Ptr > // should pass void * for fewer template instantiations
#ifdef _MSC_VER
	__declspec(restrict)
#elif __GNUC__
	[[gnu::malloc]]
#endif
	void * AllocAndHandleFail(size_t const nBytes, Ptr const... old)
	{
		if (nBytes > 0) // could be removed for implementations known not to return null
		{
		#if OEL_NEW_HANDLER
			for (;;)
			{
				auto p = AllocFunc::call(nBytes, old...);
				if (p)
					return p;

				auto const handler = std::get_new_handler();
				if (!handler)
					OEL_ABORT(allocFailMsg);

				(*handler)();
			}
		#else
			auto p = AllocFunc::call(nBytes, old...);
			if (p)
				return p;
			else
				BadAlloc::raise();
		#endif
		}
		else
		{	return nullptr;
		}
	}
}

template< typename T >
T * allocator<T>::allocate(size_t count)
{
#if OEL_MEM_BOUND_DEBUG_LVL >= 2
	OEL_ASSERT(count <= max_size());
#endif
	using F = _detail::Malloc<_alignment()>; // just alignof(T) would increase template instantiations
	return static_cast<T *>( _detail::AllocAndHandleFail<F>(sizeof(T) * count) );
}

template< typename T >
T * allocator<T>::reallocate(T * ptr, size_t count)
{
#if OEL_MEM_BOUND_DEBUG_LVL >= 2
	OEL_ASSERT(count <= max_size());
#endif
	using F = _detail::Realloc<_alignment()>;
	void * vp{ptr};
	return static_cast<T *>( _detail::AllocAndHandleFail<F>(sizeof(T) * count, vp) );
}

template< typename T >
inline void allocator<T>::deallocate(T * ptr, size_t) noexcept
{
	_detail::Free<_alignment()>(ptr);
}

} // namespace oel
