#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "auxi/contiguous_container_iterator.h"
#include "compat/default.h"
#include "align_allocator.h"

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


/// Tag to select dynarray constructor that allocates storage. A const instance named reserve is provided to pass
struct reserve_tag
{	explicit reserve_tag() {}
}
const reserve;

////////////////////////////////////////////////////////////////////////////////

/// dynarray<dynarray<T>> is efficient
template<typename T, typename Alloc>
is_trivially_relocatable<Alloc> specify_trivial_relocate(dynarray<T, Alloc>);

template<typename T, typename A> inline
void swap(dynarray<T, A> & a, dynarray<T, A> & b) noexcept  { a.swap(b); }

template<typename T, typename A> inline
typename dynarray<T, A>::iterator  erase_unordered(dynarray<T, A> & ctr, typename dynarray<T, A>::iterator position)
	{ return ctr.erase_unordered(position); }

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
* Many functions require that relocating objects of template argument T is equivalent to memcpy without destructor call
* (true for most types). This is checked when compiling with is_trivially_relocatable, a trait which must be
* specialized manually for each type that is not trivially copyable.
* There are some notable exceptions for which trivially relocatable T is not required:
* all constructors, operator =, swap, assign, emplace_back, push_back, pop_back and erase_back.
*
* The default allocator supports over-aligned types (e.g. __m256)  */
template<typename T, typename Alloc>
class dynarray
{
public:
	using value_type      = T;
	using allocator_type  = Alloc;
	using reference       = T &;
	using const_reference = const T &;
	using pointer         = typename std::allocator_traits<Alloc>::pointer;
	using const_pointer   = typename std::allocator_traits<Alloc>::const_pointer;
	using size_type       = typename std::allocator_traits<Alloc>::size_type;  ///< Allowed to be signed
	using difference_type = typename std::allocator_traits<Alloc>::difference_type;

#if OEL_MEM_BOUND_DEBUG_LVL >= 2
	using iterator       = contiguous_ctnr_iterator< pointer, dynarray<T, Alloc> >;
	using const_iterator = contiguous_ctnr_iterator< const_pointer, dynarray<T, Alloc> >;

	#define OEL_DYNARR_ITERATOR(ptr)        iterator{ptr, this}            // these are macros to avoid function call
	#define OEL_DYNARR_CONST_ITER(constPtr) const_iterator{constPtr, this} // overhead in builds without inlining
#else
	using iterator       = pointer;
	using const_iterator = const_pointer;

	#define OEL_DYNARR_ITERATOR(ptr)        (ptr)
	#define OEL_DYNARR_CONST_ITER(constPtr) (constPtr)
#endif
	using reverse_iterator       = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;


	dynarray() noexcept                     : _m() {}
	explicit dynarray(const Alloc & alloc)  : _m(alloc) {}

	/// Construct empty dynarray with space reserved for at least capacity elements
	dynarray(reserve_tag, size_type capacity, const Alloc & alloc = Alloc{});

	/** @brief Uses default initialization for elements, can be significantly faster for non-class T
	*
	* Non-class T objects get indeterminate values. http://en.cppreference.com/w/cpp/language/default_initialization  */
	dynarray(size_type size, default_init_tag, const Alloc & alloc = Alloc{});
	explicit dynarray(size_type size, const Alloc & alloc = Alloc{});  ///< (Value-initializes elements, same as std::vector)
	dynarray(size_type size, const T & fillVal, const Alloc & alloc = Alloc{});

	dynarray(std::initializer_list<T> init, const Alloc & alloc = Alloc{})  : dynarray(from_range, init, alloc) {}

	/** @brief Equivalent to std::vector(begin(source), end(source), alloc)
	*
	* If you need to construct from some std::istream, check out boost/range/istream_range.hpp  */
	template<typename InputRange>
	dynarray(from_range_tag, const InputRange & source, const Alloc & alloc = Alloc{});

	dynarray(dynarray && other) noexcept;
	dynarray(dynarray && other, const Alloc & alloc) noexcept;
	dynarray(const dynarray & other);
	dynarray(const dynarray & other, const Alloc & alloc)  : dynarray(from_range, other, alloc) {}

	~dynarray() noexcept;

	dynarray & operator =(dynarray && other) noexcept;
	dynarray & operator =(const dynarray & other);

	dynarray & operator =(std::initializer_list<T> il)  { assign(il);  return *this; }

