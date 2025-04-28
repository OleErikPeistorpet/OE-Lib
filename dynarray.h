#pragma once

// Copyright 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "allocator.h"
#include "auxi/dynarray_iterator.h"
#include "auxi/impl_algo.h"
#include "optimize_ext/default.h"
#include "view/move.h"

#include <algorithm>

/** @file
*/

namespace oel
{

using std::type_identity_t;


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

	constexpr dynarray() noexcept(noexcept( Alloc{} ))  : dynarray(Alloc{}) {}
	constexpr explicit dynarray(Alloc a) noexcept       : _m(a) {}

	//! Construct empty dynarray with space reserved for exactly capacity elements
	dynarray(reserve_tag, size_type capacity, Alloc a = Alloc{})   : _m(a) { _initReserve(capacity); }

	//! Default-initializes elements, can be significantly faster if T is scalar or has trivial default constructor
	/**
	* @copydetails resize_for_overwrite(size_type)  */
	dynarray(size_type size, for_overwrite_t, Alloc a = Alloc{});
	//! (Value-initializes elements, same as std::vector)
	explicit dynarray(size_type size, Alloc a = Alloc{});

	template< typename InputRange >
	dynarray(from_range_t, InputRange && r, Alloc a = Alloc{})   : _m(a) { append_range(r); }

	dynarray(std::initializer_list<T> il, Alloc a = Alloc{})     : _m(a) { append_range(il); }

	dynarray(dynarray && other) noexcept                         : _m(std::move(other._m)) {}
	dynarray(dynarray && other, type_identity_t<Alloc> a);
	explicit dynarray(const dynarray & other)
		:	dynarray( other, _alloTrait::select_on_container_copy_construction(other._m.allo) ) {}

	explicit dynarray(const dynarray & other, type_identity_t<Alloc> a)   : _m(a) { append_range(other); }

	~dynarray() = default;

	dynarray & operator =(dynarray && other) &
		noexcept( _alloTrait::propagate_on_container_move_assignment::value or _alloTrait::is_always_equal::value );
	//! Requires that allocator_type is always equal or does not have propagate_on_container_copy_assignment
	dynarray & operator =(const dynarray & other) &;
	dynarray & operator =(const dynarray &&) = delete;

	dynarray & operator =(std::initializer_list<T> il) &  { assign_range(il);  return *this; }

	OEL_ALWAYS_INLINE
	friend void swap(dynarray & a, dynarray & b) noexcept   { a._m.swap(b._m); }

	template< typename InputRange >
	void assign_range(InputRange && source);

	//! Almost same as std::vector::append_range (C++23)
	/** @pre `source` shall not refer to any elements in this dynarray if reallocation happens.
	*	Reallocation is caused by `capacity() - size() < n`, where `n` is number of source elements
	*
	* If an exception is thrown, the dynarray will keep all elements already appended during the operation. */
	template< typename InputRange = std::initializer_list<T> >
	void append_range(InputRange && source);

	//! Default-initializes added elements, can be significantly faster if T is scalar or trivially constructible
	/**
	* Objects of scalar type get indeterminate values. http://en.cppreference.com/w/cpp/language/default_initialization  */
	void resize_for_overwrite(size_type n)   { _doResize<_detail::DefaultInit>(n); }
	void resize(size_type n)                 { _doResize<_detail::ValueInit>(n); }

	//! Almost same as std::vector::insert_range
	/**
	* Requires that source models std::ranges::forward_range or that `source.size()` is valid,
	* in addition to that T is trivially relocatable. */
	template< typename Range >
	iterator insert_range(const_iterator pos, Range && source) &;

	iterator insert(const_iterator pos, T && val) &       { return emplace(pos, std::move(val)); }
	iterator insert(const_iterator pos, const T & val) &  { return emplace(pos, val); }

	template< typename... Args >
	iterator emplace(const_iterator pos, Args &&... args) &;

	//! Beware, passing an element of same dynarray is often unsafe (otherwise same as std::vector::emplace_back)
	/** @pre `args` shall not refer to any element of this container, unless `size() < capacity()` */
	template< typename... Args >
	T &  emplace_back(Args &&... args) &;

	//! Beware, passing an element of same dynarray is often unsafe (otherwise same as std::vector::push_back)
	/** @pre `val` shall not be a reference to an element of this container, unless `size() < capacity()` */
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

	//! Erase the element at pos without maintaining order of elements after pos
	/**
	* The iterator pos remains valid, same as if it was returned by `erase`.
	* Constant complexity (compared to linear in the distance between pos and `end()` for normal erase). */
	void     unordered_erase(iterator pos);

