#pragma once

// Copyright © 2014 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "core_util.h"

#ifndef OETL_NO_BOOST
#include <boost/align/aligned_alloc.hpp>
#endif
#include <memory>
#include <string.h>


namespace oetl
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
namespace oetl {
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

template<typename T, typename U>
struct is_trivially_relocatable< std::pair<T, U> > : bool_constant<
	is_trivially_relocatable<T>::value && is_trivially_relocatable<U>::value > {};

template<typename T, typename... Us>
struct is_trivially_relocatable< std::tuple<T, Us...> > : bool_constant<
	is_trivially_relocatable<T>::value && is_trivially_relocatable< std::tuple<Us...> >::value > {};

template<>
struct is_trivially_relocatable< std::tuple<> > : std::true_type {};

/// Might not be safe with all std library implementations, only verified for Visual C++ 2013
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

template<typename T>
struct is_trivially_relocatable< boost::intrusive_ptr<T> > : std::true_type {};

#endif


/// Tag to select a specific constructor. The static instance ini_size is provided as a convenience
struct ini_size_tag {};
static ini_size_tag const ini_size;



/// Like std::aligned_storage<Size, Align>::type, but guaranteed to support alignment of up to 64
template<size_t Size, size_t Align>
struct aligned_storage_t {};


////////////////////////////////////////////////////////////////////////////////

// The rest of the file is implementation details


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
	using CanDefaultAlloc = bool_constant<
#		if _WIN64 || defined(__x86_64__)  // 16 byte alignment on 64-bit Windows/Linux
			Align <= 16 >;
#		else
			Align <= ALIGNOF(std::max_align_t) >;
#		endif

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

/** @brief Argument-dependent lookup non-member begin, defaults to std::begin
*
* For use in implementation of classes with begin member  */
template<typename Iterable> inline
auto adl_begin(Iterable & ib)       -> decltype(begin(ib))  { return begin(ib); }
/// Const version of adl_begin
template<typename Iterable> inline
auto adl_begin(const Iterable & ib) -> decltype(begin(ib))  { return begin(ib); }

/** @brief Argument-dependent lookup non-member end, defaults to std::end
*
* For use in implementation of classes with end member  */
template<typename Iterable> inline
auto adl_end(Iterable & ib)       -> decltype(end(ib))  { return end(ib); }
/// Const version of adl_end
template<typename Iterable> inline
auto adl_end(const Iterable & ib) -> decltype(end(ib))  { return end(ib); }

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

	template<typename ValT, typename ForwardItor> inline
	void Destroy(std::false_type, ForwardItor first, ForwardItor last)
	{
		for (; first != last; ++first)
			(*first).~ValT();
	}

	template<typename ForwardItor> inline
	void Destroy(ForwardItor first, ForwardItor last) NOEXCEPT
	{
		using ValT = typename std::iterator_traits<ForwardItor>::value_type;
		Destroy<ValT>(std::has_trivial_destructor<ValT>(), first, last);
	}


	template<typename ValT, typename ForwardItor> inline
	void UninitFillDefault(std::true_type, ForwardItor first, ForwardItor const last)
	{
		for (; first != last; ++first)
			::new(std::addressof(*first)) ValT;
	}

	template<typename ValT, typename ForwardItor>
	void UninitFillDefault(std::false_type, ForwardItor first, ForwardItor const last)
	{	// default constructor potentially throws
		ForwardItor const init = first;
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


	template<typename DestValT, typename InputItor, typename Count, typename ForwardItor> inline
	end_iterators<InputItor, ForwardItor>  UninitCopyN(std::true_type, InputItor first, Count count, ForwardItor & dest)
	{	// nothrow constructible
		for (; 0 < count; --count)
		{
			::new(std::addressof(*dest)) DestValT(*first);
			++dest; ++first;
		}
		return {first, dest};
	}

	template<typename DestValT, typename InputItor, typename Count, typename ForwardItor>
	end_iterators<InputItor, ForwardItor>  UninitCopyN(std::false_type, InputItor first, Count count, ForwardItor dest)
	{
		ForwardItor const destBegin = dest;
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
	using ValT = typename std::iterator_traits<ForwardIterator>::value_type;
	_detail::UninitFillDefault<ValT>(std::is_nothrow_default_constructible<ValT>(), first, last);
}

/// Copies count elements from an iterable beginning at first to an uninitialized memory area beginning at dest
template<typename InputIterator, typename Count, typename ForwardIterator>
end_iterators<InputIterator, ForwardIterator>
	uninitialized_copy_n(InputIterator first, Count count, ForwardIterator dest)
{
	using ValT = typename std::iterator_traits<ForwardIterator>::value_type;
	return _detail::UninitCopyN<ValT>(std::is_nothrow_constructible<ValT, decltype(*first)>(),
									  first, count, dest);
}

} // namespace oetl