	void       swap(dynarray & other) noexcept;

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
	* @param source an array, STL container, iterator_range or such. (Look up Boost.Range 2.0 concepts.)
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
	* Strong exception guarantee, this function has no effect if an exception is thrown. */
	template<typename InputIterator, typename = decltype( *std::declval<InputIterator>() )>
	InputIterator append(InputIterator first, size_type count);
	/**
	* @brief Add at end the elements from range (in same order)
	* @param source an array, STL container, iterator_range or such. (Look up Boost.Range 2.0 concepts)
	* @return iterator pointing to first of the new elements in dynarray, or end if source is empty
	*
	* Otherwise same as append(InputIterator, size_type)  */
	template<typename InputRange>
	iterator      append(const InputRange & source);
	/// Equivalent to calling append(const InputRange &) with il as argument
	iterator      append(std::initializer_list<T> il);
	/// Equivalent to std::vector::insert(end(), count, val), but with strong exception guarantee
	void          append(size_type count, const T & val);

	/**
	* @brief Uses default initialization for added elements, can be significantly faster for non-class T
	*
	* Non-class T objects get indeterminate values. http://en.cppreference.com/w/cpp/language/default_initialization  */
	void       resize(size_type newSize, default_init_tag);
	void       resize(size_type newSize);  ///< (Value-initializes added elements, same as std::vector::resize)

	/// Performs list-initialization of element if there is no matching constructor
	template<typename... Args>
	void       emplace_back(Args &&... elemInitArgs);

	void       push_back(T && val)       { emplace_back(std::move(val)); }
	void       push_back(const T & val)  { emplace_back(val); }

	/// Performs list-initialization of element if there is no matching constructor
	template<typename... Args>
	iterator   emplace(const_iterator position, Args &&... elemInitArgs);

	iterator   insert(const_iterator position, T && val)       { return emplace(position, std::move(val)); }
	iterator   insert(const_iterator position, const T & val)  { return emplace(position, val); }

	/// After the call, any previous iterator to the back element will be equal to end()
	void       pop_back() noexcept;

	/**
	* @brief Erase the element at position from dynarray without maintaining order of elements.
	*
	* Constant complexity (compared to linear in the distance between position and last for normal erase).
	* @return Iterator pointing to the location that followed the element erased,
	*	which is the end if position was at the last element. */
	iterator   erase_unordered(iterator position);

	iterator   erase(iterator position) noexcept;

	iterator   erase(iterator first, iterator last) noexcept;

	/// Equivalent to erase(first, end()) (but potentially faster), making first the new end
	void       erase_back(iterator first) noexcept;

	void       clear() noexcept        { erase_back(begin()); }

	bool       empty() const noexcept  { return _m.data == _m.end; }

	size_type  size() const noexcept   { return _m.end - _m.data; }

	void       reserve(size_type minCapacity);

	/// It's a good idea to check that size() < capacity() before calling to avoid useless reallocation
	void       shrink_to_fit();

	size_type  capacity() const noexcept  { return _m.reserveEnd - _m.data; }

	allocator_type get_allocator() const  { return _m; }

	iterator        begin() noexcept         { return OEL_DYNARR_ITERATOR(_m.data); }
	const_iterator  begin() const noexcept   { return OEL_DYNARR_CONST_ITER(_m.data); }
	const_iterator  cbegin() const noexcept  { return begin(); }

	iterator        end() noexcept           { return OEL_DYNARR_ITERATOR(_m.end); }
	const_iterator  end() const noexcept     { return OEL_DYNARR_CONST_ITER(_m.end); }
	const_iterator  cend() const noexcept    { return end(); }

	reverse_iterator       rbegin() noexcept        { return reverse_iterator{end()}; }
	const_reverse_iterator rbegin() const noexcept  { return const_reverse_iterator{end()}; }

	reverse_iterator       rend() noexcept          { return reverse_iterator{begin()}; }
	const_reverse_iterator rend() const noexcept    { return const_reverse_iterator{begin()}; }

	T *             data() noexcept        { return _m.data; }
	const T *       data() const noexcept  { return _m.data; }

	reference       front() noexcept        { return *begin(); }
	const_reference front() const noexcept  { return *begin(); }

	reference       back() noexcept         { return *OEL_DYNARR_ITERATOR(_m.end - 1); }
	const_reference back() const noexcept   { return *OEL_DYNARR_CONST_ITER(_m.end - 1); }

	reference       at(size_type index);
	const_reference at(size_type index) const;

