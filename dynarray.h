#pragma once

// Copyright 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "allocator.h"
#include "auxi/dynarray_iterator.h"
#include "auxi/forward_t.h"
#include "auxi/impl_algo.h"
#include "optimize_ext/default.h"
#include "view/move.h"

#include <algorithm>

/** @file
*/

namespace oel
{

//! `r | to_dynarray()` is equivalent to `r | std::ranges::to<dynarray>()`
/**
* Example, convert array of std::bitset to `dynarray<std::string>`:
@code
std::bitset<8> arr[] {3, 5, 7, 11};
auto result = arr | view::transform(OEL_MEMBER_FN(to_string)) | to_dynarray();
@endcode  */
template< typename Alloc = allocator<> >
constexpr auto to_dynarray(Alloc a = {})
	{
		return _detail::ToDynarrPartial<Alloc>{std::move(a)};
	}

//! dynarray is trivially relocatable if Alloc is
template< typename T, typename Alloc >
is_trivially_relocatable<Alloc> specify_trivial_relocate(dynarray<T, Alloc>);


#if OEL_MEM_BOUND_DEBUG_LVL
inline namespace debug
{
#endif

//! Resizable array, dynamically allocated. Very similar to std::vector, but faster in some cases.
/**
* In general, only that which differs from std::vector is documented.
*
* There is a general requirement that template argument T is trivially relocatable or noexcept move
* constructible (checked when compiling). Most types can be relocated trivially, but it often needs to be
* declared manually. See is_trivially_relocatable (fwd.h). Performance is better if T is trivially relocatable.
* Furthermore, a few functions require that T is trivially relocatable (noexcept movable is not enough):
* emplace, insert, insert_range
*
* Note that the allocator model is not quite standard: `destroy` is never used, `construct` may not be called
* if T is trivially constructible and is not called when relocating elements. Also, Alloc::value_type need not
* be the same as T, it will be rebound, so you can use e.g. `std::pmr::polymorphic_allocator<>`.
*
* For any function which takes a range, `end(range)` is not needed if `range.size()` is valid.
*/
template< typename T, typename Alloc/* = oel::allocator*/ >
class dynarray
{
	using _alloTrait = typename std::allocator_traits<Alloc>::template rebind_traits<T>;

public:
	using value_type      = T;
	using allocator_type  = typename _alloTrait::allocator_type;
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


	constexpr dynarray() noexcept(noexcept(Alloc{}))   : dynarray(Alloc{}) {}
	constexpr explicit dynarray(Alloc a) noexcept      : _m(a) {}

	//! Construct empty dynarray with space reserved for exactly capacity elements
	dynarray(reserve_tag, size_type capacity, Alloc a = Alloc{})   : _m(a) { _initReserve(capacity); }

	/** @brief Default-initializes elements, can be significantly faster if T is scalar or has trivial default constructor
	*
	* @copydetails resize_for_overwrite(size_type)  */
	dynarray(size_type size, for_overwrite_t, Alloc a = Alloc{});
	//! (Value-initializes elements, same as std::vector)
	explicit dynarray(size_type size, Alloc a = Alloc{});

	template< typename InputRange >
	dynarray(from_range_t, InputRange && r, Alloc a = Alloc{})   : _m(a) { append(r); }

	dynarray(std::initializer_list<T> il, Alloc a = Alloc{})     : _m(a) { append(il); }

	dynarray(dynarray && other) noexcept                : _m(std::move(other._m)) {}
	dynarray(dynarray && other, Alloc a);
	explicit dynarray(const dynarray & other)           : dynarray( other,
	                                                      _alloTrait::select_on_container_copy_construction(other._m) ) {}
	explicit dynarray(const dynarray & other, Alloc a)  : _m(a) { append(other); }

	dynarray & operator =(dynarray && other) &
		noexcept(_alloTrait::propagate_on_container_move_assignment::value or _alloTrait::is_always_equal::value);
	//! Requires that allocator_type is always equal or does not have propagate_on_container_copy_assignment
	dynarray & operator =(const dynarray & other) &;
	dynarray & operator =(const dynarray &&) = delete;

	dynarray & operator =(std::initializer_list<T> il) &  { assign(il);  return *this; }

	friend void swap(dynarray & a, dynarray & b) noexcept
		{
			using std::swap;
			_internBase & x = a._m;  _internBase & y = b._m;
			swap(x, y);

			[[maybe_unused]] allocator_type & a0 = a._m;
			[[maybe_unused]] allocator_type & a1 = b._m;
			if constexpr (_alloTrait::propagate_on_container_swap::value)
				swap(a0, a1);
			else // Standard says this is undefined if allocators compare unequal
				OEL_ASSERT(a0 == a1);
		}

	//! Almost same as std::vector::assign_range (C++23)
	/** @return Iterator `begin(source)` incremented by the number of elements in source
	*
	* Any elements held before the call are either assigned to or destroyed. */
	template< typename InputRange >
	auto assign(InputRange && source) -> borrowed_iterator_t<InputRange>;

	void assign(size_type count, const T & val)   { clear();  append(count, val); }

	/**
	* @brief Almost same as std::vector::append_range (C++23)
	* @pre source shall not refer to any elements in this dynarray if reallocation happens.
	*	Reallocation is caused by `capacity() - size() < n`, where `n` is number of source elements
	* @return Iterator `begin(source)` incremented by the number of elements in source  */
	template< typename InputRange >
	auto append(InputRange && source) -> borrowed_iterator_t<InputRange>;

	//! Equivalent to `std::vector::insert(end(), il)`
	void append(std::initializer_list<T> il)   { append<>(il); }
	/**
	* @brief Same as `std::vector::insert(end(), count, val)`
	* @pre val shall not be a reference to an element of this dynarray if reallocation happens.
	*	Reallocation is caused by `capacity() - size() < count` */
	void append(size_type count, const T & val);

	/**
	* @brief Default-initializes added elements, can be significantly faster if T is scalar or trivially constructible
	*
	* Objects of scalar type get indeterminate values. http://en.cppreference.com/w/cpp/language/default_initialization  */
	void resize_for_overwrite(size_type n)   { _doResize< _detail::DefaultInit<allocator_type> >(n); }
	void resize(size_type n)                 { _doResize<_uninitFill>(n); }

	/**
	* @brief Almost same as std::vector::insert_range
	* @param source must model std::ranges::forward_range or `source.size()` must be valid. */
	template< typename Range >
	iterator insert_range(const_iterator pos, Range && source) &;

	iterator insert(const_iterator pos, T && val) &       { return emplace(pos, std::move(val)); }
	iterator insert(const_iterator pos, const T & val) &  { return emplace(pos, val); }

	template< typename... Args >  OEL_ALWAYS_INLINE
	iterator emplace(const_iterator pos, Args &&... args) &
		{
			return _doEmplace< forward_t<Args>... >(pos, static_cast<Args &&>(args)...);
		}

	//! Beware, passing an element of same dynarray is often unsafe (otherwise same as std::vector::emplace_back)
	/** @pre args shall not refer to any element of this container, unless `size() < capacity()` */
	template< typename... Args >
	T &  emplace_back(Args &&... args) &;

	//! Beware, passing an element of same dynarray is often unsafe (otherwise same as std::vector::push_back)
	/** @pre val shall not be a reference to an element of this container, unless `size() < capacity()` */
	void push_back(T && val)       { emplace_back(std::move(val)); }
	//! @copydoc push_back(T &&)
	void push_back(const T & val)  { emplace_back(val); }

	void pop_back() noexcept
		{
			OEL_ASSERT(_m.data < _m.end); // not empty
			--_m.end;
			_m.end-> ~T();
			(void) _debugSizeUpdater{_m};
		}

	//! Erase the element at pos without maintaining order of elements after pos.
	/**
	* Constant complexity (compared to linear in the distance between pos and end() for normal erase).
	* @return iterator corresponding to the same index in the sequence as pos, same as for std containers. */
	iterator  unordered_erase(iterator pos) &;

	iterator  erase(iterator pos) &;

	iterator  erase(iterator first, const_iterator last) &;
	//! Equivalent to `erase(first, end())`, but potentially faster and does not require assignable T
	void      erase_to_end(iterator first) noexcept;

	void      clear() noexcept   { erase_to_end(begin()); }

	void      reserve(size_type minCap)
		{
			if (capacity() < minCap)
				_realloc(_calcCapChecked(minCap), size());
		}
	//! It's probably a good idea to check that size < capacity before calling, maybe add some treshold to size
	void      shrink_to_fit();

	[[nodiscard]] bool empty() const noexcept  { return _m.data == _m.end; }

	size_type size() const noexcept            { return static_cast<size_t>(_m.end - _m.data); }

	size_type capacity() const noexcept        { return static_cast<size_t>(_m.reservEnd - _m.data); }

	constexpr size_type max_size() const noexcept   { return _alloTrait::max_size(_m) - _allocateWrap::sizeForHeader; }

	//! How much smaller capacity is than the number passed to allocator_type::allocate
	static constexpr size_type allocate_size_overhead() noexcept   { return _allocateWrap::sizeForHeader; }

	allocator_type get_allocator() const noexcept   { return _m; }

	iterator       begin() noexcept          { return _detail::MakeDynarrIter           (_m, _m.data); }
	const_iterator begin() const noexcept    { return _detail::MakeDynarrIter<const T *>(_m, _m.data); }
	const_iterator cbegin() const noexcept   { return begin(); }

	iterator       end() noexcept          { return _detail::MakeDynarrIter           (_m, _m.end); }
	const_iterator end() const noexcept    { return _detail::MakeDynarrIter<const T *>(_m, _m.end); }
	OEL_ALWAYS_INLINE
	const_iterator cend() const noexcept   { return end(); }

	reverse_iterator       rbegin() noexcept         { return       reverse_iterator{end()}; }
	const_reverse_iterator rbegin() const noexcept   { return const_reverse_iterator{end()}; }
	const_reverse_iterator crbegin() const noexcept  { return const_reverse_iterator{end()}; }

	OEL_ALWAYS_INLINE
	reverse_iterator       rend() noexcept         { return       reverse_iterator{begin()}; }
	OEL_ALWAYS_INLINE
	const_reverse_iterator rend() const noexcept   { return const_reverse_iterator{begin()}; }
	OEL_ALWAYS_INLINE
	const_reverse_iterator crend() const noexcept  { return const_reverse_iterator{begin()}; }

	T *       data() noexcept         { return _m.data; }
	const T * data() const noexcept   { return _m.data; }

	T &       front() noexcept        { return (*this)[0]; }
	const T & front() const noexcept  { return (*this)[0]; }

	T &       back() noexcept         { return *_detail::MakeDynarrIter           (_m, _m.end - 1); }
	const T & back() const noexcept   { return *_detail::MakeDynarrIter<const T *>(_m, _m.end - 1); }

	T &       operator[](size_type index) noexcept        { OEL_ASSERT(index < size());  return _m.data[index]; }
	const T & operator[](size_type index) const noexcept  { OEL_ASSERT(index < size());  return _m.data[index]; }

	OEL_ALWAYS_INLINE
	T &       at(size_type index)
		{
			const auto & cSelf = *this;
			return const_cast<T &>(cSelf.at(index));
		}
	const T & at(size_type index) const
		{
			if (index < size()) // would be unsafe with signed size_type
				return _m.data[index];
			else
				_detail::OutOfRange::raise();
		}

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
	using _allocateWrap = _detail::DebugAllocateWrapper<allocator_type, T *>;
	using _internBase   = _detail::DynarrBase<T *>;
	using _uninitFill   = _detail::UninitFill<allocator_type>;
	using _debugSizeUpdater = _detail::DebugSizeInHeaderUpdater<_internBase>;
	using _argAlloc_7KQw  = Alloc; // guarding against name collision due to inheritance (MSVC)
	using _usedAlloc_7KQw = allocator_type;

	struct _dataOwner : public _internBase, public _usedAlloc_7KQw
	{
		using B = ::oel::_detail::DynarrBase<value_type *>;

		using B::data;
		using B::end;
		using B::reservEnd;

		constexpr _dataOwner(_argAlloc_7KQw & a) noexcept
		 :	B{}, _usedAlloc_7KQw{std::move(a)}
		{}

		constexpr _dataOwner(_dataOwner && other) noexcept
		 :	B{other}, _usedAlloc_7KQw{std::move(other)}
		{
			other.reservEnd = other.end = other.data = nullptr;
		}

		~_dataOwner()
		{
			if (data)
			{
				::oel::_detail::Destroy(data, end);

				auto cap = static_cast<::std::size_t>(reservEnd - data);
				::oel::_detail::DebugAllocateWrapper<_usedAlloc_7KQw, value_type *>::dealloc(*this, data, cap);
			}
		}
	}
	_m; // exception safety helper, the only non-static data member

	void _resetData(T *const newData, size_type const newCap)
	{
		if (_m.data)
			_allocateWrap::dealloc(_m, _m.data, capacity());
		// Beware, sets _m.data with no _debugSizeUpdater
		_m.data      = newData;
		_m.reservEnd = newData + newCap;
	}

	void _initReserve(size_type const capToCheck)
	{
		_m.end = _m.data = _allocateChecked(capToCheck);
		_m.reservEnd = _m.data + capToCheck;
	}

	void _moveInternBase(_internBase & src) noexcept
	{
		_internBase & dest = _m;
		dest = src;
		src  = {};
	}


	auto _spareCapacity() const
	{
		return static_cast<size_type>(_m.reservEnd - _m.end);
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
			_detail::LengthError::raise();
	}

	size_type _calcCapAdd(size_type const nAdd, size_type const oldSize) const
	{
		if (nAdd <= SIZE_MAX / 2 / sizeof(T)) // assumes that allocating greater than SIZE_MAX / 2 always fails
			return _calcCapUnchecked(oldSize + nAdd);
		else
			_detail::LengthError::raise();
	}

	size_type _calcCapAddOne() const
	{
		constexpr auto startBytesGood = std::max(3 * sizeof(void *), 4 * sizeof(int));
		constexpr auto minGrow = (startBytesGood + sizeof(T) - 1) / sizeof(T);
		auto const c = capacity();
		return c + std::max(c, minGrow); // growth factor is 2
	}

	T * _allocateChecked(size_type const n)
	{
		if (n <= max_size())
			return _allocateWrap::allocate(_m, n);
		else
			_detail::LengthError::raise();
	}


	void _realloc(size_type const newCap, size_type const oldSize)
	{
		if constexpr( oel::allocator_can_realloc<allocator_type>() )
		{
			auto const p = _allocateWrap::realloc(_m, _m.data, newCap);
			_m.data = p;
			_m.end = p + oldSize;
			_m.reservEnd = p + newCap;
		}
		else
		{	auto const newData = _allocateWrap::allocate(_m, newCap);
			_m.end = _detail::Relocate(_m.data, oldSize, newData);
			_resetData(newData, newCap);
		}
		(void) _debugSizeUpdater{_m};
	}

	// These are not defined inline as a compiler hint
	void _growByOne();
	void _growBy(size_type const);


	template< typename UninitFiller >
	void _doResize(size_type const newSize)
	{
		reserve(newSize);

		T *const newEnd = _m.data + newSize;
		if (_m.end < newEnd)
			UninitFiller::call(_m.end, newEnd, _m);
		else
			_detail::Destroy(newEnd, _m.end);

		_debugSizeUpdater guard{_m};
		_m.end = newEnd;
	}


	template< typename InputIter >
	InputIter _doAssign(InputIter src, size_type const count)
	{
		_debugSizeUpdater guard{_m};

		if constexpr (can_memmove_with<T *, InputIter>)
		{
			if (capacity() < count)
			{	// Deallocating first might be better,
				// but then the _m pointers would have to be nulled in case allocate throws
				_resetData(_allocateChecked(count), count);
				_m.end = _m.reservEnd;
			}
			else
			{	_m.end = _m.data + count;
			}
			_detail::MemcpyCheck(src, count, _m.data);

			return src + count;
		}
		else
		{	auto cpy = [](InputIter src_, T *__restrict dest, T * dLast)
			{
				while (dest != dLast)
				{
					*dest = *src_;
					++dest; ++src_;
				}
				return src_;
			};
			T * newEnd;
			if (capacity() < count)
			{
				auto const newData = _allocateChecked(count);
				// Old elements might hold some limited resource, probably good to destroy them before constructing new
				_detail::Destroy(_m.data, _m.end);
				_resetData(newData, count);
				_m.end = newData;
				newEnd = _m.reservEnd;
			}
			else
			{	newEnd = _m.data + count;
				if (newEnd < _m.end)
				{	// downsizing, assign new and destroy rest
					src = cpy(std::move(src), _m.data, newEnd);
					erase_to_end(_detail::MakeDynarrIter(_m, newEnd));
				}
				else // assign to old elements as far as we can
				{	src = cpy(std::move(src), _m.data, _m.end);
				}
			}
			while (_m.end != newEnd)
			{	// each iteration updates _m.end for exception safety
				_alloTrait::construct(_m, _m.end, *src);
				++src; ++_m.end;
			}
			return src;
		}
	}

	template< typename InputIter >
	InputIter _doAppend(InputIter src, size_type const count)
	{
		if (_spareCapacity() < count) OEL_UNLIKELY
			_growBy(count);

		if constexpr (can_memmove_with<T *, InputIter>)
		{
			_detail::MemcpyCheck(src, count, _m.end);
			src += count;
			_m.end += count;
		}
		else
		{	T *__restrict dest = _m.end;
			auto const   dLast = dest + count;
			OEL_TRY_
			{
				while (dest != dLast)
				{
					_alloTrait::construct(_m, dest, *src);
					++dest; ++src;
				}
			}
			OEL_CATCH_ALL
			{
				_m.end = dest;
				OEL_RETHROW;
			}
			_m.end = dest;
		}
		_debugSizeUpdater guard{_m};

		return src;
	}


	T * _insertReallocImpl(size_type const newCap, T *const pos, size_type const count)
	{
		auto const newData = _allocateWrap::allocate(_m, newCap);
		// Exception free from here
		auto const nBefore = pos - _m.data;
		auto const nAfter  = _m.end - pos;
		T *const newPos = _detail::Relocate(_m.data, nBefore, newData);
		_m.end          = _detail::Relocate(pos, nAfter, newPos + count);

		_resetData(newData, newCap);
		return newPos;
	}

	T * _insRangeRealloc(T *const pos, size_type const count)
	{
		auto newCap = _calcCapAdd(count, size());
		return _insertReallocImpl(newCap, pos, count);
	}

	T * _emplaceRealloc(T * pos, T & destroyOnFail)
	{
		struct Guard
		{
			T * destroy;

			~Guard()
			{
				if (destroy)
					destroy-> ~T();
			}
		} exit{&destroyOnFail};

		pos = _insertReallocImpl(_calcCapAddOne(), pos, 1);
		exit.destroy = nullptr;
		return pos;
	}

	template< typename... Args >
	iterator _doEmplace(const_iterator pos, Args... args)
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

		// Temporary in case constructor throws or args refer to an element of this dynarray
		alignas(T) unsigned char tmp[sizeof(T)];
		_alloTrait::construct(_m, reinterpret_cast<T *>(&tmp), static_cast<Args &&>(args)...);
		if (_m.end < _m.reservEnd)
		{	// Relocate [pos, end) to [pos + 1, end + 1)
			size_t const bytesAfterPos{sizeof(T) * (_m.end - pPos)};
			std::memmove(
				static_cast<void *>(pPos + 1),
				static_cast<const void *>(pPos),
				bytesAfterPos );
			++_m.end;
		}
		else
		{	pPos = _emplaceRealloc(pPos, reinterpret_cast<T &>(tmp));
		}
		std::memcpy(static_cast<void *>(pPos), &tmp, sizeof(T)); // relocate the new element to pos

		return _detail::MakeDynarrIter(_m, pPos);
	}
};

template< typename T, typename Alloc >
template< typename Range >
typename dynarray<T, Alloc>::iterator
	dynarray<T, Alloc>::insert_range(const_iterator pos, Range && source) &
{
	OEL_DYNARR_INSERT_STEP1
#undef OEL_DYNARR_INSERT_STEP1

	static_assert( _detail::rangeIsForwardOrSized<Range>,
		"insert_range requires that source models std::ranges::forward_range or that source.size() is valid" );

	auto       first = adl_begin(source);
	auto const count = _detail::UDist(source);

	size_t const bytesAfterPos{sizeof(T) * (_m.end - pPos)};
	T * dLast;
	if (_spareCapacity() >= count)
	{
		dLast = pPos + count;
		// Relocate elements to make space, leaving [pos, pos + count) uninitialized (conceptually)
		std::memmove(static_cast<void *>(dLast), static_cast<const void *>(pPos), bytesAfterPos);
		_m.end += count;
	}
	else
	{	pPos  = _insRangeRealloc(pPos, count);
		dLast = pPos + count;
	}
	// Construct new
	if constexpr (can_memmove_with<T *, decltype(first)>)
	{
		_detail::MemcpyCheck(first, count, pPos);
	}
	else
	{	T *__restrict dest = pPos;
		OEL_TRY_
		{
			while (dest != dLast)
			{
				_alloTrait::construct(_m, dest, *first);
				++dest; ++first;
			}
		}
		OEL_CATCH_ALL
		{	// relocate back to fill hole
			std::memmove(static_cast<void *>(dest), static_cast<const void *>(dLast), bytesAfterPos);
			_m.end -= (dLast - dest);
			OEL_RETHROW;
		}
	}
	return _detail::MakeDynarrIter(_m, pPos);
}


template< typename T, typename Alloc >
void dynarray<T, Alloc>::_growByOne()
{
	_realloc(_calcCapAddOne(), size());
}

template< typename T, typename Alloc >
#if defined _MSC_VER and !__has_cpp_attribute(unlikely)
	__declspec(noinline) // to get the compiler to inline calling function
#endif
void dynarray<T, Alloc>::_growBy(size_type const count)
{
	auto const s = size();
	_realloc(_calcCapAdd(count, s), s);
}

template< typename T, typename Alloc >
template< typename... Args >
inline T & dynarray<T, Alloc>::emplace_back(Args &&... args) &
{
	if (_m.end == _m.reservEnd)
		_growByOne();

	_alloTrait::construct(_m, _m.end, static_cast<Args &&>(args)...);

	_debugSizeUpdater guard{_m};

	return *(_m.end++);
}

template< typename T, typename Alloc >
inline void dynarray<T, Alloc>::append(size_type count, const T & val)
{
	if (_spareCapacity() < count) OEL_UNLIKELY
		_growBy(count);

	auto const pos = _m.end;
	_uninitFill::template call< forward_t<const T &> >(pos, pos + count, _m, val);

	_m.end += count;
	(void) _debugSizeUpdater{_m};
}

template< typename T, typename Alloc >
template< typename InputRange >
inline auto dynarray<T, Alloc>::append(InputRange && source)
->	borrowed_iterator_t<InputRange>
{
	if constexpr (_detail::rangeIsForwardOrSized<InputRange>)
	{
		return _doAppend(adl_begin(source), _detail::UDist(source));
	}
	else
	{	auto it = adl_begin(source);
		auto l  = adl_end(source);
		for (; it != l; ++it)
			emplace_back(*it);

		return it;
	}
}

template< typename T, typename Alloc >
template< typename InputRange >
inline auto dynarray<T, Alloc>::assign(InputRange && source)
->	borrowed_iterator_t<InputRange>
{
	if constexpr (_detail::rangeIsForwardOrSized<InputRange>)
	{
		return _doAssign(adl_begin(source), _detail::UDist(source));
	}
	else
	{	clear();
		return append(source);
	}
}


template< typename T, typename Alloc >
dynarray<T, Alloc>::dynarray(size_type n, for_overwrite_t, Alloc a)
 :	_m(a)
{
	_initReserve(n);
	_detail::DefaultInit<allocator_type>::call(_m.data, _m.reservEnd, _m);

	_m.end = _m.reservEnd;
	(void) _debugSizeUpdater{_m};
}

template< typename T, typename Alloc >
dynarray<T, Alloc>::dynarray(size_type n, Alloc a)
 :	_m(a)
{
	_initReserve(n);
	_uninitFill::call(_m.data, _m.reservEnd, _m);

	_m.end = _m.reservEnd;
	(void) _debugSizeUpdater{_m};
}

template< typename T, typename Alloc >
dynarray<T, Alloc>::dynarray(dynarray && other, Alloc a)
 :	_m(a) // moves from a
{
	if constexpr (!_alloTrait::is_always_equal::value)
	{
		allocator_type & myA = _m;
		if (myA != other._m)
		{
			append(other | view::move);
			return;
		}
	}
	_moveInternBase(other._m);
}

template< typename T, typename Alloc >
dynarray<T, Alloc> &
	dynarray<T, Alloc>::operator =(dynarray && other) &
	noexcept(_alloTrait::propagate_on_container_move_assignment::value or _alloTrait::is_always_equal::value)
{
	[[maybe_unused]] allocator_type & myA = _m;
	if constexpr( !(_alloTrait::propagate_on_container_move_assignment::value or _alloTrait::is_always_equal::value) )
	    if (myA != other._m)
		{
			assign(other | view::move);
			return *this;
		}

	// Take allocated memory from other
	if (_m.data)
	{
		_detail::Destroy(_m.data, _m.end);
		_allocateWrap::dealloc(_m, _m.data, capacity());
	}
	_moveInternBase(other._m);
	if constexpr (_alloTrait::propagate_on_container_move_assignment::value)
		myA = static_cast<allocator_type &&>(other._m);

	return *this;
}

template< typename T, typename Alloc >
dynarray<T, Alloc> &
	dynarray<T, Alloc>::operator =(const dynarray & other) &
{
	static_assert(!_alloTrait::propagate_on_container_copy_assignment::value or _alloTrait::is_always_equal::value,
	              "Alloc propagate_on_container_copy_assignment unsupported");
	if (this != &other) // avoid memcpy data to itself
		assign(other);

	return *this;
}


template< typename T, typename Alloc >
void dynarray<T, Alloc>::shrink_to_fit()
{
	auto const used = size();
	if (0 < used)
	{
		_realloc(used, used);
	}
	else
	{	_m.end = nullptr;
		_resetData(nullptr, 0);
	}
}

template< typename T, typename Alloc >
void dynarray<T, Alloc>::erase_to_end(iterator first) noexcept
{
	T *const newEnd{to_pointer_contiguous(first)};
	OEL_ASSERT(_m.data <= newEnd and newEnd <= _m.end);

	_detail::Destroy(newEnd, _m.end);
	_m.end = newEnd;

	(void) _debugSizeUpdater{_m};
}

template< typename T, typename Alloc >
inline typename dynarray<T, Alloc>::iterator
	dynarray<T, Alloc>::unordered_erase(iterator pos) &
{
	if constexpr (is_trivially_relocatable<T>::value)
	{
		T & elem = *pos;
		elem.~T();

		--_m.end;
		_debugSizeUpdater guard{_m};

		auto & mem = reinterpret_cast< _detail::RelocateWrap<T> & >(elem);
		mem       = *reinterpret_cast< _detail::RelocateWrap<T> * >(_m.end); // relocate last element to pos
	}
	else
	{	*pos = std::move(back());
		pop_back();
	}
	return pos;
}

template< typename T, typename Alloc >
typename dynarray<T, Alloc>::iterator
	dynarray<T, Alloc>::erase(iterator pos) &
{
	_debugSizeUpdater guard{_m};

	T *const ptr{to_pointer_contiguous(pos)};
	OEL_ASSERT(_m.data <= ptr and ptr < _m.end);
	if constexpr (is_trivially_relocatable<T>::value)
	{
		ptr-> ~T();
		auto const next = ptr + 1;
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
typename dynarray<T, Alloc>::iterator
	dynarray<T, Alloc>::erase(iterator first, const_iterator last) &
{
	_debugSizeUpdater guard{_m};

	T *            dest{to_pointer_contiguous(first)};
	const T *const pLast{to_pointer_contiguous(last)};
	OEL_ASSERT(_m.data <= dest and dest <= pLast and pLast <= _m.end);

	if constexpr (is_trivially_relocatable<T>::value)
	{
		_detail::Destroy(dest, pLast);
		auto const nAfter = _m.end - pLast;
		std::memmove( // relocate [last, end) to [first, first + nAfter)
			static_cast<void *>(dest),
			static_cast<const void *>(pLast),
			sizeof(T) * nAfter );
		_m.end = dest + nAfter;
	}
	else if (dest < pLast) // must avoid self-move-assigning the elements
	{
		dest = std::move(const_cast<T *>(pLast), _m.end, dest);
		_detail::Destroy(dest, _m.end);
		_m.end = dest;
	}
	return first;
}


template< typename InputRange, typename Alloc = allocator<> >
dynarray(from_range_t, InputRange &&, Alloc = {})
->	dynarray<
		iter_value_t< iterator_t<InputRange> >,
		Alloc
	>;

#if defined __GNUC__ and __GNUC__ < 12
	template< typename T, typename A >
	explicit dynarray(const dynarray<T, A> &) -> dynarray<T, A>;
#endif

#if OEL_MEM_BOUND_DEBUG_LVL
} // namespace debug
#endif

} // oel
