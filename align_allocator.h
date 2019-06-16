#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "auxi/type_traits.h"

#if !defined(OEL_NO_BOOST) and __cpp_aligned_new < 201606
#include <boost/align/aligned_alloc.hpp>
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

	template<size_t> inline
	void * OpNew(size_t nBytes, true_type)
	{
		return ::operator new(nBytes);
	}

	template<size_t> inline
	void OpDelete(void * ptr, true_type) noexcept
	{
		::operator delete(ptr);
	}

#if __cpp_aligned_new >= 201606
	template<size_t Align> inline
	void * OpNew(size_t nBytes, false_type)
	{
		return ::operator new(nBytes, std::align_val_t{Align});
	}

	template<size_t Align> inline
	void OpDelete(void * ptr, false_type) noexcept
	{
		::operator delete(ptr, std::align_val_t{Align});
	}
#elif !defined(OEL_NO_BOOST)
	template<size_t Align>
	void * OpNew(size_t const nBytes, false_type)
	{
		if (nBytes > 0) // test could be removed if using MSVC _aligned_malloc
		{
			for (;;)
			{
				void * p = boost::alignment::aligned_alloc(Align, nBytes);
				if (p)
					return p;

				auto handler = std::get_new_handler();
				if (!handler)
					OEL_THROW(std::bad_alloc{}, "Failed allocator::allocate");

				(*handler)();
			}
		}
		else
		{	return nullptr;
		}
	}

	template<size_t> inline
	void OpDelete(void * ptr, false_type) noexcept
	{
		boost::alignment::aligned_free(ptr);
	}
#else
	template<size_t Align>
	void * OpNew(size_t, false_type)
	{
		static_assert(Align == 0,
			"The requested alignment requires Boost or a compiler supporting over-aligned dynamic allocation (C++17)");
		return {};
	}

	template<size_t> void OpDelete(void *, false_type);
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
	_detail::OpDelete<alignof(T)>(ptr, _detail::CanDefaultAlloc<alignof(T)>());
}

} // namespace oel
