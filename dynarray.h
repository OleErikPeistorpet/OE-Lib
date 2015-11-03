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

////////////////////////////////////////////////////////////////////////////////

/// dynarray<dynarray<T>> is efficient
template<typename T, typename Alloc>
is_trivially_relocatable<Alloc> specify_trivial_relocate(dynarray<T, Alloc>);

template<typename T, typename A> inline
void swap(dynarray<T, A> & a, dynarray<T, A> & b) noexcept  { a.swap(b); }

/// Overloads generic erase_unordered(Container &, OutputIterator) (in util.h)
template<typename T, typename A> inline
typename dynarray<T, A>::iterator  erase_unordered(dynarray<T, A> & ctr, typename dynarray<T, A>::iterator position)
	{ return ctr.erase_unordered(position); }

/// Overloads generic erase_back(Container &, Container::iterator) (in util.h)
template<typename T, typename A> inline
void erase_back(dynarray<T, A> & ctr, typename dynarray<T, A>::iterator first) noexcept  { ctr.erase_back(first); }

/**
* @brief Resizable array, dynamically allocated. Very similar to std::vector, but much faster in many cases.
*
* Efficiency is better if template argument T is trivially relocatable, which is true for most types, but needs
* to be declared manually for each type that is not trivially copyable. See specify_trivial_relocate(T &&).
* There are a few notable exceptions for which trivially relocatable T is required (checked when compiling):
* insert and its variations, and erase(iterator, iterator)
*
* The default allocator supports over-aligned types (e.g. __m256).
* In general, only that which differs from std::vector is documented. */
template<typename T, typename Alloc/* = oel::allocator<T> */>
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

#if OEL_MEM_BOUND_DEBUG_LVL
	using iterator       = contiguous_ctnr_iterator< pointer, dynarray<T, Alloc> >;
	using const_iterator = contiguous_ctnr_iterator< const_pointer, dynarray<T, Alloc> >;
#else
	using iterator       = pointer;
	using const_iterator = const_pointer;