	reference       operator[](size_type index) noexcept;
	const_reference operator[](size_type index) const noexcept;



////////////////////////////////////////////////////////////////////////////////
//
// Implementation only in rest of the file


private:
	using _allocTrait = std::allocator_traits<Alloc>;
	using _uSizeT = make_unsigned_t<std::ptrdiff_t>;

	struct _staticAssertRelocate
	{
		static_assert(is_trivially_relocatable<T>::value,
			"This function requires trivially relocatable T, see declaration of is_trivially_relocatable");
	};

	template<bool IsEmptyAlloc>
	struct _scopedPtrBase
	{
		dynarray & owner;

		_scopedPtrBase(dynarray & owner) : owner(owner) {}

		pointer Allocate(size_type n)
		{
			return owner._m.allocate(n);
		}
		void Deallocate(pointer p, size_type n)
		{
			owner._m.deallocate(p, n);
		}
	};

	template<> struct _scopedPtrBase<true>
	{
		_scopedPtrBase(dynarray &) {}

		pointer Allocate(size_type n)
		{
			return Alloc{}.allocate(n);
		}
		void Deallocate(pointer p, size_type n)
		{
			Alloc{}.deallocate(p, n);
		}
	};

	struct _scopedPtr : private _scopedPtrBase< std::is_empty<Alloc>::value >
	{
		pointer ptr;
		pointer allocEnd;

		_scopedPtr(dynarray & owner, size_type allocSize) :
			_scopedPtrBase(owner),
			ptr(Allocate(allocSize)),
			allocEnd(ptr + allocSize) {
		}
		_scopedPtr(const _scopedPtr &) = delete;
		void operator =(const _scopedPtr &) = delete;
		~_scopedPtr()
		{
			if (ptr)
				Deallocate(ptr, allocEnd - ptr);
		}
	};

	struct _dynarrValues
	{
		pointer data;       // Pointer to beginning of data buffer
		pointer end;        // Pointer to one past the back object
		pointer reserveEnd; // Pointer to end of allocated memory
	};

	struct _dataOwner : public _dynarrValues, public Alloc
	{
		_dataOwner() : _dynarrValues(), Alloc() {
		}
		_dataOwner(const Alloc & a)
		 :	_dynarrValues(), Alloc(a) {
		}
		_dataOwner(const Alloc & a, size_type allocSize) : Alloc(a)
		{
			data = allocate(allocSize);
			reserveEnd = data + allocSize;
		}
		_dataOwner(_dataOwner && other)
		 :	_dynarrValues(other), Alloc(std::move(other))
		{
			other.reserveEnd = other.end = other.data = nullptr;
		}
		_dataOwner(const _dataOwner &) = delete;
		void operator =(const _dataOwner &) = delete;
		~_dataOwner()
		{
			if (data)
				deallocate(data, reserveEnd - data);
		}

	} _m;


	void _resetData(pointer const newData, size_type oldCapacity)
	{
		if (_m.data)
			_m.deallocate(_m.data, oldCapacity);

		_m.data = newData;
	}

	void _swapData(_scopedPtr & s)
	{
		using std::swap;
		swap(_m.data, s.ptr);
		swap(_m.reserveEnd, s.allocEnd);
	}

#if _MSC_VER && OEL_MEM_BOUND_DEBUG_LVL < 2 && _ITERATOR_DEBUG_LEVEL == 0
	#define OEL_FORCEINLINE __forceinline
#else
	#define OEL_FORCEINLINE inline
#endif

	size_type _unusedCapacity() const
	{
		return _m.reserveEnd - _m.end;
	}

	size_type _calcCapAddOne() const
	{
		enum { minGrow = sizeof(T *) >= sizeof(T) ?
				2 * sizeof(T *) / sizeof(T) :   // Want to grow by 2 * sizeof(T *) bytes,
				(sizeof(T) <= 2040 ? 2 : 1) };  // at least 2 elements if they fit in a 4K page
		// Growth factor is 1.5
		size_type c = capacity();
		return c + (std::max)(c / 2, size_type(minGrow));
	}

	size_type _calcCap(size_type const newSize) const
	{
		size_type c = capacity();
		return (std::max)(c + c / 2, newSize);
	}

	template<typename CntigusIter>
	pointer _uninitCopyData(std::true_type, CntigusIter const first, CntigusIter, size_type const count)
	{
		// Behaviour undefined by standard if first is null
		::memcpy(data(), to_pointer_contiguous(first), sizeof(T) * count);
		return _m.data + count;
	}

