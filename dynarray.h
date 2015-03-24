#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "contiguous_iterator.h"
#include "container_shared.h"

#ifndef OEL_NO_BOOST
	#include <boost/iterator/iterator_categories.hpp>
#endif
#include <stdexcept>
#include <algorithm>


namespace oel
{

#ifndef OEL_NO_BOOST
	template<typename Iterator>
	using iterator_traversal_t = typename boost::iterator_traversal<Iterator>::type;

	using boost::forward_traversal_tag;
	using boost::single_pass_traversal_tag;
#else
	template<typename Iterator>
	using iterator_traversal_t = typename std::iterator_traits<Iterator>::iterator_category;

	using forward_traversal_tag = std::forward_iterator_tag;
	using single_pass_traversal_tag = std::input_iterator_tag;
#endif


using std::out_of_range;


/// Type to indicate that a container constructor must allocate storage. A const instance named reserve is provided to pass
struct reserve_tag {};
reserve_tag const reserve;

////////////////////////////////////////////////////////////////////////////////

template<typename, typename> class dynarray;  // forward declare

/// dynarray<dynarray<_>> is all right
template<typename T, typename A>
struct is_trivially_relocatable< dynarray<T, A> > : std::true_type {};

template<typename T, typename A> inline
void swap(dynarray<T, A> & a, dynarray<T, A> & b) NOEXCEPT  { a.swap(b); }

/**
* @brief Erase the element at position from dynarray without maintaining order of elements.
*
* Constant complexity (compared to linear in the distance between position and last for normal erase).
* @return Iterator pointing to the location that followed the element erased,
*	which is the end if position was at the last element. */
template<typename T, typename A>
typename dynarray<T, A>::iterator  erase_unordered(dynarray<T, A> & ctr, typename dynarray<T, A>::iterator position);

template<typename T, typename A>
bool operator==(const dynarray<T, A> & left, const dynarray<T, A> & right)
{
	return left.size() == right.size() &&
		   std::equal(left.begin(), left.end(), right.begin());
}
template<typename T, typename A> inline
bool operator!=(const dynarray<T, A> & left, const dynarray<T, A> & right)  { return !(left == right); }

/**
* @brief Resizable array, dynamically allocated. Very similar to std::vector, but much faster in many cases.
*
* Relocating objects of template argument T must be equivalent to memcpy without destructor call (true for most types).
* This is checked when compiling with is_trivially_relocatable, a trait which must be specialized manually for each
* type that is not trivially copyable.
*
* The default allocator supports over-aligned types (e.g. __m256)  */
template<typename T, typename Alloc = allocator<T> >
class dynarray
{
public:
	using value_type      = T;
	using pointer         = T *;
	using const_pointer   = const T *;
	using reference       = T &;
	using const_reference = const T &;

	using iterator       = contiguous_iterator< T, dynarray<T, Alloc> >;
	using const_iterator = contiguous_iterator< T const, dynarray<T, Alloc> >;
#if OEL_MEM_BOUND_DEBUG_LVL >= 2
	#define OEL_DYNARR_ITERATOR(ptr)        iterator{ptr, this}
	#define OEL_DYNARR_CONST_ITER(constPtr) const_iterator{constPtr, this}
#else
	#define OEL_DYNARR_ITERATOR(ptr)        (ptr)
	#define OEL_DYNARR_CONST_ITER(constPtr) (constPtr)
#endif
	using reverse_iterator       = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

	using difference_type = typename std::iterator_traits<iterator>::difference_type;
	using size_type       = typename Alloc::size_type;

	dynarray() NOEXCEPT  : _data(nullptr), _end(nullptr), _reserveEnd(nullptr) {}

	/** @brief Construct empty dynarray with space reserved for at least capacity elements
	* @throw std::bad_alloc if the allocation request does not succeed (same for all operations that add capacity)  */
	dynarray(reserve_tag, size_type capacity);

	/// Elements are default initialized, meaning non-class T produces indeterminate values
	dynarray(size_type size, default_init_tag);
	dynarray(size_type size, const T & fillVal);

	dynarray(std::initializer_list<T> init);

	dynarray(dynarray && other) NOEXCEPT;
	dynarray(const dynarray & other);

	~dynarray() NOEXCEPT;

	dynarray & operator =(dynarray && other) NOEXCEPT   { swap(other);  return *this; }
	dynarray & operator =(const dynarray & other)       { assign(other);  return *this; }

