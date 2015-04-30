#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "contiguous_iterator.h"
#include "container_core.h"

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
struct reserve_tag
{	explicit reserve_tag() {}
}
const reserve;

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

/// Non-member erase_back, overloads generic erase_back(Container &, Container::iterator)
template<typename T, typename A> inline
void erase_back(dynarray<T, A> & ctr, typename dynarray<T, A>::iterator first)  { ctr.erase_back(first); }

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
	using allocator_type  = Alloc;
	using reference       = T &;
	using const_reference = const T &;
	using pointer         = typename std::allocator_traits<Alloc>::pointer;
	using const_pointer   = typename std::allocator_traits<Alloc>::const_pointer;
	using size_type       = typename std::allocator_traits<Alloc>::size_type;
	using difference_type = typename std::allocator_traits<Alloc>::difference_type;

#if OEL_MEM_BOUND_DEBUG_LVL >= 2
	using iterator       = cntigus_ctr_dbg_iterator< pointer, dynarray<T, Alloc> >;
	using const_iterator = cntigus_ctr_dbg_iterator< const_pointer, dynarray<T, Alloc> >;

	#define OEL_DYNARR_ITERATOR(ptr)        iterator{ptr, this}
	#define OEL_DYNARR_CONST_ITER(constPtr) const_iterator{constPtr, this}
#else
	using iterator       = pointer;
	using const_iterator = const_pointer;

	#define OEL_DYNARR_ITERATOR(ptr)        (ptr)
	#define OEL_DYNARR_CONST_ITER(constPtr) (constPtr)