#endif
	using reverse_iterator       = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;


	dynarray() noexcept                              : _m(Alloc{}) {}
	explicit dynarray(const Alloc & alloc) noexcept  : _m(alloc) {}

	/// Construct empty dynarray with space reserved for at least capacity elements
	dynarray(reserve_tag, size_type capacity, const Alloc & alloc = Alloc{})  : _m(alloc, capacity) {}

	/** @brief Uses default initialization for elements, can be significantly faster for non-class T
	*
	* Non-class T objects get indeterminate values. http://en.cppreference.com/w/cpp/language/default_initialization  */
	dynarray(size_type size, default_init_tag, const Alloc & alloc = Alloc{});
	explicit dynarray(size_type size, const Alloc & alloc = Alloc{});  ///< (Value-initializes elements, same as std::vector)
	dynarray(size_type size, const T & fillVal, const Alloc & alloc = Alloc{});

	dynarray(std::initializer_list<T> init, const Alloc & alloc = Alloc{});

	/** @brief Equivalent to std::vector(begin(source), sLast, alloc),
	*	where sLast is either begin + source.size() or end(source)
	*
	* If you need to construct from some std::istream, check out boost/range/istream_range.hpp  */
	template<typename InputRange>
	dynarray(from_range_tag, const InputRange & source, const Alloc & alloc = Alloc{})  : _m(alloc) { assign(source); }

	dynarray(dynarray && other) noexcept    : _m(std::move(other._m)) {}
	/// If using custom Alloc, behaviour is undefined (assertion unless NDEBUG)
	/// when alloc != other.get_allocator() and T is not trivially relocatable
	dynarray(dynarray && other, const Alloc & alloc) noexcept;
	dynarray(const dynarray & other);
	dynarray(const dynarray & other, const Alloc & alloc);

	~dynarray() noexcept;

	/// If using custom Alloc with propagate_on_container_move_assignment false, behaviour is undefined
	/// when get_allocator() != other.get_allocator() and T is not trivially relocatable
	dynarray & operator =(dynarray && other) noexcept;
	dynarray & operator =(const dynarray & other);

	dynarray & operator =(std::initializer_list<T> il)  { assign(il);  return *this; }

	void       swap(dynarray & other) noexcept;

	/**
	* @brief Replace the contents with source range
	* @param source an array, STL container, gsl::array_view, boost::iterator_range or such
	*	Shall not be a subset of same dynarray, except if begin(source) points to first element of dynarray.
	* @return iterator begin(source) incremented to equal the end of source range
	*
	* Any elements held before the call are either assigned to or destroyed. */
	template<typename InputRange>
	auto   assign(const InputRange & source) -> decltype(::adl_begin(source));

	/**
	* @brief Add at end the elements from range (in same order), return iterator to new
	* @param source an array, STL container, gsl::array_view, boost::iterator_range or such. Can be this dynarray.
	* @return iterator pointing to first of the new elements in dynarray, or end if source is empty
	*
	* Strong exception guarantee, this function has no effect if an exception is thrown.
	* Otherwise equivalent to std::vector::insert(end(), begin(source), sourceBeginPlusSizeOrEnd)  */
	template<typename InputRange>
	iterator   append(const InputRange & source);
	/**
	* @brief Same as append(const InputRange &), except returning past-the-last of source
	* @return begin(source) incremented to end of source. The iterator is already invalidated (do not dereference) if
	*	first pointed into same dynarray and there was insufficient capacity to avoid reallocation. */
	template<typename InputRange>
	auto   append_ret_src(const InputRange & source) -> decltype(::adl_begin(source));
	/// Equivalent to calling append(const InputRange &) with il as argument
	iterator   append(std::initializer_list<T> il);
	/// Equivalent to std::vector::insert(end(), count, val), but with strong exception guarantee
	void       append(size_type count, const T & val);

	/**
	* @brief Uses default initialization for added elements, can be significantly faster for non-class T
	*
	* Non-class T objects get indeterminate values. http://en.cppreference.com/w/cpp/language/default_initialization  */
	void       resize(size_type newSize, default_init_tag)  { _resizeImpl(newSize, _detail::UninitFillDefault<Alloc, T>); }
	/// (Value-initializes added elements, same as std::vector::resize)
	void       resize(size_type newSize)                    { _resizeImpl(newSize, _detail::UninitFill<Alloc, T>); }

	/// Equivalent to std::vector::insert(position, begin(source), sLast),
	/// where sLast is either begin + source.size() or end(source)
	template<typename ForwardRange>
	iterator   insert_m(const_iterator position, const ForwardRange & source);
	/**
	* @brief Same as insert_m(const_iterator, const ForwardRange &), except returning past-the-last of source
	* @return iterator begin(source) incremented to equal the end of source range  */
	template<typename ForwardRange>
	auto   insert_ret_src(const_iterator position, const ForwardRange & source) -> decltype(::adl_begin(source));

	/// Performs list-initialization of element if there is no matching constructor
	template<typename... Args>
	iterator   emplace(const_iterator position, Args &&... elemInitArgs);

	iterator   insert(const_iterator position, T && val)       { return emplace(position, std::move(val)); }
	iterator   insert(const_iterator position, const T & val)  { return emplace(position, val); }

	/// Performs list-initialization of element if there is no matching constructor
	template<typename... Args>
	void       emplace_back(Args &&... elemInitArgs);

	void       push_back(T && val)       { emplace_back(std::move(val)); }
	void       push_back(const T & val)  { emplace_back(val); }

	/// After the call, any previous iterator to the back element will be equal to end()
	void       pop_back() noexcept;

	/**
	* @brief Erase the element at position from dynarray without maintaining order of elements.
	*
	* Constant complexity (compared to linear in the distance between position and last for normal erase).
	* @return Iterator pointing to the location that followed the element erased,
	*	which is the end if position was at the last element. */
	iterator   erase_unordered(iterator position);

	iterator   erase(iterator position);

	iterator   erase(iterator first, iterator last) noexcept;

	/// Equivalent to erase(first, end()) (but potentially faster), making first the new end
	void       erase_back(iterator first) noexcept;

	void       clear() noexcept        { erase_back(begin()); }

	bool       empty() const noexcept  { return _m.data == _m.end; }

	size_type  size() const noexcept   { return _m.end - _m.data; }

	void       reserve(size_type minCapacity)  { if (capacity() < minCapacity) _growTo(minCapacity); }

	/// It's a good idea to check that size() < capacity() before calling to avoid useless reallocation
	void       shrink_to_fit();

	size_type  capacity() const noexcept  { return _m.reserveEnd - _m.data; }

	allocator_type get_allocator() const  { return _m; }

	iterator        begin() noexcept         { return _makeIterator(_m.data, this); }
	const_iterator  begin() const noexcept   { return _makeConstIter(_m.data, this); }
	const_iterator  cbegin() const noexcept  { return begin(); }

	iterator        end() noexcept           { return _makeIterator(_m.end, this); }
	const_iterator  end() const noexcept     { return _makeConstIter(_m.end, this); }
	const_iterator  cend() const noexcept    { return end(); }

	reverse_iterator       rbegin() noexcept        { return reverse_iterator{end()}; }
	const_reverse_iterator rbegin() const noexcept  { return const_reverse_iterator{end()}; }

	reverse_iterator       rend() noexcept          { return reverse_iterator{begin()}; }
	const_reverse_iterator rend() const noexcept    { return const_reverse_iterator{begin()}; }

	T *             data() noexcept        { return _m.data; }
	const T *       data() const noexcept  { return _m.data; }

	reference       front() noexcept        { return *begin(); }
	const_reference front() const noexcept  { return *begin(); }

	reference       back() noexcept         { return *_makeIterator(_m.end - 1, this); }
	const_reference back() const noexcept   { return *_makeConstIter(_m.end - 1, this); }

	reference       at(size_type index);
	const_reference at(size_type index) const;

	reference       operator[](size_type index) noexcept;
	const_reference operator[](size_type index) const noexcept;

	friend bool operator==(const dynarray & left, const dynarray & right)
	{
		return left.size() == right.size() &&
			   std::equal(left.begin(), left.end(), right.begin());
	}
	friend bool operator!=(const dynarray & left, const dynarray & right)  { return !(left == right); }



