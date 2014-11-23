#pragma once

// Copyright © 2014 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "basic_util.h"

#ifndef OETL_NO_BOOST
#include <boost/align/aligned_alloc.hpp>
#endif
#include <memory>
#include <string.h>


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

template<typename T>
struct is_trivially_relocatable< std::shared_ptr<T> > : std::true_type {};

template<typename T, typename U>
struct is_trivially_relocatable< std::pair<T, U> > : bool_constant<
	is_trivially_relocatable<T>::value && is_trivially_relocatable<U>::value > {};

template<typename T, typename... Us>
struct is_trivially_relocatable< std::tuple<T, Us...> > : bool_constant<
	is_trivially_relocatable<T>::value && is_trivially_relocatable< std::tuple<Us...> >::value > {};

template<>
struct is_trivially_relocatable< std::tuple<> > : std::true_type {};

/// Might not be safe with all STL implementations, only verified for Visual C++ 2013
template<typename C, typename Tr>
struct is_trivially_relocatable< std::basic_string<C, Tr> >
 :	std::true_type {};

#if 0

template<typename T, typename Ctr>
struct is_trivially_relocatable< std::stack<T, Ctr> >
 :	is_trivially_relocatable<Ctr> {};

template<typename T, typename Ctr>
struct is_trivially_relocatable< std::queue<T, Ctr> >
 :	is_trivially_relocatable<Ctr> {};

template<typename T, typename Ctr, typename C>
struct is_trivially_relocatable< std::priority_queue<T, Ctr, C> >
 :	is_trivially_relocatable<Ctr> {};

template<typename T, size_t S>
struct is_trivially_relocatable< std::array<T, S> >
 :	is_trivially_relocatable<T> {};

template<typename T>
struct is_trivially_relocatable< std::deque<T> > : std::true_type {};

template<typename T>
struct is_trivially_relocatable< std::list<T> > : std::true_type {};

template<typename T>
struct is_trivially_relocatable< std::forward_list<T> > : std::true_type {};

template<typename T, typename C>
struct is_trivially_relocatable< std::set<T, C> > : std::true_type {};

template<typename T, typename C>
struct is_trivially_relocatable< std::multi_set<T, C> >
 :	std::true_type {};

template<typename K, typename T, typename C>
struct is_trivially_relocatable< std::map<K, T, C> >
 :	std::true_type {};

template<typename K, typename T, typename C>
struct is_trivially_relocatable< std::multi_map<K, T, C> >
 :	std::true_type {};

template<typename T, typename H, typename P>
struct is_trivially_relocatable< std::unordered_set<T, H, P> >
 :	std::true_type {};

template<typename T, typename H, typename P>
struct is_trivially_relocatable< std::unordered_multi_set<T, H, P> >
 :	std::true_type {};

template<typename K, typename T, typename H, typename P>
struct is_trivially_relocatable< std::unordered_map<K, T, H, P> >
 :	std::true_type {};

template<typename K, typename T, typename H, typename P>
struct is_trivially_relocatable< std::unordered_multi_map<K, T, H, P> >
 :	std::true_type {};

template<typename T>
struct is_trivially_relocatable< boost::circular_buffer<T> > : std::true_type {};

#endif


/// Tag to select a specific constructor
struct init_size_t {};
/// An instance of init_size_t to pass
static init_size_t const init_size;


////////////////////////////////////////////////////////////////////////////////

// The rest are advanced utilities, not for users


/// Like std::aligned_storage<Size, Align>::type, but guaranteed to support alignment of up to 64
template<size_t Size, size_t Align>
struct aligned_storage_t {};

#if _MSC_VER
#	define OETL_ALIGN_PRE(amount)  __declspec(align(amount))
#	define OETL_ALIGN_POST(amount)
#else
#	define OETL_ALIGN_PRE(amount)
#	define OETL_ALIGN_POST(amount) __attribute__(aligned(amount))
#endif

#define OETL_STORAGE_ALIGNED_TO(align)  \
	template<size_t Size>  \
	struct aligned_storage_t<Size, align>  \
	{  \
		OETL_ALIGN_PRE(align) unsigned char data[Size];  \
	} OETL_ALIGN_POST(align)