#endif
	using reverse_iterator       = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

	dynarray() NOEXCEPT  : _data(nullptr), _end(nullptr), _reserveEnd(nullptr) {}

	/** @brief Construct empty dynarray with space reserved for at least capacity elements
	* @throw std::bad_alloc if the allocation request does not succeed (same for all operations that add capacity)  */
	dynarray(reserve_tag, size_type capacity);

	/** @brief Elements are default initialized, can be significantly faster than dynarray(size_type size)
	*
	* Non-class T objects get indeterminate values. http://en.cppreference.com/w/cpp/language/default_initialization  */
	dynarray(size_type size, default_init_tag);
	explicit dynarray(size_type size);  ///< (Elements are value initialized, same as std::vector)
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
	template< typename InputIterator, typename = enable_if_t<!std::is_arithmetic<InputIterator>::value> >
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
	/// Equivalent to std::vector::insert(end(), count, val)
	void          append(size_type count, const T & val);

	/** @brief Added elements are default initialized, can be significantly faster than other resize
	*
	* Non-class T objects get indeterminate values. http://en.cppreference.com/w/cpp/language/default_initialization  */
	void       resize(size_type newSize, default_init_tag);
	void       resize(size_type newSize);  ///< (Value initializes added elements, same as std::vector::resize)

	template<typename... Args>
	void       emplace_back(Args &&... elemInitArgs);

	void       push_back(T && val)       { emplace_back(std::move(val)); }
	void       push_back(const T & val)  { emplace_back(val); }

	template<typename... Args>
	iterator   emplace(const_iterator position, Args &&... elemInitArgs);

	iterator   insert(const_iterator position, T && val)       { return emplace(position, std::move(val)); }
	iterator   insert(const_iterator position, const T & val)  { return emplace(position, val); }

	/// After the call, any previous iterator to the back element will be equal to end()
	void       pop_back() NOEXCEPT;

	iterator   erase(iterator position) NOEXCEPT;

	iterator   erase(iterator first, iterator last) NOEXCEPT;

	/// Equivalent to erase(first, end()) (but potentially faster)
	void       erase_back(iterator first) NOEXCEPT;

	void       clear() NOEXCEPT        { erase_back(begin()); }

	bool       empty() const NOEXCEPT  { return _data.get() == _end; }

	size_type  size() const NOEXCEPT   { return _end - _data.get(); }

	void       reserve(size_type minCapacity);

	/// It's a good idea to check that size() < capacity() before calling to avoid useless reallocation
	void       shrink_to_fit();

	size_type  capacity() const NOEXCEPT  { return _reserveEnd - _data.get(); }

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
			Alloc{}.deallocate(ptr, 0); // 0 should be capacity()
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

	struct _staticAssertRelocate
	{
		static_assert(is_trivially_relocatable<T>::value,
			"Template argument T must be trivially relocatable, see documentation for is_trivially_relocatable");
	};


	void _uninitCopyData(std::false_type, const_pointer first, const_pointer last, size_type)
	{	// not trivially copyable T
		_end = _detail::UninitCopy(first, last, _data.get());
		_reserveEnd = _end;
	}

	void _uninitCopyData(std::true_type, const_pointer first, const_pointer, size_type const count)
	{
		::memcpy(_data.get(), first, sizeof(T) * count);
		_reserveEnd = _end = _data.get() + count;
	}

	size_type _unusedCapacity() const
	{
		return _reserveEnd - _end;
	}

	size_type _insertOneCalcCap() const
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
			_end = _data.get() + count;
			_reserveEnd = _end;
		}
		else
		{	_end = _data.get() + count;
		}
		// Not portable. Check for self assignment or use memmove?
		::memcpy(_data.get(), to_pointer_contiguous(first), sizeof(T) * count);
	}

	template<typename InputIter>
	void _assignImpl(std::false_type, InputIter first, InputIter const last, size_type const count)
	{	// non-trivial assign
		if (capacity() < count)
		{	// not enough room, allocate new array and construct new
			_smartPtr newData{_alloc(count)};
			pointer const newEnd = _detail::UninitCopy(first, last, newData.get());
			_detail::Destroy(_data.get(), _end);
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
			for (pointer dest = _data.get(); dest != _end; ++dest)
			{
				*dest = *first;
				++first;
			}
			_end = _detail::UninitCopy(first, last, _end);
		}
	}

	template<typename ForwTravRange>
	void _assign(const ForwTravRange & src, forward_traversal_tag)
	{
		using IterSrc = decltype(adl_begin(src));
		_assignImpl(can_memmove_with<pointer, IterSrc>(),
					adl_begin(src), adl_end(src), oel::count(src));
	}

	template<typename InputRange>
	void _assign(const InputRange & src, single_pass_traversal_tag)
	{	// cannot count input objects before assigning
		clear();
		for (auto && v : src)
			emplace_back( std::forward<decltype(v)>(v) );
	}

	template<typename CopyFunc>
	OEL_FORCEINLINE iterator _appendImpl(size_type const count, CopyFunc makeNewElems)
	{
		pointer pos;
		if (_unusedCapacity() >= count)
		{
			pos = _end;
			makeNewElems(_end, count);
		}
		else
		{
			size_type const newCapacity = _appendCalcCap(count);
			_smartPtr newData{_alloc(newCapacity)};

			size_type const oldSize = size();
			pos = newData.get() + oldSize;
			makeNewElems(pos, count);
			// Do not assign member variables until after copy of new elements in case of exception
			_end = pos;
			_reserveEnd = newData.get() + newCapacity;

			::memcpy(newData.get(), _data.get(), sizeof(T) * oldSize);  // relocate old
			_data.swap(newData);
		}
		_end += count;

		return OEL_DYNARR_ITERATOR(pos);
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
		_appendImpl( count,
				[first](pointer dest, size_type nElems)
				{	// Behaviour undefined by standard if first points to null
					::memcpy(dest, to_pointer_contiguous(first), sizeof(T) * nElems);
				} );

	#if OEL_MEM_BOUND_DEBUG_LVL
		return last; // in the case of append self, bypasses check in iterator's operator +
	#else
		return first + count;
	#endif
	}

	template<typename InputIter>
	InputIter _appendN(std::false_type, InputIter first, size_type const count)
	{	// slower copy
		_appendImpl( count,
				[&first](pointer dest, size_type nElems)
				{
					first = _detail::UninitCopyN(first, nElems, dest).src_end;
				} );
		return first;
	}

	template<typename CntigusRange>
	OEL_FORCEINLINE iterator _append(std::true_type, forward_traversal_tag, const CntigusRange & src)
	{	// use memcpy
		auto const nElems = oel::count(src);
		_appendN(std::true_type{}, adl_begin(src), nElems);

		return end() - nElems;
	}

	template<typename ForwTravRange>
	iterator _append(std::false_type, forward_traversal_tag, const ForwTravRange & src)
	{	// multi-pass iterator
		auto first = adl_begin(src);
		auto last = adl_end(src);
		return _appendImpl( oel::count(src),
				[=](pointer dest, size_type)
				{
					_detail::UninitCopy(first, last, dest);
				} );
	}

	template<typename InputRange>
	iterator _append(std::false_type, single_pass_traversal_tag, const InputRange & src)
	{	// slowest
		size_type const oldSize = size();
		try
		{
			for (auto && v : src)
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
		_staticAssertRelocate();

		size_type allocSize = capacity();
		if (newSize < allocSize)
		{
			pointer const newEnd = _data.get() + newSize;
			if (_end < newEnd) // then construct new
				initNewElems(_end, newEnd);
			else // downsizing
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
			// Fill new elements before relocating old data, in case of copying an element from this
			::memcpy(newData.get(), _data.get(), sizeof(T) * oldSize);
			_data.swap(newData);
		}
	}
};