////////////////////////////////////////////////////////////////////////////////
//
// Implementation only in rest of the file


private:
#if OEL_MEM_BOUND_DEBUG_LVL
	using _makeIterator  = iterator;
	using _makeConstIter = const_iterator;
#else
	static iterator       _makeIterator(pointer pos, dynarray *)              { return pos; }
	static const_iterator _makeConstIter(const_pointer pos, const dynarray *) { return pos; }
#endif

#if _MSC_VER && OEL_MEM_BOUND_DEBUG_LVL == 0 && _ITERATOR_DEBUG_LEVEL == 0
	#define OEL_FORCEINLINE __forceinline
#else
	#define OEL_FORCEINLINE inline
#endif

	using _allocTrait = std::allocator_traits<Alloc>;

	struct _assertNothrowMoveConstruct
	{
		OEL_WHEN_EXCEPTIONS_ON(
			static_assert(std::is_nothrow_move_constructible<T>::value || is_trivially_relocatable<T>::value,
				"This function requires that T is noexcept move constructible or trivially relocatable") );
	};


	using _scopedPtrBase = _detail::AllocRefOptimizeEmpty< Alloc, std::is_empty<Alloc>::value >;

	struct _scopedPtr : private _scopedPtrBase
	{
		pointer ptr;
		pointer allocEnd;

		_scopedPtr(dynarray & parent, size_type const allocSize)
		 :	_scopedPtrBase{parent._m}
		{
			ptr = this->Get().allocate(allocSize);
			allocEnd = ptr + allocSize;
		}
		_scopedPtr(const _scopedPtr &) = delete;
		void operator =(const _scopedPtr &) = delete;
		~_scopedPtr()
		{
			if (ptr)
				this->Get().deallocate(ptr, allocEnd - ptr);
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
		using _dynarrValues::data;
		using _dynarrValues::end;
		using _dynarrValues::reserveEnd;

		_dataOwner(const Alloc & a)
		 :	_dynarrValues(), Alloc(a) {
		}
		_dataOwner(const Alloc & a, size_type const capacity)
		 :	Alloc(a)
		{
			end = data = this->allocate(capacity);
			reserveEnd = data + capacity;
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
				this->deallocate(data, reserveEnd - data);
		}

	} _m; // One and only data member of dynarray


	void _resetData(pointer const newData, size_type oldCapacity)
	{
		if (_m.data)
			_m.deallocate(_m.data, oldCapacity);

		_m.data = newData;
	}

	void _swapBuf(_scopedPtr & s)
	{
		using std::swap;
		swap(_m.data, s.ptr);
		swap(_m.reserveEnd, s.allocEnd);
	}

	void _moveAssignAlloc(std::true_type, _dataOwner & d)
	{
		static_cast<Alloc &>(_m) = std::move(d);
	}

	void _moveAssignAlloc(std::false_type, _dataOwner &) {}

	void _swapAlloc(std::true_type, _dataOwner & d)
	{
		using std::swap;
		swap(static_cast<Alloc &>(_m), static_cast<Alloc &>(d));
	}

	void _swapAlloc(std::false_type, _dataOwner & d)
	{	// propagate_on_container_swap false, standard says this is undefined if allocators compare unequal
		OEL_ASSERT(get_allocator() == static_cast<Alloc &>(d));
	}


	template<typename InputIter>
	InputIter _uninitCopy(std::false_type, InputIter first, size_type, T * dest, T * destEnd)
	{	// cannot use memcpy
		return _detail::UninitCopy(first, dest, destEnd, _m);
	}

	template<typename CntigusIter>
	CntigusIter _uninitCopy(std::true_type, CntigusIter const first, size_type const count, T * dest, T *)
	{
		// Behaviour undefined by standard if first is null
		::memcpy(dest, to_pointer_contiguous(first), sizeof(T) * count);
		return first + count;
	}

	void _initPostAllocate(const T *const first, size_type const count)
	{
		_m.end = _m.reserveEnd;
		_uninitCopy(is_trivially_copyable<T>(), first, count, data(), _m.end);
	}


	bool _indexValid(size_type index) const
	{
		using USizeT = make_unsigned_t<std::ptrdiff_t>;
		return static_cast<USizeT>(index) < static_cast<USizeT>(_m.end - _m.data);
	}

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

	size_type _calcCapMin(size_type const newSize) const
	{
		size_type c = capacity();
		return (std::max)(c + c / 2, newSize);
	}


	template<typename FuncTakingLast = _detail::NoOp, typename T1 = T>
	enable_if_t<is_trivially_relocatable<T1>::value == false>
		_relocateData(T * dFirst, T * dLast, size_type, FuncTakingLast extraCleanupIfException = {})
	{
		_detail::UninitCopy(std::make_move_iterator(_m.data), dFirst, dLast, _m, extraCleanupIfException);
		_detail::Destroy(_m.data, _m.end);
	}

	template<typename... Unused, typename T1 = T>
	enable_if_t<is_trivially_relocatable<T1>::value>
		_relocateData(T * dest, T *, size_type nElems, Unused...)
	{
		::memcpy(dest, data(), sizeof(T) * nElems);
	}


	void _growTo(size_type newCapacity)
	{
		_scopedPtr newData{*this, newCapacity};

		pointer const newEnd = newData.ptr + size();
		_relocateData(newData.ptr, newEnd, size());

		_m.end = newEnd;
		_swapBuf(newData);
	}

	template<typename UninitFillFunc>
	void _resizeImpl(size_type const newSize, UninitFillFunc initNewElems)
	{	// note: initNewElems cannot hold a reference to element of this
		if (capacity() < newSize)
			_growTo(_calcCapMin(newSize));

		T *const newEnd = data() + newSize;
		if (_m.end < newEnd) // then construct new
			initNewElems(_m.end, newEnd, _m);
		else // downsizing
			_detail::Destroy(newEnd, _m.end);

		_m.end = newEnd;
	}


	void _eraseUnordered(std::true_type /*trivialRelocate*/, iterator pos)
	{
		OEL_ASSERT_MEM_BOUND( _indexValid(pos - begin()) ); // pos must be an iterator of this, not another dynarray

		T & elem = *pos;
		elem.~T();
		--_m.end;
		using RawStore = aligned_storage_t<sizeof(T), OEL_ALIGNOF(T)>;
		reinterpret_cast<RawStore &>(elem) = reinterpret_cast<RawStore &>(*_m.end);
	}

	void _eraseUnordered(std::false_type, iterator pos)
	{
		*pos = std::move(back());
		pop_back();
	}

	void _erase(std::true_type /*trivialRelocate*/, iterator const pos)
	{
		T *const ptr = to_pointer_contiguous(pos);
		OEL_ASSERT_MEM_BOUND(_m.data <= ptr && ptr < _m.end);

		ptr-> ~T();
		const T * next = ptr + 1;
		::memmove(ptr, next, sizeof(T) * (_m.end - next)); // relocate [pos + 1, end) to [pos, end - 1)
		--_m.end;
	}

	void _erase(std::false_type, iterator pos)
	{
		_m.end = to_pointer_contiguous(std::move(pos + 1, end(), pos));
		(*_m.end).~T();
	}


	template<typename Range, typename IterTrav> // pass dummy int to prefer this overload
	static auto _count(const Range & r, IterTrav, int) -> decltype( static_cast<size_type>(oel::ssize(r)) )
														   { return static_cast<size_type>(oel::ssize(r)); }
	template<typename Range>
	static size_type _count(const Range & r, forward_traversal_tag, long)
	{
		return std::distance(::adl_begin(r), ::adl_end(r));
	}

	template<typename Range>
	static std::false_type _count(const Range &, single_pass_traversal_tag, long) { return {}; }

	// Potentially confusing: return either size_type or false_type
	template<typename Iter, typename Range>
	static auto _count(const Range & r) -> decltype(_count(r, iterator_traversal_t<Iter>(), int{}))
										   { return _count(r, iterator_traversal_t<Iter>(), int{}); }


	void _moveUnequalAlloc(dynarray & src)
	{
		OEL_ASSERT(is_trivially_relocatable<T>::value);

		_assignImpl(std::true_type{}, src.begin(), src.size());
		src._m.end = src._m.data; // elements in src conceptually destroyed
	}

	template<typename CntigusIter>
	CntigusIter _assignImpl(std::true_type /*trivialCopy*/, CntigusIter const first, size_type const count)
	{
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

		return first + count;
	}

	template<typename InputIter>
	InputIter _assignImpl(std::false_type, InputIter src, size_type const count)
	{
		auto copy = [](InputIter src, iterator dest, iterator dLast)
			{
				while (dest != dLast)
				{
					*dest = *src;
					++src; ++dest;
				}
				return src;
			};
		if (capacity() < count)
		{	// not enough room, allocate new array and construct new
			_scopedPtr newData{*this, count};

			T *const newEnd = newData.ptr + count;
			src = _detail::UninitCopy(src, newData.ptr, newEnd, _m);
			_detail::Destroy(_m.data, _m.end);

			_m.end = newEnd;
			_swapBuf(newData);
		}
		else if (size() >= count)
		{	// enough elements, copy new and destroy old
			iterator const newEnd = begin() + count;
			src = copy(src, begin(), newEnd);
			erase_back(newEnd);
		}
		else
		{	// enough room, assign to old elements and construct rest
			src = copy(src, begin(), end());
			T *const newEnd = data() + count;
			src = _detail::UninitCopy(src, _m.end, newEnd, _m);
			_m.end = newEnd;
		}
		return src;
	}

	template<typename InputIter, typename InputRange>
	InputIter _assign(const InputRange & src, size_type count)
	{
		return _assignImpl(can_memmove_with<T *, InputIter>(), ::adl_begin(src), count);
	}

	template<typename InputIter, typename InputRange>
	InputIter _assign(const InputRange & src, std::false_type)
	{	// single pass iterator, no size available
		clear();
		InputIter it = ::adl_begin(src);
		for (auto last = ::adl_end(src); it != last; ++it)
			emplace_back(*it);

		return it;
	}

	template<typename Ret, typename InputIter, typename InputRange, typename RetSelect>
	Ret _append(const InputRange & src, std::false_type, RetSelect retSelect)
	{	// single pass iterator, no size available
		size_type const oldSize = size();
		InputIter it = ::adl_begin(src);
		OEL_TRY_
		{
			for (auto last = ::adl_end(src); it != last; ++it)
				emplace_back(*it);
		}
		OEL_CATCH_ALL
		{
			erase_back(begin() + oldSize);
			OEL_WHEN_EXCEPTIONS_ON(throw);
		}
		return retSelect(begin() + oldSize, it);
	}

	template<typename Ret, typename InputIter, typename InputRange, typename RetSelect>
	OEL_FORCEINLINE Ret _append(const InputRange & src, size_type count, RetSelect retSelect)
	{
		auto last = _appendN(can_memmove_with<T *, InputIter>(), ::adl_begin(src), count);
		return retSelect(end() - count, last);
	}

	template<typename InputIter>
	InputIter _appendN(std::false_type, InputIter first, size_type const count)
	{	// cannot use memcpy
		_appendImpl( count,
			[&first](T * dest, size_type count, Alloc & al)
			{
				first = _detail::UninitCopy(first, dest, dest + count, al);
			} );
		return first;
	}

	template<typename CntigusIter>
	OEL_FORCEINLINE CntigusIter _appendN(std::true_type, CntigusIter const first, size_type const count)
	{
		CntigusIter last = first + count;
	#if OEL_MEM_BOUND_DEBUG_LVL
		if (count > 0)
		{	// Dereference to catch out of range errors if the iterators have internal checks
			(void)*first;
			(void)*(last - 1);
		}
	#endif
		_appendImpl( count,
			[first](T * dest, size_type count, Alloc &)
			{	// Behaviour undefined by standard if first points to null
				::memcpy(dest, to_pointer_contiguous(first), sizeof(T) * count);
			} );
		return last; // has been invalidated in the case of append self and reallocation
	}

	template<typename CopyOrFillFunc>
	OEL_FORCEINLINE void _appendImpl(size_type const count, CopyOrFillFunc makeNewElems)
	{
		_assertNothrowMoveConstruct();

		if (_unusedCapacity() >= count)
		{
			makeNewElems(_m.end, count, _m);
		}
		else
		{
			_scopedPtr newData{*this, _calcCapMin(size() + count)};

			size_type const oldSize = size();
			pointer pos = newData.ptr + oldSize;
			makeNewElems(pos, count, _m);
			// Exception free from here
			_relocateData(newData.ptr, pos, oldSize);

			_m.end = pos;
			_swapBuf(newData);
		}
		_m.end += count;
	}


	template<typename CalcCapFunc, typename MakeFunc, typename... Args>
	T * _insertRealloc(T *const pos, size_type const nAfterPos, size_type const nToAdd,
							CalcCapFunc calcCap, MakeFunc addNew, Args &&... args)
	{
		_scopedPtr newData{*this, calcCap(*this, size() + nToAdd)};

		size_type const nBeforePos = pos - data();
		T *const newPos = newData.ptr + nBeforePos;
		T *const next = addNew(*this, newPos, nToAdd, std::forward<Args>(args)...);
		// Exception free from here
		::memcpy(newData.ptr, data(), sizeof(T) * nBeforePos); // relocate prefix
		::memcpy(next, pos, sizeof(T) * nAfterPos);   // relocate suffix

		_m.end = next + nAfterPos;
		_swapBuf(newData);

		return newPos;
	}

	struct _emplaceMakeElem
	{	template<typename... Args>
		T * operator()(dynarray & da, T *const newPos, size_type, Args &&... args) const
		{
			_allocTrait::construct(da._m, newPos, std::forward<Args>(args)...);
			return newPos + 1;
		}
	};

	template<typename Range>
	auto _insertImpl(const_iterator pos, const Range & src) -> std::pair<iterator, decltype(::adl_begin(src))>
	{
	#define OEL_DYNARR_INSERT_STEP0  \
		_detail::AssertTrivialRelocate<T>();  \
		\
		auto pPos = const_cast<T *>(to_pointer_contiguous(pos));  \
		OEL_ASSERT_MEM_BOUND(_m.data <= pPos && pPos <= _m.end);  \
		size_type const nAfterPos = _m.end - pPos;

		OEL_DYNARR_INSERT_STEP0
		auto first = ::adl_begin(src);
		auto nElems = _count<decltype(first)>(src);

		static_assert(!std::is_same<decltype(nElems), std::false_type>::value,
					  "insert_m/insert_ret_src requires that source models Forward Range (Boost concept)");
		using CanMemmove = can_memmove_with<T *, decltype(first)>;
		if (_unusedCapacity() >= nElems)
		{
			T *const dLast = pPos + nElems;
			// Relocate elements to make space, conceptually destroying [pos, pos + nElems)
			::memmove(dLast, pPos, sizeof(T) * nAfterPos);
			_m.end += nElems;
			// Construct new
			OEL_CONST_COND if (CanMemmove::value)
			{
				first = _uninitCopy(CanMemmove(), first, nElems, pPos, dLast);
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
					::memmove(dest, dLast, sizeof(T) * nAfterPos);
					_m.end -= (dLast - dest);
					OEL_WHEN_EXCEPTIONS_ON(throw);
				}
			}
		}
		else // not enough room, reallocate
		{
			pPos = _insertRealloc( pPos, nAfterPos, nElems,
					[](const dynarray & self, size_type newSize) { return self._calcCapMin(newSize); },
					[&first](dynarray & self, T * newPos, size_type nElems)
					{
						T *const next = newPos + nElems;
						first = self._uninitCopy(CanMemmove(), first, nElems, newPos, next);
						return next;
					} );
		}
		return {_makeIterator(pPos, this), first};
	}
};

