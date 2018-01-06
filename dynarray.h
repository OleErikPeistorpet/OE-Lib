#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "auxi/contiguous_iterator.h"
#include "compat/default.h"
#include "align_allocator.h"
#include "make_unique.h" // not needed, just convenient

#include <algorithm>


namespace oel
{
#ifdef OEL_DEBUG_ABI
inline namespace debug
{
#endif

//! dynarray<dynarray<T>> is efficient
template<typename T, typename Alloc>
is_trivially_relocatable<Alloc> specify_trivial_relocate(dynarray<T, Alloc>);

template<typename T, typename A> OEL_ALWAYS_INLINE inline
void swap(dynarray<T, A> & a, dynarray<T, A> & b) OEL_NOEXCEPT_NDEBUG  { a.swap(b); }

//! Overloads generic erase_unstable(RandomAccessContainer &, RandomAccessContainer::size_type) (in range_algo.h)
template<typename T, typename A> inline
void erase_unstable(dynarray<T, A> & d, typename dynarray<T, A>::size_type index)  { d.erase_unstable(d.begin() + index); }

//! Overloads generic assign(Container &, const InputRange &) (in range_algo.h)
template<typename T, typename A, typename InputRange> inline
void assign(dynarray<T, A> & dest, const InputRange & source)  { dest.assign(source); }
//! Overloads generic append(Container &, const InputRange &) (in range_algo.h)
template<typename T, typename A, typename InputRange> inline
void append(dynarray<T, A> & dest, const InputRange & source)  { dest.append(source); }
//! Overloads generic append(Container &, Container::size_type, const T &) (in range_algo.h)
template<typename T, typename A> inline
void append(dynarray<T, A> & dest, typename dynarray<T, A>::size_type n, const T & val)  { dest.append(n, val); }
//! Overloads generic oel::insert (in range_algo.h)
template<typename T, typename A, typename ForwardRange> inline
typename dynarray<T, A>::iterator
	insert(dynarray<T, A> & dest, typename dynarray<T, A>::const_iterator pos, const ForwardRange & source)
	{ return dest.insert_r(pos, source); }

/**
* @brief Resizable array, dynamically allocated. Very similar to std::vector, but much faster in many cases.
*
* Efficiency is better if template argument T is trivially relocatable, which is true for most types, but needs
* to be declared manually for each type that is not trivially copyable. See specify_trivial_relocate(T &&).
* Also, if T is trivially relocatable, it does not need to be move or copy constructible/assignable
* except when an instance to be moved/copied is passed as an argument by the user.
* There are a few notable functions for which trivially relocatable T is required (checked when compiling):
* emplace, insert, insert_r and erase(iterator, iterator)
*
* For any operation which may reallocate and is specified to have strong exception guarantee for std::vector:
* If T's move constructor is not noexcept and T is not trivially relocatable, dynarray will use the throwing
* move constructor, and the exception guarantee is lowered to basic.
*
* The default allocator supports over-aligned types (e.g. __m256).
*
* In general, only that which differs from std::vector is documented. */
template<typename T, typename Alloc/* = oel::allocator */>
class dynarray
{
	using _allocTrait = std::allocator_traits<Alloc>;
	using _internBase = _detail::DynarrBase<T, typename _allocTrait::pointer>;
	using _allocateWrap = _detail::DebugAllocateWrapper<_internBase, Alloc, typename _allocTrait::pointer>;

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
	using iterator       = contiguous_ctnr_iterator<pointer, _internBase>;
	using const_iterator = contiguous_ctnr_iterator<const_pointer, _internBase>;
#else
	using iterator       = pointer;
	using const_iterator = const_pointer;
#endif
	using reverse_iterator       = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;


	dynarray() noexcept                          : _m(Alloc{}) {}
	explicit dynarray(const Alloc & a) noexcept  : _m(a) {}

	//! Construct empty dynarray with space reserved for at least minCap elements
	dynarray(reserve_tag, size_type minCap, const Alloc & a = Alloc{})  : _m(a, minCap) {}

	/** @brief Uses default initialization for elements, can be significantly faster for non-class T
	*
	* @copydetails resize(size_type, default_init_tag)  */
	dynarray(size_type size, default_init_tag, const Alloc & a = Alloc{});
	explicit dynarray(size_type size, const Alloc & a = Alloc{});  //!< (Value-initializes elements, same as std::vector)
	dynarray(size_type size, const T & fillVal, const Alloc & a = Alloc{});

	/** @brief Equivalent to `std::vector(begin(r), end(r), a)`, where end(r) is not needed if r.size() exists
	*
	* To move instead of copy, wrap r with view::move (the same applies for all functions taking a range template)
	* Example, construct from a standard istream with formatting (using Boost):
	* @code
	#include <boost/range/istream_range.hpp>
	// ...
	auto result = dynarray<int>(boost::range::istream_range<int>(someStream));
	@endcode  */
	template<typename InputRange, typename /*EnableIfRange*/ = decltype( ::adl_cbegin(std::declval<InputRange>()) )>
	explicit dynarray(const InputRange & r, const Alloc & a = Alloc{})  : _m(a) { assign(r); }

	dynarray(std::initializer_list<T> il, const Alloc & a = Alloc{})  : _m(a, il.size())
	                                                                  { _initPostAllocate(il.begin(), il.size()); }
	dynarray(dynarray && other) noexcept               : _m(std::move(other._m)) {}
	dynarray(dynarray && other, const Alloc & a);
	dynarray(const dynarray & other);
	dynarray(const dynarray & other, const Alloc & a)  : _m(a, other.size())
	                                                   { _initPostAllocate(other.data(), other.size()); }
	~dynarray() noexcept;

	dynarray & operator =(dynarray && other) OEL_NOEXCEPT(_allocTrait::propagate_on_container_move_assignment::value
	                                                      || is_always_equal<Alloc>::value);
	//! Treats allocator_type as if it does not have propagate_on_container_copy_assignment
	dynarray & operator =(const dynarray & other)    { assign(other);  return *this; }

	dynarray & operator =(std::initializer_list<T> il)  { assign(il);  return *this; }

	void      swap(dynarray & other) OEL_NOEXCEPT_NDEBUG;

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
	* `std::vector::insert(end(), begin(source), end(source))`, where end(source) is not needed if source.size() exists. */
	template<typename InputRange>
	auto      append(const InputRange & source) -> decltype(::adl_begin(source));
	//! @brief Equivalent to `std::vector::insert(end(), il)`, but
	//!	with strong exception guarantee if T is noexcept move constructible or trivially relocatable
	void      append(std::initializer_list<T> il)   { append<>(il); }
	//! @brief Equivalent to `std::vector::insert(end(), count, val)`, but
	//!	with strong exception guarantee if T is noexcept move constructible or trivially relocatable
	void      append(size_type count, const T & val);

	/**
	* @brief Uses default initialization for added elements, can be significantly faster for non-class T
	*
	* Non-class T objects get indeterminate values. http://en.cppreference.com/w/cpp/language/default_initialization  */
	void      resize(size_type n, default_init_tag)  { _resizeImpl(n, _detail::UninitDefaultConstruct<Alloc, T>); }
	//! (Value-initializes added elements, same as std::vector::resize)
	void      resize(size_type n)                    { _resizeImpl(n, _detail::UninitFill<Alloc, T>); }

	//! @brief Equivalent to `std::vector::insert(pos, begin(source), end(source))`,
	//!	where end(source) is not needed if source.size() exists
	template<typename ForwardRange>
	iterator  insert_r(const_iterator pos, const ForwardRange & source);

	iterator  insert(const_iterator pos, std::initializer_list<T> il)  { return insert_r(pos, il); }

	iterator  insert(const_iterator pos, T && val)       { return emplace(pos, std::move(val)); }
	iterator  insert(const_iterator pos, const T & val)  { return emplace(pos, val); }

	//! The default allocator performs list-initialization of element if there is no matching constructor
	template<typename... Args>
	iterator  emplace(const_iterator pos, Args &&... elemInitArgs);

	//! The default allocator performs list-initialization of element if there is no matching constructor
	template<typename... Args>
	reference emplace_back(Args &&... args);

	void      push_back(T && val)       { emplace_back(std::move(val)); }
	void      push_back(const T & val)  { emplace_back(val); }

	void      pop_back() OEL_NOEXCEPT_NDEBUG;

	/**
	* @brief Erase the element at pos without maintaining order of elements after pos.
	*
	* Constant complexity (compared to linear in the distance between pos and end() for normal erase).
	* @return iterator corresponding to the same index in the sequence as pos, same as for std containers. */
	iterator  erase_unstable(iterator pos)  { _eraseUnorder(pos, is_trivially_relocatable<T>());  return pos; }

	iterator  erase(iterator pos)           { _erase(pos, is_trivially_relocatable<T>());  return pos; }

	iterator  erase(iterator first, iterator last);
	//! Equivalent to `erase(first, end())` (but potentially faster), making first the new end
	void      erase_to_end(iterator first) OEL_NOEXCEPT_NDEBUG;

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

	iterator       begin() noexcept         OEL_ALWAYS_INLINE { return _makeIter<iterator>(_m.data); }
	const_iterator begin() const noexcept   OEL_ALWAYS_INLINE { return _makeIter<const_iterator>(_m.data); }
	const_iterator cbegin() const noexcept  OEL_ALWAYS_INLINE { return begin(); }

	iterator       end() noexcept           OEL_ALWAYS_INLINE { return _makeIter<iterator>(_m.end); }
	const_iterator end() const noexcept     OEL_ALWAYS_INLINE { return _makeIter<const_iterator>(_m.end); }
	const_iterator cend() const noexcept    OEL_ALWAYS_INLINE { return end(); }

	reverse_iterator       rbegin() noexcept        OEL_ALWAYS_INLINE { return reverse_iterator{end()}; }
	const_reverse_iterator rbegin() const noexcept  OEL_ALWAYS_INLINE { return const_reverse_iterator{end()}; }

	reverse_iterator       rend() noexcept        OEL_ALWAYS_INLINE { return reverse_iterator{begin()}; }
	const_reverse_iterator rend() const noexcept  OEL_ALWAYS_INLINE { return const_reverse_iterator{begin()}; }

	T *             data() noexcept        OEL_ALWAYS_INLINE { return _m.data; }
	const T *       data() const noexcept  OEL_ALWAYS_INLINE { return _m.data; }

	reference       front() OEL_NOEXCEPT_NDEBUG        { return *begin(); }
	const_reference front() const OEL_NOEXCEPT_NDEBUG  { return *begin(); }

	reference       back() OEL_NOEXCEPT_NDEBUG         { return *_makeIter<iterator>(_m.end - 1); }
	const_reference back() const OEL_NOEXCEPT_NDEBUG   { return *_makeIter<const_iterator>(_m.end - 1); }

	reference       at(size_type index);
	const_reference at(size_type index) const;

	reference       operator[](size_type index) OEL_NOEXCEPT_NDEBUG;
	const_reference operator[](size_type index) const OEL_NOEXCEPT_NDEBUG;

	friend bool operator==(const dynarray & left, const dynarray & right)
		{
			return left.size() == right.size() &&
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
	struct _memOwner : public _internBase, public Alloc
	{
		using _internBase::data; // owning pointer
		using _internBase::end;
		using _internBase::reservEnd;

		_memOwner(const Alloc & a)
		 :	_internBase(), Alloc(a) {
		}
		_memOwner(const Alloc & a, size_type const capacity)
		 :	Alloc(a)
		{
			end = data = _allocate(*this, capacity);
			reservEnd = data + capacity;
		}

		_memOwner(_memOwner && other)
		 :	_internBase(other), Alloc(std::move(other))
		{
			other.reservEnd = other.end = other.data = nullptr;
			_allocateWrap::UpdateAfterMove(*this);
		}
		_memOwner(const _memOwner &) = delete;
		void operator =(const _memOwner &) = delete;

		~_memOwner()
		{
			_allocateWrap::Deallocate(*this, data, reservEnd - data);
		}

	} _m; // the only data member


	using _allocRef = _detail::AllocRefOptimized<Alloc>;

	struct _scopedPtr : private _allocRef
	{
		pointer data;  // owner
		pointer allocEnd;

		_scopedPtr(_memOwner & a, size_type const allocSize)
		 :	_allocRef(a)
		{
			data = _allocate(a, allocSize);
			allocEnd = data + allocSize;
		}
		_scopedPtr(const _scopedPtr &) = delete;
		void operator =(const _scopedPtr &) = delete;

		~_scopedPtr()
		{
			auto && a = this->Get();
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
		_allocateWrap::Deallocate(_m, _m.data, capacity());
		_m.data = newData;
	}

	static pointer _allocate(_memOwner & a, size_type n)
	{
		if (n <= _allocTrait::max_size(a) - _allocateWrap::sizeForHeader)
			return _allocateWrap::Allocate(a, n); // allocate should throw if subtraction wrapped around
		else
			_detail::Throw::LengthError("Going over dynarray max_size");
	}

	template<typename Iterator>
	Iterator _makeIter(pointer p) const
	{
	#if OEL_MEM_BOUND_DEBUG_LVL >= 2
		if (_m.data)
		{
			auto const h = _allocateWrap::HeaderOf(_m.data);
			return {p, h, h->id};
		}
		else
		{	return {p, nullptr, reinterpret_cast<std::uintptr_t>(this)};
		}
	#elif OEL_MEM_BOUND_DEBUG_LVL
		return {p, &_m};
	#else
		return p;
	#endif
	}


	void _moveInternBase(_internBase & src)
	{
		static_cast<_internBase &>(_m) = src;
		src.reservEnd = src.end = src.data = nullptr;
		_allocateWrap::UpdateAfterMove(_m);
	}

	void _moveAssignAlloc(std::true_type, Alloc & src)
	{
		Alloc & a = _m;
		a = std::move(src);
	}

	OEL_ALWAYS_INLINE void _moveAssignAlloc(std::false_type, Alloc &) {}

	void _swapAlloc(std::true_type, Alloc & a)
	{
		using std::swap;
		swap(static_cast<Alloc &>(_m), a);
	}

	void _swapAlloc(std::false_type, Alloc & a)
	{	// propagate_on_container_swap false, standard says this is undefined if allocators compare unequal
		OEL_ASSERT(static_cast<Alloc &>(_m) == a);
		(void) a;
	}


	template<typename InputIter>
	void _uninitCopy(false_type, InputIter first, size_type, T * dest, T * destEnd)
	{	// cannot use memcpy
		_detail::UninitCopy<Alloc>(first, dest, destEnd, _m);
	}

	template<typename CntigusIter>
	void _uninitCopy(true_type, CntigusIter first, size_type n, T * dest, T *)
	{
		_detail::MemcpyMaybeNull(dest, to_pointer_contiguous(first), sizeof(T) * n);
	}

	void _initPostAllocate(const T *const first, size_type const n)
	{
		_m.end = _m.reservEnd;
		_uninitCopy(is_trivially_copyable<T>(), first, n, _m.data, _m.end);
	}


	size_type _unusedCapacity() const
	{
		return _m.reservEnd - _m.end;
	}

	static size_type _calcCapAddOne(size_type const oldCap, size_type = 0)
	{
		enum
		{	a = 3 * sizeof(void *),
			b = 4 * sizeof(int),
			startBytesGood = a > b ? a : b,
			minGrow = 2 * sizeof(T) <= startBytesGood ?
				startBytesGood / sizeof(T) :
				(sizeof(T) <= 8 * sizeof(int) ? 2 : 1)
		};
		OEL_CONST_COND if (minGrow > 1)
			return oldCap + std::max<size_type>(oldCap / 2, minGrow);
		else
			return oldCap + oldCap / 2 + 1;
	}

	static size_type _calcCap(size_type oldCap, size_type newSize)
	{	// growth factor is 1.5
		return (std::max)(oldCap + oldCap / 2, newSize);
	}


	template< typename... FuncTakingLast, typename T_ = T,
		enable_if<!is_trivially_relocatable<T_>::value> = 0 >
	void _relocateData(T * dFirst, T * dLast, size_type, FuncTakingLast... extraCleanupIfException)
	{
		_detail::UninitCopy<Alloc>(std::make_move_iterator(_m.data), dFirst, dLast, _m, extraCleanupIfException...);
		_detail::Destroy(_m.data, _m.end);
	}

	template< typename... Unused, typename T_ = T,
		enable_if<is_trivially_relocatable<T_>::value> = 0 >
	void _relocateData(T * dest, T *, size_type n, Unused...)
	{
		_detail::MemcpyMaybeNull(dest, data(), sizeof(T) * n);
	}


	void _growTo(size_type newCap)
	{
		_scopedPtr newBuf{_m, newCap};

		pointer const newEnd = newBuf.data + size();
		_relocateData(newBuf.data, newEnd, size());

		_m.end = newEnd;
		newBuf.Swap(_m);
	}

	template<typename UninitFillFunc>
	void _resizeImpl(size_type const newSize, UninitFillFunc initAdded)
	{	// note: initAdded cannot hold a reference to element of this
		if (capacity() < newSize)
			_growTo(_calcCap(capacity(), newSize));

		T *const newEnd = _m.data + newSize;
		if (_m.end < newEnd)
			initAdded(_m.end, newEnd, _m);
		else
			_detail::Destroy(newEnd, _m.end);

		_m.end = newEnd;
	}


	void _eraseUnorder(iterator const pos, true_type /*trivialRelocate*/)
	{
		OEL_ASSERT(_m._internBase::DerefValid(pos._pElem));

		T & elem = *pos;
		elem.~T();
		--_m.end;
		auto &
	#ifndef _MSC_VER
			__attribute__((may_alias))
	#endif
			raw = reinterpret_cast<aligned_union_t<T> &>(elem);
		raw = reinterpret_cast<aligned_union_t<T> &>(*_m.end); // relocate last element to pos
	}

	void _eraseUnorder(iterator pos, false_type)
	{
		*pos = std::move(back());
		pop_back();
	}

	void _erase(iterator pos, true_type /*trivialRelocate*/)
	{
		T *const ptr = to_pointer_contiguous(pos);
		OEL_ASSERT(_m.data <= ptr && ptr < _m.end);

		ptr-> ~T();
		const T * next = ptr + 1;
		std::memmove(ptr, next, sizeof(T) * (_m.end - next)); // relocate [pos + 1, end) to [pos, end - 1)
		--_m.end;
	}

	void _erase(iterator pos, false_type)
	{
		iterator last = std::move(pos + 1, end(), pos);
		_m.end = to_pointer_contiguous(last);
		(*_m.end).~T();
	}


	template<typename Range, typename IterTrav> // pass dummy int to prefer this overload
	static auto _sizeOrEnd(const Range & r, IterTrav, int)
	 -> decltype( oel::ssize(r), size_type() ) { return oel::ssize(r); }

	template<typename Range>
	static size_type _sizeOrEnd(const Range & r, forward_traversal_tag, long)
	{
		return std::distance(::adl_begin(r), ::adl_end(r));
	}

	template<typename Range>
	static auto _sizeOrEnd(const Range & r, single_pass_traversal_tag, long)
	 -> decltype(::adl_end(r)) { return ::adl_end(r); }

	// Returns element count as size_type if possible, else adl_end(r)
	template<typename Iter, typename Range>
	static auto _sizeOrEnd(const Range & r) -> decltype(_sizeOrEnd(r, iterator_traversal_t<Iter>(), 0))
	                                           { return _sizeOrEnd(r, iterator_traversal_t<Iter>(), 0); }


	void _allocUnequalMove(dynarray & src, false_type)
	{
		_assignImpl(std::make_move_iterator(src._m.data), src.size(), false_type{});
	}

	void _allocUnequalMove(dynarray & src, true_type /*trivialRelocate*/)
	{
		_detail::Destroy(_m.data, _m.end);
		_assignImpl(src.begin(), src.size(), true_type{});
		src._m.end = src._m.data; // elements in src conceptually destroyed
	}

	template<typename CntigusIter>
	CntigusIter _assignImpl(CntigusIter const first, size_type const count, true_type /*trivialCopy*/)
	{
	#if OEL_MEM_BOUND_DEBUG_LVL
		if (count != 0)
		{	// Dereference to catch out of range errors if the iterators have internal checks
			(void) *first;
			(void) *(first + (count - 1));
		}
	#endif
		if (capacity() < count)
		{
			// Deallocating first might be better, but then the _m pointers would have to be nulled in case allocate throws
			_resetData(_allocate(_m, count));
			_m.end = _m.data + count;
			_m.reservEnd = _m.end;
		}
		else
		{	_m.end = _m.data + count;
		}
		// Not portable. Check for self assignment or use memmove?
		_detail::MemcpyMaybeNull(data(), to_pointer_contiguous(first), sizeof(T) * count);

		return first + count;
	}

	template<typename InputIter>
	InputIter _assignImpl(InputIter src, size_type const count, false_type)
	{
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
			_allocTrait::construct(_m, _m.end, *src);
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

	template<typename CntigusIter>
	CntigusIter _append(CntigusIter const first, size_type const n, true_type)
	{
		CntigusIter last = first + n;
	#if OEL_MEM_BOUND_DEBUG_LVL
		if (n != 0)
		{	// Dereference to catch out of range errors if the iterators have internal checks
			(void) *first;
			(void) *(last - 1);
		}
	#endif
		_appendImpl( n,
			[first](T * dest, size_type n_, Alloc &)
			{
				_detail::MemcpyMaybeNull(dest, to_pointer_contiguous(first), sizeof(T) * n_);
			} );
		return last; // has been invalidated in the case of append self and reallocation
	}

	template<typename MakeFuncAppend>
	void _appendImpl(size_type const count, MakeFuncAppend makeNew)
	{
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
	void _appendRealloc(size_type const count, MakeFuncAppend makeNew)
	{
		_scopedPtr newBuf{_m, _calcCap(capacity(), size() + count)};

		size_type const oldSize = size();
		pointer const pos = newBuf.data + oldSize;
		makeNew(pos, count, _m);
		_relocateData(newBuf.data, pos, oldSize,
			[count](T * pos_) { _detail::Destroy(pos_, pos_ + count); } );

		_m.end = pos;
		newBuf.Swap(_m);
	}

	template<typename... Args>
	void _emplaceBackRealloc(Args &&... args)
	{
		_scopedPtr newBuf{_m, _calcCapAddOne(capacity())};

		pointer const pos = newBuf.data + size();
		_allocTrait::construct(_m, pos, std::forward<Args>(args)...);
#ifdef _MSC_VER
	#pragma warning(suppress : 4100) // unreferenced formal parameter
#endif
		_relocateData( newBuf.data, pos, size(),
				[](T * pos_) { pos_-> ~T(); } );
		_m.end = pos;
		newBuf.Swap(_m);
	}


	template<typename CalcCapFunc, typename MakeFuncInsert, typename... Args>
	T * _insertRealloc(T *const pos, size_type const nAfterPos, size_type const nToAdd,
	                   CalcCapFunc calcNewCap, MakeFuncInsert makeNew, Args &&... args)
	{
		_scopedPtr newBuf{_m, calcNewCap(capacity(), size() + nToAdd)};

		size_type const nBefore = pos - data();
		T *const newPos = newBuf.data + nBefore;
		T *const afterAdded = makeNew(*this, newPos, nToAdd, std::forward<Args>(args)...);
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
		T * operator()(dynarray & d, T *const newPos, size_type, Args &&... args) const
		{
			_allocTrait::construct(d._m, newPos, std::forward<Args>(args)...);
			return newPos + 1;
		}
	};
};

template<typename T, typename Alloc> template<typename... Args>
typename dynarray<T, Alloc>::iterator
	dynarray<T, Alloc>::emplace(const_iterator pos, Args &&... args)
{
#define OEL_DYNARR_INSERT_STEP0  \
	_detail::AssertTrivialRelocate<T>();  \
	\
	auto pPos = const_cast<T *>(to_pointer_contiguous(pos));  \
	OEL_ASSERT(_m.data <= pPos && pPos <= _m.end);  \
	size_type const nAfterPos = _m.end - pPos;

	OEL_DYNARR_INSERT_STEP0
	if (_m.end < _m.reservEnd)
	{
		// Temporary in case constructor throws or source is an element of this dynarray at pos or after
		aligned_union_t<T> tmp;
		_allocTrait::construct(_m, reinterpret_cast<T *>(&tmp), std::forward<Args>(args)...);
		// Relocate [pos, end) to [pos + 1, end + 1), conceptually destroying element at pos
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

	static_assert(std::is_base_of< forward_traversal_tag, iterator_traversal_t<decltype(first)> >::value,
				  "insert_r requires that source models Forward Range (Boost concept)");

	OEL_DYNARR_INSERT_STEP0

	using CanMemmove = can_memmove_with<T *, decltype(first)>;
	size_type const count = _sizeOrEnd<decltype(first)>(src);
	if (_unusedCapacity() >= count)
	{
		T *const dLast = pPos + count;
		// Relocate elements to make space, conceptually destroying [pos, pos + count)
		std::memmove(dLast, pPos, sizeof(T) * nAfterPos);
		_m.end += count;
		// Construct new
		OEL_CONST_COND if (CanMemmove::value)
		{
			_uninitCopy(CanMemmove(), first, count, pPos, dLast);
		}
		else
		{
			T * dest = pPos;
			OEL_TRY_
			{
				while (dest != dLast)
				{
					_allocTrait::construct(_m, dest, *first);
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
			[first](dynarray & self, T * newPos, size_type count_)
			{
				T *const dLast = newPos + count_;
				self._uninitCopy(CanMemmove(), first, count_, newPos, dLast);
				return dLast;
			} );
	}
	return _makeIter<iterator>(pPos);
}
#undef OEL_DYNARR_INSERT_STEP0

template<typename T, typename Alloc> template<typename... Args>
inline T & dynarray<T, Alloc>::emplace_back(Args &&... args)
{
	if (_m.end < _m.reservEnd)
		_allocTrait::construct(_m, _m.end, std::forward<Args>(args)...);
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
	if (a != other._m && !is_always_equal<Alloc>::value)
		_allocUnequalMove(other, is_trivially_relocatable<T>());
	else
		_moveInternBase(other._m);
}

template<typename T, typename Alloc>
dynarray<T, Alloc> & dynarray<T, Alloc>::operator =(dynarray && other)
	OEL_NOEXCEPT(_allocTrait::propagate_on_container_move_assignment::value || is_always_equal<Alloc>::value)
{
	if (static_cast<Alloc &>(_m) != other._m &&
		!_allocTrait::propagate_on_container_move_assignment::value)
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
dynarray<T, Alloc>::dynarray(size_type n, default_init_tag, const Alloc & a)
 :	_m(a, n)
{
	_m.end = _m.reservEnd;
	_detail::UninitDefaultConstruct<Alloc>(_m.data, _m.end, _m);
}

template<typename T, typename Alloc>
dynarray<T, Alloc>::dynarray(size_type n, const Alloc & a)
 :	_m(a, n)
{
	_m.end = _m.reservEnd;
	_detail::UninitFill<Alloc>(_m.data, _m.end, _m);
}

template<typename T, typename Alloc>
dynarray<T, Alloc>::dynarray(size_type n, const T & val, const Alloc & a)
 :	_m(a, n)
{
	_m.end = _m.reservEnd;
	_detail::UninitFill<Alloc>(_m.data, _m.end, _m, val);
}

template<typename T, typename Alloc>
dynarray<T, Alloc>::~dynarray() noexcept
{
	_detail::Destroy(_m.data, _m.end);
}

template<typename T, typename Alloc>
void dynarray<T, Alloc>::swap(dynarray & other) OEL_NOEXCEPT_NDEBUG
{
	_internBase & a = _m;
	_internBase & b = other._m;
	std::swap(a, b);
	_allocateWrap::UpdateAfterMove(a);
	_allocateWrap::UpdateAfterMove(b);
	_swapAlloc(typename _allocTrait::propagate_on_container_swap(), other._m);
}


template<typename T, typename Alloc>
void dynarray<T, Alloc>::shrink_to_fit()
{
	OEL_WHEN_EXCEPTIONS_ON(
		static_assert(std::is_nothrow_move_constructible<T>::value || is_trivially_relocatable<T>::value,
			"This function requires that T is noexcept move constructible or trivially relocatable"); )

	size_type const used = size();
	pointer newData;
	if (0 < used)
	{
		OEL_TRY_
		{	// TODO: test code gen
			newData = _allocateWrap::Allocate(_m, used);
		}
		OEL_CATCH_ALL
		{
			return;
		}
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
	return _assignImpl(::adl_begin(src), _sizeOrEnd<IterSrc>(src),
	                   can_memmove_with<T *, IterSrc>());
}

template<typename T, typename Alloc>
inline void dynarray<T, Alloc>::append(size_type n, const T & val)
{
	_appendImpl( n,
		[&val](T * dest, size_type n_, Alloc & a)
		{
			_detail::UninitFill(dest, dest + n_, a, val);
		} );
}

template<typename T, typename Alloc> template<typename InputRange>
inline auto dynarray<T, Alloc>::append(const InputRange & src) -> decltype(::adl_begin(src))
{
	using IterSrc = decltype(::adl_begin(src));
	return _append( ::adl_begin(src), _sizeOrEnd<IterSrc>(src), can_memmove_with<T *, IterSrc>() );
}


template<typename T, typename Alloc>
inline void dynarray<T, Alloc>::pop_back() OEL_NOEXCEPT_NDEBUG
{
	OEL_ASSERT(_m.data < _m.end);
	--_m.end;
	(*_m.end).~T();
}

template<typename T, typename Alloc>
inline void dynarray<T, Alloc>::erase_to_end(iterator first) OEL_NOEXCEPT_NDEBUG
{
	T *const newEnd = to_pointer_contiguous(first);
	OEL_ASSERT(_m.data <= newEnd && newEnd <= _m.end);
	_detail::Destroy(newEnd, _m.end);
	_m.end = newEnd;
}

template<typename T, typename Alloc>
typename dynarray<T, Alloc>::iterator  dynarray<T, Alloc>::erase(iterator first, iterator last)
{
	_detail::AssertTrivialRelocate<T>();

	T *const pFirst = to_pointer_contiguous(first);
	T *const pLast = to_pointer_contiguous(last);
	OEL_ASSERT(_m.data <= pFirst && pFirst <= pLast && pLast <= _m.end);
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

template<typename T, typename Alloc>
inline T & dynarray<T, Alloc>::operator[](size_type i) OEL_NOEXCEPT_NDEBUG
{
	OEL_ASSERT(i < size());
	return _m.data[i];
}
template<typename T, typename Alloc>
inline const T & dynarray<T, Alloc>::operator[](size_type i) const OEL_NOEXCEPT_NDEBUG
{
	OEL_ASSERT(i < size());
	return _m.data[i];
}

#ifdef OEL_DEBUG_ABI
} // namespace debug
#endif

namespace _detail
{
	template<typename T, typename Pointer>
	struct DynarrBase
	{
		using value_type = T;

		Pointer data;      // Pointer to beginning of data buffer
		Pointer end;       // Pointer to one past the back object
		Pointer reservEnd; // Pointer to end of allocated memory

		bool DerefValid(typename std::pointer_traits<Pointer>::template rebind<T const> pos) const
		{
			return static_cast<size_t>(pos - data) < static_cast<size_t>(end - data);
		}
	};
}

} // namespace oel