OETL_STORAGE_ALIGNED_TO(1);
OETL_STORAGE_ALIGNED_TO(2);
OETL_STORAGE_ALIGNED_TO(4);
OETL_STORAGE_ALIGNED_TO(8);
OETL_STORAGE_ALIGNED_TO(16);
OETL_STORAGE_ALIGNED_TO(32);
OETL_STORAGE_ALIGNED_TO(64);

#undef OETL_STORAGE_ALIGNED_TO
#undef OETL_ALIGN_PRE
#undef OETL_ALIGN_POST


namespace _detail
{
	template<size_t Align>
	struct CanDefaultAlloc : bool_constant<
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

	inline void OpDelete(std::true_type, void * ptr)
	{
		::operator delete[](ptr);
	}

#ifndef OETL_NO_BOOST
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
	struct AssertRelocate
	{
		static_assert(is_trivially_relocatable<T>::value,
			"Template argument T must be trivially relocatable, see documentation for is_trivially_relocatable");
	};



	template<typename, typename Iter> inline
	void Destroy(std::true_type, Iter, Iter)
	{}	// optimization for non-optimized builds

	template<typename ValT, typename ForwardIter> inline
	void Destroy(std::false_type, ForwardIter first, ForwardIter last)
	{
		for (; first != last; ++first)
			(*first).~ValT();
	}

	template<typename ForwardIter> inline
	void Destroy(ForwardIter first, ForwardIter last) NOEXCEPT
	{
		typedef typename std::iterator_traits<ForwardIter>::value_type ValT;
		Destroy<ValT>(std::has_trivial_destructor<ValT>(), first, last);
	}


	template<typename ValT, typename ForwardIter> inline
	void UninitFillDefault(std::true_type, ForwardIter first, ForwardIter const last)
	{
		for (; first != last; ++first)
			::new(std::addressof(*first)) ValT;
	}

	template<typename ValT, typename ForwardIter>
	void UninitFillDefault(std::false_type, ForwardIter first, ForwardIter const last)
	{	// default constructor potentially throws
		ForwardIter const init = first;
		try
		{
			for (; first != last; ++first)
				::new(std::addressof(*first)) ValT;
		}
		catch (...)
		{	// Destroy the objects that were constructed before the exception
			Destroy(init, first);
			throw;
		}
	}


	template<typename DestValT, typename InputIter, typename Count, typename ForwardIter> inline
	range_ends<InputIter, ForwardIter>  UninitCopyN(std::true_type, InputIter first, Count count, ForwardIter & dest)
	{	// nothrow constructible
		for (; 0 < count; --count)
		{
			::new(std::addressof(*dest)) DestValT(*first);
			++dest; ++first;
		}
		return {first, dest};
	}

	template<typename DestValT, typename InputIter, typename Count, typename ForwardIter>
	range_ends<InputIter, ForwardIter>  UninitCopyN(std::false_type, InputIter first, Count count, ForwardIter dest)
	{
		ForwardIter const destBegin = dest;
		try
		{
			return UninitCopyN<DestValT>(std::true_type{}, first, count, dest); // dest passed by reference
		}
		catch (...)
		{
			Destroy(destBegin, dest);
			throw;
		}
	}
} // namespace _detail

/// Default initializes objects (in uninitialized memory) in range [first, last)
template<typename ForwardIterator> inline
void uninitialized_fill_default(ForwardIterator first, ForwardIterator last)
{
	typedef typename std::iterator_traits<ForwardIterator>::value_type ValT;
	_detail::UninitFillDefault<ValT>(std::is_nothrow_default_constructible<ValT>(), first, last);
}

/// Copies count elements from a range beginning at first to an uninitialized memory area beginning at dest
template<typename InputIterator, typename Count, typename ForwardIterator>
range_ends<InputIterator, ForwardIterator>
	uninitialized_copy_n(InputIterator first, Count count, ForwardIterator dest)
{
	typedef typename std::iterator_traits<ForwardIterator>::value_type ValT;
	return _detail::UninitCopyN<ValT>(std::is_nothrow_constructible<ValT, decltype(*first)>(),
									  first, count, dest);
}

} // namespace oetl