	template<typename InputIter>
	pointer _uninitCopyData(std::false_type, InputIter first, InputIter last, size_type)
	{
		return _detail::UninitCopy(first, last, data(), _m);
	}

	template<typename ForwardTravRange>
	void _init(const ForwardTravRange & src, forward_traversal_tag)
	{
		size_type const nElems = oel::count(src);
		_m.data = _m.allocate(nElems);
		_m.end = _uninitCopyData(is_trivially_copyable<T>(),
								 ::adl_begin(src), ::adl_end(src), nElems);
		_m.reserveEnd = _m.end;
	}

	template<typename InputRange>
	void _init(const InputRange & src, single_pass_traversal_tag)
	{
		for (auto && v : src)
			emplace_back( std::forward<decltype(v)>(v) );
	}

	void _eraseUnordered(std::true_type, iterator pos)
	{	// trivial relocate
		(*pos).~T();
		--_m.end;
		using RawStore = aligned_storage_t<sizeof(T), OEL_ALIGNOF(T)>;
		*reinterpret_cast<RawStore *>(to_pointer_contiguous(pos)) = reinterpret_cast<RawStore &>(*_m.end);
	}

	void _eraseUnordered(std::false_type, iterator pos)
	{
		*pos = std::move(*(_m.end - 1));
		pop_back();
	}

	template<typename CntigusIter>
	void _assignImpl(std::true_type, CntigusIter const first, CntigusIter, size_type const count)
	{	// fast assign
	#if OEL_MEM_BOUND_DEBUG_LVL
		if (count > 0)
		{	// Dereference to catch out of range errors if the iterators have internal checks
			(void)*first;
			(void)*(first + (count - 1));
		}
	#endif
		if (capacity() < count)
		{
			_resetData(_m.allocate(count), capacity());
			_m.end = _m.data + count;
			_m.reserveEnd = _m.end;
		}
		else
		{	_m.end = _m.data + count;
		}
		// Not portable. Check for self assignment or use memmove?
		::memcpy(data(), to_pointer_contiguous(first), sizeof(T) * count);
	}

	template<typename InputIter>
	void _assignImpl(std::false_type, InputIter first, InputIter const last, size_type const count)
	{	// non-trivial assign
		if (capacity() < count)
		{	// not enough room, allocate new array and construct new
			_scopedPtr newData{*this, count};
			_m.end = _detail::UninitCopy(first, last, newData.ptr, _m);
			_swapData(newData);
		}
		else if (size() >= count)
		{	// enough elements, copy new and destroy old
			iterator newEnd = std::copy(first, last, begin());
			erase_back(newEnd);
		}
		else
		{	// enough room, assign to old elements and construct rest
			for (pointer dest = _m.data; dest != _m.end; ++dest)
			{
				*dest = *first;
				++first;
			}
			_m.end = _detail::UninitCopy(first, last, _m.end, _m);
		}
	}

	template<typename ForwTravRange>
	void _assign(const ForwTravRange & src, forward_traversal_tag)
	{
		using IterSrc = decltype(::adl_begin(src));
		_assignImpl(can_memmove_with<pointer, IterSrc>(),
					::adl_begin(src), ::adl_end(src), oel::count(src));
	}

	template<typename InputRange>
	void _assign(const InputRange & src, single_pass_traversal_tag)
	{	// cannot count input objects before assigning
		clear();
		for (auto && v : src)
			emplace_back( std::forward<decltype(v)>(v) );
	}

	void _relocateData(std::false_type, T *const newData, T & pushedElem)
	{	// relocate elements by move constructor and destructor
		try
		{	_detail::UninitCopy(std::make_move_iterator(_m.data), std::make_move_iterator(_m.end), newData, _m);
		}
		catch (...)
		{
			pushedElem.~T();
			throw;
		}
		_detail::Destroy(_m.data, _m.end);
	}

	void _relocateData(std::true_type, T * newData, T &)
	{
		::memcpy(newData, data(), sizeof(T) * size());
	}

	template<typename... Args>
	void _emplaceBackRealloc(Args &&... args)
	{
		_scopedPtr newData{*this, _calcCapAddOne()};

		pointer const pos = newData.ptr + size();
		_allocTrait::construct(_m, pos, std::forward<Args>(args)...);

		_relocateData(is_trivially_relocatable<T>(), newData.ptr, *pos);

		_swapData(newData);
		_m.end = pos;
	}