	dynarray & operator =(std::initializer_list<T> il)  { assign(il);  return *this; }

	void       swap(dynarray & other) NOEXCEPT;

	/**
	* @brief Replace the contents with count copies from range beginning at first
	* @param first iterator to first element to assign from.
	*	Shall not point into same dynarray, except if it is the begin iterator.
	* @param count number of elements to assign
	* @return end of source range (first incremented by count)
	*
	* Any elements held before the call are either assigned to or destroyed.
	* Performance note: This function is efficient only for random-access traversal iterator  */
	template<typename ForwardTravIterator>
	ForwardTravIterator assign(ForwardTravIterator first, size_type count);
	/**
	* @brief Replace the contents with range
	* @param source object which begin and end can be called on (an array, STL container or iterator_range).
	*	Shall not be a subset of same dynarray, except if begin(source) points to first element of dynarray.
	*
	* Any elements held before the call are either assigned to or destroyed. */
	template<typename InputRange>
	void                assign(const InputRange & source);

	/**
	* @brief Add count elements at end from range beginning at first (in same order)
	* @param first iterator to first element to append
	* @param count number of elements to append
	* @return first incremented count times. The value is undefined and shall be ignored if
	*	first pointed into same dynarray and there was insufficient capacity to avoid reallocation.
	*
	* Causes reallocation if the pre-call size + count is greater than capacity. On reallocation, all iterators
	* and references are invalidated. Otherwise, any previous end iterator will point to the first element added.
	* Strong exception safety, aka. commit or rollback semantics. */
	template<typename InputIterator>
	InputIterator append(InputIterator first, size_type count);
	/**
	* @brief Add at end the elements from range (in same order)
	* @param source object which begin and end can be called on (an array, STL container or iterator_range)
	* @return iterator pointing to first of the new elements in dynarray, or end if range is empty
	*
	* Otherwise same as append(InputIterator, size_type)  */
	template<typename InputRange>
	iterator      append(const InputRange & source);
	/// Equivalent to calling append(const InputRange &) with il as argument  */
	iterator      append(std::initializer_list<T> il);

	template<typename... ArgTs>
	void       emplace_back(ArgTs &&... args);

	void       push_back(T && val)       { emplace_back(std::move(val)); }
	void       push_back(const T & val)  { emplace_back(val); }

	template<typename... ArgTs>
	iterator   emplace(const_iterator position, ArgTs &&... args);

	iterator   insert(const_iterator position, T && val)       { return emplace(position, std::move(val)); }
	iterator   insert(const_iterator position, const T & val)  { return emplace(position, val); }

	/// After the call, any previous iterator to the back element will be equal to end()
	void       pop_back() NOEXCEPT;

	iterator   erase(iterator position) NOEXCEPT;

	iterator   erase(iterator first, iterator last) NOEXCEPT;

	/// Equivalent to erase(first, end()) (but potentially faster)
	void       erase_back(iterator first) NOEXCEPT;

	/// Added elements are default initialized, meaning non-class T produces indeterminate values
	void       resize(size_type newSize, default_init_tag);
	void       resize(size_type newSize, const T & addVal);

	void       clear() NOEXCEPT        { erase_back(begin()); }

	bool       empty() const NOEXCEPT  { return _data.get() == _end; }

	size_type  size() const NOEXCEPT   { return _end - _data.get(); }

	void       reserve(size_type minCapacity);

	/// It's a good idea to check that size() < capacity() before calling to avoid useless reallocation
	void       shrink_to_fit();

	size_type  capacity() const NOEXCEPT  { return _reserveEnd - data(); }

	iterator        begin() NOEXCEPT         { return OEL_DYNARR_ITERATOR(_data.get()); }
	const_iterator  begin() const NOEXCEPT   { return OEL_DYNARR_CONST_ITER(_data.get()); }
	const_iterator  cbegin() const NOEXCEPT  { return begin(); }

	iterator        end() NOEXCEPT           { return OEL_DYNARR_ITERATOR(_end); }
	const_iterator  end() const NOEXCEPT     { return OEL_DYNARR_CONST_ITER(_end); }
	const_iterator  cend() const NOEXCEPT    { return end(); }

	reverse_iterator       rbegin() NOEXCEPT        { return reverse_iterator{end()}; }
	const_reverse_iterator rbegin() const NOEXCEPT  { return const_reverse_iterator{end()}; }

