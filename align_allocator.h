#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "core_util.h"

#ifndef OEL_NO_BOOST
	#include <boost/align/aligned_alloc.hpp>
#endif
#include <string.h> // for memset


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
	template<typename U, typename... Args>
	enable_if_t< std::is_constructible<U, Args...>::value >
		construct(U * raw, Args &&... args)
	{
		::new(static_cast<void *>(raw)) U(std::forward<Args>(args)...);
	}
	/// U not constructible from Args, list-initialization
	template<typename U, typename... Args>
	enable_if_t< !std::is_constructible<U, Args...>::value >
		construct(U * raw, Args &&... args)
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
	void * OpNew(std::true_type, size_t nBytes)
	{
		return ::operator new(nBytes);
	}

	inline void OpDelete(std::true_type, void * ptr)
	{
		::operator delete(ptr);
	}

#ifndef OEL_NO_BOOST
	template<size_t Align>
	void * OpNew(std::false_type, size_t const nBytes)
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
	using CanDefaultAlloc = _detail::CanDefaultAlloc<OEL_ALIGNOF(T)>;
#ifdef OEL_NO_BOOST
	static_assert(CanDefaultAlloc::value,
		"The value of Align is not supported by operator new. Boost v1.56 required (and OEL_NO_BOOST not defined).");
#endif
	void * p = _detail::OpNew<OEL_ALIGNOF(T)>(CanDefaultAlloc{}, sizeof(T) * nObjects);
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


	template<typename T> struct AssertTrivialRelocate
	{
		static_assert(is_trivially_relocatable<T>::value,
			"The function requires trivially relocatable T, see declaration of is_trivially_relocatable");
	};


	template<typename Alloc, bool = std::is_empty<Alloc>::value>
	struct AllocRefOptimizeEmpty
	{
		Alloc & alloc;
	#if _MSC_VER
		void operator =(AllocRefOptimizeEmpty) = delete;
	#endif
		AllocRefOptimizeEmpty(Alloc & a) : alloc(a) {}

		Alloc & Get() { return alloc; }
	};

	template<typename Alloc>
	struct AllocRefOptimizeEmpty<Alloc, true>
	{
		void operator =(AllocRefOptimizeEmpty) = delete;

		AllocRefOptimizeEmpty(Alloc &) {}

		Alloc Get() { return Alloc{}; }
	};


	template<typename T>
	void Destroy(T * first, T *const last) noexcept
	{	// first > last is OK, does nothing
		OEL_CONST_COND if (!is_trivially_destructible<T>::value) // for speed with optimizations off (debug build)
		{
			for (; first < last; ++first)
				first-> ~T();
		}
	}


	struct NoOp
	{	void operator()(...) const {}
	};

	template<typename Alloc, typename InputIter, typename T, typename FuncTakingLast = NoOp>
	InputIter UninitCopy(InputIter src, T * dest, T *const dLast, Alloc & alloc, FuncTakingLast extraCleanup = {})
	{
		T *const destBegin = dest;
		OEL_TRY_
		{
			while (dest != dLast)
			{
				std::allocator_traits<Alloc>::construct(alloc, dest, *src);
				++src; ++dest;
			}
		}
		OEL_CATCH_ALL
		{
			_detail::Destroy(destBegin, dest);
			extraCleanup(dLast);
			OEL_WHEN_EXCEPTIONS_ON(throw);
		}
		return src;
	}

	template<typename InputIter, typename T>
	InputIter UninitCopyA(InputIter src, T * dest, T *const dLast)
	{
		allocator<T> a;
		return _detail::UninitCopy(src, dest, dLast, a);
	}


	template<typename Alloc, typename T, typename... Arg>
	void UninitFillImpl(std::false_type, T * first, T *const last, Alloc & alloc, const Arg &... arg)
	{
		T *const init = first;
		OEL_TRY_
		{
			for (; first != last; ++first)
				std::allocator_traits<Alloc>::construct(alloc, first, arg...);
		}
		OEL_CATCH_ALL
		{
			_detail::Destroy(init, first);
			OEL_WHEN_EXCEPTIONS_ON(throw);
		}
	}

	template<typename T, typename Alloc> inline
	void UninitFillImpl(std::true_type, T * first, T * last, Alloc &, int val = 0)
	{
		::memset(first, val, last - first);
	}

	template<typename T, typename Alloc, typename... Arg> inline
	void UninitFill(T *const first, T *const last, Alloc & alloc, const Arg &... arg)
	{
		// TODO: investigate libstdc++ std::fill
		_detail::UninitFillImpl(bool_constant<std::is_integral<T>::value && sizeof(T) == 1>(),
								first, last, alloc, arg...);
	}

	template<typename T, typename... Arg> inline
	void UninitFillA(T *const first, T *const last, const Arg &... arg)
	{
		allocator<T> a;
		UninitFill(first, last, a, arg...);
	}

	template<typename T, typename Alloc = allocator<T>> inline
	void UninitFillDefault(T *const first, T *const last, Alloc & a = Alloc{})
	{
		OEL_CONST_COND if (!is_trivially_default_constructible<T>::value)
			_detail::UninitFillImpl(std::false_type{}, first, last, a);
	}
}

} // namespace oel