template<typename T, typename Alloc> template<typename... Args>
typename dynarray<T, Alloc>::iterator  dynarray<T, Alloc>::emplace(const_iterator pos, Args &&... args)
{
	OEL_DYNARR_INSERT_STEP0

	if (_m.end < _m.reserveEnd) // then new element fits
	{
		// Temporary in case constructor throws or source is an element of this dynarray at pos or after
		using RawStore = aligned_storage_t<sizeof(T), OEL_ALIGNOF(T)>;
		RawStore tmp;
		_allocTrait::construct(_m, reinterpret_cast<T *>(&tmp), std::forward<Args>(args)...);
		// Relocate [pos, end) to [pos + 1, end + 1), conceptually destroying element at pos
		::memmove(pPos + 1, pPos, sizeof(T) * nAfterPos);
		++_m.end;

		*reinterpret_cast<RawStore *>(pPos) = tmp; // relocate the new element to pos
	}
	else
	{	pPos = _insertRealloc( pPos, nAfterPos, {},
				[](const dynarray & self, size_type) { return self._calcCapAddOne(); },
				_emplaceMakeElem{}, std::forward<Args>(args)... );
	}
	return _makeIterator(pPos, this);
}
#undef OEL_DYNARR_INSERT_STEP0

template<typename T, typename Alloc> template<typename... Args>
void dynarray<T, Alloc>::emplace_back(Args &&... args)
{
	if (_m.end < _m.reserveEnd)
	{	_allocTrait::construct(_m, _m.end, std::forward<Args>(args)...);
	}
	else
	{
		_scopedPtr newData{*this, _calcCapAddOne()};

		pointer const pos = newData.ptr + size();
		_allocTrait::construct(_m, pos, std::forward<Args>(args)...);
#if _MSC_VER
	#pragma warning(suppress : 4100) // unreferenced formal parameter
#endif
		_relocateData( newData.ptr, pos, size(),
					   [](T * pos) { pos-> ~T(); } );
		_swapBuf(newData);
		_m.end = pos;
	}
	++_m.end;
}