	template<typename CopyFunc>
	OEL_FORCEINLINE iterator _appendImpl(size_type const count, CopyFunc makeNewElems)
	{
		_staticAssertRelocate();

		pointer pos;
		if (_unusedCapacity() >= count)
		{
			pos = _m.end;
			makeNewElems(_m.end, count, _m);
		}
		else
		{
			_scopedPtr newData{*this, _calcCap(size() + count)};

			size_type const oldSize = size();
			pos = newData.ptr + oldSize;
			makeNewElems(pos, count, _m);
			// Exception free from here
			::memcpy(newData.ptr, data(), sizeof(T) * oldSize);  // relocate old

			_m.end = pos;
			_swapData(newData);
		}
		_m.end += count;

		return OEL_DYNARR_ITERATOR(pos);
	}

	template<typename CntigusIter>
	OEL_FORCEINLINE CntigusIter _appendN(std::true_type, CntigusIter const first, size_type const count)
	{	// use memcpy
	#if OEL_MEM_BOUND_DEBUG_LVL
		CntigusIter last = first + count;
		if (count > 0)
		{	// Dereference to catch out of range errors if the iterators have internal checks
			(void)*first;
			(void)*(last - 1);
		}
	#endif
		_appendImpl( count,
				[first](T * dest, size_type nElems, Alloc &)
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
				[&first](T * dest, size_type nElems, Alloc & alloc)
				{
					first = _detail::UninitCopyN(first, nElems, dest, alloc).src_end;
				} );
		return first;
	}

	template<typename CntigusRange>
	OEL_FORCEINLINE iterator _append(std::true_type, forward_traversal_tag, const CntigusRange & src)
	{	// use memcpy
		auto const nElems = oel::count(src);
		_appendN(std::true_type{}, ::adl_begin(src), nElems);

		return end() - nElems;
	}

