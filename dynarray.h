#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "auxi/algo_detail.h"
#include "auxi/dynarray_iterator.h"
#include "trivial_relocate/default.h"
#include "align_allocator.h"

#include <algorithm>


/** @file
*/

namespace oel
{

//! Mirroring std::pmr and boost::container::pmr
namespace pmr
{
#if defined OEL_HAS_STD_PMR or (!defined OEL_NO_BOOST and BOOST_VERSION >= 106000)
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
template<typename T, typename A> inline
void erase_unstable(dynarray<T, A> & d, typename dynarray<T, A>::size_type index)  { d.erase_unstable(d.begin() + index); }

//! @name GenericContainerInsert
//!@{
// Overloads of generic functions for inserting into container (in range_algo.h)
template<typename T, typename A, typename InputRange> inline
void assign(dynarray<T, A> & dest, const InputRange & source)  { dest.assign(source); }

template<typename T, typename A, typename InputRange> inline
void append(dynarray<T, A> & dest, const InputRange & source)  { dest.append(source); }

template<typename T, typename A> inline
void append(dynarray<T, A> & dest, typename dynarray<T, A>::size_type n, const T & val)  { dest.append(n, val); }

template<typename T, typename A, typename ForwardRange> inline
typename dynarray<T, A>::iterator
	insert(dynarray<T, A> & dest, typename dynarray<T, A>::const_iterator pos, const ForwardRange & source)
	{ return dest.insert_r(pos, source); }
//!@}

#ifdef OEL_DYNARRAY_IN_DEBUG
inline namespace debug
{
#endif

#ifdef OEL_HAS_DEDUCTION_GUIDES
template<typename InputRange,
         typename Alloc = allocator<iterator_value_t< range_iterator_t<InputRange> >>
        >
explicit dynarray(const InputRange &, Alloc = Alloc{})
->	dynarray<iterator_value_t< range_iterator_t<InputRange> >, Alloc>;
#endif

/**
* @brief Resizable array, dynamically allocated. Very similar to std::vector, but much faster in many cases.
*
* In general, only that which differs from std::vector is documented.
*
* Efficiency is better if template argument T is trivially relocatable, which is true for most types, but needs
* to be declared manually for each type that is not trivially copyable. See specify_trivial_relocate(T &&).
* Also, if T is trivially relocatable, it does not need to be copyable or even move constructible (you can use
* emplace_back, pop_back, erase). There are a few notable functions for which trivially relocatable T is required
* (checked when compiling): emplace, insert, insert_r and `erase(first, last)`
*
* About strong exception guarantee: For all functions which for std::vector are specified to have no effect
* if an exception is thrown: If T's move constructor is not noexcept and T is not trivially relocatable,
* dynarray will use the throwing move constructor, and the exception guarantee is lowered to basic.
*
* The default allocator supports over-aligned types (e.g. __m256)  */
template<typename T, typename Alloc/* = oel::allocator */>
class dynarray
{
	using _allocTrait = std::allocator_traits<Alloc>;

public:
	using value_type      = T;
	using allocator_type  = Alloc;
	using reference       = T &;
	using const_reference = const T &;
	using pointer         = typename _allocTrait::pointer;
	using const_pointer   = typename _allocTrait::const_pointer;
	using difference_type = typename _allocTrait::difference_type;
	using size_type       = size_t;

#if OEL_MEM_BOUND_DEBUG_LVL
	using iterator       = debug::dynarray_iterator<pointer, T>;
	using const_iterator = debug::dynarray_iterator<const_pointer, T>;
#else
	using iterator       = pointer;
	using const_iterator = const_pointer;
#endif
	using reverse_iterator       = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;


	constexpr dynarray() noexcept                : _m(Alloc{}) {}
	explicit dynarray(const Alloc & a) noexcept  : _m(a) {}

	//! Construct empty dynarray with space reserved for at least minCap elements
	dynarray(reserve_tag, size_type minCap, const Alloc & a = Alloc{})  : _m(a, minCap) {}

	/** @brief Default-initializes elements, can be significantly faster if T is scalar or has trivial default constructor
	*
	* @copydetails resize(size_type, default_init_t)  */
	dynarray(size_type size, default_init_t, const Alloc & a = Alloc{});
	explicit dynarray(size_type size, const Alloc & a = Alloc{});  //!< (Value-initializes elements, same as std::vector)
	dynarray(size_type size, const T & fillVal, const Alloc & a = Alloc{});

	/** @brief Equivalent to `std::vector(begin(r), end(r), a)`, where `end(r)` is not needed if r.size() exists
	*
	* To move instead of copy, wrap r with view::move (the same applies for all functions taking a range template) <br>
	* Example, construct from a standard istream with formatting (using Boost):
	* @code
	#include <boost/range/istream_range.hpp>
	// ...
	auto result = dynarray<int>(boost::range::istream_range<int>(someStream));
	@endcode  */
	template< typename InputRange,
		typename /*EnableIfRange*/ = range_iterator_t<InputRange> >
	explicit dynarray(const InputRange & r, const Alloc & a = Alloc{})  : _m(a) { append(r); }

	dynarray(std::initializer_list<T> il, const Alloc & a = Alloc{})  : _m(a, il.size())
	                                                                  { _initPostAllocate(il.begin()); }
	dynarray(dynarray && other) noexcept               : _m(std::move(other._m)) {}
	dynarray(dynarray && other, const Alloc & a);
	dynarray(const dynarray & other);
	dynarray(const dynarray & other, const Alloc & a)  : _m(a, other.size())
	                                                   { _initPostAllocate(other.data()); }
	~dynarray() noexcept;

	dynarray & operator =(dynarray && other) &
		noexcept(_allocTrait::propagate_on_container_move_assignment::value or is_always_equal<Alloc>::value);
	//! Treats allocator_type as if it does not have propagate_on_container_copy_assignment
	dynarray & operator =(const dynarray & other) &    { assign(other);  return *this; }

	dynarray & operator =(std::initializer_list<T> il) &  { assign(il);  return *this; }

	void      swap(dynarray & other) noexcept(_allocTrait::propagate_on_container_swap::value
	                                          or is_always_equal<Alloc>::value);
	/**
	* @brief Replace the contents with source range
	* @param source an array, STL container, gsl::span, boost::iterator_range or such.
	* @pre `begin(source)` shall not point to any element in this dynarray except the front.
	* @return iterator `begin(source)` incremented by the number of elements in source
	*
	* Any elements held before the call are either assigned to or destroyed. */
	template<typename InputRange>
	auto      assign(const InputRange & source) -> decltype(::adl_begin(source));

	void      assign(size_type count, const T & val)  { clear();  append(count, val); }

	/**
	* @brief Add at end the elements from range (return past-the-last of source)
	* @param source an array, STL container, gsl::span, boost::iterator_range or such.
	* @pre Behavior is undefined if all of the following apply: source refers to any elements in this dynarray,
	*	source.size() does not exist and source does not model Forward Range (Boost concept)
	* @return `begin(source)` incremented by source size. The iterator is already invalidated (do not dereference) if
	*	`begin(source)` pointed into this dynarray and there was insufficient capacity to avoid reallocation.
	*
	* Strong exception guarantee if T is noexcept move constructible or trivially relocatable. Otherwise equivalent to
	* `std::vector::insert(end(), begin(source), end(source))`, where `end(source)` is not needed if source.size() exists. */
	template<typename InputRange>
	auto      append(const InputRange & source) -> decltype(::adl_begin(source));
	//! @brief Equivalent to `std::vector::insert(end(), il)`, but
	//!	with strong exception guarantee if T is noexcept move constructible or trivially relocatable
	void      append(std::initializer_list<T> il)   { append<>(il); }
	//! @brief Equivalent to `std::vector::insert(end(), count, val)`, but
	//!	with strong exception guarantee if T is noexcept move constructible or trivially relocatable
	void      append(size_type count, const T & val);

	/**
	* @brief Default-initializes added elements, can be significantly faster if T is scalar or trivially constructible
	*
	* Objects of scalar type get indeterminate values. http://en.cppreference.com/w/cpp/language/default_initialization  */
	void      resize(size_type n, default_init_t)  { _resizeImpl(n, _detail::UninitDefaultConstruct<Alloc, T>); }
	//! (Value-initializes added elements, same as std::vector::resize)
	void      resize(size_type n)                  { _resizeImpl(n, _uninitFill{}); }

	//! @brief Equivalent to `std::vector::insert(pos, begin(source), end(source))`,
	//!	where `end(source)` is not needed if source.size() exists
	template<typename ForwardRange>
	iterator  insert_r(const_iterator pos, const ForwardRange & source);

	iterator  insert(const_iterator pos, std::initializer_list<T> il)  { return insert_r(pos, il); }

	iterator  insert(const_iterator pos, T && val)       { return emplace(pos, std::move(val)); }
	iterator  insert(const_iterator pos, const T & val)  { return emplace(pos, val); }

	//! Does list-initialization `T{...}` of element if no constructor matches Args (using default allocator)
	template<typename... Args>
	iterator  emplace(const_iterator pos, Args &&... elemInitArgs);

	//! Does list-initialization `T{...}` of element if no constructor matches Args (using default allocator)
	template<typename... Args>
	reference emplace_back(Args &&... args);

	void      push_back(T && val)       { emplace_back(std::move(val)); }
	void      push_back(const T & val)  { emplace_back(val); }

	void      pop_back() noexcept(nodebug);

	/**
	* @brief Erase the element at pos without maintaining order of elements after pos.
	*
	* Constant complexity (compared to linear in the distance between pos and end() for normal erase).
	* @return iterator corresponding to the same index in the sequence as pos, same as for std containers. */
	iterator  erase_unstable(iterator pos)  { _eraseUnorder(pos, is_trivially_relocatable<T>());  return pos; }

	iterator  erase(iterator pos)           { _erase(pos, is_trivially_relocatable<T>());  return pos; }

	iterator  erase(iterator first, const_iterator last);
	//! Equivalent to `erase(first, end())`, but potentially faster and does not require trivially relocatable T
	void      erase_to_end(iterator first) noexcept(nodebug);

	void      clear() noexcept        { erase_to_end(begin()); }

	bool      empty() const noexcept  { return _m.data == _m.end; }

	size_type size() const noexcept   { return _m.end - _m.data; }

	void      reserve(size_type minCap)  { if (capacity() < minCap) _growTo(minCap); }

	//! It's a good idea to check that size() < capacity() before calling to avoid useless reallocation
	void      shrink_to_fit();

	size_type capacity() const noexcept  { return _m.reservEnd - _m.data; }

	size_type max_size() const noexcept  { return _allocTrait::max_size(_m) - _allocateWrap::sizeForHeader; }

	//! How much smaller capacity is than the number passed to allocator_type::allocate
	static constexpr size_type allocate_size_overhead()  { return _allocateWrap::sizeForHeader; }

	allocator_type get_allocator() const noexcept  { return _m; }

	iterator       begin() noexcept          OEL_ALWAYS_INLINE { return _makeIter<iterator>(_m.data); }
	const_iterator begin() const noexcept    OEL_ALWAYS_INLINE { return _makeIter<const_iterator>(_m.data); }
	const_iterator cbegin() const noexcept   OEL_ALWAYS_INLINE { return begin(); }

	iterator       end() noexcept          OEL_ALWAYS_INLINE { return _makeIter<iterator>(_m.end); }
	const_iterator end() const noexcept    OEL_ALWAYS_INLINE { return _makeIter<const_iterator>(_m.end); }
	const_iterator cend() const noexcept   OEL_ALWAYS_INLINE { return end(); }

	reverse_iterator       rbegin() noexcept         OEL_ALWAYS_INLINE { return reverse_iterator{end()}; }
	const_reverse_iterator rbegin() const noexcept   OEL_ALWAYS_INLINE { return const_reverse_iterator{end()}; }

	reverse_iterator       rend() noexcept         OEL_ALWAYS_INLINE { return reverse_iterator{begin()}; }
	const_reverse_iterator rend() const noexcept   OEL_ALWAYS_INLINE { return const_reverse_iterator{begin()}; }

	T *             data() noexcept         OEL_ALWAYS_INLINE { return _m.data; }
	const T *       data() const noexcept   OEL_ALWAYS_INLINE { return _m.data; }

	reference       front() noexcept(nodebug)        { return *begin(); }
	const_reference front() const noexcept(nodebug)  { return *begin(); }

	reference       back() noexcept(nodebug)         { return *_makeIter<iterator>(_m.end - 1); }
	const_reference back() const noexcept(nodebug)   { return *_makeIter<const_iterator>(_m.end - 1); }

	reference       at(size_type index);
	const_reference at(size_type index) const;

	reference       operator[](size_type index) noexcept(nodebug)        { OEL_ASSERT(index < size());
	                                                                       return _m.data[index]; }
	const_reference operator[](size_type index) const noexcept(nodebug)  { OEL_ASSERT(index < size());
	                                                                       return _m.data[index]; }
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
	using _allocateWrap = _detail::DebugAllocateWrapper<Alloc, pointer>;
	using _uninitFill = _detail::UninitFill<Alloc>;
	using _internBase = _detail::DynarrBase<pointer>;
	using _debugSizeUpdater = _detail::DebugSizeInHeaderUpdater<_internBase>;

	template<typename> // template to allow constexpr constructor when Alloc copy constructor is not constexpr
	struct _memOwner : public _internBase, public Alloc
	{
		using _internBase::data; // owning pointer
		using _internBase::end;
		using _internBase::reservEnd;

		constexpr _memOwner(const Alloc & a)
		 :	_internBase(), Alloc(a) {
		}
		_memOwner(const Alloc & a, size_type const capacity)
		 :	Alloc(a)
		{
			end = data = _allocate(*this, capacity);
			reservEnd = data + capacity;
		}

		_memOwner(_memOwner && other) noexcept
		 :	_internBase(other), Alloc(std::move(other))
		{
			other.reservEnd = other.end = other.data = nullptr;
		}

		~_memOwner()
		{
			if (data)
				_allocateWrap::Deallocate(*this, data, reservEnd - data);
		}
	};
	_memOwner<Alloc> _m; // the only data member


	struct _scopedPtr : private _detail::RefOptimizeEmpty<Alloc>
	{
		pointer data;  // owner
		pointer allocEnd;

		_scopedPtr(Alloc & a, size_type const allocSize)
		 :	_scopedPtr::RefOptimizeEmpty{a},
			data{_allocate(a, allocSize)},
			allocEnd{data + allocSize} {
		}

		_scopedPtr(_scopedPtr &&) = delete;

		~_scopedPtr()
		{
			auto && a = this->Get();
			if (data)
				_allocateWrap::Deallocate(a, data, allocEnd - data);
		}

		void Swap(_internBase & other)
		{
			using std::swap;
			swap(other.data, data);
			swap(other.reservEnd, allocEnd);
		}
	};

	void _resetData(pointer const newData)
	{
		if (_m.data)
			_allocateWrap::Deallocate(_m, _m.data, capacity());

		_m.data = newData;
	}

	static pointer _allocate(Alloc & a, size_type const n)
	{
		if (n <= _allocTrait::max_size(a) - _allocateWrap::sizeForHeader)
			return _allocateWrap::Allocate(a, n); // allocate should throw if subtraction wrapped around
		else
			_detail::Throw::LengthError("Going over dynarray max_size");
	}

	template<typename Iterator>
	Iterator _makeIter(pointer p) const noexcept
	{
	#if OEL_MEM_BOUND_DEBUG_LVL
		if (_m.data)
		{
			const auto *const h = OEL_DEBUG_HEADER_OF(_m.data);
			return {p, h, h->id};
		}
		else
		{	return { p, &_detail::headerNoAllocation,
				reinterpret_cast<std::uintptr_t>(this) };
		}
	#else
		return p;
	#endif
	}


	void _moveInternBase(_internBase & src)
	{
		static_cast<_internBase &>(_m) = src;
		src.reservEnd = src.end = src.data = nullptr;
	}

	void _moveAssignAlloc(std::true_type, Alloc & src)
	{
		Alloc & a = _m;
		a = std::move(src);
	}

	OEL_ALWAYS_INLINE void _moveAssignAlloc(std::false_type, Alloc &) {}

	void _swapAlloc(std::true_type, Alloc & a) noexcept
	{
		using std::swap;
		swap(static_cast<Alloc &>(_m), a);
	}

	void _swapAlloc(std::false_type, Alloc & a)
	{	// propagate_on_container_swap false, standard says this is undefined if allocators compare unequal
		OEL_ASSERT(static_cast<Alloc &>(_m) == a);
		(void) a;
	}

#ifdef __GNUC__
	#pragma GCC diagnostic push
	#if __GNUC__ >= 8
	#pragma GCC diagnostic ignored "-Wclass-memaccess"
	#endif
#endif

	void _initPostAllocate(const T * src)
	{
		_debugSizeUpdater guard{_m};

		_m.end = _m.reservEnd;
		_detail::UninitCopy<Alloc>(src, _m.data, _m.end, _m);
	}


	size_type _unusedCapacity() const
	{
		return _m.reservEnd - _m.end;
	}

	static size_type _calcCapAddOne(size_type const oldCap, size_type = 0)
	{
		constexpr auto startBytesGood = oel_max(3 * sizeof(void *), 4 * sizeof(int));
		constexpr auto minGrow = oel_max<size_t>( startBytesGood / sizeof(T),
				sizeof(T) <= 8 * sizeof(int) ? 2 : 1 );
		OEL_CONST_COND if (minGrow > 1)
			return oldCap + oel_max(oldCap / 2, minGrow);
		else
			return oldCap + oldCap / 2 + 1;
	}

	static size_type _calcCap(size_type oldCap, size_type newSize)
	{	// growth factor is 1.5
		return oel_max(oldCap + oldCap / 2, newSize);
	}


	template<typename... FuncTakingLast>
	void _relocateImpl(std::false_type, T * dFirst, T * dLast, size_type, FuncTakingLast... extraCleanupIfException)
	{	// could combine to single loop in special case when Construct is noexcept
		_detail::UninitCopy<Alloc>(std::make_move_iterator(_m.data), dFirst, dLast, _m, extraCleanupIfException...);
		_detail::Destroy(_m.data, _m.end);
	}

	template<typename... Unused>
	void _relocateImpl(std::true_type, T * dest, T *, size_type n, Unused...)
	{
		_detail::MemcpyCheck(_m.data, n, dest);
	}

	template<typename... FuncTakingLast>
	OEL_ALWAYS_INLINE void _relocateData(T * dest, T * dLast, size_type n, FuncTakingLast... extraCleanupIfException)
	{
		_relocateImpl(is_trivially_relocatable<T>(), dest, dLast, n, extraCleanupIfException...);
	}


	void _growTo(size_type newCap)
	{
		_debugSizeUpdater guard{_m};

		_scopedPtr newBuf{_m, newCap};

		pointer const newEnd = newBuf.data + size();
		_relocateData(newBuf.data, newEnd, size());

		_m.end = newEnd;
		newBuf.Swap(_m);
	}

	template<typename UninitFillFunc>
	void _resizeImpl(size_type const newSize, UninitFillFunc initAdded)
	{	// note: initAdded cannot hold a reference to element of this
		_debugSizeUpdater guard{_m};

		if (capacity() < newSize)
			_growTo(_calcCap(capacity(), newSize));

		T *const newEnd = _m.data + newSize;
		if (_m.end < newEnd)
			initAdded(_m.end, newEnd, _m);
		else
			_detail::Destroy(newEnd, _m.end);

		_m.end = newEnd;
	}


	void _eraseUnorder(iterator pos, false_type) // non-trivial relocation
	{
		*pos = std::move(back());
		pop_back();
	}

	void _eraseUnorder(iterator const pos, true_type)
	{
		_debugSizeUpdater guard{_m};

		T *const ptr = to_pointer_contiguous(pos);
		OEL_ASSERT(_m.data <= ptr and ptr < _m.end);

		ptr-> ~T();
		--_m.end;
		auto mem = reinterpret_cast<aligned_union_t<T> *>(ptr);
		*mem = *reinterpret_cast<aligned_union_t<T> *>(_m.end); // relocate last element to pos
	}

	void _erase(iterator const pos, true_type /*trivialRelocate*/)
	{
		_debugSizeUpdater guard{_m};

		T *const ptr = to_pointer_contiguous(pos);
		OEL_ASSERT(_m.data <= ptr and ptr < _m.end);

		ptr-> ~T();
		const T * next = ptr + 1;
		std::memmove(ptr, next, sizeof(T) * (_m.end - next)); // relocate [pos + 1, end) to [pos, end - 1)
		--_m.end;
	}

	void _erase(iterator const pos, false_type)
	{
		_debugSizeUpdater guard{_m};

		iterator last = std::move(pos + 1, end(), pos);
		_m.end = to_pointer_contiguous(last);
		(*_m.end).~T();
	}


	template<typename Range, typename IterTrav> // pass dummy int to prefer this overload
	static auto _sizeOrEnd(const Range & r, IterTrav, int)
	->	decltype( oel::ssize(r), size_type() ) { return oel::ssize(r); }

	template<typename Range>
	static size_type _sizeOrEnd(const Range & r, forward_traversal_tag, long)
	{
		return std::distance(::adl_begin(r), ::adl_end(r));
	}

	template<typename Range>
	static auto _sizeOrEnd(const Range & r, single_pass_traversal_tag, long)
	{
		return ::adl_end(r);
	}
	// Returns element count as size_type if possible, else adl_end(r)
	template<typename Iter, typename Range>
	static auto _sizeOrEnd(const Range & r)
	{
		return _sizeOrEnd(r, iterator_traversal_t<Iter>(), 0);
	}


	void _allocUnequalMove(dynarray & src, false_type)
	{
		_assignImpl(std::make_move_iterator(src._m.data), src.size(), false_type{});
	}

	void _allocUnequalMove(dynarray & src, true_type /*trivialRelocate*/)
	{
		_debugSizeUpdater guard{src._m};

		_detail::Destroy(_m.data, _m.end);
		_assignImpl(src.begin(), src.size(), true_type{});
		src._m.end = src._m.data; // elements in src conceptually destroyed
	}


	template<typename ContiguousIter>
	ContiguousIter _assignImpl(ContiguousIter const first, size_type const count, true_type /*trivialCopy*/)
	{
		_debugSizeUpdater guard{_m};

		if (capacity() < count)
		{
			// Deallocating first might be better, but then the _m pointers would have to be nulled in case allocate throws
			_resetData(_allocate(_m, count));
			_m.end = _m.reservEnd = _m.data + count;
		}
		else
		{	_m.end = _m.data + count;
		}
		// Not portable. Check for self assignment or use memmove?
		_detail::MemcpyCheck(first, count, _m.data);

		return first + count;
	}

	template<typename InputIter>
	InputIter _assignImpl(InputIter src, size_type const count, false_type)
	{
		_debugSizeUpdater guard{_m};

		auto copy = [](InputIter src_, pointer dest, pointer dLast)
		{
			while (dest != dLast)
			{
				*dest = *src_;
				++src_; ++dest;
			}
			return src_;
		};
		pointer newEnd;
		if (capacity() < count)
		{
			pointer const newData = _allocate(_m, count);
			// Old elements might hold some limited resource, destroying them before constructing new is probably good
			_detail::Destroy(_m.data, _m.end);
			_resetData(newData);
			_m.end = newData;
			_m.reservEnd = newData + count;
			newEnd = _m.reservEnd;
		}
		else
		{
			newEnd = _m.data + count;
			if (newEnd < _m.end)
			{	// downsizing, assign new and destroy rest
				src = copy(src, _m.data, newEnd);
				erase_to_end(_makeIter<iterator>(newEnd));
			}
			else // assign to old elements as far as we can
			{	src = copy(src, _m.data, _m.end);
			}
		}
		while (_m.end < newEnd)
		{	// put rest of new in uninitialized part
			_detail::Construct<Alloc>(_m, _m.end, *src);
			++src; ++_m.end;
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
		_debugSizeUpdater guard{_m};

		if (_unusedCapacity() >= count)
			makeNew(_m.end, count, _m);
		else
			_appendRealloc(count, makeNew);

		_m.end += count;
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
		_scopedPtr newBuf{_m, _calcCap(capacity(), oldSize + count)};

		pointer const pos = newBuf.data + oldSize;
		makeNew(pos, count, _m);
		_relocateData( newBuf.data, pos, oldSize,
			[count](T * pos_) { _detail::Destroy(pos_, pos_ + count); } );

		_m.end = pos;
		newBuf.Swap(_m);
	}

	template<typename... Args>
	void _emplaceBackRealloc(Args &&... args)
	{
		_scopedPtr newBuf{_m, _calcCapAddOne(capacity())};

		pointer const pos = newBuf.data + size();
		_detail::Construct<Alloc>(_m, pos, std::forward<Args>(args)...);
		_relocateData( newBuf.data, pos, size(),
			OEL_SUPPRESS_WARN_UNUSED
				[](T * pos_) { pos_-> ~T(); } );
		_m.end = pos;
		newBuf.Swap(_m);
	}


	template<typename CalcCapFunc, typename MakeFuncInsert, typename... Args>
	T * _insertRealloc(T *const pos, size_type const nAfterPos, size_type const nToAdd,
	                   CalcCapFunc calcNewCap, MakeFuncInsert const makeNew, Args &&... args)
	{
		_scopedPtr newBuf{_m, calcNewCap(capacity(), size() + nToAdd)};

		size_type const nBefore = pos - data();
		T *const newPos = newBuf.data + nBefore;
		T *const afterAdded = makeNew(newPos, nToAdd, _m, std::forward<Args>(args)...);
		// Exception free from here
		if (_m.data)
		{
			std::memcpy(newBuf.data, data(), sizeof(T) * nBefore); // relocate prefix
			std::memcpy(afterAdded, pos, sizeof(T) * nAfterPos);  // relocate suffix
		}
		_m.end = afterAdded + nAfterPos;
		newBuf.Swap(_m);

		return newPos;
	}

	struct _emplaceMakeElem
	{
		template<typename... Args>
		T * operator()(T *const newPos, size_type, Alloc & a, Args &&... args) const
		{
			_detail::Construct(a, newPos, std::forward<Args>(args)...);
			return newPos + 1;
		}
	};
};

template<typename T, typename Alloc> template<typename... Args>
typename dynarray<T, Alloc>::iterator
	dynarray<T, Alloc>::emplace(const_iterator pos, Args &&... args)
{
#define OEL_DYNARR_INSERT_STEP1  \
	_detail::AssertTrivialRelocate<T>{};  \
	\
	_debugSizeUpdater guard{_m};  \
	\
	auto pPos = const_cast<T *>(to_pointer_contiguous(pos));  \
	OEL_ASSERT(_m.data <= pPos and pPos <= _m.end);  \
	size_type const nAfterPos = _m.end - pPos;

	OEL_DYNARR_INSERT_STEP1
	if (_m.end < _m.reservEnd)
	{
		// Temporary in case constructor throws or source is an element of this dynarray at pos or after
		aligned_union_t<T> tmp;
		_detail::Construct<Alloc>(_m, reinterpret_cast<T *>(&tmp), std::forward<Args>(args)...);
		// Relocate [pos, end) to [pos + 1, end + 1), leaving memory at pos uninitialized (conceptually)
		std::memmove(pPos + 1, pPos, sizeof(T) * nAfterPos);
		++_m.end;

		std::memcpy(pPos, &tmp, sizeof(T)); // relocate the new element to pos
	}
	else
	{	pPos = _insertRealloc(pPos, nAfterPos, {}, _calcCapAddOne,
		                      _emplaceMakeElem{}, std::forward<Args>(args)...);
	}
	return _makeIter<iterator>(pPos);
}

template<typename T, typename Alloc> template<typename ForwardRange>
typename dynarray<T, Alloc>::iterator
	dynarray<T, Alloc>::insert_r(const_iterator pos, const ForwardRange & src)
{
	auto first = ::adl_begin(src);
	size_type const count = _sizeOrEnd<decltype(first)>(src);

	static_assert(std::is_base_of< forward_traversal_tag, iterator_traversal_t<decltype(first)> >::value,
				  "insert_r requires that source models Forward Range (Boost concept)");

	OEL_DYNARR_INSERT_STEP1
#undef OEL_DYNARR_INSERT_STEP1

	if (_unusedCapacity() >= count)
	{
		T *const dLast = pPos + count;
		// Relocate elements to make space, leaving [pos, pos + count) uninitialized (conceptually)
		std::memmove(dLast, pPos, sizeof(T) * nAfterPos);
		_m.end += count;
		// Construct new
		OEL_CONST_COND if (can_memmove_with<T *, decltype(first)>::value)
		{
			_detail::UninitCopy<Alloc>(first, pPos, dLast, _m);
		}
		else
		{
			T * dest = pPos;
			OEL_TRY_
			{
				while (dest != dLast)
				{
					_detail::Construct<Alloc>(_m, dest, *first);
					++first; ++dest;
				}
			}
			OEL_CATCH_ALL
			{	// relocate back to fill hole
				std::memmove(dest, dLast, sizeof(T) * nAfterPos);
				_m.end -= (dLast - dest);
				OEL_WHEN_EXCEPTIONS_ON(throw);
			}
		}
	}
	else // not enough room, reallocate
	{
		pPos = _insertRealloc( pPos, nAfterPos, count, _calcCap,
			[first](T * newPos, size_type count_, Alloc & a)
			{
				T *const dLast = newPos + count_;
				_detail::UninitCopy(first, newPos, dLast, a);
				return dLast;
			} );
	}
	return _makeIter<iterator>(pPos);
}

template<typename T, typename Alloc> template<typename... Args>
inline T & dynarray<T, Alloc>::emplace_back(Args &&... args)
{
	_debugSizeUpdater guard{_m};

	if (_m.end < _m.reservEnd)
		_detail::Construct<Alloc>(_m, _m.end, std::forward<Args>(args)...);
	else
		_emplaceBackRealloc(std::forward<Args>(args)...);

	pointer const pos = _m.end;
	++_m.end;

	return *pos;
}


template<typename T, typename Alloc>
dynarray<T, Alloc>::dynarray(dynarray && other, const Alloc & a)
 :	_m(a)
{
	OEL_CONST_COND if (!is_always_equal<Alloc>::value and a != other._m)
		_allocUnequalMove(other, is_trivially_relocatable<T>());
	else
		_moveInternBase(other._m);
}

template<typename T, typename Alloc>
dynarray<T, Alloc> &  dynarray<T, Alloc>::operator =(dynarray && other) &
	noexcept(_allocTrait::propagate_on_container_move_assignment::value or is_always_equal<Alloc>::value)
{
	OEL_CONST_COND if (!_allocTrait::propagate_on_container_move_assignment::value
	               and static_cast<Alloc &>(_m) != other._m)
	{
		_allocUnequalMove(other, is_trivially_relocatable<T>());
	}
	else // take allocated memory from other
	{
		if (_m.data)
		{
			_detail::Destroy(_m.data, _m.end);
			_allocateWrap::Deallocate(_m, _m.data, capacity());
		}
		_moveInternBase(other._m);
		_moveAssignAlloc(typename _allocTrait::propagate_on_container_move_assignment(), other._m);
	}
	return *this;
}

template<typename T, typename Alloc>
inline dynarray<T, Alloc>::dynarray(const dynarray & other)
 :	dynarray(other, _allocTrait::select_on_container_copy_construction(other._m)) {
}

template<typename T, typename Alloc>
dynarray<T, Alloc>::dynarray(size_type n, default_init_t, const Alloc & a)
 :	_m(a, n)
{
	_debugSizeUpdater guard{_m};

	_m.end = _m.reservEnd;
	_detail::UninitDefaultConstruct<Alloc>(_m.data, _m.end, _m);
}

template<typename T, typename Alloc>
dynarray<T, Alloc>::dynarray(size_type n, const Alloc & a)
 :	_m(a, n)
{
	_debugSizeUpdater guard{_m};

	_m.end = _m.reservEnd;
	_uninitFill{}(_m.data, _m.end, _m);
}

template<typename T, typename Alloc>
dynarray<T, Alloc>::dynarray(size_type n, const T & val, const Alloc & a)
 :	_m(a, n)
{
	_debugSizeUpdater guard{_m};

	_m.end = _m.reservEnd;
	_uninitFill{}(_m.data, _m.end, _m, val);
}

template<typename T, typename Alloc>
dynarray<T, Alloc>::~dynarray() noexcept
{
	_detail::Destroy(_m.data, _m.end);
}

template<typename T, typename Alloc>
void dynarray<T, Alloc>::swap(dynarray & other)
	noexcept(_allocTrait::propagate_on_container_swap::value or is_always_equal<Alloc>::value)
{
	_internBase & a = _m;
	_internBase & b = other._m;
	std::swap(a, b);
	_swapAlloc(typename _allocTrait::propagate_on_container_swap(), other._m);
}


template<typename T, typename Alloc>
void dynarray<T, Alloc>::shrink_to_fit()
{
	OEL_WHEN_EXCEPTIONS_ON(
		static_assert(std::is_nothrow_move_constructible<T>::value or is_trivially_relocatable<T>::value,
			"This function requires that T is noexcept move constructible or trivially relocatable"); )

	_debugSizeUpdater guard{_m};

	size_type const used = size();
	pointer newData;
	if (0 < used)
	{
		newData = _allocateWrap::Allocate(_m, used);
		pointer const newEnd = newData + used;
		_relocateData(newData, newEnd, used);
		_m.end = newEnd;
	}
	else
	{	_m.end = newData = nullptr;
	}
	_resetData(newData); // careful, cannot change _m.reservEnd until after
	_m.reservEnd = _m.end;
}


template<typename T, typename Alloc> template<typename InputRange>
inline auto dynarray<T, Alloc>::assign(const InputRange & src) -> decltype(::adl_begin(src))
{
	using IterSrc = decltype(::adl_begin(src));
	return _assignImpl(::adl_begin(src),
	                   _sizeOrEnd<IterSrc>(src),
	                   can_memmove_with<T *, IterSrc>());
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
inline auto dynarray<T, Alloc>::append(const InputRange & src) -> decltype(::adl_begin(src))
{
	using IterSrc = decltype(::adl_begin(src));
	return _append(::adl_begin(src),
	               _sizeOrEnd<IterSrc>(src),
	               can_memmove_with<T *, IterSrc>());
}


template<typename T, typename Alloc>
inline void dynarray<T, Alloc>::pop_back() noexcept(nodebug)
{
	OEL_ASSERT(_m.data < _m.end);
	_debugSizeUpdater guard{_m};

	--_m.end;
	(*_m.end).~T();
}

template<typename T, typename Alloc>
inline void dynarray<T, Alloc>::erase_to_end(iterator first) noexcept(nodebug)
{
	_debugSizeUpdater guard{_m};

	T *const newEnd = to_pointer_contiguous(first);
	OEL_ASSERT(_m.data <= newEnd and newEnd <= _m.end);
	_detail::Destroy(newEnd, _m.end);
	_m.end = newEnd;
}

template<typename T, typename Alloc>
typename dynarray<T, Alloc>::iterator  dynarray<T, Alloc>::erase(iterator first, const_iterator last)
{
	_detail::AssertTrivialRelocate<T>{};

	_debugSizeUpdater guard{_m};

	T *const      pFirst = to_pointer_contiguous(first);
	const T *const pLast = to_pointer_contiguous(last);
	OEL_ASSERT(_m.data <= pFirst and pFirst <= pLast and pLast <= _m.end);
	if (pFirst < pLast)
	{
		_detail::Destroy(pFirst, pLast);
		size_type const nAfterLast = _m.end - pLast;
		// Relocate [last, end) to [first, first + nAfterLast)
		std::memmove(pFirst, pLast, sizeof(T) * nAfterLast);
		_m.end = pFirst + nAfterLast;
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
		return _m.data[i];
	else
		_detail::Throw::OutOfRange("Bad index dynarray::at");
}

#ifdef OEL_DYNARRAY_IN_DEBUG
}
#endif

namespace _detail
{
	template<typename Pointer>
	struct DynarrBase
	{
		Pointer data;      // Pointer to beginning of data buffer
		Pointer end;       // Pointer to one past the back object
		Pointer reservEnd; // Pointer to end of allocated memory
	};
}

} // namespace oel