	iterator erase(const_iterator pos) &;

	iterator erase(const_iterator first, const_iterator last) &;
	//! Equivalent to `erase(first, end())`, but potentially faster and does not require assignable T
	void     erase_to_end(const_iterator first) noexcept;

	void     clear() noexcept   { erase_to_end(begin()); }

	void     reserve(size_type minCap)
		{
			if( capacity() < minCap )
				_realloc(_calcCapChecked(minCap), size());
		}
	//! It's probably a good idea to check that size < capacity before calling, maybe add some treshold to size
	void      shrink_to_fit();

	[[nodiscard]] bool empty() const noexcept  { return _m.data == _m.end; }

	size_type size() const noexcept            { return static_cast<size_t>(_m.end - _m.data); }

	size_type capacity() const noexcept        { return static_cast<size_t>(_m.reservEnd - _m.data); }

	constexpr size_type max_size() const noexcept   { return _alloTrait::max_size(_m.allo) - _allocateWrap::sizeForHeader; }

	//! How much smaller capacity is than the number passed to allocator_type::allocate
	static constexpr size_type allocate_size_overhead() noexcept   { return _allocateWrap::sizeForHeader; }

	allocator_type get_allocator() const noexcept   { return _m.allo; }

	iterator       begin() noexcept          { return _detail::MakeDynarrIter           (_m, _m.data); }
	const_iterator begin() const noexcept    { return _detail::MakeDynarrIter<const T *>(_m, _m.data); }
	const_iterator cbegin() const noexcept   { return begin(); }

	iterator       end() noexcept          { return _detail::MakeDynarrIter           (_m, _m.end); }
	const_iterator end() const noexcept    { return _detail::MakeDynarrIter<const T *>(_m, _m.end); }
	OEL_ALWAYS_INLINE
	const_iterator cend() const noexcept   { return end(); }

	auto      rbegin() noexcept         { return std::reverse_iterator{end()}; }
	auto      rbegin() const noexcept   { return std::reverse_iterator{end()}; }
	auto      crbegin() const noexcept  { return std::reverse_iterator{end()}; }

	auto      rend() noexcept         { return std::reverse_iterator{begin()}; }
	auto      rend() const noexcept   { return std::reverse_iterator{begin()}; }
	auto      crend() const noexcept  { return std::reverse_iterator{begin()}; }

	T *       data() noexcept         { return _m.data; }
	const T * data() const noexcept   { return _m.data; }

	T &       front() noexcept        { return (*this)[0]; }
	const T & front() const noexcept  { return (*this)[0]; }

	OEL_ALWAYS_INLINE
	T &       back() noexcept         { return end()[-1]; }
	OEL_ALWAYS_INLINE
	const T & back() const noexcept   { return end()[-1]; }

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
			if( index < size() ) // would be unsafe with signed size_type
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

	struct _dataOwner
	{
		T * data{};
		T * end{};
		T * reservEnd{};
		OEL_NO_UNIQUE_ADDRESS allocator_type allo;

		constexpr _dataOwner(Alloc & a) noexcept
		 :	allo(std::move(a)) {}

		constexpr _dataOwner(_dataOwner && other) noexcept
		 :	allo(std::move(other.allo))
		{
			moveDataFrom(other);
		}

		~_dataOwner()
		{
			destroyAndDealloc();
		}

		void moveDataFrom(_dataOwner & other) noexcept
		{
			data      = other.data;
			end       = other.end;
			reservEnd = other.reservEnd;
			other.reservEnd = other.end = other.data = nullptr;
		}

		void swap(_dataOwner & other) noexcept
		{
			using std::swap;

			swap(data, other.data);
			swap(end, other.end);
			swap(reservEnd, other.reservEnd);

			if constexpr( _alloTrait::propagate_on_container_swap::value )
				swap(allo, other.allo);
			else // Standard says this is undefined if allocators compare unequal
				OEL_ASSERT(allo == other.allo);
		}

		void destroyAndDealloc()
			noexcept(noexcept( _allocateWrap::dealloc(allo, data, size_t{}) ))
		{
			if( data )
			{
				_detail::Destroy(data, end);

				auto cap = static_cast<size_t>(reservEnd - data);
				_allocateWrap::dealloc(allo, data, cap);
			}
		}
	}
	_m; // exception safety helper, the only non-static data member

	using _debugSizeUpdater = _detail::DebugSizeInHeaderUpdater<_dataOwner>;

	void _resetData(T *const newData, size_type const newCap)
	{
		if( _m.data )
			_allocateWrap::dealloc(_m.allo, _m.data, capacity());

		// Beware, sets _m.data with no _debugSizeUpdater
		_m.data      = newData;
		_m.reservEnd = newData + newCap;
	}

