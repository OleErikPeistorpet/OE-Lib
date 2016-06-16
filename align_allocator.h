#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "core_util.h"

#ifndef OEL_NO_BOOST
	#include <boost/align/aligned_alloc.hpp>
#endif
#include <cstddef>  // for max_align_t


namespace oel
{

/// Same as std::aligned_storage<Size, Align>::type, with support for alignment above that of std::max_align_t (up to 128)
template<size_t Size, size_t Align>
struct aligned_storage_t;
/// Currently only one type, meant to support multiple
template<typename T>
using aligned_union_t = aligned_storage_t<sizeof(T), OEL_ALIGNOF(T)>;



/// An automatic alignment allocator. Does not compile if the alignment of T is not supported.
template<typename T>
struct allocator
{
	using value_type = T;
	using propagate_on_container_move_assignment = std::true_type;

	T * allocate(size_t nObjects);
	void deallocate(T * ptr, size_t);

	/// U constructible from Args, direct-initialization
	template<typename U, typename... Args,
	         enable_if< std::is_constructible<U, Args...>::value > = 0>
	void construct(U * raw, Args &&... args)
	{
		::new(static_cast<void *>(raw)) U(std::forward<Args>(args)...);
	}
	/// U not constructible from Args, list-initialization
	template<typename U, typename... Args,
	         enable_if< !std::is_constructible<U, Args...>::value > = 0>
	void construct(U * raw, Args &&... args)
	{
		::new(static_cast<void *>(raw)) U{std::forward<Args>(args)...};
	}

	allocator() = default;
	template<typename U>
	allocator(const allocator<U> &) noexcept {}

	template<typename U>
	friend bool operator==(allocator<T>, allocator<U>) noexcept { return true; }
	template<typename U>
	friend bool operator!=(allocator<T>, allocator<U>) noexcept { return false; }
};


namespace _detail
{
	template<typename T>
	typename T::is_always_equal IsAlwaysEqual(int);
	template<typename T>
	std::is_empty<T>            IsAlwaysEqual(long);
}

/// Part of std::allocator_traits for C++17
template<typename Alloc>
using is_always_equal_allocator  = decltype( _detail::IsAlwaysEqual<Alloc>(int{}) );



////////////////////////////////////////////////////////////////////////////////
//
// The rest of the file is not for users (implementation)


/// @cond INTERNAL

#if _MSC_VER
	#define OEL_ALIGNAS(amount, alignee) __declspec(align(amount)) alignee
#else
	#define OEL_ALIGNAS(amount, alignee) alignee __attribute__(( aligned(amount) ))
#endif

#define OEL_STORAGE_ALIGNED_TO(alignment)  \
	template<size_t Size>  \
	struct aligned_storage_t<Size, alignment>  \
	{  \
		OEL_ALIGNAS(alignment, unsigned char data[Size]);  \
	}

OEL_STORAGE_ALIGNED_TO(1);
OEL_STORAGE_ALIGNED_TO(2);
OEL_STORAGE_ALIGNED_TO(4);
OEL_STORAGE_ALIGNED_TO(8);
OEL_STORAGE_ALIGNED_TO(16);
OEL_STORAGE_ALIGNED_TO(32);
OEL_STORAGE_ALIGNED_TO(64);
OEL_STORAGE_ALIGNED_TO(128);

#undef OEL_STORAGE_ALIGNED_TO


namespace _detail
{
	template<size_t Align>
	using CanDefaultAlloc = bool_constant<
		#if _WIN64 || defined(__x86_64__)  // 16 byte alignment on 64-bit Windows/Linux
			Align <= 16 >;
		#else
			Align <= OEL_ALIGNOF(std::max_align_t) >;
		#endif

	template<size_t> inline
	void * OpNew(size_t nBytes, true_type)
	{
		return ::operator new(nBytes);
	}

	inline void OpDelete(void * ptr, true_type)
	{
		::operator delete(ptr);
	}

#ifndef OEL_NO_BOOST
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

			#if !__GLIBCXX__ || (__GNUC__ == 4 && __GNUC_MINOR__ >= 9) || __GNUC__ > 4
				auto handler = std::get_new_handler();
			#else
				auto handler = std::set_new_handler(nullptr);
				std::set_new_handler(handler);
			#endif
				if (!handler)
					OEL_THROW(std::bad_alloc{});

				(*handler)();
			}
		}
		else
		{	return nullptr;
		}
	}

	inline void OpDelete(void * ptr, false_type)
	{
		boost::alignment::aligned_free(ptr);
	}
#endif
}
/// @endcond

template<typename T>
inline T * allocator<T>::allocate(size_t nObjects)
{
	using CanDefaultAlloc = _detail::CanDefaultAlloc<OEL_ALIGNOF(T)>;
#ifdef OEL_NO_BOOST
	static_assert(CanDefaultAlloc::value,
		"The value of Align is not supported by operator new. Boost v1.56 required (and OEL_NO_BOOST not defined).");
#endif
	void * p = _detail::OpNew<OEL_ALIGNOF(T)>(sizeof(T) * nObjects, CanDefaultAlloc{});
	return static_cast<T *>(p);
}

template<typename T>
inline void allocator<T>::deallocate(T * ptr, size_t)
{
	_detail::OpDelete(ptr, _detail::CanDefaultAlloc<OEL_ALIGNOF(T)>());
}

} // namespace oel