// Definitions of public functions

template<typename T, typename Alloc>
inline dynarray<T, Alloc>::dynarray(reserve_tag, size_type capacity) :
	_data(_alloc(capacity)),
	_end(_data.get()),
	_reserveEnd(_data.get() + capacity) {
}

template<typename T, typename Alloc>
inline dynarray<T, Alloc>::dynarray(size_type size, default_init_tag) :
	_data(_alloc(size)),
	_end(_data.get() + size), _reserveEnd(_end)
{
	_detail::UninitFillDefault(_data.get(), _end);
}

template<typename T, typename Alloc>
dynarray<T, Alloc>::dynarray(size_type size) :
	_data(_alloc(size)),
	_end(_data.get() + size), _reserveEnd(_end)
{
	_detail::UninitFill(_data.get(), _end);
}

template<typename T, typename Alloc>
dynarray<T, Alloc>::dynarray(size_type size, const T & val) :
	_data(_alloc(size)),
	_end(_data.get() + size), _reserveEnd(_end)
{
	std::uninitialized_fill(_data.get(), _end, val);
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
	_detail::Destroy(_data.get(), _end);
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
void dynarray<T, Alloc>::assign(const InputRange & src)
{
	using InIter = decltype(adl_begin(src));
	_assign(src, iterator_traversal_t<InIter>());
}

template<typename T, typename Alloc>
void dynarray<T, Alloc>::append(size_type count, const T & val)
{
	_appendImpl( count,
			[&val](pointer dest, size_type nElems)
			{
				std::uninitialized_fill_n(dest, nElems, val);
			} );
}

template<typename T, typename Alloc> template<typename InputIterator, typename>
OEL_FORCEINLINE InputIterator dynarray<T, Alloc>::append(InputIterator first, size_type count)
{
	_staticAssertRelocate();

	return _appendN(can_memmove_with<pointer, InputIterator>(), first, count);
}

template<typename T, typename Alloc> template<typename InputRange>
OEL_FORCEINLINE typename dynarray<T, Alloc>::iterator  dynarray<T, Alloc>::append(const InputRange & src)
{
	_staticAssertRelocate();

	using IterSrc = decltype(adl_begin(src));
	return _append(can_memmove_with<pointer, IterSrc>(),
				   iterator_traversal_t<IterSrc>(),
				   src);
}

template<typename T, typename Alloc>
OEL_FORCEINLINE typename dynarray<T, Alloc>::iterator  dynarray<T, Alloc>::append(std::initializer_list<T> il)
{
	return append<>(il);
}

template<typename T, typename Alloc> template<typename... Args>
void dynarray<T, Alloc>::emplace_back(Args &&... args)
{
	_staticAssertRelocate();

	if (_end < _reserveEnd)
	{
		std::allocator_traits<Alloc>::construct(Alloc{}, _end, std::forward<Args>(args)...);
	}
	else
	{
		size_type const newCapacity = _insertOneCalcCap();
		_smartPtr newData{_alloc(newCapacity)};

		size_type const oldSize = size();
		pointer const pos = newData.get() + oldSize;
		std::allocator_traits<Alloc>::construct(Alloc{}, pos, std::forward<Args>(args)...);

		_end = pos;
		_reserveEnd = newData.get() + newCapacity;
		// Make new element before relocating old data to support push_back from this
		::memcpy(newData.get(), _data.get(), sizeof(T) * oldSize);
		_data.swap(newData);
	}
	++_end;
}

template<typename T, typename Alloc> template<typename... Args>
typename dynarray<T, Alloc>::iterator  dynarray<T, Alloc>::emplace(const_iterator pos, Args &&... args)
{
	_staticAssertRelocate();

	auto const posPtr = const_cast<pointer>(to_pointer_contiguous(pos));
	MEM_BOUND_ASSERT_CHEAP(_data.get() <= posPtr && posPtr <= _end);

	size_type const nAfterPos = _end - posPtr;

	if (_end < _reserveEnd) // then new element fits
	{
		// Temporary in case constructor throws or source is an element of this dynarray at pos or after
		using RawStore = aligned_storage_t<sizeof(T), OEL_ALIGNOF(T)>;
		RawStore tmp;
		std::allocator_traits<Alloc>::construct(Alloc{}, reinterpret_cast<pointer>(&tmp), std::forward<Args>(args)...);
		// Move [pos, end) to [pos + 1, end + 1), conceptually destroying element at pos
		::memmove(posPtr + 1, posPtr, nAfterPos * sizeof(T));
		++_end;

		*reinterpret_cast<RawStore *>(posPtr) = tmp; // relocate the new element to pos

		return OEL_DYNARR_ITERATOR(posPtr);
	}
	else
	{	// not enough room, reallocate
		size_type const newCapacity = _insertOneCalcCap();

		_smartPtr newData{_alloc(newCapacity)};

		size_type const nBeforePos = posPtr - _data.get();
		pointer const newPos = newData.get() + nBeforePos;
		std::allocator_traits<Alloc>::construct(Alloc{}, newPos, std::forward<Args>(args)...);
		pointer const next = newPos + 1;
		// Behaviour undefined by standard if data is null
		::memcpy(newData.get(), _data.get(), nBeforePos * sizeof(T)); // relocate prefix
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
	_staticAssertRelocate();

	if (capacity() < minCapacity)
	{
		pointer const newData = _alloc(minCapacity);

		_reserveEnd = newData + minCapacity;
		// Relocate elements
		::memcpy(newData, _data.get(), size() * sizeof(T));
		_end = newData + size();

		_data.reset(newData);
	}
}

template<typename T, typename Alloc>
void dynarray<T, Alloc>::shrink_to_fit()
{
	_staticAssertRelocate();

	size_type const usedSize = size();
	pointer newData;
	if (0 < usedSize)
	{
		newData = _alloc(usedSize);
		::memcpy(newData, _data.get(), usedSize * sizeof(T)); // relocate elements
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
	OEL_MEM_BOUND_ASSERT(_data.get() < _end);
	--_end;
	_end-> ~T();
}

template<typename T, typename Alloc>
void dynarray<T, Alloc>::resize(size_type newSize, default_init_tag)
{
	_resizeImpl(newSize, _detail::UninitFillDefault<T>);
}

template<typename T, typename Alloc>
void dynarray<T, Alloc>::resize(size_type newSize)
{
	_resizeImpl(newSize, _detail::UninitFill<T>);
}

template<typename T, typename Alloc>
inline typename dynarray<T, Alloc>::iterator  dynarray<T, Alloc>::erase(iterator pos) NOEXCEPT
{
	_staticAssertRelocate();

	pointer const posPtr = to_pointer_contiguous(pos);
	MEM_BOUND_ASSERT_CHEAP(_data.get() <= posPtr && posPtr < _end);

	posPtr-> ~T();
	pointer const next = posPtr + 1;
	::memmove(posPtr, next, (_end - next) * sizeof(T)); // relocate [pos + 1, end) to [pos, end - 1)
	--_end;
	return pos;
}

template<typename T, typename Alloc>
typename dynarray<T, Alloc>::iterator  dynarray<T, Alloc>::erase(iterator first, iterator last) NOEXCEPT
{
	_staticAssertRelocate();

	pointer const pFirst = to_pointer_contiguous(first);
	pointer const pLast = to_pointer_contiguous(last);
	MEM_BOUND_ASSERT_CHEAP(_data.get() <= pFirst);  // if pLast > _end, caller will find out when memmove crashes
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
	MEM_BOUND_ASSERT_CHEAP(_data.get() <= newEnd && newEnd <= _end);
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
