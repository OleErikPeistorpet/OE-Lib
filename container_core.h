#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "basic_util.h"

#ifndef OEL_NO_BOOST
	#include <boost/align/aligned_alloc.hpp>
	#include <boost/optional/optional_fwd.hpp>
	#include <boost/smart_ptr/intrusive_ptr.hpp>
#endif
#include <memory>
#include <string.h>


namespace oel
{
/**
* @brief Trait that specifies that T does not have a pointer member to any of its data members, including
*	inherited, and a T object does not need to notify any observers if its memory address changes.
*
* https://github.com/facebook/folly/blob/master/folly/docs/FBVector.md#object-relocation
* http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n4158.pdf
*
* To specify that a type is trivially relocatable define a specialization like this:
@code
namespace oel {
template<> struct is_trivially_relocatable<MyType> : std::true_type {};
}
@endcode  */
template<typename T>
#if !OEL_TRIVIAL_RELOCATE_DEFAULT
	struct is_trivially_relocatable : is_trivially_copyable<T> {};
#else
	struct is_trivially_relocatable : std::true_type {};
#endif

/// std::unique_ptr assumed trivially relocatable if the deleter is
template<typename T, typename Del>
struct is_trivially_relocatable< std::unique_ptr<T, Del> >
 :	is_trivially_relocatable<Del> {};

template<typename T>
struct is_trivially_relocatable< std::shared_ptr<T> > : std::true_type {};

template<typename T>
struct is_trivially_relocatable< std::weak_ptr<T> > : std::true_type {};

#if _MSC_VER || __GLIBCXX__
	/// Might not be safe with all std library implementations, only verified for Visual C++ 2013 and GCC 4
	template<typename C, typename Tr>
	struct is_trivially_relocatable< std::basic_string<C, Tr> > : std::true_type {};
#endif

#ifndef OEL_NO_BOOST
	template<typename T>
	struct is_trivially_relocatable< boost::optional<T> > : is_trivially_relocatable<T> {};

	template<typename T>
	struct is_trivially_relocatable< boost::intrusive_ptr<T> > : std::true_type {};
#endif



/// Tag to specify default initialization. The const instance default_init is provided for convenience
struct default_init_tag
{	explicit default_init_tag() {}
}
const default_init;



/** @brief Argument-dependent lookup non-member begin, defaults to std::begin
*
* For use in implementation of classes with begin member  */
template<typename Range> inline
auto adl_begin(Range & r)       -> decltype(begin(r))  { return begin(r); }
/// Const version of adl_begin
template<typename Range> inline
auto adl_begin(const Range & r) -> decltype(begin(r))  { return begin(r); }

/** @brief Argument-dependent lookup non-member end, defaults to std::end
*
* For use in implementation of classes with end member  */
template<typename Range> inline
auto adl_end(Range & r)       -> decltype(end(r))  { return end(r); }
/// Const version of adl_end
template<typename Range> inline
auto adl_end(const Range & r) -> decltype(end(r))  { return end(r); }



/// Like std::aligned_storage<Size, Align>::type, but guaranteed to support alignment of up to 64
template<size_t Size, size_t Align>
struct aligned_storage_t {};



/// An automatic alignment allocator. Does not compile if the alignment of T is not supported.
template<typename T>
struct allocator
{
	using value_type = T;
	using propagate_on_container_move_assignment = std::true_type;

	template<typename U> struct rebind
	{
		using other = allocator<U>;
	};

	T * allocate(size_t nObjs);

	void deallocate(T * ptr, size_t);

	allocator() = default;
	template<typename U>
	allocator(const allocator<U> &) noexcept {}
};

template<typename T> inline
bool operator==(allocator<T>, allocator<T>) noexcept { return true; }
template<typename T> inline
bool operator!=(allocator<T>, allocator<T>) noexcept { return false; }



////////////////////////////////////////////////////////////////////////////////
//
// The rest of the file is not for users (implementation details)


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
		return ::operator new[](nBytes);
	}

	inline void OpDelete(std::true_type, void * ptr)
	{
		::operator delete[](ptr);
	}

#ifndef OEL_NO_BOOST
	// TODO: Should use new_handler or let both OpNew overloads use custom failure function
	template<size_t Align>
	void * OpNew(std::false_type, size_t nBytes)
	{
		void * p = boost::alignment::aligned_alloc(Align, nBytes);
		if (p)
			return p;
		else
			throw std::bad_alloc();
	}

	inline void OpDelete(std::false_type, void * ptr)
	{
		boost::alignment::aligned_free(ptr);
	}
#endif
}

template<typename T>
inline T * allocator<T>::allocate(size_t nObjs)
{
	void * p = _detail::OpNew<OEL_ALIGNOF(T)>(_detail::CanDefaultAlloc<OEL_ALIGNOF(T)>(),
											  sizeof(T) * nObjs);
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
	range_ends<InputIter, T *> UninitCopyN(InputIter first, size_t count, T * dest)
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
		return {first, dest};
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
		OEL_CONST_COND if (!std::has_trivial_default_constructor<T>::value)
			_detail::UninitFillImpl(std::false_type{}, first, last);
	}
}

} // namespace oel
