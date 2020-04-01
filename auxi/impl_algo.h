#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "../util.h"

#include <cstring>
#include <stdexcept>


namespace oel
{
namespace _detail
{
	struct Throw
	{	// Exception throwing has been split out from templates to avoid bloat
		[[noreturn]] static void outOfRange(const char * what)
		{
			OEL_THROW(std::out_of_range(what), what);
		}

		[[noreturn]] static void lengthError(const char * what)
		{
			OEL_THROW(std::length_error(what), what);
		}
	};

////////////////////////////////////////////////////////////////////////////////

	template< typename T >
	struct Construct
	{	// Must always call operator() with T * (other type will compile but bypass Alloc)

		template< typename Alloc, typename... Args >
		static auto call(Alloc & a, T *__restrict p, Args &&... args)
		->	decltype(a.construct(p, static_cast<Args &&>(args)...))
			       { a.construct(p, static_cast<Args &&>(args)...); }

		template< typename Alloc, typename... Args,
		          enable_if< std::is_constructible<T, Args...>::value > = 0
		>
		static void call(Alloc &, void *__restrict p, Args &&... args)
		{	// T constructible from Args
			::new(p) T(static_cast<Args &&>(args)...);
		}

		template< typename Alloc, typename... Args,
		          enable_if< ! std::is_constructible<T, Args...>::value > = 0
		>
		static void call(Alloc &, void *__restrict p, Args &&... args)
		{
			::new(p) T{static_cast<Args &&>(args)...}; // list-initialization
		}
	};


	template< typename T >
	void Destroy(T * first, const T * last) noexcept
	{	// first > last is OK, does nothing
		if (!std::is_trivially_destructible<T>::value) // for speed with non-optimized builds
		{
			for (; first < last; ++first)
				first-> ~T();
		}
	}


#if !defined _MSC_VER or _MSC_VER >= 2000
	#define OEL_CHECK_NULL_MEMCPY  1
#endif
	template< typename ContiguousIter >
	void MemcpyCheck(ContiguousIter const src, size_t const nElems, void *const dest)
	{	// memcpy(nullptr, nullptr, 0) is UB. Unfortunately, checking can have significant performance hit,
		// probably due to functions no longer being inlined. GCC known to need the check in some cases
	#if OEL_CHECK_NULL_MEMCPY or OEL_MEM_BOUND_DEBUG_LVL
		if (nElems != 0)
	#endif
		{	// Dereference to detect out of range errors if the iterator has internal check
		#if OEL_MEM_BOUND_DEBUG_LVL
			(void) *src;
			(void) *(src + (nElems - 1));
		#endif
			std::memcpy(dest, to_pointer_contiguous(src), sizeof(*src) * nElems);
		}
	}


	template< typename T,
		enable_if< is_trivially_relocatable<T>::value > = 0
	>
	inline T * Relocate(T *__restrict src, size_t const n, T *__restrict dest)
	{
		T *const dLast = dest + n;
	#if OEL_CHECK_NULL_MEMCPY
		if (src)
	#endif
		{	std::memcpy(
				static_cast<void *>(dest),
				static_cast<const void *>(src),
				sizeof(T) * n );
		}
		return dLast;
	}

	template< typename T, typename... None >
	T * Relocate(T *__restrict src, size_t const n, T *__restrict dest, None...)
	{
		OEL_WHEN_EXCEPTIONS_ON(
			static_assert( std::is_nothrow_move_constructible<T>::value,
				"dynarray requires that T is noexcept move constructible or trivially relocatable" );
		)
		for (size_t i{}; i < n; ++i)
		{
			::new(static_cast<void *>(dest + i)) T( std::move(src[i]) );
			src[i].~T();
		}
		return dest;
	}
	#undef OEL_CHECK_NULL_MEMCPY


	template< typename Alloc, typename ContiguousIter, typename T,
	          enable_if< can_memmove_with<T *, ContiguousIter>::value > = 0
	>
	inline void UninitCopy(ContiguousIter src, T * dFirst, T * dLast, Alloc &)
	{
		_detail::MemcpyCheck(src, dLast - dFirst, dFirst);
	}

	template< typename Alloc, typename InputIter, typename T,
	          enable_if< ! can_memmove_with<T *, InputIter>::value > = 0
	>
	InputIter UninitCopy(InputIter src, T *__restrict dest, T *const dLast, Alloc & allo)
	{
		T *const dFirst = dest;
		OEL_TRY_
		{
			while (dest != dLast)
			{
				Construct<T>::call(allo, dest, *src);
				++src; ++dest;
			}
		}
		OEL_CATCH_ALL
		{
			_detail::Destroy(dFirst, dest);
			OEL_WHEN_EXCEPTIONS_ON(throw);
		}
		return src;
	}


	template< typename Alloc >
	struct UninitFill
	{
		template< typename T >
		using IsByte = bool_constant< sizeof(T) == 1 and
			(std::is_integral<T>::value or std::is_enum<T>::value) >;

		template< typename T, typename... Args,
		          enable_if< !IsByte<T>::value > = 0
		>
		static void call(T *__restrict first, T *const last, Alloc & allo, const Args &... args)
		{
			T *const init = first;
			OEL_TRY_
			{
				for (; first != last; ++first)
					Construct<T>::call(allo, first, args...);
			}
			OEL_CATCH_ALL
			{
				_detail::Destroy(init, first);
				OEL_WHEN_EXCEPTIONS_ON(throw);
			}
		}

		template< typename T,
		          enable_if< IsByte<T>::value > = 0
		>
		static void call(T *const first, T * last, Alloc &, T val)
		{
			std::memset(first, static_cast<int>(val), last - first);
		}

		template< typename T,
		          enable_if< std::is_trivial<T>::value > = 0
		>
		static void call(T *const first, T * last, Alloc &)
		{
			std::memset(first, 0, sizeof(T) * (last - first));
		}
	};

	struct UninitDefaultConstruct
	{
		template< typename Alloc, typename T >
		static void call(T *__restrict first, T *const last, Alloc & a)
		{
			if (!std::is_trivially_default_constructible<T>::value)
				UninitFill<Alloc>::call(first, last, a);
		}
	};



	template< typename Range, typename IT > // pass dummy int to prefer this overload
	auto doSizeOrEnd(Range & r, IT, int)
	->	decltype((size_t) oel::ssize(r)) { return oel::ssize(r); }

	template< typename Range >
	size_t doSizeOrEnd(Range & r, std::forward_iterator_tag, long)
	{
		size_t n = 0;
		auto it = begin(r);  auto const last = end(r);
		while (it != last)
		{ ++n; ++it; }
		return n;
	}

	template< typename Range >
	auto doSizeOrEnd(Range & r, std::input_iterator_tag, long) { return end(r); }

	// If r is sized or multi-pass, returns element count as size_t, else end(r)
	template< typename Range >
	auto CountOrEnd(Range && r)
	{
		using It = decltype(begin(r));
		return _detail::doSizeOrEnd(r, iter_category<It>(), 0);
	}
}

} // namespace oel
