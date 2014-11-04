#pragma once

// Copyright © 2014 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "basic_util.h"

#include <boost/align/aligned_alloc.hpp>
#include <memory>
#include <cstring>
#include <stdexcept>


namespace oetl
{
/**
* @brief Trait that specifies whether moving a T object to a new location and immediately destroying the source object is
* equivalent to memcpy and not calling destructor on the source.
*
* https://github.com/facebook/folly/blob/master/folly/docs/FBVector.md#object-relocation
* http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2008/n2754.html
* @par
* To specify that a type is trivially relocatable define a specialization like this:
@code
template<> struct oetl::is_trivially_relocatable<MyType> : std::true_type {};
@endcode  */
template<typename T>
struct is_trivially_relocatable : is_trivially_copyable<T> {};

/// std::unique_ptr assumed trivially relocatable if the deleter is
template<typename T, typename Del>
struct is_trivially_relocatable< std::unique_ptr<T, Del> >
 :	is_trivially_relocatable<Del> {};

#if _MSC_VER && _MSC_VER < 1900
/// Might not be safe with all STL implementations, only verified for Visual C++ 2013
template<typename C, typename Tr>
struct is_trivially_relocatable< std::basic_string<C, Tr> >
 :	std::true_type {};
#endif


////////////////////////////////////////////////////////////////////////////////

// The rest are advanced utilities, not for users


namespace _detail
{
	template<size_t Align>
	struct CanDefaultAlloc : std::integral_constant< bool,
#		if _WIN64 || defined(__x86_64__)  // 16 byte alignment on 64-bit Windows/Linux
			Align <= 16 >
#		else
			Align <= ALIGNOF(std::max_align_t) >
#		endif
	{};

	template<size_t> inline
	void * OpNew(std::true_type, size_t nBytes)
	{
		return ::operator new[](nBytes);
	}

	// TODO: Should use new_handler or let both OpNew overloads use custom failure function
	template<size_t Align> inline
	void * OpNew(std::false_type, size_t nBytes)
	{
		void * p = boost::alignment::aligned_alloc(Align, nBytes);
		if (p)
			return p;
		else
			throw std::bad_alloc();
	}

	inline void OpDelete(std::true_type, void * ptr)
	{
		::operator delete[](ptr);
	}

	inline void OpDelete(std::false_type, void * ptr)
	{
		boost::alignment::aligned_free(ptr);
	}
}

/// An alignment-aware, non-standard allocator
template<typename T>
struct allocator
{
	using size_type = size_t;

	T * allocate(size_type nObjs)
	{
		_detail::CanDefaultAlloc<ALIGNOF(T)> defAlloc;
		void * p = _detail::OpNew<ALIGNOF(T)>(defAlloc, nObjs * sizeof(T));
		return static_cast<T *>(p);
	}

	void deallocate(T * ptr)
	{
		_detail::OpDelete(_detail::CanDefaultAlloc<ALIGNOF(T)>(), ptr);
	}
};

////////////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////////////

namespace _detail
{
	template<typename T>
	void Destroy(std::true_type, T *, T *) {}  // optimization for non-optimized builds

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


	template<typename, typename Iter>
	void UninitFillDefault(std::true_type, Iter, Iter) {}  // optimization for non-optimized builds

	template<typename ValT, typename ForwardIter> inline
	void UninitFillDefault(std::false_type, ForwardIter first, ForwardIter const last)
	{	// not trivial default constructor
		auto init = first;
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
	typedef typename std::iterator_traits<ForwardIterator>::value_type ValT;
	auto destBegin = dest;
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
	typedef typename std::iterator_traits<ForwardIterator>::value_type ValT;
	_detail::UninitFillDefault<ValT>(std::has_trivial_default_constructor<ValT>(), first, last);
}

} // namespace oetl