	void _initReserve(size_type const capToCheck)
	{
		_m.end = _m.data = _allocateChecked(capToCheck);
		_m.reservEnd = _m.data + capToCheck;
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
		if( newSize <= max_size() )
			return _calcCapUnchecked(newSize);
		else
			_detail::LengthError::raise();
	}

	size_type _calcCapAdd(size_type const nAdd, size_type const oldSize) const
	{
		if( nAdd <= SIZE_MAX / 2 / sizeof(T) ) // assumes that allocating greater than SIZE_MAX / 2 always fails
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
		if( n <= max_size() )
			return _allocateWrap::allocate(_m.allo, n);
		else
			_detail::LengthError::raise();
	}


	void _realloc(size_type const newCap, size_type const oldSize)
	{
		if constexpr( oel::allocator_can_realloc<allocator_type>() )
		{
			auto const p = _allocateWrap::realloc(_m.allo, _m.data, newCap);
			_m.data = p;
			_m.end = p + oldSize;
			_m.reservEnd = p + newCap;
		}
		else
		{	auto const newData = _allocateWrap::allocate(_m.allo, newCap);
			_m.end = _detail::Relocate(_m.data, oldSize, newData);
			_resetData(newData, newCap);
		}
		(void) _debugSizeUpdater{_m};
	}

	void _growByOne()
	{
		_realloc(_calcCapAddOne(), size());
	}
	// Not defined inline as a compiler hint
	void _growBy(size_type);


	template< typename UninitFiller >
	void _doResize(size_type const newSize)
	{
		reserve(newSize);

		T *const newEnd = _m.data + newSize;
		if( _m.end < newEnd )
			UninitFiller::call(_m.end, newEnd, _m.allo);
		else
			_detail::Destroy(newEnd, _m.end);

		_debugSizeUpdater guard{_m};
		_m.end = newEnd;
	}


	template< typename InputIter >
	void _doAssign(InputIter src, size_type const count)
	{
		_debugSizeUpdater guard{_m};

		if constexpr( can_memmove_with<T *, InputIter> )
		{
			if( capacity() < count )
			{	// Deallocating first might be better,
				// but then the _m pointers would have to be nulled in case allocate throws
				_resetData(_allocateChecked(count), count);
				_m.end = _m.reservEnd;
			}
			else
			{	_m.end = _m.data + count;
			}

			_detail::MemcpyCheck(src, count, _m.data);
		}
		else
		{	auto cpy = [](InputIter src_, T *__restrict dest, T * dLast)
			{
				while( dest != dLast )
				{
					*dest = *src_;
					++dest; ++src_;
				}
				return src_;
			};

			T * newEnd;
			if( capacity() < count )
			{
				auto const newData = _allocateChecked(count);
				_detail::Destroy(_m.data, _m.end);
				_resetData(newData, count);
				_m.end = newData;
				newEnd = _m.reservEnd;
			}
			else
			{	newEnd = _m.data + count;
				if( newEnd <= _m.end )
				{	// enough elements, assign new and destroy rest
					cpy(std::move(src), _m.data, newEnd);
					erase_to_end(_detail::MakeDynarrIter(_m, newEnd));
					return;
				}
				else // upsizing, assign to old elements as far as we can
				{	src = cpy(std::move(src), _m.data, _m.end);
				}
			}
			do	// each iteration updates _m.end for exception safety
			{	_alloTrait::construct(_m.allo, _m.end, *src);
				++_m.end; ++src;
			}
			while( _m.end != newEnd );
		}
	}

	template< typename InputIter >
	void _doAppend(InputIter src, size_type const count)
	{
		if( _spareCapacity() < count )
			_growBy(count);

		_debugSizeUpdater guard{_m};
		if constexpr( can_memmove_with<T *, InputIter> )
		{
			_detail::MemcpyCheck(src, count, _m.end);
			_m.end += count;
		}
		else
		{	auto const newEnd = _m.end + count;
			while( _m.end != newEnd )
			{
				_alloTrait::construct(_m.allo, _m.end, *src);
				++_m.end; ++src;
			}
		}
	}


	T * _insertReallocImpl(size_type const newCap, T *const pos, size_type const count)
	{
		auto const newData = _allocateWrap::allocate(_m.allo, newCap);
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
				if( destroy )
					destroy-> ~T();
			}
		} exit{&destroyOnFail};

		pos = _insertReallocImpl(_calcCapAddOne(), pos, 1);
		exit.destroy = nullptr;
		return pos;
	}
};