	reverse_iterator       rend() NOEXCEPT          { return reverse_iterator{begin()}; }
	const_reverse_iterator rend() const NOEXCEPT    { return const_reverse_iterator{begin()}; }

	pointer         data() NOEXCEPT        { return _data.get(); }
	const_pointer   data() const NOEXCEPT  { return _data.get(); }

	reference       front() NOEXCEPT        { return *begin(); }
	const_reference front() const NOEXCEPT  { return *begin(); }

	reference       back() NOEXCEPT         { return *OEL_DYNARR_ITERATOR(_end - 1); }
	const_reference back() const NOEXCEPT   { return *OEL_DYNARR_CONST_ITER(_end - 1); }

	reference       at(size_type index);
	const_reference at(size_type index) const;

	reference       operator[](size_type index) NOEXCEPT;
	const_reference operator[](size_type index) const NOEXCEPT;



////////////////////////////////////////////////////////////////////////////////
//
// Implementation only in rest of the file


private:
	static pointer _alloc(size_type count)
	{
		return Alloc{}.allocate(count);
	}

	struct _dealloc
	{	void operator()(pointer ptr)
		{
			Alloc{}.deallocate(ptr);
		}
	};

	using _smartPtr = std::unique_ptr<T[], _dealloc>;

	_smartPtr _data;       // Smart pointer to beginning of data buffer
	pointer   _end;        // Pointer to one past the last object (back)
	pointer   _reserveEnd; // Pointer to end of allocated memory


#if _MSC_VER && OEL_MEM_BOUND_DEBUG_LVL < 2 && _ITERATOR_DEBUG_LEVEL == 0
	#define OEL_FORCEINLINE __forceinline
#else
	#define OEL_FORCEINLINE inline
#endif

	void _uninitCopyData(std::false_type, const_pointer first, const_pointer last, size_type)
	{	// not trivially copyable T
		_end = _detail::UninitCopy(first, last, data());
		_reserveEnd = _end;
	}

	void _uninitCopyData(std::true_type, const_pointer first, const_pointer, size_type const count)
	{
		::memcpy(data(), first, count * sizeof(T));
		_reserveEnd = _end = data() + count;
	}

	size_type _unusedCapacity() const
	{
		return _reserveEnd - _end;
	}

	size_type _calcCapAddOne() const
	{
		enum { MIN_GROW = sizeof(pointer) >= sizeof(T) ?
				2 * sizeof(pointer) / sizeof(T) :
				(sizeof(T) <= 2040 ? 2 : 1) };
		size_type reserved = capacity();
		// Grow by 50%, or at least MIN_GROW elements
		return reserved + std::max(reserved / 2, size_type(MIN_GROW));
	}

	size_type _appendCalcCap(size_type const toAdd) const
	{
		size_type reserved = capacity();
		reserved += reserved / 2;
		return std::max(reserved, size() + toAdd);
	}

	template<typename CntigusIter>
	void _assignImpl(std::true_type, CntigusIter const first, CntigusIter, size_type const count)
	{	// fast assign
	#if OEL_MEM_BOUND_DEBUG_LVL
		OEL_PUSH_IGNORE_UNUSED_VALUE
		if (count > 0)
		{	*first;  // Dereference to catch out of range errors if the iterators have internal checks
			*(first + (count - 1));
		}
		OEL_POP_DIAGNOSTIC
	#endif
		if (capacity() < count)
		{
			_data.reset(_alloc(count));
			_end = data() + count;
			_reserveEnd = _end;
		}
		else
		{	_end = data() + count;
		}
		// Not portable. Check for self assignment or use memmove?
		::memcpy(data(), to_pointer_contiguous(first), count * sizeof(T));
	}

	template<typename InputIter>
	void _assignImpl(std::false_type, InputIter first, InputIter const last, size_type const count)
	{	// non-trivial assign
		if (capacity() < count)
		{	// not enough room, allocate new array and construct new
			_smartPtr newData{_alloc(count)};
			pointer const newEnd = _detail::UninitCopy(first, last, newData.get());
			_detail::Destroy(data(), _end);
			_reserveEnd = _end = newEnd;
			_data.swap(newData);
		}
		else if (size() >= count)
		{	// enough elements, copy new and destroy old
			iterator newEnd = std::copy(first, last, begin());
			erase_back(newEnd);
		}
		else
		{	// enough room, assign to old elements and construct rest
			for (pointer dest = data(); dest != _end; ++dest)
			{
				*dest = *first;
				++first;
			}
			_end = _detail::UninitCopy(first, last, _end);
		}
	}

