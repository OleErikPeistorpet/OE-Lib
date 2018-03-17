#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "util.h"

#if !defined(OEL_NO_BOOST) && __cpp_aligned_new < 201606
	#include <boost/align/aligned_alloc.hpp>
#endif
#include <cstddef> // for max_align_t
#include <limits>


namespace oel
{

//! Same as std::aligned_storage_t (C++14), with support for alignment above that of std::max_align_t
template<size_t Size, size_t Align>
struct aligned_storage_t;
//! A trivial type of same size and alignment as type T, suitable for use as uninitialized storage for an object
template<typename T>
using aligned_union_t = aligned_storage_t<sizeof(T), alignof(T)>;



//! An automatic alignment allocator. Does not compile if the alignment of T is not supported.
template<typename T>
struct allocator
{
	using value_type = T;
	using propagate_on_container_move_assignment = std::true_type;

	T * allocate(size_t nObjects);
	void deallocate(T * ptr, size_t) noexcept;

	//! U constructible from Args, direct-initialization
	template<typename U, typename... Args,
	         enable_if< std::is_constructible<U, Args...>::value > = 0>
	void construct(U * raw, Args &&... args)
		{
			::new(static_cast<void *>(raw)) U(std::forward<Args>(args)...);
		}
	//! U not constructible from Args, list-initialization
	template<typename U, typename... Args,
	         enable_if< !std::is_constructible<U, Args...>::value > = 0>
	void construct(U * raw, Args &&... args)
		{
			::new(static_cast<void *>(raw)) U{std::forward<Args>(args)...};
		}

	constexpr size_t max_size() const  { return std::numeric_limits<size_t>::max() / sizeof(T); }

	allocator() = default;
	template<typename U>  OEL_ALWAYS_INLINE
	allocator(const allocator<U> &) noexcept {}

	template<typename U>
	friend bool operator==(allocator<T>, allocator<U>) noexcept { return true; }
	template<typename U>
	friend bool operator!=(allocator<T>, allocator<U>) noexcept { return false; }
};



//! Part of std::allocator_traits for C++17
template<typename T>
struct is_always_equal;



////////////////////////////////////////////////////////////////////////////////
//
// The rest of the file is not for users (implementation)


//! @cond INTERNAL

namespace _detail
{
	template<size_t Align>
	using CanDefaultAlloc = bool_constant<
		#if defined(__STDCPP_DEFAULT_NEW_ALIGNMENT__)
			Align <= __STDCPP_DEFAULT_NEW_ALIGNMENT__
		#elif _WIN64 || defined(__x86_64__)  // 16 byte alignment on 64-bit Windows/Linux
			Align <= 16
		#else
			Align <= alignof(
				#if OEL_GCC_VERSION != 408
					std
				#endif
					::max_align_t)
		#endif
		>;

	template<size_t> inline
	void * OpNew(size_t nBytes, true_type)
	{
		return ::operator new(nBytes);
	}

	template<size_t> inline
	void OpDelete(void * ptr, true_type)
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
	void OpDelete(void * ptr, false_type)
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

			#if !defined(__GLIBCXX__) || OEL_GCC_VERSION >= 409
				auto handler = std::get_new_handler();
			#else
				auto handler = std::set_new_handler(nullptr);
				std::set_new_handler(handler);
			#endif
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
	void OpDelete(void * ptr, false_type)
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
inline T * allocator<T>::allocate(size_t nObjects)
{
	void * p = _detail::OpNew<alignof(T)>
		(sizeof(T) * nObjects, _detail::CanDefaultAlloc<alignof(T)>());
	return static_cast<T *>(p);
}

template<typename T>
OEL_ALWAYS_INLINE inline void allocator<T>::deallocate(T * ptr, size_t) noexcept
{
	_detail::OpDelete<alignof(T)>(ptr, _detail::CanDefaultAlloc<alignof(T)>());
}


namespace _detail
{
	template<typename T>
	typename T::is_always_equal IsAlwaysEqual(int);

	template<typename T>
	std::is_empty<T> IsAlwaysEqual(long);
}

} // namespace oel

template<typename T>
struct oel::is_always_equal : decltype( _detail::IsAlwaysEqual<T>(0) ) {};


#ifdef _MSC_VER
	#define OEL_ALIGNAS(amount) __declspec(align(amount))
#else
	#define OEL_ALIGNAS(amount) __attribute__(( aligned(amount) ))
#endif

template<std::size_t Size, std::size_t Align>
struct OEL_ALIGNAS(Align) oel::aligned_storage_t
{
	unsigned char data[Size];
};

//! @endcond