	template<typename ForwTravRange>
	iterator _append(std::false_type, forward_traversal_tag, const ForwTravRange & src)
	{	// multi-pass iterator
		auto first = ::adl_begin(src);
		auto last = ::adl_end(src);
		return _appendImpl( oel::count(src),
				[=](T * dest, size_type, Alloc & alloc)
				{
					_detail::UninitCopy(first, last, dest, alloc);
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

	template<typename... Args>
	iterator _emplaceRealloc(T *const pos, size_type const nAfterPos, Args &&... args)
	{
		_scopedPtr newData{*this, _calcCapAddOne()};

		size_type const nBeforePos = pos - data();
		T *const newPos = newData.ptr + nBeforePos;
		_allocTrait::construct(_m, newPos, std::forward<Args>(args)...); // add new
		// Exception free from here
		T *const next = newPos + 1;
		::memcpy(newData.ptr, data(), sizeof(T) * nBeforePos); // relocate prefix
		::memcpy(next, pos, sizeof(T) * nAfterPos);   // relocate suffix

		_m.end = next + nAfterPos;
		_swapData(newData);

		return OEL_DYNARR_ITERATOR(newPos);
	}

	template<typename UninitFillFunc>
	void _resizeImpl(size_type const newSize, UninitFillFunc initNewElems)
	{
		_staticAssertRelocate();

		if (newSize <= capacity())
		{
			pointer const newEnd = data() + newSize;
			if (_m.end < newEnd) // then construct new
				initNewElems(_m.end, newEnd, _m);
			else // downsizing
				_detail::Destroy(newEnd, _m.end);

			_m.end = newEnd;
		}
		else
		{	// not enough room, reallocate
			_scopedPtr newData{*this, _calcCap(newSize)};

			size_type const oldSize = size();
			pointer const newEnd = newData.ptr + newSize;
			initNewElems(newData.ptr + oldSize, newEnd, _m);
			// Exception free from here
			::memcpy(newData.ptr, data(), sizeof(T) * oldSize);  // relocate old

			_m.end = newEnd;
			_swapData(newData);
		}
	}
};

template<typename T, typename Alloc> template<typename... Args>
void dynarray<T, Alloc>::emplace_back(Args &&... args)
{
	if (_m.end < _m.reserveEnd)
		_allocTrait::construct(_m, _m.end, std::forward<Args>(args)...);
	else
		_emplaceBackRealloc(std::forward<Args>(args)...);

	++_m.end;
}

template<typename T, typename Alloc> template<typename... Args>
typename dynarray<T, Alloc>::iterator  dynarray<T, Alloc>::emplace(const_iterator pos, Args &&... args)
{
	_staticAssertRelocate();

	auto const posPtr = const_cast<T *>(to_pointer_contiguous(pos));
	OEL_BOUND_ASSERT_CHEAP(data() <= posPtr && posPtr <= _m.end);

	size_type const nAfterPos = _m.end - posPtr;

	if (_m.end < _m.reserveEnd) // then new element fits
	{
		// Temporary in case constructor throws or source is an element of this dynarray at pos or after
		using RawStore = aligned_storage_t<sizeof(T), OEL_ALIGNOF(T)>;
		RawStore tmp;
		_allocTrait::construct(_m, reinterpret_cast<T *>(&tmp), std::forward<Args>(args)...);
		// Move [pos, end) to [pos + 1, end + 1), conceptually destroying element at pos
		::memmove(posPtr + 1, posPtr, sizeof(T) * nAfterPos);
		++_m.end;

		*reinterpret_cast<RawStore *>(posPtr) = tmp; // relocate the new element to pos

		return OEL_DYNARR_ITERATOR(posPtr);
	}
	else
	{	return _emplaceRealloc(posPtr, nAfterPos, std::forward<Args>(args)...);
	}
}

template<typename T, typename Alloc>
inline dynarray<T, Alloc>::dynarray(reserve_tag, size_type capacity, const Alloc & alloc)
 :	_m(alloc, capacity)
{
	_m.end = _m.data;
}

template<typename T, typename Alloc>
dynarray<T, Alloc>::dynarray(size_type size, default_init_tag, const Alloc & alloc)
 :	_m(alloc, size)
{
	_m.end = _m.reserveEnd;
	_detail::UninitFillDefault(_m.data, _m.end, _m);
}

template<typename T, typename Alloc>
dynarray<T, Alloc>::dynarray(size_type size, const Alloc & alloc)
 :	_m(alloc, size)
{
	_m.end = _m.reserveEnd;
	_detail::UninitFill(_m.data, _m.end, _m);
}

template<typename T, typename Alloc>
dynarray<T, Alloc>::dynarray(size_type size, const T & val, const Alloc & alloc)
 :	_m(alloc, size)
{
	_m.end = _m.reserveEnd;
	_detail::UninitFill(_m.data, _m.end, _m, val);
}

template<typename T, typename Alloc>
inline dynarray<T, Alloc>::dynarray(dynarray && other) noexcept
 :	_m(std::move(other._m)) {
}

template<typename T, typename Alloc>
inline dynarray<T, Alloc>::dynarray(dynarray && other, const Alloc & alloc) noexcept
 :	_m(alloc)
{
	if (std::is_empty<Alloc>::value || alloc == other._m)
	{
		static_cast<dynarrValues &>(_m) = other._m;
		other._m.reserveEnd = other._m.end = other._m.data = nullptr;
	}
	else
	{
		OEL_MEM_BOUND_ASSERT(is_trivially_relocatable<T>::value || std::is_nothrow_move_constructible<T>::value);

		if (is_trivially_relocatable<T>::value)
		{
			::memcpy(_m.data, other.data(), sizeof(T) * other.size());
			_m.reserveEnd = _m.end = _m.data + other.size();
			// Elements in other conceptually destroyed
			other._m.end = other._m.data;
		}
		else
		{
			_m.end = _detail::UninitCopy(std::make_move_iterator(other.begin()), std::make_move_iterator(other.end()),
										 _m.data, _m);
			_m.reserveEnd = _m.end;
		}
	}
}

template<typename T, typename Alloc> template<typename InputRange>
dynarray<T, Alloc>::dynarray(from_range_tag, const InputRange & src, const Alloc & alloc)
 :	_m(alloc)
{
	using InIter = decltype(::adl_begin(src));
	_init(src, iterator_traversal_t<InIter>());
}

template<typename T, typename Alloc>
inline dynarray<T, Alloc>::dynarray(const dynarray & other)
 :	dynarray(from_range, other, _allocTrait::select_on_container_copy_construction(other._m)) {
}

template<typename T, typename Alloc>
dynarray<T, Alloc>::~dynarray() noexcept
{
	_detail::Destroy(_m.data, _m.end);
}

template<typename T, typename Alloc>
dynarray<T, Alloc> & dynarray<T, Alloc>::operator =(dynarray && other) noexcept
{
	OEL_CONST_COND if (_allocTrait::propagate_on_container_move_assignment::value || static_cast<Alloc &>(_m) == other._m)
	{
		using std::swap;
		swap(_m.data, other._m.data);
		swap(_m.end, other._m.end);
		swap(_m.reserveEnd, other._m.reserveEnd);
		OEL_CONST_COND if (_allocTrait::propagate_on_container_move_assignment::value)
			std::swap(static_cast<Alloc &>(_m), static_cast<Alloc &>(other._m));
	}
	else
	{
		OEL_MEM_BOUND_ASSERT(is_trivially_relocatable<T>::value || std::is_nothrow_move_assignable<T>::value);

		OEL_CONST_COND if (is_trivially_relocatable<T>::value)
		{
			_assignImpl(std::true_type{}, other.begin(), other.end(), other.size());
			other._m.end = other._m.data;
		}
		else
		{	assign(std::make_move_iterator(other.begin()), other.size());
		}
	}
	return *this;
}

template<typename T, typename Alloc>
dynarray<T, Alloc> & dynarray<T, Alloc>::operator =(const dynarray & other)
{
	OEL_CONST_COND if (_allocTrait::propagate_on_container_copy_assignment::value)
	{
		clear();
		if (_m.data)
			_m.deallocate(_m.data, capacity());

		static_cast<Alloc &>(_m) = other._m;
	}

	assign(other);
	return *this;
}

template<typename T, typename Alloc>
void dynarray<T, Alloc>::swap(dynarray & other) noexcept
{
	using std::swap;
	swap(_m.data, other._m.data);
	swap(_m.end, other._m.end);
	swap(_m.reserveEnd, other._m.reserveEnd);
	OEL_CONST_COND if (_allocTrait::propagate_on_container_swap::value)
		swap(static_cast<Alloc &>(_m), static_cast<Alloc &>(other._m));
}

template<typename T, typename Alloc> template<typename ForwardTravIterator>
ForwardTravIterator dynarray<T, Alloc>::assign(ForwardTravIterator first, size_type count)
{
	static_assert(std::is_convertible< iterator_traversal_t<ForwardTravIterator>, forward_traversal_tag >::value,
				  "Type of first must meet requirements of Forward Traversal Iterator (Boost concept)");

	ForwardTravIterator const last = std::next(first, count);
	_assignImpl(can_memmove_with<pointer, ForwardTravIterator>(),
				first, last, count);
	return last;
}

template<typename T, typename Alloc> template<typename InputRange>
inline void dynarray<T, Alloc>::assign(const InputRange & src)
{
	using InIter = decltype(::adl_begin(src));
	_assign(src, iterator_traversal_t<InIter>());
}

template<typename T, typename Alloc>
inline void dynarray<T, Alloc>::append(size_type count, const T & val)
{
	_appendImpl( count,
			[&val](T * dest, size_type nElems, Alloc & alloc)
			{
				_detail::UninitFill(dest, dest + nElems, alloc, val);
			} );
}

template<typename T, typename Alloc> template<typename InputIterator, typename>
OEL_FORCEINLINE InputIterator dynarray<T, Alloc>::append(InputIterator first, size_type count)
{
	return _appendN(can_memmove_with<pointer, InputIterator>(), first, count);
}

template<typename T, typename Alloc> template<typename InputRange>
OEL_FORCEINLINE typename dynarray<T, Alloc>::iterator  dynarray<T, Alloc>::append(const InputRange & src)
{
	using IterSrc = decltype(::adl_begin(src));
	return _append(can_memmove_with<pointer, IterSrc>(),
				   iterator_traversal_t<IterSrc>(),
				   src);
}

template<typename T, typename Alloc>
OEL_FORCEINLINE typename dynarray<T, Alloc>::iterator  dynarray<T, Alloc>::append(std::initializer_list<T> il)
{
	return append<>(il);
}

template<typename T, typename Alloc>
void dynarray<T, Alloc>::reserve(size_type minCapacity)
{
	_staticAssertRelocate();

	size_type const oldCap = capacity();
	if (oldCap < minCapacity)
	{
		pointer const newData = _m.allocate(minCapacity);

		_m.reserveEnd = newData + minCapacity;
		// Relocate elements
		::memcpy(newData, data(), sizeof(T) * size());
		_m.end = newData + size();

		_resetData(newData, oldCap);
	}
}

template<typename T, typename Alloc>
void dynarray<T, Alloc>::shrink_to_fit()
{
	_staticAssertRelocate();

	size_type const used = size();
	pointer newData;
	if (0 < used)
	{
		newData = _m.allocate(used);

		::memcpy(newData, data(), sizeof(T) * used); // relocate elements
		_m.end = newData + used;
	}
	else
	{	_m.end = newData = nullptr;
	}
	_resetData(newData, capacity());
	_m.reserveEnd = _m.end;
}

template<typename T, typename Alloc>
inline void dynarray<T, Alloc>::resize(size_type newSize, default_init_tag)
{
	_resizeImpl(newSize, _detail::UninitFillDefault<Alloc, T>);
}

template<typename T, typename Alloc>
inline void dynarray<T, Alloc>::resize(size_type newSize)
{
	_resizeImpl(newSize, _detail::UninitFill<Alloc, T>);
}

template<typename T, typename Alloc>
inline void dynarray<T, Alloc>::pop_back() noexcept
{
	OEL_MEM_BOUND_ASSERT(data() < _m.end);
	--_m.end;
	(*_m.end).~T();
}

template<typename T, typename Alloc>
inline typename dynarray<T, Alloc>::iterator  dynarray<T, Alloc>::erase_unordered(iterator pos)
{
	_eraseUnordered(is_trivially_relocatable<T>(), pos);
	return pos;
}

template<typename T, typename Alloc>
typename dynarray<T, Alloc>::iterator  dynarray<T, Alloc>::erase(iterator pos) noexcept
{
	_staticAssertRelocate();

	T *const posPtr = to_pointer_contiguous(pos);
	OEL_BOUND_ASSERT_CHEAP(data() <= posPtr && posPtr < _m.end);

	posPtr-> ~T();
	const T * next = posPtr + 1;
	::memmove(posPtr, next, sizeof(T) * (_m.end - next)); // relocate [pos + 1, end) to [pos, end - 1)
	--_m.end;

	return pos;
}

template<typename T, typename Alloc>
typename dynarray<T, Alloc>::iterator  dynarray<T, Alloc>::erase(iterator first, iterator last) noexcept
{
	_staticAssertRelocate();

	T *const pFirst = to_pointer_contiguous(first);
	T *const pLast = to_pointer_contiguous(last);
	OEL_BOUND_ASSERT_CHEAP(data() <= pFirst);
	OEL_MEM_BOUND_ASSERT(pFirst <= pLast && pLast <= _m.end);
	if (pFirst < pLast)
	{
		_detail::Destroy(pFirst, pLast);
		size_type const nAfterLast = _m.end - pLast;
		// Relocate [last, end) to [first, first + nAfterLast)
		::memmove(pFirst, pLast, sizeof(T) * nAfterLast);
		_m.end = pFirst + nAfterLast;
	}
	return first;
}

template<typename T, typename Alloc>
inline void dynarray<T, Alloc>::erase_back(iterator first) noexcept
{
	pointer const newEnd = to_pointer_contiguous(first);
	OEL_BOUND_ASSERT_CHEAP(data() <= newEnd && newEnd <= _m.end);
	_detail::Destroy(newEnd, _m.end);
	_m.end = newEnd;
}

template<typename T, typename Alloc>
inline typename dynarray<T, Alloc>::reference  dynarray<T, Alloc>::at(size_type index)
{
	const auto & cThis = *this;
	return const_cast<reference>(cThis.at(index));
}
template<typename T, typename Alloc>
typename dynarray<T, Alloc>::const_reference  dynarray<T, Alloc>::at(size_type index) const
{
	if (static_cast<_uSizeT>(size()) > static_cast<_uSizeT>(index))
		return _m.data[index];
	else
		throw out_of_range("Invalid index dynarray::at");
}

template<typename T, typename Alloc>
inline typename dynarray<T, Alloc>::reference  dynarray<T, Alloc>::operator[](size_type index) noexcept
{
	// cast needed because size_type could be signed
	OEL_MEM_BOUND_ASSERT(static_cast<_uSizeT>(size()) > static_cast<_uSizeT>(index));
	return _m.data[index];
}
template<typename T, typename Alloc>
inline typename dynarray<T, Alloc>::const_reference  dynarray<T, Alloc>::operator[](size_type index) const noexcept
{
	OEL_MEM_BOUND_ASSERT(static_cast<_uSizeT>(size()) > static_cast<_uSizeT>(index));
	return _m.data[index];
}

#undef OEL_DYNARR_ITERATOR
#undef OEL_DYNARR_CONST_ITER
#undef OEL_FORCEINLINE

} // namespace oel