	template<typename ForwTravRange>
	void _assign(const ForwTravRange & range, forward_traversal_tag)
	{
		using IterSrc = decltype(adl_begin(range));
		_assignImpl(can_memmove_with<pointer, IterSrc>(),
					adl_begin(range), adl_end(range), oel::count(range));
	}

	template<typename InputRange>
	void _assign(const InputRange & range, single_pass_traversal_tag)
	{	// cannot count input objects before assigning
		clear();
		for (auto && v : range)
			emplace_back( std::forward<decltype(v)>(v) );
	}

	template<typename CopyFunc>
	pointer _appendNonTrivial(size_type const count, CopyFunc makeNewElems)
	{
		pointer appendPos;
		if (_unusedCapacity() >= count)
		{
			appendPos = _end;
			_end = makeNewElems(_end, count);
		}
		else
		{
			size_type const oldSize = size();
			size_type const newCapacity = _calcCapAdd(count);
			_smartPtr newData{_alloc(newCapacity)};

			appendPos = newData.get() + oldSize;
			_end = makeNewElems(appendPos, count);
			// Do not assign member variables until after copy of new elements in case of exception
			_reserveEnd = newData.get() + newCapacity;

			::memcpy(newData.get(), data(), oldSize * sizeof(T));  // relocate old
			_data.swap(newData);
		}
		return appendPos;
	}

	template<typename CntigusIter>
	OEL_FORCEINLINE CntigusIter _appendN(std::true_type, CntigusIter const first, size_type const count)
	{	// use memcpy
	#if OEL_MEM_BOUND_DEBUG_LVL
		CntigusIter last = first + count;

		OEL_PUSH_IGNORE_UNUSED_VALUE
		if (count > 0)  // Dereference to catch out of range errors if the iterators have internal checks
		{	*first; *(last - 1);
		}
		OEL_POP_DIAGNOSTIC
	#endif
		if (_unusedCapacity() >= count)
		{
			// Behaviour undefined by standard if first points to null
			::memcpy(_end, to_pointer_contiguous(first), count * sizeof(T));
		}
		else
		{
			size_type const newCapacity = _calcCapAdd(count);
			pointer const newData = _alloc(newCapacity);

			_reserveEnd = newData + newCapacity;

			::memcpy(newData, data(), size() * sizeof(T));
			_end = newData + size();
			::memcpy(_end, to_pointer_contiguous(first), count * sizeof(T));
			// Copy new elements before deallocating old buffer, in case this->_data is the source
			_data.reset(newData);
		}
		_end += count;

	#if OEL_MEM_BOUND_DEBUG_LVL
		return last; // in the case of append self, bypasses check in iterator's operator +
	#else
		return first + count;
	#endif
	}

	template<typename InputIter>
	InputIter _appendN(std::false_type, InputIter first, size_type const count)
	{	// slower copy
		_appendNonTrivial( count,
				[&first](pointer dest, size_type count)
				{
					auto const res = _detail::UninitCopyN(first, count, dest);
					first = res.src_end;
					return res.dest_end;
				} );
		return first;
	}

	template<typename CntigusRange>
	OEL_FORCEINLINE iterator _append(std::true_type, forward_traversal_tag, const CntigusRange & range)
	{	// use memcpy
		auto const nElems = oel::count(range);
		_appendN(std::true_type{}, adl_begin(range), nElems);

		return end() - nElems;
	}

	template<typename ForwTravRange>
	iterator _append(std::false_type, forward_traversal_tag, const ForwTravRange & range)
	{	// multi-pass iterator
		auto first = adl_begin(range);
		auto last = adl_end(range);
		pointer const pos = _appendNonTrivial( oel::count(range),
				[=](pointer dest, size_type)
				{
					return _detail::UninitCopy(first, last, dest);
				} );
		return OEL_DYNARR_ITERATOR(pos);
	}

	template<typename InputRange>
	iterator _append(std::false_type, single_pass_traversal_tag, const InputRange & range)
	{	// slowest
		size_type const oldSize = size();
		try
		{
			for (auto && v : range)
				emplace_back( std::forward<decltype(v)>(v) );
		}
		catch (...)
		{
			erase_back(begin() + oldSize);
			throw;
		}
		return begin() + oldSize;
	}

