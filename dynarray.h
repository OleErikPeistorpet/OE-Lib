#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "auxi/algo_detail.h"
#include "auxi/dynarray_iterator.h"
#include "optimize_ext/default.h"
#include "align_allocator.h"
#include "range_view.h"

#include <algorithm>

#ifdef __has_include
#if __has_include(<memory_resource>) and (__cplusplus > 201500 or _HAS_CXX17)
	#include <memory_resource>

	#define OEL_HAS_STD_PMR  1
#endif
#endif

/** @file
*/

namespace oel
{

//! Mirroring std::pmr and boost::container::pmr
namespace pmr
{
#if defined OEL_HAS_STD_PMR or !defined OEL_NO_BOOST
	#ifdef OEL_HAS_STD_PMR
	using std::pmr::polymorphic_allocator;
	#else
	using boost::container::pmr::polymorphic_allocator;
	#endif

	template<typename T>
	using dynarray = oel::dynarray< T, polymorphic_allocator<T> >;
#endif
}

//! dynarray<dynarray<T>> is efficient
template<typename T, typename Alloc>
is_trivially_relocatable<Alloc> specify_trivial_relocate(dynarray<T, Alloc>);

template<typename T, typename A>  OEL_ALWAYS_INLINE inline
void swap(dynarray<T, A> & a, dynarray<T, A> & b) noexcept( noexcept(a.swap(b)) )  { a.swap(b); }

//! Overloads generic erase_unstable(RandomAccessContainer &, RandomAccessContainer::size_type) (in range_algo.h)
template<typename T, typename A>  inline
void erase_unstable(dynarray<T, A> & d, typename dynarray<T, A>::size_type index)  { d.erase_unstable(d.begin() + index); }

//! @name GenericContainerInsert
//!@{
// Overloads of generic functions for inserting into container (in range_algo.h)
template<typename T, typename A, typename InputRange>  inline
void assign(dynarray<T, A> & dest, const InputRange & source)  { dest.assign(source); }

template<typename T, typename A, typename InputRange>  inline
void append(dynarray<T, A> & dest, const InputRange & source)  { dest.append(source); }

template<typename T, typename A>  inline
void append(dynarray<T, A> & dest, typename dynarray<T, A>::size_type n, const T & val)  { dest.append(n, val); }

template<typename T, typename A, typename ForwardRange>  inline
typename dynarray<T, A>::iterator
	insert(dynarray<T, A> & dest, typename dynarray<T, A>::const_iterator pos, const ForwardRange & source)
	{
		return dest.insert_r(pos, source);
	}
//!@}

#ifdef OEL_DYNARRAY_IN_DEBUG
inline namespace debug
{
#endif

#ifdef OEL_HAS_DEDUCTION_GUIDES
template<typename InputRange,
         typename Alloc = allocator<iter_value_t< iterator_t<InputRange> >>
        >
explicit dynarray(const InputRange &, Alloc = Alloc{})
->	dynarray<iter_value_t< iterator_t<InputRange> >, Alloc>;
#endif

/**
* @brief Resizable array, dynamically allocated. Very similar to std::vector, but faster in many cases.
*
* In general, only that which differs from std::vector is documented.
*
* For functions that may reallocate, there is a requirement that template argument T is trivially relocatable or
* noexcept move constructible (checked when compiling). Most types can be relocated trivially, but it often needs
* to be declared manually. See specify_trivial_relocate(T &&). Performance is better if T is trivially relocatable.
* Furthermore, a few functions require that T is trivially relocatable (noexcept movable is not enough):
* emplace, insert, insert_r and `erase(first, last)`
*
* The default allocator supports over-aligned types (e.g. __m256)  */
template<typename T, typename Alloc/* = oel::allocator */>
class dynarray   : private _detail::DynarrBase<Alloc>
{
public:
	using value_type      = T;
	using allocator_type  = Alloc;
	using reference       = T &;
	using const_reference = const T &;
	using difference_type = std::ptrdiff_t;
	using size_type       = size_t;

#if OEL_MEM_BOUND_DEBUG_LVL
	using iterator       = debug::dynarray_iterator<T *, T>;
	using const_iterator = debug::dynarray_iterator<const T *, T>;
#else
	using iterator       = T *;
	using const_iterator = const T *;
#endif
	using reverse_iterator       = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;


	constexpr dynarray() noexcept                : _base(Alloc{}) {}
	explicit dynarray(const Alloc & a) noexcept  : _base(a) {}

	//! Construct empty dynarray with space reserved for at least minCap elements
	dynarray(reserve_tag, size_type minCap, const Alloc & a = Alloc{})   : _base(a, minCap) {}

	/** @brief Default-initializes elements, can be significantly faster if T is scalar or has trivial default constructor
	*
	* @copydetails resize_default_init(size_type)  */
	dynarray(size_type size, default_init_t, const Alloc & a = Alloc{});
	//! (Value-initializes elements, same as std::vector)
	explicit dynarray(size_type size, const Alloc & a = Alloc{});
	dynarray(size_type size, const T & fillVal, const Alloc & a = Alloc{});

	/** @brief Equivalent to `std::vector(begin(r), end(r), a)`, where `end(r)` is not needed if r.size() exists
	*
	* To move instead of copy, wrap r with view::move (the same applies for all functions taking a range template) <br>
	* Example, construct from a standard istream with formatting (using Boost):
	@code
	#include <boost/range/istream_range.hpp>
	// ...
	auto result = dynarray<int>(boost::range::istream_range<int>(someStream));
	@endcode  */
	template< typename InputRange,
	          typename /*EnableIfRange*/ = iterator_t<InputRange> >
	explicit dynarray(const InputRange & r, const Alloc & a = Alloc{})  : _base(a) { append(r); }

	dynarray(std::initializer_list<T> il, const Alloc & a = Alloc{})  : _base(a, il.size())
	                                                                  { _initPostAllocate(il.begin()); }
	dynarray(dynarray && other) noexcept                = default;
	dynarray(dynarray && other, const Alloc & a);
	dynarray(const dynarray & other)                    : dynarray(other, AllocTrait::
	                                                        select_on_container_copy_construction(other.m.second())) {}
	dynarray(const dynarray & other, const Alloc & a)   : _base(a, other.size())
	                                                    { _initPostAllocate(other.data()); }
	~dynarray() noexcept;

	dynarray & operator =(dynarray && other) &
		noexcept(AllocTrait::propagate_on_container_move_assignment::value or is_always_equal<Alloc>::value);
	//! Acts as if allocator_type does not have propagate_on_container_copy_assignment
	dynarray & operator =(const dynarray & other) &    { assign(other);  return *this; }

	dynarray & operator =(std::initializer_list<T> il) &  { assign(il);  return *this; }

	void       swap(dynarray & other)
		noexcept(AllocTrait::propagate_on_container_swap::value or is_always_equal<Alloc>::value);
	/**
	* @brief Replace the contents with source range
	* @param source an array, STL container, iterator_range, gsl::span or such.
	* @pre source shall not refer to any elements in this dynarray (same as std::vector::assign)
	* @return iterator `begin(source)` incremented by the number of elements in source
	*
	* Any elements held before the call are either assigned to or destroyed. */
	template<typename InputRange>
	auto assign(const InputRange & source) -> iterator_t<InputRange const>;

	void assign(size_type count, const T & val)   { clear(); append(count, val); }

	/**
	* @brief Add at end the elements from source range
	* @pre Behavior is undefined if all of the following apply: source refers to any elements in this dynarray,
	*	source.size() does not exist and source does not model forward_range (C++20 concept)
	* @return `begin(source)` incremented by source size. The iterator is already invalidated (do not dereference) if
	*	`begin(source)` pointed into this dynarray and there was insufficient capacity to avoid reallocation.
	*
	* Passing references to this dynarray is supported. The function is otherwise equivalent to
	* `std::vector::insert(end(), begin(source), end(source))`, where `end(source)` is not needed if source.size() exists. */
	template<typename InputRange>
	auto append(const InputRange & source) -> iterator_t<InputRange const>;
	//! Equivalent to `std::vector::insert(end(), il)`
	void append(std::initializer_list<T> il)    { append<>(il); }
	//! Equivalent to `std::vector::insert(end(), count, val)`
	void append(size_type count, const T & val);

	/**
	* @brief Default-initializes added elements, can be significantly faster if T is scalar or trivially constructible
	*
	* Objects of scalar type get indeterminate values. http://en.cppreference.com/w/cpp/language/default_initialization  */
	void resize_default_init(size_type n)   { _resizeImpl(n, _detail::UninitDefaultConstruct<Alloc, T>); }
	void resize(size_type n)                { _resizeImpl(n, _uninitFill{}); }

	//! @brief Equivalent to `std::vector::insert(pos, begin(source), end(source))`,
	//!	where `end(source)` is not needed if source.size() exists
	template<typename ForwardRange>
	iterator  insert_r(const_iterator pos, const ForwardRange & source) &;

	iterator  insert(const_iterator pos, std::initializer_list<T> il) &  { return insert_r(pos, il); }

	iterator  insert(const_iterator pos, T && val) &       { return emplace(pos, std::move(val)); }
	iterator  insert(const_iterator pos, const T & val) &  { return emplace(pos, val); }

	//! Does list-initialization `T{...}` of element if no constructor matches Args (using default allocator)
	template<typename... Args>
	iterator  emplace(const_iterator pos, Args &&... elemInitArgs) &;

	//! Does list-initialization `T{...}` of element if no constructor matches Args (using default allocator)
	template<typename... Args>
	reference emplace_back(Args &&... args) &;

	void      push_back(T && val)       { emplace_back(std::move(val)); }
	void      push_back(const T & val)  { emplace_back(val); }

	void      pop_back() noexcept(nodebug);

	/**
	* @brief Erase the element at pos without maintaining order of elements after pos.
	*
	* Constant complexity (compared to linear in the distance between pos and end() for normal erase).
	* @return iterator corresponding to the same index in the sequence as pos, same as for std containers. */
	iterator  erase_unstable(iterator pos) &  { _eraseUnorder(pos, is_trivially_relocatable<T>());  return pos; }

	iterator  erase(iterator pos) &           { _erase(pos, is_trivially_relocatable<T>());  return pos; }

	iterator  erase(iterator first, const_iterator last) &;
	//! Equivalent to `erase(first, end())`, but potentially faster and does not require trivially relocatable T
	void      erase_to_end(iterator first) noexcept(nodebug);

	void      clear() noexcept         { erase_to_end(begin()); }

	bool      empty() const noexcept   { return m.first.data == m.first.end; }

	size_type size() const noexcept    { return m.first.end - m.first.data; }

	void      reserve(size_type minCap)   { if (capacity() < minCap) _growTo(minCap); }

	//! It's a good idea to check that size() < capacity() before calling to avoid useless reallocation
	void      shrink_to_fit();

	size_type capacity() const noexcept   { return m.first.capEnd - m.first.data; }

	size_type max_size() const noexcept   { return AllocTrait::max_size(m.second()) - AllocateWrap::sizeForHeader; }

	//! How much smaller capacity is than the number passed to allocator_type::allocate
	static constexpr size_type allocate_size_overhead()  { return AllocateWrap::sizeForHeader; }

	allocator_type get_allocator() const noexcept   { return m.second(); }

	iterator       begin() noexcept          OEL_ALWAYS_INLINE { return _makeIter(m.first.data); }
	const_iterator begin() const noexcept    OEL_ALWAYS_INLINE { return _makeIter<const T *>(m.first.data); }
	const_iterator cbegin() const noexcept   OEL_ALWAYS_INLINE { return begin(); }

	iterator       end() noexcept          OEL_ALWAYS_INLINE { return _makeIter(m.first.end); }
	const_iterator end() const noexcept    OEL_ALWAYS_INLINE { return _makeIter<const T *>(m.first.end); }
	const_iterator cend() const noexcept   OEL_ALWAYS_INLINE { return end(); }

	reverse_iterator       rbegin() noexcept        OEL_ALWAYS_INLINE { return reverse_iterator{end()}; }
	const_reverse_iterator rbegin() const noexcept  OEL_ALWAYS_INLINE { return const_reverse_iterator{end()}; }

	reverse_iterator       rend() noexcept        OEL_ALWAYS_INLINE { return reverse_iterator{begin()}; }
	const_reverse_iterator rend() const noexcept  OEL_ALWAYS_INLINE { return const_reverse_iterator{begin()}; }

	T *             data() noexcept         OEL_ALWAYS_INLINE { return m.first.data; }
	const T *       data() const noexcept   OEL_ALWAYS_INLINE { return m.first.data; }

	reference       front() noexcept(nodebug)        { return *begin(); }
	const_reference front() const noexcept(nodebug)  { return *begin(); }

	reference       back() noexcept(nodebug)         { return *_makeIter(m.first.end - 1); }
	const_reference back() const noexcept(nodebug)   { return *_makeIter<const T *>(m.first.end - 1); }

	reference       at(size_type index);
	const_reference at(size_type index) const;

	reference       operator[](size_type index) noexcept(nodebug)        { OEL_ASSERT(index < size());
	                                                                       return m.first.data[index]; }
	const_reference operator[](size_type index) const noexcept(nodebug)  { OEL_ASSERT(index < size());
	                                                                       return m.first.data[index]; }
	friend bool operator==(const dynarray & left, const dynarray & right)
		{
			return left.size() == right.size() and
			       std::equal(left.begin(), left.end(), right.begin());
		}
	friend bool operator!=(const dynarray & left, const dynarray & right)  { return !(left == right); }

	friend bool operator <(const dynarray & left, const dynarray & right)
		{
			return std::lexicographical_compare(left.begin(), left.end(), right.begin(), right.end());
		}
	friend bool operator >(const dynarray & left, const dynarray & right)  { return right < left; }



////////////////////////////////////////////////////////////////////////////////
//
// Implementation only in rest of the file


private:
	using _base = _detail::DynarrBase<Alloc>;
	using _base::AllocTrait;
	using _base::AllocateWrap;
	using _base::pointer;
	using _uninitFill = _detail::UninitFill<Alloc>;
	using _debugSizeUpdater = _detail::DebugSizeInHeaderUpdater<_base::Values>;

	using _base::m;

	struct _scopedPtr : private _detail::RefOptimizeEmpty<Alloc>
	{
		pointer data;  // owner
		pointer memEnd;

		struct Span
		{
			pointer p;
			size_type n;
		};

		using A = typename _scopedPtr::Ty;

		_scopedPtr(A & alloc, Span const buf)
		 :	_scopedPtr::RefOptimizeEmpty{alloc},
			data{buf.p},
			memEnd{data + buf.n} {
		}

		_scopedPtr(_scopedPtr &&) = delete;

		~_scopedPtr()
		{
			if (data)
				_detail::DebugAllocateWrapper<A, pointer>::deallocate(this->get(), data, memEnd - data);
		}
	};

	void _swap(_scopedPtr & other)
	{
		using std::swap;
		swap(m.first.data, other.data);
		swap(m.first.capEnd, other.memEnd);
	}

	using _span = typename _scopedPtr::Span;

	_span _allocateAdd(size_type const nAdd, size_type const oldSize)
	{
		if (nAdd <= std::numeric_limits<size_type>::max() / sizeof(T) / 2)
		{
			size_type newCap = _calcNewCap(oldSize + nAdd);
			return {AllocateWrap::allocate(m.second(), newCap), newCap};
		}
		_detail::Throw::LengthError(this->lenErrorMsg);
	}

	_span _allocateAddOne(size_type = 0, size_type = 0)
	{
		constexpr auto startBytesGood = oel_max(3 * sizeof(void *), 4 * sizeof(int));
		constexpr auto minGrow = (startBytesGood - 1) / sizeof(T) + 1;
		size_type c = capacity();
		c += oel_max(c, minGrow); // growth factor is 2

		return {AllocateWrap::allocate(m.second(), c), c};
	}

	size_type _calcNewCap(size_type const newSize) const
	{
		return oel_max(2 * capacity(), newSize);
	}


	size_type _unusedCapacity() const
	{
		return m.first.capEnd - m.first.end;
	}

	template<typename Ptr>
	OEL_ALWAYS_INLINE auto _makeIter(Ptr pos) const noexcept
	{
	#if OEL_MEM_BOUND_DEBUG_LVL
		return _detail::MakeDynarrayIter(pos, m.first.data, this);
	#else
		return pos;
	#endif
	}


	void _moveAssignAlloc(std::true_type, Alloc & src) noexcept
	{
		m.second() = std::move(src);
	}

	OEL_ALWAYS_INLINE void _moveAssignAlloc(std::false_type, Alloc &) {}

	void _swapAlloc(std::true_type, Alloc & a) noexcept
	{
		using std::swap;
		swap(m.second(), a);
	}

	void _swapAlloc(std::false_type, Alloc & a)
	{	// propagate_on_container_swap false, standard says this is undefined if allocators compare unequal
		OEL_ASSERT(m.second() == a);
		(void) a;
	}


	void _initPostAllocate(const T * src)
	{
		_debugSizeUpdater guard{m.first};

		m.first.end = m.first.capEnd;
		_detail::UninitCopy(src, m.first.data, m.first.end, m.second());
	}

#ifdef __GNUC__
	#pragma GCC diagnostic push
	#if __GNUC__ >= 8
	#pragma GCC diagnostic ignored "-Wclass-memaccess"
	#endif
#endif

	T * _relocateImpl(T *__restrict dest, size_type, false_type) const noexcept
	{
		OEL_WHEN_EXCEPTIONS_ON(
			static_assert(std::is_nothrow_move_constructible<T>::value,
				"Reallocation in dynarray requires that T is noexcept move constructible or trivially relocatable");
		)
		T *__restrict src = m.first.data;
		while (src != m.first.end)
		{
			::new(static_cast<void *>(dest)) T(std::move(*src));
			src-> ~T();
			++src; ++dest;
		}
		return dest;
	}

	T * _relocateImpl(T *const dest, size_type const n, true_type) const noexcept
	{
		T * dLast = dest + n;
		_detail::MemcpyCheck(m.first.data, n, dest);
		return dLast;
	}
	// Returns destination end
	OEL_ALWAYS_INLINE T * _relocateData(T * dest, size_type n)
	{
		return _relocateImpl(dest, n, is_trivially_relocatable<T>());
	}


	void _growTo(size_type newCap)
	{
		_debugSizeUpdater guard{m.first};

		_scopedPtr newBuf{m.second(), {this->allocateExact(newCap), newCap}};
		m.first.end = _relocateData(newBuf.data, size());
		_swap(newBuf);
	}

	template<typename UninitFillFunc>
	void _resizeImpl(size_type const newSize, UninitFillFunc initAdded)
	{	// note: initAdded cannot hold a reference to element of this
		_debugSizeUpdater guard{m.first};

		if (capacity() < newSize)
			_growTo(_calcNewCap(newSize));

		T *const newEnd = m.first.data + newSize;
		if (m.first.end < newEnd)
			initAdded(m.first.end, newEnd, m.second());
		else
			_detail::Destroy(newEnd, m.first.end);

		m.first.end = newEnd;
	}


	void _eraseUnorder(iterator pos, false_type) // non-trivial relocation
	{
		*pos = std::move(back());
		pop_back();
	}

	void _eraseUnorder(iterator const pos, true_type)
	{
		_debugSizeUpdater guard{m.first};

		T *const ptr = std::addressof(*pos);
		ptr-> ~T();
		--m.first.end;
		auto mem = reinterpret_cast<aligned_union_t<T> *>(ptr);
		*mem = *reinterpret_cast<aligned_union_t<T> *>(m.first.end); // relocate last element to pos
	}

	void _erase(iterator const pos, true_type /*trivialRelocate*/)
	{
		_debugSizeUpdater guard{m.first};

		T *const ptr = to_pointer_contiguous(pos);
		OEL_ASSERT(m.first.data <= ptr and ptr < m.first.end);

		ptr-> ~T();
		const T * next = ptr + 1;
		std::memmove(ptr, next, sizeof(T) * (m.first.end - next)); // relocate [pos + 1, end) to [pos, end - 1)
		--m.first.end;
	}

	void _erase(iterator const pos, false_type)
	{
		_debugSizeUpdater guard{m.first};

		iterator last = std::move(pos + 1, end(), pos);
		m.first.end = to_pointer_contiguous(last);
		(*m.first.end).~T();
	}


	void _doElementwiseMove(dynarray & src, false_type)
	{
		assign(view::move(src));
	}

	void _doElementwiseMove(dynarray & src, true_type /*trivialRelocate*/)
	{
		_debugSizeUpdater guard{src.m.first};

		_detail::Destroy(m.first.data, m.first.end);
		_assignImpl(src.begin(), src.size(), true_type{});
		src.m.first.end = src.m.first.data; // elements relocated from src
	}

	void _elementwiseMove(dynarray & src)
	{
		constexpr bool canBypassConstruct = !_detail::AllocHasConstruct<Alloc, T &&>::value;
		_doElementwiseMove( src,
			bool_constant< is_trivially_relocatable<T>::value and canBypassConstruct >{} );
	}


	template<typename ContiguousIter>
	ContiguousIter _assignImpl(ContiguousIter const first, size_type const count, true_type /*trivialCopy*/)
	{
		_debugSizeUpdater guard{m.first};

		if (capacity() < count)
		{
			// Deallocating first might be better, but then the m pointers would have to be nulled in case allocate throws
			T *const newData = this->allocateExact(count);
			this->deallocate();
			m.first.data = newData;
			m.first.capEnd = m.first.end = newData + count;
		}
		else
		{	m.first.end = m.first.data + count;
		}
		// Not portable. Check for self assignment or use memmove?
		_detail::MemcpyCheck(first, count, m.first.data);

		return first + count;
	}

	template<typename InputIter>
	InputIter _assignImpl(InputIter src, size_type const count, false_type)
	{
		_debugSizeUpdater guard{m.first};

		auto copy = [](InputIter src_, T * dest, T * dLast)
		{
			while (dest != dLast)
			{
				*dest = *src_;
				++src_; ++dest;
			}
			return src_;
		};
		T * newEnd;
		if (capacity() < count)
		{
			T *const newData = this->allocateExact(count);
			// Old elements might hold some limited resource, destroying them before constructing new is probably good
			_detail::Destroy(m.first.data, m.first.end);
			this->deallocate();
			m.first.data = newData;
			m.first.end  = newData;
			m.first.capEnd = newData + count;
			newEnd = m.first.capEnd;
		}
		else
		{	newEnd = m.first.data + count;
			if (newEnd < m.first.end)
			{	// downsizing, assign new and destroy rest
				src = copy(src, m.first.data, newEnd);
				erase_to_end(_makeIter(newEnd));
			}
			else // assign to old elements as far as we can
			{	src = copy(src, m.first.data, m.first.end);
			}
		}
		while (m.first.end < newEnd)
		{	// each iteration updates end for exception safety
			_detail::Construct(m.second(), m.first.end, *src);
			++src; ++m.first.end;
		}
		return src;
	}

	template<typename InputIter, typename Sentinel>
	InputIter _assignImpl(InputIter first, Sentinel const last, false_type)
	{	// single pass iterator, no size available
		clear();
		for (; first != last; ++first)
			emplace_back(*first);

		return first;
	}

	template<typename InputIter, typename Sentinel>
	InputIter _append(InputIter first, Sentinel const last, false_type)
	{	// single pass iterator, no size available
		size_type const oldSize = size();
		OEL_TRY_
		{
			for (; first != last; ++first)
				emplace_back(*first);
		}
		OEL_CATCH_ALL
		{
			erase_to_end(begin() + oldSize);
			OEL_WHEN_EXCEPTIONS_ON(throw);
		}
		return first;
	}

	template<typename InputIter>
	InputIter _append(InputIter src, size_type const n, false_type)
	{	// cannot use memcpy
		_appendImpl( n,
			[&src](T * dest, size_type n_, Alloc & a)
			{
				src = _detail::UninitCopy(src, dest, dest + n_, a);
			} );
		return src;
	}

	template<typename ContiguousIter>
	ContiguousIter _append(ContiguousIter const first, size_type const n, true_type)
	{
		ContiguousIter last = first + n;
		_appendImpl( n,
			[first](T * dest, size_type n_, Alloc &)
			{
				_detail::MemcpyCheck(first, n_, dest);
			} );
		return last; // has been invalidated in the case of append self and reallocation
	}

	template<typename MakeFuncAppend>
	void _appendImpl(size_type const count, MakeFuncAppend const makeNew)
	{
		_debugSizeUpdater guard{m.first};

		if (_unusedCapacity() >= count)
			makeNew(m.first.end, count, m.second());
		else
			_appendRealloc(count, makeNew);

		m.first.end += count;
	}

	template<typename MakeFuncAppend>
#ifdef _MSC_VER
	__declspec(noinline) // to get the compiler to inline calling function
#else
	__attribute__((noinline))
#endif
	void _appendRealloc(size_type const count, MakeFuncAppend const makeNew)
	{
		size_type const oldSize = size();
		_scopedPtr newBuf{m.second(), _allocateAdd(count, oldSize)};

		T *const pos = newBuf.data + oldSize;
		makeNew(pos, count, m.second());
		_relocateData(newBuf.data, oldSize);

		m.first.end = pos;
		_swap(newBuf);
	}

	template<typename... Args>
	void _emplaceBackRealloc(Args &&... args)
	{
		_scopedPtr newBuf{m.second(), _allocateAddOne()};

		T *const pos = newBuf.data + size();
		_detail::Construct(m.second(), pos, static_cast<Args &&>(args)...);
		_relocateData(newBuf.data, size());

		m.first.end = pos;
		_swap(newBuf);
	}


	template< _span(dynarray::*AllocFunc)(size_t, size_t),
		typename MakeFuncInsert, typename... Args >
	T * _insertRealloc(
		T *const pos, size_type const nAfterPos, size_type const nAdd,
		MakeFuncInsert const makeNew, Args &&... args)
	{
		_scopedPtr newBuf{m.second(), (this->*AllocFunc)(nAdd, size())};

		size_type const nBefore = pos - data();
		T *const newPos = newBuf.data + nBefore;
		T *const afterAdded = makeNew(newPos, nAdd, m.second(), static_cast<Args &&>(args)...);
		// Exception free from here
		if (m.first.data)
		{
			std::memcpy(newBuf.data, data(), sizeof(T) * nBefore); // relocate prefix
			std::memcpy(afterAdded, pos, sizeof(T) * nAfterPos);  // relocate suffix
		}
		m.first.end = afterAdded + nAfterPos;
		_swap(newBuf);

		return newPos;
	}

	struct _emplaceMakeElem
	{
		template<typename... Args>
		T * operator()(T *const newPos, size_type, Alloc & a, Args &&... args) const
		{
			_detail::Construct(a, newPos, static_cast<Args &&>(args)...);
			return newPos + 1;
		}
	};
};

template<typename T, typename Alloc> template<typename... Args>
typename dynarray<T, Alloc>::iterator
	dynarray<T, Alloc>::emplace(const_iterator pos, Args &&... args) &
{
#define OEL_DYNARR_INSERT_STEP1  \
	_detail::AssertTrivialRelocate<T>{};  \
	\
	_debugSizeUpdater guard{m.first};  \
	\
	auto pPos = const_cast<T *>(to_pointer_contiguous(pos));  \
	OEL_ASSERT(m.first.data <= pPos and pPos <= m.first.end);  \
	size_type const nAfterPos = m.first.end - pPos;

	OEL_DYNARR_INSERT_STEP1
	if (m.first.end < m.first.capEnd)
	{
		// Temporary in case constructor throws or source is an element of this dynarray at pos or after
		aligned_union_t<T> tmp;
		_detail::Construct(m.second(), reinterpret_cast<T *>(&tmp), static_cast<Args &&>(args)...);
		// Relocate [pos, end) to [pos + 1, end + 1), leaving memory at pos uninitialized (conceptually)
		std::memmove(pPos + 1, pPos, sizeof(T) * nAfterPos);
		++m.first.end;

		std::memcpy(pPos, &tmp, sizeof(T)); // relocate the new element to pos
	}
	else
	{	pPos = _insertRealloc< &dynarray::_allocateAddOne >
			(pPos, nAfterPos, {}, _emplaceMakeElem{}, static_cast<Args &&>(args)...);
	}
	return _makeIter(pPos);
}

template<typename T, typename Alloc> template<typename ForwardRange>
typename dynarray<T, Alloc>::iterator
	dynarray<T, Alloc>::insert_r(const_iterator pos, const ForwardRange & src) &
{
	auto first = oel::adl_begin(src);
	auto const count = _detail::SizeOrEnd(src);

	static_assert( std::is_same<decltype(count), size_t const>::value,
			"insert_r requires that begin(source) is a ForwardIterator (multi-pass)" );

	OEL_DYNARR_INSERT_STEP1
#undef OEL_DYNARR_INSERT_STEP1

	if (_unusedCapacity() >= count)
	{
		T *const dLast = pPos + count;
		// Relocate elements to make space, leaving [pos, pos + count) uninitialized (conceptually)
		std::memmove(dLast, pPos, sizeof(T) * nAfterPos);
		m.first.end += count;
		// Construct new
		OEL_CONST_COND if (can_memmove_with<T *, decltype(first)>::value)
		{
			_detail::UninitCopy(first, pPos, dLast, m.second());
		}
		else
		{	T * dest = pPos;
			OEL_TRY_
			{
				while (dest != dLast)
				{
					_detail::Construct(m.second(), dest, *first);
					++first; ++dest;
				}
			}
			OEL_CATCH_ALL
			{	// relocate back to fill hole
				std::memmove(dest, dLast, sizeof(T) * nAfterPos);
				m.first.end -= (dLast - dest);
				OEL_WHEN_EXCEPTIONS_ON(throw);
			}
		}
	}
	else // not enough room
	{
		auto make = [first](T *const newPos, size_type count_, Alloc & a)
		{
			T *const dLast = newPos + count_;
			_detail::UninitCopy(first, newPos, dLast, a);
			return dLast;
		};
		pPos = _insertRealloc< &dynarray::_allocateAdd >(pPos, nAfterPos, count, make);
	}
	return _makeIter(pPos);
}

template<typename T, typename Alloc> template<typename... Args>
inline T & dynarray<T, Alloc>::emplace_back(Args &&... args) &
{
	_debugSizeUpdater guard{m.first};

	if (m.first.end < m.first.capEnd)
		_detail::Construct(m.second(), m.first.end, static_cast<Args &&>(args)...);
	else
		_emplaceBackRealloc(static_cast<Args &&>(args)...);

	T *const pos = m.first.end;
	++m.first.end;

	return *pos;
}


template<typename T, typename Alloc>
dynarray<T, Alloc>::dynarray(dynarray && other, const Alloc & a)
 :	_base(a)
{
	OEL_CONST_COND if (!is_always_equal<Alloc>::value and a != other.m.second())
		_elementwiseMove(other);
	else
		m.first.stealBuf(other.m.first);
}

template<typename T, typename Alloc>
dynarray<T, Alloc> &  dynarray<T, Alloc>::operator =(dynarray && other) &
	noexcept(AllocTrait::propagate_on_container_move_assignment::value or is_always_equal<Alloc>::value)
{
	OEL_CONST_COND if (!AllocTrait::propagate_on_container_move_assignment::value
	               and m.second() != other.m.second())
	{
		_elementwiseMove(other);
	}
	else // take allocated memory from other
	{
		if (m.first.data)
		{
			_detail::Destroy(m.first.data, m.first.end);
			AllocateWrap::deallocate(m.second(), m.first.data, m.first.capEnd - m.first.data);
		}
		m.first.stealBuf(other.m.first);
		_moveAssignAlloc(typename AllocTrait::propagate_on_container_move_assignment{}, other.m.second());
	}
	return *this;
}

template<typename T, typename Alloc>
dynarray<T, Alloc>::dynarray(size_type n, default_init_t, const Alloc & a)
 :	_base(a, n)
{
	_debugSizeUpdater guard{m.first};

	m.first.end = m.first.capEnd;
	_detail::UninitDefaultConstruct(m.first.data, m.first.end, m.second());
}

template<typename T, typename Alloc>
dynarray<T, Alloc>::dynarray(size_type n, const Alloc & a)
 :	_base(a, n)
{
	_debugSizeUpdater guard{m.first};

	m.first.end = m.first.capEnd;
	_uninitFill{}(m.first.data, m.first.end, m.second());
}

template<typename T, typename Alloc>
dynarray<T, Alloc>::dynarray(size_type n, const T & val, const Alloc & a)
 :	_base(a, n)
{
	_debugSizeUpdater guard{m.first};

	m.first.end = m.first.capEnd;
	_uninitFill{}(m.first.data, m.first.end, m.second(), val);
}

template<typename T, typename Alloc>
dynarray<T, Alloc>::~dynarray() noexcept
{
	_detail::Destroy(m.first.data, m.first.end);
}

template<typename T, typename Alloc>
void dynarray<T, Alloc>::swap(dynarray & other)
	noexcept(AllocTrait::propagate_on_container_swap::value or is_always_equal<Alloc>::value)
{
	std::swap(m.first, other.m.first);
	_swapAlloc(typename AllocTrait::propagate_on_container_swap{}, other.m.second());
}


template<typename T, typename Alloc>
void dynarray<T, Alloc>::shrink_to_fit()
{
	_debugSizeUpdater guard{m.first};

	size_type const used = size();
	T * newData;
	if (0 < used)
	{
		newData = AllocateWrap::allocate(m.second(), used);
		m.first.end = _relocateData(newData, used);
	}
	else
	{	m.first.end = newData = nullptr;
	}
	this->deallocate();
	m.first.data = newData;
	m.first.capEnd = m.first.end;
}


template<typename T, typename Alloc> template<typename InputRange>
auto dynarray<T, Alloc>::assign(const InputRange & src) -> iterator_t<InputRange const>
{
	return _assignImpl(
		oel::adl_begin(src),
		_detail::SizeOrEnd(src),
		can_memmove_with< T *, iterator_t<InputRange const> >() );
}

template<typename T, typename Alloc>
inline void dynarray<T, Alloc>::append(size_type n, const T & val)
{
	_appendImpl( n,
		[&val](T * dest, size_type n_, Alloc & a)
		{
			_uninitFill{}(dest, dest + n_, a, val);
		} );
}

template<typename T, typename Alloc> template<typename InputRange>
inline auto dynarray<T, Alloc>::append(const InputRange & src) -> iterator_t<InputRange const>
{
	return _append(oel::adl_begin(src),
	               _detail::SizeOrEnd(src),
	               can_memmove_with< T *, iterator_t<InputRange const> >());
}


template<typename T, typename Alloc>
inline void dynarray<T, Alloc>::pop_back() noexcept(nodebug)
{
	OEL_ASSERT(m.first.data < m.first.end);
	_debugSizeUpdater guard{m.first};

	--m.first.end;
	(*m.first.end).~T();
}

template<typename T, typename Alloc>
inline void dynarray<T, Alloc>::erase_to_end(iterator first) noexcept(nodebug)
{
	_debugSizeUpdater guard{m.first};

	T *const newEnd = to_pointer_contiguous(first);
	OEL_ASSERT(m.first.data <= newEnd and newEnd <= m.first.end);
	_detail::Destroy(newEnd, m.first.end);
	m.first.end = newEnd;
}

template<typename T, typename Alloc>
typename dynarray<T, Alloc>::iterator  dynarray<T, Alloc>::erase(iterator first, const_iterator last) &
{
	(void) _detail::AssertTrivialRelocate<T>{};

	_debugSizeUpdater guard{m.first};

	T *const      pFirst = to_pointer_contiguous(first);
	const T *const pLast = to_pointer_contiguous(last);
	OEL_ASSERT(m.first.data <= pFirst and pFirst <= pLast and pLast <= m.first.end);
	if (pFirst < pLast)
	{
		_detail::Destroy(pFirst, pLast);
		size_type const nAfterLast = m.first.end - pLast;
		// Relocate [last, end) to [first, first + nAfterLast)
		std::memmove(pFirst, pLast, sizeof(T) * nAfterLast);
		m.first.end = pFirst + nAfterLast;
	}
	return first;
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

template<typename T, typename Alloc>
OEL_ALWAYS_INLINE inline T & dynarray<T, Alloc>::at(size_type i)
{
	const auto & cSelf = *this;
	return const_cast<reference>(cSelf.at(i));
}
template<typename T, typename Alloc>
const T & dynarray<T, Alloc>::at(size_type i) const
{
	if (i < size()) // would be unsafe with signed size_type
		return m.first.data[i];
	else
		_detail::Throw::OutOfRange("Bad index dynarray::at");
}

#ifdef OEL_DYNARRAY_IN_DEBUG
}
#endif

} // namespace oel