template<typename T, typename Alloc>
dynarray<T, Alloc>::dynarray(dynarray && other, const Alloc & alloc) noexcept
 :	_m(alloc)
{
	if (alloc != other._m && !std::is_empty<Alloc>::value)
	{	_moveUnequalAlloc(other);
	}
	else
	{
		static_cast<_dynarrValues &>(_m) = other._m;
		other._m.reserveEnd = other._m.end = other._m.data = nullptr;
	}
}

template<typename T, typename Alloc>
dynarray<T, Alloc> & dynarray<T, Alloc>::operator =(dynarray && other) noexcept
{
	if (static_cast<Alloc &>(_m) != other._m &&
		!_allocTrait::propagate_on_container_move_assignment::value)
	{
		_detail::Destroy(_m.data, _m.end);
		_moveUnequalAlloc(other);
	}
	else if (this != &other)
	{
		if (_m.data)
		{
			_detail::Destroy(_m.data, _m.end);
			_m.deallocate(_m.data, capacity());
		}
		static_cast<_dynarrValues &>(_m) = other._m;
		_moveAssignAlloc(_allocTrait::propagate_on_container_move_assignment(), other._m);
		other._m.reserveEnd = other._m.end = other._m.data = nullptr;
	}
	return *this;
}

template<typename T, typename Alloc>
inline dynarray<T, Alloc> & dynarray<T, Alloc>::operator =(const dynarray & other)
{
	// No support for allocators that propagate on container copy assignment and compare unequal
	OEL_ASSERT(!_allocTrait::propagate_on_container_copy_assignment::value || get_allocator() == other.get_allocator());

	assign(other);
	return *this;
}