	template<typename UninitFillFunc>
	void _resizeImpl(size_type const newSize, UninitFillFunc initNewElems)
	{
		_detail::AssertRelocate<T>();

		size_type allocSize = capacity();
		if (newSize < allocSize)
		{
			pointer const newEnd = data() + newSize;
			if (_end < newEnd) // then construct new
				initNewElems(_end, newEnd);
			else // destroy old
				_detail::Destroy(newEnd, _end);

			_end = newEnd;
		}
		else
		{	// not enough room, reallocate
			allocSize += allocSize / 2;
			allocSize = std::max(allocSize, newSize);

			_smartPtr newData{_alloc(allocSize)};

			size_type const oldSize = size();
			pointer const newEnd = newData.get() + newSize;
			initNewElems(newData.get() + oldSize, newEnd);
			_end = newEnd;
			_reserveEnd = newData.get() + allocSize;

			// Fill new elements before reallocating old data, in case of copying an element from this
			::memcpy(newData.get(), data(), oldSize * sizeof(T));  // relocate old
			_data.swap(newData);
		}
	}

	void _relocateData(std::true_type, pointer newData)
	{
		::memcpy(newData, data(), size() * sizeof(T));
	}

	void _relocateData(std::false_type, pointer newData)
	{
		static_assert(std::is_nothrow_move_constructible<T>::value,
					  "T must be trivially relocatable or have noexcept move constructor");
		for (pointer p = data(); p < _end; ++p)
			::new(newData++) T(std::move(*p));
	}
};

// Definitions of public functions

template<typename T, typename Alloc>
inline dynarray<T, Alloc>::dynarray(reserve_tag, size_type capacity) :
	_data(_alloc(capacity)),
	_end(data()),
	_reserveEnd(data() + capacity) {
}

template<typename T, typename Alloc>
inline dynarray<T, Alloc>::dynarray(size_type size, default_init_tag) :
	_data(_alloc(size)),
	_end(data() + size), _reserveEnd(_end)
{
	oel::uninitialized_fill_default(data(), _end);
}

template<typename T, typename Alloc>
dynarray<T, Alloc>::dynarray(size_type size, const T & val) :
	_data(_alloc(size)),
	_end(data() + size), _reserveEnd(_end)
{
	std::uninitialized_fill(data(), _end, val);
}

template<typename T, typename Alloc>
inline dynarray<T, Alloc>::dynarray(dynarray && other) NOEXCEPT :
	_data(std::move(other._data)),
	_end(other._end),
	_reserveEnd(other._reserveEnd)
{
	other._reserveEnd = other._end = nullptr;
}

template<typename T, typename Alloc>
dynarray<T, Alloc>::dynarray(const dynarray & other) :
	_data( _alloc(other.size()) )
{
	_uninitCopyData(is_trivially_copyable<T>(),
					other.data(), other._end, other.size());
}

template<typename T, typename Alloc>
dynarray<T, Alloc>::dynarray(std::initializer_list<T> init) :
	_data( _alloc(init.size()) )
{
	_uninitCopyData(is_trivially_copyable<T>(),
					init.begin(), init.end(), init.size());
}

template<typename T, typename Alloc>
dynarray<T, Alloc>::~dynarray() NOEXCEPT
{
	_detail::Destroy(data(), _end);
}

template<typename T, typename Alloc>
void dynarray<T, Alloc>::swap(dynarray & other) NOEXCEPT
{
	using std::swap;
	swap(_data, other._data);
	swap(_end, other._end);
	swap(_reserveEnd, other._reserveEnd);
}

template<typename T, typename Alloc> template<typename ForwardTravIterator>
ForwardTravIterator dynarray<T, Alloc>::assign(ForwardTravIterator first, size_type count)
{
#ifndef OEL_NO_BOOST
	static_assert(boost::is_convertible< iterator_traversal_t<ForwardTravIterator>, forward_traversal_tag >::value,
				  "Type of first must meet requirements of Forward Traversal Iterator");
#endif
	auto const last = std::next(first, count);
	_assignImpl(can_memmove_with<pointer, ForwardTravIterator>(),
				first, last, count);
	return last;
}

template<typename T, typename Alloc> template<typename InputRange>
void dynarray<T, Alloc>::assign(const InputRange & source)
{
	using InIter = decltype(adl_begin(source));
	_assign(source, iterator_traversal_t<InIter>());
}

template<typename T, typename Alloc> template<typename InputIterator>
OEL_FORCEINLINE InputIterator dynarray<T, Alloc>::append(InputIterator first, size_type count)
{
	_detail::AssertRelocate<T>();

	return _appendN(can_memmove_with<pointer, InputIterator>(), first, count);
}

template<typename T, typename Alloc> template<typename InputRange>
OEL_FORCEINLINE typename dynarray<T, Alloc>::iterator  dynarray<T, Alloc>::append(const InputRange & source)
{
	_detail::AssertRelocate<T>();

	using IterSrc = decltype(adl_begin(source));
	return _append(can_memmove_with<pointer, IterSrc>(),
				   iterator_traversal_t<IterSrc>(),
				   source);
}

template<typename T, typename Alloc>
OEL_FORCEINLINE typename dynarray<T, Alloc>::iterator  dynarray<T, Alloc>::append(std::initializer_list<T> il)
{
	return append<>(il);
}

template<typename T, typename Alloc> template<typename... ArgTs>
void dynarray<T, Alloc>::emplace_back(ArgTs &&... args)
{
	_detail::AssertRelocate<T>();

	if (_end < _reserveEnd)
	{
		::new(_end) T(std::forward<ArgTs>(args)...);
	}
	else
	{
		size_type const newCapacity = _calcCapAddOne();
		_smartPtr newData{_alloc(newCapacity)};

		pointer const newEnd = newData.get() + size();
		::new(newEnd) T(std::forward<ArgTs>(args)...);
		// Exception free from here
		_reserveEnd = newData.get() + newCapacity;
		// Make new element before reallocating old data to support push_back from this
		_relocateData(is_trivially_relocatable<T>(), newData.get());
		_end = newEnd;
		_data.swap(newData);
	}
	++_end;
}

template<typename T, typename Alloc> template<typename... ArgTs>
typename dynarray<T, Alloc>::iterator  dynarray<T, Alloc>::emplace(const_iterator pos, ArgTs &&... args)
{
	_detail::AssertRelocate<T>();

	auto const posPtr = const_cast<pointer>(to_pointer_contiguous(pos));
	MEM_BOUND_ASSERT_CHEAP(data() <= posPtr && posPtr <= _end);

	size_type const nAfterPos = _end - posPtr;

	if (_end < _reserveEnd) // then new element fits
	{
		// Temporary in case constructor throws or source is an element of this dynarray at pos or after
		using RawStore = aligned_storage_t<sizeof(T), OEL_ALIGNOF(T)>;
		RawStore local;
		::new(&local) T(std::forward<ArgTs>(args)...);
		// Move [pos, end) to [pos + 1, end + 1), conceptually destroying element at pos
		::memmove(posPtr + 1, posPtr, nAfterPos * sizeof(T));
		++_end;

		*reinterpret_cast<RawStore *>(posPtr) = local; // relocate the new element to pos

		return OEL_DYNARR_ITERATOR(posPtr);
	}
	else
	{	// not enough room, reallocate
		size_type const newCapacity = _calcCapAddOne();

		_smartPtr newData{_alloc(newCapacity)};

		size_type const nBeforePos = posPtr - data();
		pointer const newPos = newData.get() + nBeforePos;
		::new(newPos) T(std::forward<ArgTs>(args)...);   // add new
		pointer const next = newPos + 1;
		// Behaviour undefined by standard if data is null
		::memcpy(newData.get(), data(), nBeforePos * sizeof(T)); // relocate prefix
		::memcpy(next, posPtr, nAfterPos * sizeof(T));   // relocate suffix
		_end = next + nAfterPos;

		_reserveEnd = newData.get() + newCapacity;

		_data.swap(newData);

		return OEL_DYNARR_ITERATOR(newPos);
	}
}

template<typename T, typename Alloc>
void dynarray<T, Alloc>::reserve(size_type minCapacity)
{
	_detail::AssertRelocate<T>();

	if (capacity() < minCapacity)
	{
		pointer const newData = _alloc(minCapacity);

		_reserveEnd = newData + minCapacity;
		// Relocate elements
		::memcpy(newData, data(), size() * sizeof(T));
		_end = newData + size();

		_data.reset(newData);
	}
}

template<typename T, typename Alloc>
void dynarray<T, Alloc>::shrink_to_fit()
{
	_detail::AssertRelocate<T>();

	size_type const usedSize = size();
	pointer newData;
	if (0 < usedSize)
	{
		newData = _alloc(usedSize);
		::memcpy(newData, data(), usedSize * sizeof(T)); // relocate elements
		_end = newData + usedSize;
	}
	else
	{	_end = newData = nullptr;
	}
	_reserveEnd = _end;
	_data.reset(newData);
}

template<typename T, typename Alloc>
inline void dynarray<T, Alloc>::pop_back() NOEXCEPT
{
	OEL_MEM_BOUND_ASSERT(data() < _end);
	--_end;
	_end-> ~T();
}

template<typename T, typename Alloc>
void dynarray<T, Alloc>::resize(size_type newSize, default_init_tag)
{
	_resizeImpl(newSize, oel::uninitialized_fill_default<pointer>);
}

template<typename T, typename Alloc>
void dynarray<T, Alloc>::resize(size_type newSize, const T & addVal)
{
	_resizeImpl( newSize,
			[&addVal](pointer first, pointer last)
			{
				std::uninitialized_fill(first, last, addVal);
			} );
}

template<typename T, typename Alloc>
inline typename dynarray<T, Alloc>::iterator  dynarray<T, Alloc>::erase(iterator pos) NOEXCEPT
{
	_detail::AssertRelocate<T>();

	pointer const posPtr = to_pointer_contiguous(pos);
	MEM_BOUND_ASSERT_CHEAP(data() <= posPtr && posPtr < _end);

	posPtr-> ~T();
	pointer const next = posPtr + 1;
	::memmove(posPtr, next, (_end - next) * sizeof(T)); // relocate [pos + 1, end) to [pos, end - 1)
	--_end;
	return pos;
}

template<typename T, typename Alloc>
typename dynarray<T, Alloc>::iterator  dynarray<T, Alloc>::erase(iterator first, iterator last) NOEXCEPT
{
	_detail::AssertRelocate<T>();

	pointer const pFirst = to_pointer_contiguous(first);
	pointer const pLast = to_pointer_contiguous(last);
	MEM_BOUND_ASSERT_CHEAP(data() <= pFirst);  // if pLast > _end, caller will find out when memmove crashes
	OEL_MEM_BOUND_ASSERT(pFirst <= pLast && pLast <= _end);
	if (pFirst < pLast)
	{
		_detail::Destroy(pFirst, pLast);
		size_type const nAfterLast = _end - pLast;
		// Relocate [last, end) to [first, first + nAfterLast)
		::memmove(pFirst, pLast, nAfterLast * sizeof(T));
		_end = pFirst + nAfterLast;
	}
	return first;
}

template<typename T, typename Alloc>
inline void dynarray<T, Alloc>::erase_back(iterator first) NOEXCEPT
{
	pointer const newEnd = to_pointer_contiguous(first);
	MEM_BOUND_ASSERT_CHEAP(data() <= newEnd && newEnd <= _end);
	_detail::Destroy(newEnd, _end);
	_end = newEnd;
}

template<typename T, typename Alloc>
typename dynarray<T, Alloc>::reference  dynarray<T, Alloc>::at(size_type index)
{
	if (size() > index)
		return _data[index];
	else
		throw out_of_range("Invalid index dynarray::at");
}
template<typename T, typename Alloc>
typename dynarray<T, Alloc>::const_reference  dynarray<T, Alloc>::at(size_type index) const
{
	if (size() > index)
		return _data[index];
	else
		throw out_of_range("Invalid index dynarray::at");
}

template<typename T, typename Alloc>
inline typename dynarray<T, Alloc>::reference  dynarray<T, Alloc>::operator[](size_type index) NOEXCEPT
{
	OEL_MEM_BOUND_ASSERT(size() > index);
	return _data[index];
}
template<typename T, typename Alloc>
inline typename dynarray<T, Alloc>::const_reference  dynarray<T, Alloc>::operator[](size_type index) const NOEXCEPT
{
	OEL_MEM_BOUND_ASSERT(size() > index);
	return _data[index];
}

#undef OEL_DYNARR_ITERATOR
#undef OEL_DYNARR_CONST_ITER

} // namespace oel

template<typename T, typename A>
inline typename oel::dynarray<T, A>::iterator  oel::
	erase_unordered(dynarray<T, A> & ctr, typename dynarray<T, A>::iterator pos)
{
	*pos = std::move(ctr.back());
	ctr.pop_back();
	return pos;
}

#undef OEL_FORCEINLINE