template< typename T, typename Alloc >
template< typename... Args >
typename dynarray<T, Alloc>::iterator
	dynarray<T, Alloc>::emplace(const_iterator pos, Args &&... args) &
{
#define OEL_DYNARR_INSERT_STEP1  \
	static_assert( is_trivially_relocatable<T>::value,  \
		"insert, emplace require trivially relocatable T, see declaration of is_trivially_relocatable" );  \
	\
	_debugSizeUpdater guard{_m};  \
	\
	auto pPos = const_cast<T *>(to_pointer_contiguous(pos));  \
	OEL_ASSERT(_m.data <= pPos and pPos <= _m.end);

	OEL_DYNARR_INSERT_STEP1

	// Temporary in case constructor throws or args refer to an element of this dynarray
	alignas(T) unsigned char tmp[sizeof(T)];
	_alloTrait::construct(_m.allo, reinterpret_cast<T *>(&tmp), static_cast<Args &&>(args)...);
	if( _m.end < _m.reservEnd )
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

template< typename T, typename Alloc >
template< typename Range >
typename dynarray<T, Alloc>::iterator
	dynarray<T, Alloc>::insert_range(const_iterator pos, Range && source) &
{
	OEL_DYNARR_INSERT_STEP1
#undef OEL_DYNARR_INSERT_STEP1

	static_assert( _detail::rangeIsForwardOrSized<Range>,
		"insert_range requires that source models std::ranges::forward_range or that source.size() is valid" );

	auto       first = iter_uncounted(ranges::begin(source));
	auto const count = as_unsigned(ranges::distance(source));

	size_t const bytesAfterPos{sizeof(T) * (_m.end - pPos)};
	T * dLast;
	if( _spareCapacity() >= count )
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
	if constexpr( can_memmove_with< T *, decltype(first) > )
	{
		_detail::MemcpyCheck(first, count, pPos);
	}
	else
	{	T *__restrict dest = pPos;
		OEL_TRY_
		{
			while( dest != dLast )
			{
				_alloTrait::construct(_m.allo, dest, *first);
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
#if defined _MSC_VER and _MSC_VER < 1930
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
	if( _m.end == _m.reservEnd ) [[unlikely]]
		_growByOne();

	_alloTrait::construct(_m.allo, _m.end, static_cast<Args &&>(args)...);

	_debugSizeUpdater guard{_m};

	return *(_m.end++);
}

template< typename T, typename Alloc >
template< typename InputRange >
inline void dynarray<T, Alloc>::append_range(InputRange && source)
{
	if constexpr( _detail::rangeIsForwardOrSized<InputRange> )
	{
		_doAppend(
			iter_uncounted(ranges::begin(source)),
			as_unsigned(ranges::distance(source)) );
	}
	else
	{	for( auto && e : source )
			emplace_back( decltype(e)(e) );
	}
}

template< typename T, typename Alloc >
template< typename InputRange >
inline void dynarray<T, Alloc>::assign_range(InputRange && source)
{
	if constexpr( _detail::rangeIsForwardOrSized<InputRange> )
	{
		_doAssign(
			iter_uncounted(ranges::begin(source)),
			as_unsigned(ranges::distance(source)) );
	}
	else
	{	clear();
		append_range(source);
	}
}


template< typename T, typename Alloc >
dynarray<T, Alloc>::dynarray(size_type n, for_overwrite_t, Alloc a)
 :	_m(a)
{
	_initReserve(n);
	_detail::DefaultInit::call(_m.data, _m.reservEnd, _m.allo);

	_m.end = _m.reservEnd;
	(void) _debugSizeUpdater{_m};
}

template< typename T, typename Alloc >
dynarray<T, Alloc>::dynarray(size_type n, Alloc a)
 :	_m(a)
{
	_initReserve(n);
	_detail::ValueInit::call(_m.data, _m.reservEnd, _m.allo);

	_m.end = _m.reservEnd;
	(void) _debugSizeUpdater{_m};
}

template< typename T, typename Alloc >
dynarray<T, Alloc>::dynarray(dynarray && other, type_identity_t<Alloc> a)
 :	_m(a) // moves from a
{
	if constexpr( !_alloTrait::is_always_equal::value )
		if( _m.allo != other._m.allo )
		{
			append_range(other | view::move);
			return;
		}

	_m.moveDataFrom(other._m);
}

template< typename T, typename Alloc >
dynarray<T, Alloc> &
	dynarray<T, Alloc>::operator =(dynarray && other) &
	noexcept( _alloTrait::propagate_on_container_move_assignment::value or _alloTrait::is_always_equal::value )
{
	if constexpr( !(_alloTrait::propagate_on_container_move_assignment::value or _alloTrait::is_always_equal::value) )
	    if( _m.allo != other._m.allo )
		{
			assign_range(other | view::move);
			return *this;
		}

	// Take allocated memory from other
	_m.destroyAndDealloc();
	_m.moveDataFrom(other._m);
	if constexpr( _alloTrait::propagate_on_container_move_assignment::value )
		_m.allo = std::move(other._m.allo);

	return *this;
}

template< typename T, typename Alloc >
dynarray<T, Alloc> &
	dynarray<T, Alloc>::operator =(const dynarray & other) &
{
	static_assert(!_alloTrait::propagate_on_container_copy_assignment::value or _alloTrait::is_always_equal::value,
	              "Alloc propagate_on_container_copy_assignment unsupported");
	if( this != &other ) // avoid memcpy data to itself
		assign_range(other);

	return *this;
}


template< typename T, typename Alloc >
void dynarray<T, Alloc>::shrink_to_fit()
{
	auto const used = size();
	if( 0 < used )
	{
		_realloc(used, used);
	}
	else
	{	_m.end = nullptr;
		_resetData(nullptr, 0);
	}
}

template< typename T, typename Alloc >
void dynarray<T, Alloc>::erase_to_end(const_iterator first) noexcept
{
	auto const newEnd = const_cast<T *>(to_pointer_contiguous(first));
	OEL_ASSERT(_m.data <= newEnd and newEnd <= _m.end);

	_detail::Destroy(newEnd, _m.end);
	_m.end = newEnd;

	(void) _debugSizeUpdater{_m};
}

template< typename T, typename Alloc >
inline void dynarray<T, Alloc>::unordered_erase(iterator pos)
{
	if constexpr( is_trivially_relocatable<T>::value )
	{
		T & elem = *pos;
		elem.~T();

		--_m.end;
		_debugSizeUpdater guard{_m};
#if defined __GNUC__ and !defined __clang__
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
		auto mem = reinterpret_cast< _detail::RelocateWrap<T> * >(&elem);
		*mem    = *reinterpret_cast< _detail::RelocateWrap<T> * >(_m.end); // relocate last element to pos
#if defined __GNUC__ and !defined __clang__
	#pragma GCC diagnostic pop
#endif
	}
	else
	{	*pos = std::move(back());
		pop_back();
	}
}

template< typename T, typename Alloc >
typename dynarray<T, Alloc>::iterator
	dynarray<T, Alloc>::erase(const_iterator pos) &
{
	_debugSizeUpdater guard{_m};

	auto const ptr = const_cast<T *>(to_pointer_contiguous(pos));
	OEL_ASSERT(_m.data <= ptr and ptr < _m.end);
	if constexpr( is_trivially_relocatable<T>::value )
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
	return _detail::MakeDynarrIter(_m, ptr);
}

template< typename T, typename Alloc >
typename dynarray<T, Alloc>::iterator
	dynarray<T, Alloc>::erase(const_iterator first, const_iterator last) &
{
	_debugSizeUpdater guard{_m};

	auto const pFirst = const_cast<T *>(to_pointer_contiguous(first));
	const T *const pLast{to_pointer_contiguous(last)};
	OEL_ASSERT(_m.data <= pFirst and pFirst <= pLast and pLast <= _m.end);

	if constexpr( is_trivially_relocatable<T>::value )
	{
		_detail::Destroy(pFirst, pLast);
		auto const nAfter = _m.end - pLast;
		std::memmove( // relocate [last, end) to [first, first + nAfter)
			static_cast<void *>(pFirst),
			static_cast<const void *>(pLast),
			sizeof(T) * nAfter );
		_m.end = pFirst + nAfter;
	}
	else if( pFirst < pLast ) // must avoid self-move-assigning the elements
	{
		auto const dest = std::move(const_cast<T *>(pLast), _m.end, pFirst);
		_detail::Destroy(dest, _m.end);
		_m.end = dest;
	}
	return _detail::MakeDynarrIter(_m, pFirst);
}


template< typename InputRange, typename Alloc = allocator<> >
dynarray(from_range_t, InputRange &&, Alloc = {})
->	dynarray< ranges::range_value_t<InputRange>, Alloc >;

#if defined __GNUC__ and __GNUC__ < 12
	template< typename T, typename A >
	explicit dynarray(const dynarray<T, A> &) -> dynarray<T, A>;
#endif

#if OEL_MEM_BOUND_DEBUG_LVL
} // namespace debug
#endif

} // oel
