#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "basic_util.h"

#ifndef OEL_NO_BOOST
	#include <boost/align/aligned_alloc.hpp>
#endif
#include <string.h> // for memset


namespace oel
{

/// Like std::aligned_storage<Size, Align>::type, but supports alignment above that of std::max_align_t
template<size_t Size, size_t Align>
struct aligned_storage_t;



/// An automatic alignment allocator. Does not compile if the alignment of T is not supported.
template<typename T>
struct allocator
{
	using value_type = T;
	using propagate_on_container_move_assignment = std::true_type;

	T * allocate(size_t nObjects);

	void deallocate(T * ptr, size_t);

	allocator() = default;
	template<typename U>
	allocator(const allocator<U> &) noexcept {}

	template<typename U>
	friend bool operator==(allocator<T>, allocator<U>) noexcept { return true; }
	template<typename U>
	friend bool operator!=(allocator<T>, allocator<U>) noexcept { return false; }
};



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
	void * OpNew(std::true_type, size_t nBytes)
	{
		return ::operator new(nBytes);
	}

	inline void OpDelete(std::true_type, void * ptr)
	{
		::operator delete(ptr);
	}

	template<size_t Align>
	void * OpNew(std::false_type, size_t nBytes)
	{
	#ifndef OEL_NO_BOOST
		for (;;)
		{
			void * p = boost::alignment::aligned_alloc(Align, nBytes);
			if (p || nBytes == 0) // testing for 0 bytes could be removed if using MSVC _aligned_malloc
			{	return p;
			}
			else
			{
			#if !__GLIBCXX__ || (__GNUC__ == 4 && __GNUC_MINOR__ >= 9) || __GNUC__ > 4
				auto handler = std::get_new_handler();
			#else
				auto handler = std::set_new_handler(nullptr);
				std::set_new_handler(handler);
			#endif
				if (!handler)
					throw std::bad_alloc{};

				(*handler)();
			}
		}
	#else
		static_assert(Align == 0, // always false
			"The value of Align is not supported by operator new. Boost v1.56 required (and OEL_NO_BOOST not defined).");
		return nullptr;
	#endif
	}

#ifndef OEL_NO_BOOST
	inline void OpDelete(std::false_type, void * ptr)
	{
		boost::alignment::aligned_free(ptr);
	}
#endif
}
/// @endcond

template<typename T>
inline T * allocator<T>::allocate(size_t nObjects)
{
	void * p = _detail::OpNew<OEL_ALIGNOF(T)>(_detail::CanDefaultAlloc<OEL_ALIGNOF(T)>(),
											  sizeof(T) * nObjects);
	return static_cast<T *>(p);
}

template<typename T>
inline void allocator<T>::deallocate(T * ptr, size_t)
{
	_detail::OpDelete(_detail::CanDefaultAlloc<OEL_ALIGNOF(T)>(), ptr);
}

////////////////////////////////////////////////////////////////////////////////

namespace _detail
{
#if __GLIBCXX__ && __GNUC__ == 4
	template<typename T>
	using is_trivially_default_constructible = bool_constant< __has_trivial_constructor(T)
		#if __INTEL_COMPILER
			|| __is_pod(T)
		#endif
		>;

	template<typename T>
	using is_trivially_destructible = bool_constant< __has_trivial_destructor(T)
		#if __INTEL_COMPILER
			|| __is_pod(T)
		#endif
		>;
#else
	using std::is_trivially_default_constructible;
	using std::is_trivially_destructible;
#endif


	template<typename T> inline
	void Destroy(T * first, T * last) noexcept
	{	// first > last is OK, does nothing
		OEL_CONST_COND if (!is_trivially_destructible<T>::value) // for speed with optimizations off (debug build)
		{
			for (; first < last; ++first)
				first-> ~T();
		}
	}


	template<typename InputIter, typename T> inline
	T * UninitCopy(InputIter first, InputIter last, T *const dest)
	{
		return std::uninitialized_copy( first, last,
			#if _MSC_VER
				stdext::make_unchecked_array_iterator(dest) ).base();
			#else
				dest );
			#endif
	}

	template<typename InputIter, typename T>
	InputIter UninitCopyN(InputIter first, size_t count, T * dest)
	{
		T *const destBegin = dest;
		try
		{
			for (; 0 < count; --count)
			{
				::new(dest) T(*first);
				++dest; ++first;
			}
		}
		catch (...)
		{
			_detail::Destroy(destBegin, dest);
			throw;
		}
		return first;
	}


	template<typename T> inline
	void UninitFillImpl(std::true_type, T * first, T * last)
	{
		::memset(first, 0, last - first);
	}

	template<typename T>
	void UninitFillImpl(std::false_type, T * first, T *const last)
	{
		T *const init = first;
		try
		{
			for (; first != last; ++first)
				::new(first) T{};
		}
		catch (...)
		{
			_detail::Destroy(init, first);
			throw;
		}
	}

	template<typename T> inline
	void UninitFill(T *const first, T *const last)
	{
		// Could change to use memset for any POD type with most CPU architectures
		_detail::UninitFillImpl(bool_constant<std::is_integral<T>::value && sizeof(T) == 1>(),
								first, last);
	}

	template<typename T> inline
	void UninitFillDefault(T *const first, T *const last)
	{
		OEL_CONST_COND if (!is_trivially_default_constructible<T>::value)
			_detail::UninitFillImpl(std::false_type{}, first, last);
	}
}

} // namespace oel