template<typename T, typename Alloc>
inline dynarray<T, Alloc>::dynarray(const dynarray & other)
 :	dynarray(other, _allocTrait::select_on_container_copy_construction(other._m)) {
}

template<typename T, typename Alloc>
inline dynarray<T, Alloc>::dynarray(const dynarray & other, const Alloc & alloc)
 :	_m(alloc, other.size())
{
	_initPostAllocate(other.data(), other.size());
}

template<typename T, typename Alloc>
inline dynarray<T, Alloc>::dynarray(std::initializer_list<T> init, const Alloc & alloc)
 :	_m(alloc, init.size())
{
	_initPostAllocate(init.begin(), init.size());
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
dynarray<T, Alloc>::~dynarray() noexcept
{
	_detail::Destroy(_m.data, _m.end);
}

template<typename T, typename Alloc>
void dynarray<T, Alloc>::swap(dynarray & other) noexcept
{
	std::swap(static_cast<_dynarrValues &>(_m),
			  static_cast<_dynarrValues &>(other._m));
	_swapAlloc(_allocTrait::propagate_on_container_swap(), other._m);
}


template<typename T, typename Alloc>
void dynarray<T, Alloc>::shrink_to_fit()
{
	_assertNothrowMoveConstruct();

	size_type const used = size();
	pointer newData;
	if (0 < used)
	{
		newData = _m.allocate(used);

		pointer const newEnd = newData + used;
		_relocateData(newData, newEnd, used);
		_m.end = newEnd;
	}
	else
	{	_m.end = newData = nullptr;
	}
	_resetData(newData, capacity());
	_m.reserveEnd = _m.end;
}


template<typename T, typename Alloc> template<typename InputRange>
inline auto dynarray<T, Alloc>::assign(const InputRange & src) -> decltype(::adl_begin(src))
{
	using IterSrc = decltype(::adl_begin(src));
	return _assign<IterSrc>(src, _count<IterSrc>(src));
}

template<typename T, typename Alloc>
inline void dynarray<T, Alloc>::append(size_type count, const T & val)
{
	_appendImpl( count,
		[&val](T * dest, size_type count, Alloc & alloc)
		{
			_detail::UninitFill(dest, dest + count, alloc, val);
		} );
}

template<typename T, typename Alloc> template<typename InputRange>
OEL_FORCEINLINE typename dynarray<T, Alloc>::iterator  dynarray<T, Alloc>::append(const InputRange & src)
{
	using IterSrc = decltype(::adl_begin(src));
	return _append<iterator, IterSrc>( src, _count<IterSrc>(src),
									   [](iterator newPos, IterSrc) { return newPos; } );
}

template<typename T, typename Alloc> template<typename InputRange>
OEL_FORCEINLINE auto dynarray<T, Alloc>::append_ret_src(const InputRange & src) -> decltype(::adl_begin(src))
{
	using IterSrc = decltype(::adl_begin(src));
	return _append<IterSrc, IterSrc>( src, _count<IterSrc>(src),
									  [](iterator, IterSrc sLast) { return sLast; } );
}

template<typename T, typename Alloc>
OEL_FORCEINLINE typename dynarray<T, Alloc>::iterator  dynarray<T, Alloc>::append(std::initializer_list<T> il)
{
	return append<>(il);
}

template<typename T, typename Alloc> template<typename ForwardRange>
inline typename dynarray<T, Alloc>::iterator  dynarray<T, Alloc>::
	insert_m(const_iterator pos, const ForwardRange & src)
{
	return _insertImpl(pos, src).first;
}

template<typename T, typename Alloc> template<typename ForwardRange>
inline auto dynarray<T, Alloc>::insert_ret_src(const_iterator pos, const ForwardRange & src)
 -> decltype(::adl_begin(src))
{
	return _insertImpl(pos, src).second;
}


template<typename T, typename Alloc>
inline void dynarray<T, Alloc>::pop_back() noexcept
{
	OEL_ASSERT_MEM_BOUND(_m.data < _m.end);
	--_m.end;
	(*_m.end).~T();
}

template<typename T, typename Alloc>
inline void dynarray<T, Alloc>::erase_back(iterator first) noexcept
{
	pointer const newEnd = to_pointer_contiguous(first);
	OEL_ASSERT_MEM_BOUND(_m.data <= newEnd && newEnd <= _m.end);
	_detail::Destroy(newEnd, _m.end);
	_m.end = newEnd;
}

template<typename T, typename Alloc>
typename dynarray<T, Alloc>::iterator  dynarray<T, Alloc>::erase(iterator first, iterator last) noexcept
{
	_detail::AssertTrivialRelocate<T>();

	T *const pFirst = to_pointer_contiguous(first);
	T *const pLast = to_pointer_contiguous(last);
	OEL_ASSERT_MEM_BOUND(_m.data <= pFirst && pFirst <= pLast && pLast <= _m.end);
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
inline typename dynarray<T, Alloc>::iterator  dynarray<T, Alloc>::erase(iterator pos)
{
	_erase(is_trivially_relocatable<T>(), pos);
	return pos;
}

template<typename T, typename Alloc>
inline typename dynarray<T, Alloc>::iterator  dynarray<T, Alloc>::erase_unordered(iterator pos)
{
	_eraseUnordered(is_trivially_relocatable<T>(), pos);
	return pos;
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
	if (_indexValid(index))
		return _m.data[index];
	else
		OEL_THROW_(std::out_of_range("Invalid index dynarray::at"));
}

template<typename T, typename Alloc>
inline typename dynarray<T, Alloc>::reference  dynarray<T, Alloc>::operator[](size_type index) noexcept
{
	OEL_ASSERT_MEM_BOUND(_indexValid(index));
	return _m.data[index];
}
template<typename T, typename Alloc>
inline typename dynarray<T, Alloc>::const_reference  dynarray<T, Alloc>::operator[](size_type index) const noexcept
{
	OEL_ASSERT_MEM_BOUND(_indexValid(index));
	return _m.data[index];
}

#undef OEL_FORCEINLINE

} // namespace oel
