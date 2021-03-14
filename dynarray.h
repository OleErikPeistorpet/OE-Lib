#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "allocator.h"
#include "auxi/detail_forward.h"
#include "auxi/dynarray_iterator.h"
#include "auxi/impl_algo.h"
#include "optimize_ext/default.h"
#include "view/move.h"

#include <algorithm>

/** @file
*/

namespace oel
{

//! dynarray is trivially relocatable if Alloc is
template< typename T, typename Alloc >
is_trivially_relocatable<Alloc> specify_trivial_relocate(dynarray<T, Alloc>);

//! Overloads generic erase_unstable(RandomAccessContainer &, RandomAccessContainer::size_type) (in range_algo.h)
template< typename T, typename A >  inline
void erase_unstable(dynarray<T, A> & d, size_t index)  { d.erase_unstable(d.begin() + index); }

//! @name GenericContainerInsert
//!@{
// Overloads of generic functions for inserting into container (in range_algo.h)
template< typename T, typename A, typename InputRange >  inline
void assign(dynarray<T, A> & dest, InputRange && source)  { dest.assign(source); }

template< typename T, typename A, typename InputRange >  inline
void append(dynarray<T, A> & dest, InputRange && source)  { dest.append(source); }

template< typename T, typename A >  inline
void append(dynarray<T, A> & dest, size_t n, const T & val)  { dest.append(n, val); }

template< typename T, typename A, typename ForwardRange >  inline
auto insert(dynarray<T, A> & dest, typename dynarray<T, A>::const_iterator pos, ForwardRange && source)
{
	return dest.insert_r(pos, source);
}
//!@}

#ifdef OEL_DYNARRAY_IN_DEBUG
inline namespace debug
{
#endif

/**
* @brief Resizable array, dynamically allocated. Very similar to std::vector, but faster in many cases.
*
* In general, only that which differs from std::vector is documented.
*
* There is a general requirement that template argument T is trivially relocatable or noexcept move
* constructible (checked when compiling). Most types can be relocated trivially, but it often needs to be
* declared manually. See specify_trivial_relocate(T &&). Performance is better if T is trivially relocatable.
* Furthermore, a few functions require that T is trivially relocatable (noexcept movable is not enough):
* emplace, insert, insert_r
*
* Note that the allocator model is not quite standard: `destroy` is never used,
* `construct` may not be called if T is trivially constructible and is not called when relocating elements.
*/
template< typename T, typename Alloc/* = oel::allocator*/ >
class dynarray
{
	using _allocTrait = std::allocator_traits<Alloc>;

public:
	using value_type      = T;
	using allocator_type  = Alloc;
	using reference       = T &;
	using const_reference = const T &;
	using pointer         = typename _allocTrait::pointer;
	using difference_type = ptrdiff_t;
	using size_type       = size_t;

#if OEL_MEM_BOUND_DEBUG_LVL
	using iterator       = debug::dynarray_iterator<T *>;
	using const_iterator = debug::dynarray_iterator<const T *>;
#else
	using iterator       = T *;
	using const_iterator = const T *;
#endif
	using reverse_iterator       = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;


	constexpr dynarray() noexcept(noexcept(Alloc{}))        : _m(Alloc{}) {}
	constexpr explicit dynarray(const Alloc & a) noexcept   : _m(a) {}

	//! Construct empty dynarray with space reserved for exactly capacity elements
	dynarray(reserve_tag, size_type capacity, const Alloc & a = Alloc{})   : _m(a) { _initReserve(capacity); }

	/** @brief Default-initializes elements, can be significantly faster if T is scalar or has trivial default constructor
	*
	* @copydetails resize_for_overwrite(size_type)  */
	dynarray(size_type size, for_overwrite_t, const Alloc & a = Alloc{});
	//! (Value-initializes elements, same as std::vector)
	explicit dynarray(size_type size, const Alloc & a = Alloc{});
	dynarray(size_type size, const T & val, const Alloc & a = Alloc{})   : _m(a) { append(size, val); }

	/** @brief Equivalent to `std::vector(begin(r), end(r), a)`, where `end(r)` is not needed if `r.size()` exists
	*
	* To move instead of copy, wrap r with view::move (The same applies for all functions taking a range template)
	*
	* Example, construct from a standard istream with formatting (using Boost):
	@code
	#include <boost/range/istream_range.hpp>
	// Template argument for dynarray can be omitted with C++17 as shown (there exists a deduction guide)
	auto result = dynarray(boost::range::istream_range<int>(someStream));
	@endcode  */
	template< typename InputRange,
	          typename /*EnableIfRange*/ = iterator_t<InputRange> >
	explicit dynarray(InputRange && r, const Alloc & a = Alloc{})      : _m(a) { append(r); }

	dynarray(std::initializer_list<T> il, const Alloc & a = Alloc{})   : _m(a) { append(il); }

	dynarray(dynarray && other) noexcept                : _m(std::move(other._m)) {}
	dynarray(dynarray && other, const Alloc & a);
	dynarray(const dynarray & other)                    : dynarray(other,
	                                                     _allocTrait::select_on_container_copy_construction(other._m)) {}
	dynarray(const dynarray & other, const Alloc & a)   : _m(a) { append(other); }

	~dynarray() noexcept;

	dynarray & operator =(dynarray && other) &
		noexcept(_allocTrait::propagate_on_container_move_assignment::value or is_always_equal<Alloc>::value);
	//! Requires that allocator_type is always equal or does not have propagate_on_container_copy_assignment
	dynarray & operator =(const dynarray & other) &
		{
			static_assert(!_allocTrait::propagate_on_container_copy_assignment::value or is_always_equal<Alloc>::value,
			              "Alloc propagate_on_container_copy_assignment unsupported");
			assign(other);  return *this;
		}
	dynarray & operator =(std::initializer_list<T> il) &  { assign(il);  return *this; }

	void        swap(dynarray & other) noexcept;
	friend void swap(dynarray & a, dynarray & b) noexcept   OEL_ALWAYS_INLINE { a.swap(b); }

	/**
	* @brief Replace the contents with source range
	* @param source must model std::ranges::input_range, except `end(source)` is not required if `source.size()` is valid
	* @pre source shall not refer to any elements in this dynarray (same as std::vector::assign)
	* @return Iterator `begin(source)` incremented by the number of elements in source
	*
	* Any elements held before the call are either assigned to or destroyed. */
	template< typename InputRange >
	auto assign(InputRange && source)
	->	iterator_t<InputRange>           { return _doAssign(oel::adl_begin(source), _detail::CountOrEnd(source)); }

	void assign(size_type count, const T & val)   { clear();  append(count, val); }

	/**
	* @brief Add at end the elements from source range
	* @pre source shall not refer to any elements in this dynarray if reallocation happens.
	*	Reallocation is caused by `capacity() - size() < n`, where `n` is number of source elements
	* @return Iterator `begin(source)` incremented by the number of elements in source
	*
	* Otherwise equivalent to `std::vector::insert(end(), begin(source), end(source))`,
	* where `end(source)` is not needed if `source.size()` exists. */
	template< typename InputRange >
	auto append(InputRange && source)
	->	iterator_t<InputRange>             { return _append(oel::adl_begin(source), _detail::CountOrEnd(source)); }
	//! Equivalent to `std::vector::insert(end(), il)`
	void append(std::initializer_list<T> il)   { append<>(il); }
	/**
	* @brief Same as `std::vector::insert(end(), count, val)`
	* @pre val shall not be a reference to an element of this dynarray if reallocation happens.
	*	Reallocation is caused by `capacity() - size() < count` */
	void append(size_type count, const T & val);

	[[deprecated]] void resize_default_init(size_type n) { resize_for_overwrite(n); }
	/**
	* @brief Default-initializes added elements, can be significantly faster if T is scalar or trivially constructible
	*
	* Objects of scalar type get indeterminate values. http://en.cppreference.com/w/cpp/language/default_initialization  */
	void resize_for_overwrite(size_type n)   { _resizeImpl<_detail::UninitDefaultConstruct>(n); }
	void resize(size_type n)                 { _resizeImpl<_uninitFill>(n); }

	//! @brief Equivalent to `std::vector::insert(pos, begin(source), end(source))`,
	//!	where `end(source)` is not needed if `source.size()` exists
	template< typename ForwardRange >
	iterator  insert_r(const_iterator pos, ForwardRange && source) &;

	iterator  insert(const_iterator pos, T && val) &       { return emplace(pos, std::move(val)); }
	iterator  insert(const_iterator pos, const T & val) &  { return emplace(pos, val); }

	template< typename... Args >
	iterator  emplace(const_iterator pos, Args &&... elemInitArgs) &;

	/**
	* @brief Beware, passing an element of same array is often unsafe (otherwise same as std::vector::emplace_back)
	* @pre args shall not refer to any element of this dynarray, unless `size() < capacity()` */
	template< typename... Args >
	reference emplace_back(Args &&... args) &;

	/** @brief Beware, passing an element of same array is often unsafe (otherwise same as std::vector::push_back)
	* @pre val shall not be a reference to an element of this dynarray, unless `size() < capacity()` */
	void      push_back(T && val)       { emplace_back(std::move(val)); }
	//! @copydoc push_back(T &&)
	void      push_back(const T & val)  { emplace_back(val); }

	void      pop_back() noexcept;

	/**
	* @brief Erase the element at pos without maintaining order of elements after pos.
	*
	* Constant complexity (compared to linear in the distance between pos and end() for normal erase).
	* @return iterator corresponding to the same index in the sequence as pos, same as for std containers. */
	iterator  erase_unstable(iterator pos) &   { _eraseUnorder(pos);  return pos; }

	iterator  erase(iterator pos) &;

	iterator  erase(iterator first, const_iterator last) &;
	//! Equivalent to `erase(first, end())`, but potentially faster and does not require assignable T
	void      erase_to_end(iterator first) noexcept;

	void      clear() noexcept         { erase_to_end(begin()); }

	bool      empty() const noexcept   { return _m.data == _m.end; }

	size_type size() const noexcept    { return _m.end - _m.data; }

	void      reserve(size_type minCap);

	//! It's probably a good idea to check that size < capacity before calling, maybe add some treshold to size
	void      shrink_to_fit();

	size_type capacity() const noexcept   { return _m.reservEnd - _m.data; }

	constexpr size_type max_size() const noexcept   { return _allocTrait::max_size(_m) - _allocateWrap::sizeForHeader; }

	//! How much smaller capacity is than the number passed to allocator_type::allocate
	static constexpr size_type allocate_size_overhead() noexcept  { return _allocateWrap::sizeForHeader; }

	allocator_type get_allocator() const noexcept   { return _m; }

	iterator       begin() noexcept          OEL_ALWAYS_INLINE { return _makeIter(_m.data); }
	const_iterator begin() const noexcept    OEL_ALWAYS_INLINE { return _makeIter<const T *>(_m.data); }
	const_iterator cbegin() const noexcept   OEL_ALWAYS_INLINE { return begin(); }

	iterator       end() noexcept          OEL_ALWAYS_INLINE { return _makeIter(_m.end); }
	const_iterator end() const noexcept    OEL_ALWAYS_INLINE { return _makeIter<const T *>(_m.end); }
	const_iterator cend() const noexcept   OEL_ALWAYS_INLINE { return end(); }

	reverse_iterator       rbegin() noexcept        OEL_ALWAYS_INLINE { return reverse_iterator{end()}; }
	const_reverse_iterator rbegin() const noexcept  OEL_ALWAYS_INLINE { return const_reverse_iterator{end()}; }

	reverse_iterator       rend() noexcept        OEL_ALWAYS_INLINE { return reverse_iterator{begin()}; }
	const_reverse_iterator rend() const noexcept  OEL_ALWAYS_INLINE { return const_reverse_iterator{begin()}; }

	T *             data() noexcept         OEL_ALWAYS_INLINE { return _m.data; }
	const T *       data() const noexcept   OEL_ALWAYS_INLINE { return _m.data; }

	reference       front() noexcept        { return *begin(); }
	const_reference front() const noexcept  { return *begin(); }

	reference       back() noexcept         { return *_makeIter(_m.end - 1); }
	const_reference back() const noexcept   { return *_makeIter<const T *>(_m.end - 1); }

	reference       at(size_type index);
	const_reference at(size_type index) const;

	reference       operator[](size_type index) noexcept        { OEL_ASSERT(index < size());  return _m.data[index]; }
	const_reference operator[](size_type index) const noexcept  { OEL_ASSERT(index < size());  return _m.data[index]; }

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
	using _internBase = _detail::DynarrBase<pointer>;
	using _debugSizeUpdater = _detail::DebugSizeInHeaderUpdater<_internBase>;

	template< typename > // template to allow constexpr constructor when Alloc copy constructor is not constexpr
	struct _memOwner : public _internBase, public allocator_type
	{
		using _internBase::data;   // Owning pointer to beginning of data buffer
		using _internBase::end;       // Pointer to one past the back object
		using _internBase::reservEnd; // Pointer to end of allocated memory

		constexpr _memOwner(const allocator_type & a)
		 :	_internBase(), allocator_type(a) {
		}

		_memOwner(_memOwner && other) noexcept
		 :	_internBase(other), allocator_type(std::move(other))
		{
			other.reservEnd = other.end = other.data = nullptr;
		}

		~_memOwner()
		{
			if (data)
				_allocateWrap::dealloc(*this, data, reservEnd - data);
		}
	};
	_memOwner<Alloc> _m; // the only non-static data member

	// Should be very careful with potential name collisions because of inheriting from allocator
	struct _scopedPtr : private allocator_type
	{
		pointer data;  // owner
		pointer bufEnd;

		_scopedPtr(const allocator_type & a, size_type const capPrechecked)
		 :	allocator_type(a),
			data{_allocateWrap::allocate(*this, capPrechecked)},
			bufEnd{data + capPrechecked} {
		}

		_scopedPtr(_scopedPtr &&) = delete;

		~_scopedPtr()
		{
			if (data)
				_allocateWrap::dealloc(*this, data, bufEnd - data);
		}
	};

	using _uninitFill = _detail::UninitFill<decltype(_m)>;
	using _construct = _detail::Construct<T>;

	void _resetData(T *const newData)
	{
		if (_m.data)
			_allocateWrap::dealloc(_m, _m.data, capacity());

		_m.data = newData;
	}

	void _swapBuf(_scopedPtr & s)
	{
		using std::swap;
		swap(_m.data, s.data);
		swap(_m.reservEnd, s.bufEnd);
	}

	void _initReserve(size_type const capToCheck)
	{
		_m.end = _m.data = _allocateChecked(capToCheck);
		_m.reservEnd = _m.data + capToCheck;
	}


	template< typename Ptr >
	OEL_ALWAYS_INLINE auto _makeIter(Ptr pos) const noexcept
	{
	#if OEL_MEM_BOUND_DEBUG_LVL
		return _detail::MakeDynarrayIter<Ptr>(pos, _m.data, this);
	#else
		return pos;
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

	void _swapAlloc(std::false_type, Alloc & a) noexcept
	{	// propagate_on_container_swap false, standard says this is undefined if allocators compare unequal
		OEL_ASSERT(static_cast<Alloc &>(_m) == a);
		(void) a;
	}


	static constexpr auto _lenErrorMsg = "Going over dynarray max_size";

	size_type _unusedCapacity() const
	{
		return _m.reservEnd - _m.end;
	}

	size_type _calcCapUnchecked(size_type const newSize) const
	{
		return std::max(2 * capacity(), newSize);
	}

	size_type _calcCapChecked(size_type const newSize) const
	{
		if (newSize <= max_size())
			return _calcCapUnchecked(newSize);
		else
			_detail::Throw::lengthError(_lenErrorMsg);
	}

	size_type _calcCapAdd(size_type const nAdd, size_type const oldSize) const
	{
		if (nAdd <= SIZE_MAX / 2 / sizeof(T)) // assumes that allocating greater than SIZE_MAX / 2 always fails
			return _calcCapUnchecked(oldSize + nAdd);
		else
			_detail::Throw::lengthError(_lenErrorMsg);
	}

	size_type _calcCapAddOne() const
	{
		constexpr auto startBytesGood = std::max(3 * sizeof(void *), 4 * sizeof(int));
		constexpr auto minGrow = (startBytesGood + sizeof(T) - 1) / sizeof(T);
		size_type const c = capacity();
		return c + std::max(c, minGrow); // growth factor is 2
	}

	pointer _allocateChecked(size_type const n)
	{
		if (n <= max_size())
			return _allocateWrap::allocate(_m, n);
		else
			_detail::Throw::lengthError(_lenErrorMsg);
	}


	template< typename A = Alloc, enable_if< allocator_can_realloc<A>::value > = 0 >
	void _realloc(size_type const newCap, size_type const oldSize)
	{
		pointer const p = _allocateWrap::realloc(_m, _m.data, newCap);
		_m.data = p;
		_m.end = p + oldSize;
		_m.reservEnd = p + newCap;
	}

	template< typename... None >
	void _realloc(size_type const newCap, size_type const oldSize, None...)
	{
		pointer const newData = _allocateWrap::allocate(_m, newCap);
		_m.end = _detail::Relocate(_m.data, oldSize, newData);
		_resetData(newData);
		_m.reservEnd = newData + newCap;
	}


#ifdef _MSC_VER
	__declspec(noinline) // to get the compiler to inline calling function
#endif
	void _growBy(size_type const count)
	{
		size_type const s = size();
		_realloc(_calcCapAdd(count, s), s);
	}

	void _growByOne()
	{
		_realloc(_calcCapAddOne(), size());
	}


	template< typename UninitFiller >
	void _resizeImpl(size_type const newSize)
	{
		_debugSizeUpdater guard{_m};

		reserve(newSize);

		T *const newEnd = _m.data + newSize;
		if (_m.end < newEnd)
			UninitFiller::call(_m.end, newEnd, _m);
		else
			_detail::Destroy(newEnd, _m.end);

		_m.end = newEnd;
	}


	template< typename... None >
	void _eraseUnorder(iterator pos, None...)
	{
		*pos = std::move(back());
		pop_back();
	}

	template< typename T_ = T,
	          enable_if< is_trivially_relocatable<T_>::value > = 0
	>
	void _eraseUnorder(iterator const pos)
	{
		_debugSizeUpdater guard{_m};

		T *const ptr = std::addressof(*pos);
		ptr-> ~T();
		--_m.end;
		auto mem = reinterpret_cast<storage_for<T> *>(ptr);
		*mem = *reinterpret_cast<storage_for<T> *>(_m.end); // relocate last element to pos
	}


	template< typename ContiguousIter,
	          enable_if< can_memmove_with<T *, ContiguousIter>::value > = 0
	>
	ContiguousIter _doAssign(ContiguousIter const first, size_type const count)
	{
		_debugSizeUpdater guard{_m};

		if (capacity() < count)
		{
			// Deallocating first might be better, but then the _m pointers would have to be nulled in case allocate throws
			_resetData(_allocateChecked(count));
			_m.end = _m.reservEnd = _m.data + count;
		}
		else
		{	_m.end = _m.data + count;
		}
		// UB for self assign, but found to work. Add check in operator = or use memmove?
		_detail::MemcpyCheck(first, count, _m.data);

		return first + count;
	}

	template< typename InputIter, typename... None >
	InputIter _doAssign(InputIter src, size_type const count, None...)
	{
		_debugSizeUpdater guard{_m};

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
			T *const newData = _allocateChecked(count);
			// Old elements might hold some limited resource, destroying them before constructing new is probably good
			_detail::Destroy(_m.data, _m.end);
			_resetData(newData);
			_m.end = newData;
			_m.reservEnd = newData + count;
			newEnd = _m.reservEnd;
		}
		else
		{	newEnd = _m.data + count;
			if (newEnd < _m.end)
			{	// downsizing, assign new and destroy rest
				src = copy(std::move(src), _m.data, newEnd);
				erase_to_end(_makeIter(newEnd));
			}
			else // assign to old elements as far as we can
			{	src = copy(std::move(src), _m.data, _m.end);
			}
		}
		while (_m.end < newEnd)
		{	// each iteration updates _m.end for exception safety
			_construct::call(_m, _m.end, *src);
			++src; ++_m.end;
		}
		return src;
	}

	template< typename InputIter, typename Sentinel, typename... None >
	InputIter _doAssign(InputIter first, Sentinel const last, None...)
	{	// single-pass iterator and unknown count
		clear();
		for (; first != last; ++first)
			emplace_back(*first);

		return first;
	}

	template< typename InputIter, typename Sentinel >
	InputIter _append(InputIter first, Sentinel const last)
	{	// single-pass iterator and unknown count
		size_type const oldSize = size();
		OEL_TRY_
		{
			for (; first != last; ++first)
				emplace_back(*first);
		}
		OEL_CATCH_ALL
		{
			erase_to_end(begin() + oldSize);
			OEL_RETHROW;
		}
		return first;
	}

	template< typename InputIter >
	InputIter _append(InputIter src, size_type const n)
	{
		return _appendImpl(
			[src_ = std::move(src)](T * dest, size_type n_, decltype(_m) & alloc) mutable
			{
				return _detail::UninitCopy(std::move(src_), dest, dest + n_, alloc);
			},
			n );
	}

	template< typename MakeFuncAppend >
	auto _appendImpl(MakeFuncAppend makeNew, size_type const count)
	{
		_debugSizeUpdater guard{_m};

		if (_unusedCapacity() < count)
			_growBy(count);

		auto last = makeNew(_m.end, count, _m);
		_m.end += count;
		return last;
	}


	template< typename InsertHelper, typename... Args >
	T * _insertRealloc(T *const pos, Args... args)
	{
		_scopedPtr newBuf{_m, InsertHelper::calcCap(*this, args...)};

		size_type const nBefore = pos - data();
		T *const newPos = newBuf.data + nBefore;
		T *const afterAdded =
			InsertHelper::template construct<Args...>(_m, newPos, static_cast<Args &&>(args)...);
		// Exception free from here
		size_type const nAfterPos = _m.end - pos;
		_detail::Relocate(_m.data, nBefore, newBuf.data);  // prefix
		_m.end = _detail::Relocate(pos, nAfterPos, afterAdded); // suffix
		_swapBuf(newBuf);

		return newPos;
	}

	struct _emplaceHelper
	{
		template< typename... Unused >
		static size_type calcCap(dynarray & self, const Unused &...)
		{
			return self._calcCapAddOne();
		}

		template< typename... Args >
		static T * construct(decltype(_m) & alloc, T *const newPos, Args... args)
		{
			_construct::call(alloc, newPos, static_cast<Args &&>(args)...);
			return newPos + 1;
		}
	};

	struct _insertRHelper
	{
		template< typename InputIter >
		static size_type calcCap(dynarray & self, const InputIter &, size_type count)
		{
			return self._calcCapAdd(count, self.size());
		}

		template< typename InputIter, typename >
		static T * construct(decltype(_m) & alloc, T *const newPos, InputIter first, size_type count)
		{
			T *const dLast = newPos + count;
			_detail::UninitCopy(std::move(first), newPos, dLast, alloc);
			return dLast;
		}
	};
};

template< typename T, typename Alloc >
template< typename... Args >
typename dynarray<T, Alloc>::iterator
	dynarray<T, Alloc>::emplace(const_iterator pos, Args &&... args) &
{
#define OEL_DYNARR_INSERT_STEP1  \
	static_assert(is_trivially_relocatable<T>::value,  \
		"insert, emplace require trivially relocatable T, see declaration of is_trivially_relocatable");  \
	\
	_debugSizeUpdater guard{_m};  \
	\
	auto pPos = const_cast<T *>(to_pointer_contiguous(pos));  \
	OEL_ASSERT(_m.data <= pPos and pPos <= _m.end);

	OEL_DYNARR_INSERT_STEP1
	if (_m.end < _m.reservEnd)
	{
		// Temporary in case constructor throws or source is an element of this dynarray at pos or after
		storage_for<T> tmp;
		_construct::call(_m, reinterpret_cast<T *>(&tmp), static_cast<Args &&>(args)...);
		// Relocate [pos, end) to [pos + 1, end + 1), leaving memory at pos uninitialized (conceptually)
		size_type const bytesAfterPos = sizeof(T) * (_m.end - pPos);
		std::memmove(
			static_cast<void *>(pPos + 1),
			static_cast<const void *>(pPos),
			bytesAfterPos );
		++_m.end;
		std::memcpy(static_cast<void *>(pPos), &tmp, sizeof(T)); // relocate the new element to pos
	}
	else
	{	pPos = _insertRealloc< _emplaceHelper, _detail::ForwardT<Args>... >
				(pPos, static_cast<Args &&>(args)...);
	}
	return _makeIter(pPos);
}

template< typename T, typename Alloc >
template< typename ForwardRange >
typename dynarray<T, Alloc>::iterator
	dynarray<T, Alloc>::insert_r(const_iterator pos, ForwardRange && src) &
{
	auto first = oel::adl_begin(src);
	auto const count = _detail::CountOrEnd(src);

	static_assert( std::is_same<decltype(count), size_t const>::value,
			"insert_r requires that source models std::ranges::forward_range or that source.size() is valid" );

	OEL_DYNARR_INSERT_STEP1
#undef OEL_DYNARR_INSERT_STEP1

	if (_unusedCapacity() >= count)
	{
		T *const dLast = pPos + count;
		// Relocate elements to make space, leaving [pos, pos + count) uninitialized (conceptually)
		size_type const bytesAfterPos = sizeof(T) * (_m.end - pPos);
		std::memmove(
			static_cast<void *>(dLast),
			static_cast<const void *>(pPos),
			bytesAfterPos );
		_m.end += count;
		// Construct new
		if (can_memmove_with<T *, decltype(first)>::value)
		{
			_detail::UninitCopy(first, pPos, dLast, _m);
		}
		else
		{	T * dest = pPos;
			OEL_TRY_
			{
				while (dest != dLast)
				{
					_construct::call(_m, dest, *first);
					++first; ++dest;
				}
			}
			OEL_CATCH_ALL
			{	// relocate back to fill hole
				std::memmove(
					static_cast<void *>(dest),
					static_cast<const void *>(dLast),
					bytesAfterPos );
				_m.end -= (dLast - dest);
				OEL_RETHROW;
			}
		}
	}
	else
	{	pPos = _insertRealloc<_insertRHelper>(pPos, std::move(first), count);
	}
	return _makeIter(pPos);
}

template< typename T, typename Alloc >
template< typename... Args >
inline T & dynarray<T, Alloc>::emplace_back(Args &&... args) &
{
	_debugSizeUpdater guard{_m};

	if (_m.end == _m.reservEnd)
		_growByOne();

	_construct::call(_m, _m.end, static_cast<Args &&>(args)...);

	return *(_m.end++);
}


template< typename T, typename Alloc >
dynarray<T, Alloc>::dynarray(dynarray && other, const Alloc & a)
 :	_m(a)
{
	OEL_CONST_COND if (!is_always_equal<Alloc>::value and a != other._m)
		append(view::move(other));
	else
		_moveInternBase(other._m);
}

template< typename T, typename Alloc >
dynarray<T, Alloc> &  dynarray<T, Alloc>::operator =(dynarray && other) &
	noexcept(_allocTrait::propagate_on_container_move_assignment::value or is_always_equal<Alloc>::value)
{
	OEL_CONST_COND if (!_allocTrait::propagate_on_container_move_assignment::value
	               and static_cast<Alloc &>(_m) != other._m)
	{
		assign(view::move(other));
	}
	else // take allocated memory from other
	{
		if (_m.data)
		{
			_detail::Destroy(_m.data, _m.end);
			_allocateWrap::dealloc(_m, _m.data, capacity());
		}
		_moveInternBase(other._m);
		_moveAssignAlloc(typename _allocTrait::propagate_on_container_move_assignment{}, other._m);
	}
	return *this;
}

template< typename T, typename Alloc >
dynarray<T, Alloc>::dynarray(size_type n, for_overwrite_t, const Alloc & a)
 :	_m(a)
{
	_debugSizeUpdater guard{_m};

	_initReserve(n);
	_m.end = _m.reservEnd;
	_detail::UninitDefaultConstruct::call(_m.data, _m.reservEnd, _m);
}

template< typename T, typename Alloc >
dynarray<T, Alloc>::dynarray(size_type n, const Alloc & a)
 :	_m(a)
{
	_debugSizeUpdater guard{_m};

	_initReserve(n);
	_m.end = _m.reservEnd;
	_uninitFill::call(_m.data, _m.reservEnd, _m);
}

template< typename T, typename Alloc >
dynarray<T, Alloc>::~dynarray() noexcept
{
	_detail::Destroy(_m.data, _m.end);
}

template< typename T, typename Alloc >
void dynarray<T, Alloc>::swap(dynarray & other) noexcept
{
	_internBase & a = _m;
	_internBase & b = other._m;
	std::swap(a, b);
	_swapAlloc(typename _allocTrait::propagate_on_container_swap{}, other._m);
}


template< typename T, typename Alloc >
void dynarray<T, Alloc>::reserve(size_type n)
{
	if (capacity() < n)
	{
		_debugSizeUpdater guard{_m};

		_realloc(_calcCapChecked(n), size());
	}
}

template< typename T, typename Alloc >
void dynarray<T, Alloc>::shrink_to_fit()
{
	_debugSizeUpdater guard{_m};

	size_type const used = size();
	if (0 < used)
	{
		_realloc(used, used);
	}
	else
	{	_resetData(nullptr);
		_m.reservEnd = _m.end = nullptr;
	}
}


template< typename T, typename Alloc >
inline void dynarray<T, Alloc>::append(size_type n, const T & val)
{
	struct Fill
	{
		_detail::ForwardT<const T &> val_;

		auto operator()(T * dest, size_type n_, decltype(_m) & alloc) const
		{
			_uninitFill::call(dest, dest + n_, alloc, val_);
			return nullptr; // returned by _appendImpl
		}
	};
	_appendImpl(Fill{val}, n);
}


template< typename T, typename Alloc >
inline void dynarray<T, Alloc>::pop_back() noexcept
{
	OEL_ASSERT(_m.data < _m.end);
	_debugSizeUpdater guard{_m};

	--_m.end;
	(*_m.end).~T();
}

template< typename T, typename Alloc >
inline void dynarray<T, Alloc>::erase_to_end(iterator first) noexcept
{
	_debugSizeUpdater guard{_m};

	T *const newEnd = to_pointer_contiguous(first);
	OEL_ASSERT(_m.data <= newEnd and newEnd <= _m.end);
	_detail::Destroy(newEnd, _m.end);
	_m.end = newEnd;
}

template< typename T, typename Alloc >
typename dynarray<T, Alloc>::iterator  dynarray<T, Alloc>::erase(iterator pos) &
{
	_debugSizeUpdater guard{_m};

	T *const ptr = to_pointer_contiguous(pos);
	OEL_ASSERT(_m.data <= ptr and ptr < _m.end);
	if (is_trivially_relocatable<T>::value)
	{
		ptr-> ~T();
		T *const next = ptr + 1;
		std::memmove( // relocate [pos + 1, end) to [pos, end - 1)
			static_cast<void *>(ptr),
			static_cast<const void *>(next),
			sizeof(T) * (_m.end - next) );
		--_m.end;
	}
	else
	{	_m.end = std::move(ptr + 1, _m.end, ptr);
		(*_m.end).~T();
	}
	return pos;
}

template< typename T, typename Alloc >
typename dynarray<T, Alloc>::iterator  dynarray<T, Alloc>::erase(iterator first, const_iterator last) &
{
	_debugSizeUpdater guard{_m};

	T *            dest = to_pointer_contiguous(first);
	const T *const pLast = to_pointer_contiguous(last);
	OEL_ASSERT(_m.data <= dest and dest <= pLast and pLast <= _m.end);
	if (is_trivially_relocatable<T>::value)
	{
		_detail::Destroy(dest, pLast);
		size_type const nAfterLast = _m.end - pLast;
		std::memmove( // relocate [last, end) to [first, first + nAfterLast)
			static_cast<void *>(dest),
			static_cast<const void *>(pLast),
			sizeof(T) * nAfterLast );
		_m.end = dest + nAfterLast;
	}
	else if (dest < pLast) // must avoid self-move-assigning the elements
	{
		dest = std::move(const_cast<T *>(pLast), _m.end, dest);
		_detail::Destroy(dest, _m.end);
		_m.end = dest;
	}
	return first;
}


template< typename T, typename Alloc >
OEL_ALWAYS_INLINE inline T & dynarray<T, Alloc>::at(size_type i)
{
	const auto & cSelf = *this;
	return const_cast<reference>(cSelf.at(i));
}
template< typename T, typename Alloc >
const T & dynarray<T, Alloc>::at(size_type i) const
{
	if (i < size()) // would be unsafe with signed size_type
		return _m.data[i];
	else
		_detail::Throw::outOfRange("Bad index dynarray::at");
}


#ifdef OEL_HAS_DEDUCTION_GUIDES
template<
	typename InputRange,
	typename Alloc = allocator<
		iter_value_t< iterator_t<InputRange> >
      > >
explicit dynarray(InputRange &&, Alloc = {})
->	dynarray<
		iter_value_t< iterator_t<InputRange> >,
		Alloc >;
#endif

#ifdef OEL_DYNARRAY_IN_DEBUG
}
#endif

} // namespace oel
