#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "basic_util.h"

#ifndef OEL_NO_BOOST
	#include <boost/align/aligned_alloc.hpp>
	#include <boost/circular_buffer_fwd.hpp>
	#include <boost/optional/optional_fwd.hpp>
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
* @par
* To specify that a type is trivially relocatable define a specialization like this:
@code
namespace oel {
template<> struct is_trivially_relocatable<MyType> : std::true_type {};
}
@endcode  */
template<typename T>
struct is_trivially_relocatable : is_trivially_copyable<T> {};

/// std::unique_ptr assumed trivially relocatable if the deleter is
template<typename T, typename Del>
struct is_trivially_relocatable< std::unique_ptr<T, Del> >
 :	is_trivially_relocatable<Del> {};

template<typename T>
struct is_trivially_relocatable< std::shared_ptr<T> > : std::true_type {};

template<typename T>
struct is_trivially_relocatable< std::weak_ptr<T> > : std::true_type {};

#if _MSC_VER && _MSC_VER < 1900
	/// Might not be safe with all std library implementations, only verified for Visual C++ 2013
	template<typename C, typename Tr>
	struct is_trivially_relocatable< std::basic_string<C, Tr> >
	 :	std::true_type {};
#endif

#ifndef OEL_NO_BOOST
	template<typename T>
	struct is_trivially_relocatable< boost::circular_buffer<T> > : std::true_type {};

	template<typename T>
	struct is_trivially_relocatable< boost::optional<T> > : is_trivially_relocatable<T> {};
#endif



/// Tag to select a specific constructor. The static instance ini_size is provided as a convenience
struct ini_size_tag {};
static ini_size_tag const ini_size;


/**
* @brief Argument-dependent lookup non-member begin, defaults to std::begin
*
* For use in implementation of classes with begin member  */
template<typename Range> inline
auto adl_begin(Range & r)       -> decltype(begin(r))  { return begin(r); }
/// Const version of adl_begin
template<typename Range> inline
auto adl_begin(const Range & r) -> decltype(begin(r))  { return begin(r); }
/**
* @brief Argument-dependent lookup non-member end, defaults to std::end
*
* For use in implementation of classes with end member  */
template<typename Range> inline
auto adl_end(Range & r)       -> decltype(end(r))  { return end(r); }
/// Const version of adl_end
template<typename Range> inline
auto adl_end(const Range & r) -> decltype(end(r))  { return end(r); }


////////////////////////////////////////////////////////////////////////////////

// The rest are advanced utilities, not for users


namespace _detail
{
	template<size_t Align>
	using CanDefaultAlloc = std::integral_constant< bool,
		#if _WIN64 || defined(__x86_64__)  // 16 byte alignment on 64-bit Windows/Linux
			Align <= 16 >;
		#else
			Align <= ALIGNOF(std::max_align_t) >;
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

/// An alignment-aware, non-standard allocator
template<typename T>
struct allocator
{
	using size_type = size_t;

	T * allocate(size_type nObjs)
	{
		void * p = _detail::OpNew<ALIGNOF(T)>(_detail::CanDefaultAlloc<ALIGNOF(T)>(),
											  nObjs * sizeof(T));
		return static_cast<T *>(p);
	}

	void deallocate(T * ptr)
	{
		_detail::OpDelete(_detail::CanDefaultAlloc<ALIGNOF(T)>(), ptr);
	}
};

////////////////////////////////////////////////////////////////////////////////

namespace _detail
{
	template<typename T> inline
	void Destroy(std::true_type, T *, T *) {}  // for speed with optimizations off (debug build)

	template<typename T> inline
	void Destroy(std::false_type, T * first, T * last)
	{
		for (; first < last; ++first)
			first-> ~T();
	}

	template<typename T> inline
	void Destroy(T * first, T * last) NOEXCEPT
	{	// first > last is OK, does nothing
		Destroy(std::has_trivial_destructor<T>(), first, last);
	}


	template<typename, typename Iter> inline
	void UninitFillDefault(std::true_type, Iter, Iter) {}  // for speed with optimizations off

	template<typename ValT, typename ForwardIter>
	void UninitFillDefault(std::false_type, ForwardIter first, ForwardIter const last)
	{	// not trivial default constructor
		ForwardIter init = first;
		try
		{
			for (; first != last; ++first)
				::new(std::addressof(*first)) ValT;
		}
		catch (...)
		{	// Destroy the objects constructed before the exception
			for (; init != first; ++init)
				(*init).~ValT();

			throw;
		}
	}
}

/// Copies count elements from a range beginning at first to an uninitialized memory area beginning at dest
template<typename InputIterator, typename ForwardIterator>
range_ends<InputIterator, ForwardIterator> uninitialized_copy_n(InputIterator first, size_t count, ForwardIterator dest)
{
	using ValT = typename std::iterator_traits<ForwardIterator>::value_type;
	ForwardIterator destBegin = dest;
	try
	{
		for (; 0 < count; --count)
		{
			::new(std::addressof(*dest)) ValT(*first);
			++dest; ++first;
		}
	}
	catch (...)
	{	// Destroy the objects constructed before the exception
		for (; destBegin != dest; ++destBegin)
			(*destBegin).~ValT();

		throw;
	}
	return {first, dest};
}

/// Default initializes objects (in uninitialized memory) in range [first, last)
template<typename ForwardIterator> inline
void uninitialized_fill_default(ForwardIterator first, ForwardIterator last)
{
	using ValT = typename std::iterator_traits<ForwardIterator>::value_type;
	_detail::UninitFillDefault<ValT>(std::has_trivial_default_constructor<ValT>(), first, last);
}

} // namespace oel
