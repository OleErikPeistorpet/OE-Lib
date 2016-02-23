#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "auxi/contiguous_iterator.h"
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

namespace _detail
{
	template<typename Pointer>
	struct DynarrBase
	{
		Pointer data;      // Pointer to beginning of data buffer
		Pointer end;       // Pointer to one past the back object
		Pointer reservEnd; // Pointer to end of allocated memory

		template<typename ConstPtr>
		bool DerefValid(ConstPtr pos) const
		{
			return static_cast<size_t>(pos - data) < static_cast<size_t>(end - data);
		}

		Pointer Begin() const { return data; }
		Pointer End() const   { return end; }
	};
}

////////////////////////////////////////////////////////////////////////////////

/// dynarray<dynarray<T>> is efficient
template<typename T, typename Alloc>
is_trivially_relocatable<Alloc> specify_trivial_relocate(dynarray<T, Alloc>);

template<typename T, typename A> inline
void swap(dynarray<T, A> & a, dynarray<T, A> & b) noexcept  { a.swap(b); }

/// Overloads generic erase_unordered(RandomAccessContainer &, RandomAccessContainer::size_type) (in util.h)
template<typename T, typename A> inline
void erase_unordered(dynarray<T, A> & d, typename dynarray<T, A>::size_type index)  { d.erase_unordered(d.begin() + index); }
/// Useful for std::remove_if and similar (generic in util.h)
template<typename T, typename A> inline
void erase_back(dynarray<T, A> & d, typename dynarray<T, A>::iterator first) noexcept  { d.erase_back(first); }

/// Overloads generic assign(Container &, const InputRange &) (in util.h)
template<typename T, typename A, typename InputRange> inline
void assign(dynarray<T, A> & dest, const InputRange & source)  { dest.assign(source); }
/// Overloads generic append(Container &, const InputRange &) (in util.h)
template<typename T, typename A, typename InputRange> inline
void append(dynarray<T, A> & dest, const InputRange & source)  { dest.append(source); }
/// Overloads generic insert(Container &, Container::const_iterator, const InputRange &)
template<typename T, typename A, typename ForwardRange> inline
typename dynarray<T, A>::iterator  insert(dynarray<T, A> & dest, typename dynarray<T, A>::const_iterator pos,
										  const ForwardRange & source)  { return dest.insert_r(pos, source); }

/**
* @brief Resizable array, dynamically allocated. Very similar to std::vector, but much faster in many cases.
*
* Efficiency is better if template argument T is trivially relocatable, which is true for most types, but needs
* to be declared manually for each type that is not trivially copyable. See specify_trivial_relocate(T &&).
* Also, if T is trivially relocatable, it does not need to be move or copy constructible/assignable
* except when an instance to be moved/copied is passed as an argument by the user.
* There are a few notable functions for which trivially relocatable T is required (checked when compiling):
* emplace/insert/insert_r and erase(iterator, iterator)
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
	using iterator       = contiguous_ctnr_iterator< pointer, _detail::DynarrBase<pointer> >;
	using const_iterator = contiguous_ctnr_iterator< const_pointer, _detail::DynarrBase<pointer> >;
#else
	using iterator       = pointer;
	using const_iterator = const_pointer;
#endif
	using reverse_iterator       = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;


	dynarray() noexcept                          : _m(Alloc{}) {}
	explicit dynarray(const Alloc & a) noexcept  : _m(a) {}

	/// Construct empty dynarray with space reserved for at least capacity elements
	dynarray(reserve_tag, size_type capacity, const Alloc & a = Alloc{})  : _m(a, capacity) {}

	/** @brief Uses default initialization for elements, can be significantly faster for non-class T
	*
	* Non-class T objects get indeterminate values. http://en.cppreference.com/w/cpp/language/default_initialization  */
	dynarray(size_type size, default_init_tag, const Alloc & a = Alloc{});
	explicit dynarray(size_type size, const Alloc & a = Alloc{});  ///< (Value-initializes elements, same as std::vector)
	dynarray(size_type size, const T & fillVal, const Alloc & a = Alloc{});

	/** @brief Equivalent to std::vector(begin(range), sLast, a),
	*	where sLast is either end(range) or found by magic, see TODO put ref here
	*
	* If you need to construct from some std::istream, check out boost/range/istream_range.hpp  */
	template<typename InputRange, typename /*EnableIfRange*/ = decltype( ::adl_cbegin(std::declval<InputRange>()) )>
	explicit dynarray(const InputRange & range, const Alloc & a = Alloc{})  : _m(a) { assign(range); }

	dynarray(std::initializer_list<T> init, const Alloc & a = Alloc{})  : _m(a, init.size())
																		{ _initPostAllocate(init.begin(), init.size()); }
	dynarray(dynarray && other) noexcept    : _m(std::move(other._m)) {}
	/// If a != other.get_allocator() and T is not trivially relocatable,
	/// behaviour is undefined (triggers OEL_ASSERT unless NDEBUG)
	dynarray(dynarray && other, const Alloc & a) noexcept;
	dynarray(const dynarray & other);
	dynarray(const dynarray & other, const Alloc & a)  : _m(a, other.size())
													   { _initPostAllocate(other.data(), other.size()); }
	~dynarray() noexcept;

	/// If using custom Alloc with propagate_on_container_move_assignment false, behaviour is undefined
	/// when get_allocator() != other.get_allocator() and T is not trivially relocatable
	dynarray & operator =(dynarray && other) noexcept;
	dynarray & operator =(const dynarray & other);

	dynarray & operator =(std::initializer_list<T> il)  { assign(il);  return *this; }

	void      swap(dynarray & other) noexcept;

	/**
	* @brief Replace the contents with source range
	* @param source an array, STL container, gsl::span, boost::iterator_range or such
	*	Shall not be a subset of same dynarray, except if begin(source) points to first element of dynarray.
	* @return iterator begin(source) incremented to equal the end of source range
	*
	* Any elements held before the call are either assigned to or destroyed. */
	template<typename InputRange>
	auto      assign(const InputRange & source) -> decltype(::adl_begin(source));

	void      assign(size_type count, const T & val)  { clear(); append(count, val); }

	/**
	* @brief Add at end the elements from range (in same order), return iterator to new
	* @param source an array, STL container, gsl::span, boost::iterator_range or such. Can be this dynarray.
	* @return iterator pointing to first of the new elements in dynarray, or end if source is empty
	*
	* Strong exception guarantee, this function has no effect if an exception is thrown.
	* Otherwise equivalent to std::vector::insert(end(), begin(source), sLast),
	* where sLast is either end(source) or found by magic, see TODO put ref here  */
	template<typename InputRange>
	iterator  append(const InputRange & source);
	/**
	* @brief Same as append(const InputRange &), except returning past-the-last of source
	* @return begin(source) incremented to end of source. The iterator is already invalidated (do not dereference) if
	*	first pointed into same dynarray and there was insufficient capacity to avoid reallocation. */
	template<typename InputRange>
	auto  append_ret_src(const InputRange & source) -> decltype(::adl_begin(source));
	/// Equivalent to calling append(const InputRange &) with il as argument
	iterator  append(std::initializer_list<T> il);
	/// Equivalent to std::vector::insert(end(), count, val), but with strong exception guarantee
	void      append(size_type count, const T & val);

	/**
	* @brief Uses default initialization for added elements, can be significantly faster for non-class T
	*
	* Non-class T objects get indeterminate values. http://en.cppreference.com/w/cpp/language/default_initialization  */
	void      resize(size_type count, default_init_tag)  { _resizeImpl(count, _detail::UninitFillDefault<Alloc, T>); }
	/// (Value-initializes added elements, same as std::vector::resize)
	void      resize(size_type count)                    { _resizeImpl(count, _detail::UninitFill<Alloc, T>); }

	/// Equivalent to std::vector::insert(pos, begin(source), sLast),
	/// where sLast is either end(source) or found by magic, see TODO put ref here
	template<typename ForwardRange>
	iterator  insert_r(const_iterator pos, const ForwardRange & source);

	/// The default allocator performs list-initialization of element if there is no matching constructor
	template<typename... Args>
	iterator  emplace(const_iterator pos, Args &&... elemInitArgs);

	iterator  insert(const_iterator pos, T && val)       { return emplace(pos, std::move(val)); }
	iterator  insert(const_iterator pos, const T & val)  { return emplace(pos, val); }

	/// The default allocator performs list-initialization of element if there is no matching constructor
	template<typename... Args>
	void      emplace_back(Args &&... elemInitArgs);

	void      push_back(T && val)       { emplace_back(std::move(val)); }
	void      push_back(const T & val)  { emplace_back(val); }

	/// After the call, any previous iterator to the back element will be equal to end()
	void      pop_back() noexcept;

	/**
	* @brief Erase the element at pos from dynarray without maintaining order of elements.
	*
	* Constant complexity (compared to linear in the distance between pos and end() for normal erase).
	* @return Iterator corresponding to the same index in the sequence as pos, same as for std containers. */
	iterator  erase_unordered(iterator pos)  { _eraseUnordered(pos, is_trivially_relocatable<T>());
											   return pos; }
	iterator  erase(iterator pos)            { _erase(pos, is_trivially_relocatable<T>());  return pos; }

	iterator  erase(iterator first, iterator last) noexcept;
	/// Equivalent to erase(first, end()) (but potentially faster), making first the new end
	void      erase_back(iterator first) noexcept;

	void      clear() noexcept        { erase_back(begin()); }

	bool      empty() const noexcept  { return _m.data == _m.end; }

	size_type size() const noexcept   { return _m.end - _m.data; }

	void      reserve(size_type minCap)  { if (capacity() < minCap) _growTo(minCap); }

	/// It's a good idea to check that size() < capacity() before calling to avoid useless reallocation
	void      shrink_to_fit();

	size_type capacity() const noexcept  { return _m.reservEnd - _m.data; }

	allocator_type get_allocator() const  { return _m; }

	iterator       begin() noexcept         { return _iterator{_m.data, &_m}; }
	const_iterator begin() const noexcept   { return _constIter{_m.data, &_m}; }
	const_iterator cbegin() const noexcept  { return begin(); }

	iterator       end() noexcept           { return _iterator{_m.end, &_m}; }
	const_iterator end() const noexcept     { return _constIter{_m.end, &_m}; }
	const_iterator cend() const noexcept    { return end(); }

	reverse_iterator       rbegin() noexcept        { return reverse_iterator{end()}; }
	const_reverse_iterator rbegin() const noexcept  { return const_reverse_iterator{end()}; }

	reverse_iterator       rend() noexcept          { return reverse_iterator{begin()}; }
	const_reverse_iterator rend() const noexcept    { return const_reverse_iterator{begin()}; }

	T *             data() noexcept        { return _m.data; }
	const T *       data() const noexcept  { return _m.data; }

	reference       front() noexcept        { return *begin(); }
	const_reference front() const noexcept  { return *begin(); }

	reference       back() noexcept         { return *_iterator{_m.end - 1, &_m}; }
	const_reference back() const noexcept   { return *_constIter{_m.end - 1, &_m}; }

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
	using _allocTrait = std::allocator_traits<Alloc>;
	using _dynarrBase = _detail::DynarrBase<pointer>;

	using _iterator  = _detail::CtnrIteratorMaker<iterator>;
	using _constIter = _detail::CtnrIteratorMaker<const_iterator>;

#if _MSC_VER && OEL_MEM_BOUND_DEBUG_LVL == 0 && _ITERATOR_DEBUG_LEVEL == 0
	#define OEL_FORCEINLINE __forceinline
#else
	#define OEL_FORCEINLINE inline
#endif

	struct _assertNothrowMoveConstruct
	{
		OEL_WHEN_EXCEPTIONS_ON(
			static_assert(std::is_nothrow_move_constructible<T>::value || is_trivially_relocatable<T>::value,
				"This function requires that T is noexcept move constructible or trivially relocatable") );
	};


	using _scopedPtrBase = _detail::AllocRefOptimizeEmpty<Alloc>;
	struct _scopedPtr : private _scopedPtrBase
	{
		pointer ptr;
		pointer allocEnd;

		_scopedPtr(dynarray & parent, size_type const allocSize)
		 :	_scopedPtrBase(parent._m)
		{
			ptr = this->Get().allocate(allocSize);
			allocEnd = ptr + allocSize;
		}
		_scopedPtr(const _scopedPtr &) = delete;

		~_scopedPtr()
		{
			if (ptr)
				this->Get().deallocate(ptr, allocEnd - ptr);
		}
	};

	struct _dataOwner : public _dynarrBase, public Alloc
	{
		using _dynarrBase::data;
		using _dynarrBase::end;
		using _dynarrBase::reservEnd;

		_dataOwner(const Alloc & a)
		 :	_dynarrBase(), Alloc(a) {
		}
		_dataOwner(const Alloc & a, size_type const capacity)
		 :	Alloc(a)
		{
			end = data = this->allocate(capacity);
			reservEnd = data + capacity;
		}
		_dataOwner(_dataOwner && other)
		 :	_dynarrBase(other), Alloc(std::move(other))
		{
			other.reservEnd = other.end = other.data = nullptr;
		}
		_dataOwner(const _dataOwner &) = delete;
		void operator =(const _dataOwner &) = delete;

		~_dataOwner()
		{
			if (data)
				this->deallocate(data, reservEnd - data);
		}

	} _m; // One and only data member of dynarray


	void _resetData(pointer const newData)
	{
		if (_m.data)
			_m.deallocate(_m.data, capacity());

		_m.data = newData;
	}

	void _swapBuf(_scopedPtr & s)
	{
		using std::swap;
		swap(_m.data, s.ptr);
		swap(_m.reservEnd, s.allocEnd);
	}

	void _moveAssignAlloc(std::true_type, _dataOwner & o)
	{
		static_cast<Alloc &>(_m) = std::move(o);
	}

	void _moveAssignAlloc(std::false_type, _dataOwner &) {}

	void _swapAlloc(std::true_type, _dataOwner & o)
	{
		using std::swap;
		swap(static_cast<Alloc &>(_m), static_cast<Alloc &>(o));
	}

	void _swapAlloc(std::false_type, _dataOwner & o)
	{	// propagate_on_container_swap false, standard says this is undefined if allocators compare unequal
		OEL_ASSERT(get_allocator() == static_cast<Alloc &>(o));
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
		_m.end = _m.reservEnd;
		_uninitCopy(is_trivially_copyable<T>(), first, count, _m.data, _m.end);
	}


	size_type _unusedCapacity() const
	{
		return _m.reservEnd - _m.end;
	}

	static size_type _calcCapAddOne(size_type const oldCap, size_type = 0)
	{
		enum {
		#if _WIN64
			startBytesGood = 24,
		#else
			startBytesGood = 4 * sizeof(int),
		#endif
			minGrow = 2 * sizeof(T) <= startBytesGood ?
				startBytesGood / sizeof(T) :
				(sizeof(T) < 1020 ? 2 : 1)
		};
		return oldCap + std::max<size_type>(oldCap / 2, minGrow);
	}

	static size_type _calcCap(size_type oldCap, size_type newSize)
	{	// growth factor is 1.5
		return (std::max)(oldCap + oldCap / 2, newSize);
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
	void _resizeImpl(size_type const newSize, UninitFillFunc initAdded)
	{	// note: initAdded cannot hold a reference to element of this
		if (capacity() < newSize)
			_growTo(_calcCap(capacity(), newSize));

		T *const newEnd = _m.data + newSize;
		if (_m.end < newEnd) // then construct new
			initAdded(_m.end, newEnd, _m);
		else // downsizing
			_detail::Destroy(newEnd, _m.end);

		_m.end = newEnd;
	}


	void _eraseUnordered(iterator pos, std::true_type /*trivialRelocate*/)
	{
		OEL_ASSERT_MEM_BOUND(pos._container == &_m);

		T & elem = *pos;
		elem.~T();
		--_m.end;
		// Relocate last element to pos
		auto &
	#if !_MSC_VER
			__attribute__((may_alias))
	#endif
			raw = reinterpret_cast<aligned_union_t<T> &>(elem);
		raw = reinterpret_cast<aligned_union_t<T> &>(*_m.end);
	}

	void _eraseUnordered(iterator pos, std::false_type)
	{
		*pos = std::move(back());
		pop_back();
	}

	void _erase(iterator pos, std::true_type /*trivialRelocate*/)
	{
		T *const ptr = to_pointer_contiguous(pos);
		OEL_ASSERT_MEM_BOUND(_m.data <= ptr && ptr < _m.end);

		ptr-> ~T();
		const T * next = ptr + 1;
		::memmove(ptr, next, sizeof(T) * (_m.end - next)); // relocate [pos + 1, end) to [pos, end - 1)
		--_m.end;
	}

	void _erase(iterator pos, std::false_type)
	{
		iterator last = std::move(pos + 1, end(), pos);
		_m.end = to_pointer_contiguous(last);
		(*_m.end).~T();
	}


	template<typename Range, typename IterTrav> // pass dummy int to prefer this overload
	static auto _sizeOrEnd(const Range & r, IterTrav, int) -> decltype( static_cast<size_type>(oel::ssize(r)) )
															   { return static_cast<size_type>(oel::ssize(r)); }
	template<typename Range>
	static size_type _sizeOrEnd(const Range & r, forward_traversal_tag, long)
	{
		return std::distance(::adl_begin(r), ::adl_end(r));
	}

	template<typename Range>
	static auto _sizeOrEnd(const Range & r, single_pass_traversal_tag, long) -> decltype(::adl_end(r))
																				{ return ::adl_end(r); }
	// Returns element count as size_type if possible, else adl_end(r)
	template<typename Iter, typename Range>
	static auto _sizeOrEnd(const Range & r) -> decltype(_sizeOrEnd(r, iterator_traversal_t<Iter>(), int{}))
											   { return _sizeOrEnd(r, iterator_traversal_t<Iter>(), int{}); }


	void _moveUnequalAlloc(dynarray & src)
	{
		OEL_ASSERT(is_trivially_relocatable<T>::value);

		_assignImpl(src.begin(), src.size(), std::true_type{});
		src._m.end = src._m.data; // elements in src conceptually destroyed
	}

	template<typename CntigusIter>
	CntigusIter _assignImpl(CntigusIter const first, size_type const count, std::true_type /*trivialCopy*/)
	{
	#if OEL_MEM_BOUND_DEBUG_LVL
		if (count != 0)
		{	// Dereference to catch out of range errors if the iterators have internal checks
			(void)*first;
			(void)*(first + (count - 1));
		}
	#endif
		if (capacity() < count)
		{
			// Deallocating first might be better, but then the _m pointers would have to be nulled in case allocate throws
			_resetData(_m.allocate(count));
			_m.end = _m.data + count;
			_m.reservEnd = _m.end;
		}
		else
		{	_m.end = _m.data + count;
		}
		// Not portable. Check for self assignment or use memmove?
		::memcpy(data(), to_pointer_contiguous(first), sizeof(T) * count);

		return first + count;
	}

	template<typename InputIter>
	InputIter _assignImpl(InputIter src, size_type const count, std::false_type)
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
		pointer newEnd;
		if (capacity() < count)
		{	// not enough room, allocate
			pointer const newData = _m.allocate(count);
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
				iterator const it = _iterator{newEnd, &_m};
				src = copy(src, begin(), it);
				erase_back(it);
			}
			else // assign to old elements as far as we can
			{	src = copy(src, begin(), end());
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
	InputIter _assignImpl(InputIter first, Sentinel const last, std::false_type)
	{	// single pass iterator, no size available
		clear();
		for (; first != last; ++first)
			emplace_back(*first);

		return first;
	}

	template<typename Ret, typename InputIter, typename Sentinel, typename RetSelect>
	Ret _append(InputIter first, Sentinel const last, RetSelect retSelect)
	{	// single pass iterator, no size available
		size_type const oldSize = size();
		OEL_TRY_
		{
			for (; first != last; ++first)
				emplace_back(*first);
		}
		OEL_CATCH_ALL
		{
			erase_back(begin() + oldSize);
			OEL_WHEN_EXCEPTIONS_ON(throw);
		}
		return retSelect(begin() + oldSize, first);
	}

	template<typename Ret, typename InputIter, typename RetSelect>
	OEL_FORCEINLINE Ret _append(InputIter first, size_type count, RetSelect retSelect)
	{
		first = _appendN(first, count, can_memmove_with<T *, InputIter>());
		return retSelect(end() - count, first);
	}

	template<typename InputIter>
	InputIter _appendN(InputIter first, size_type const count, std::false_type)
	{	// cannot use memcpy
		_appendImpl( count,
			[&first](T * dest, size_type count, Alloc & al)
			{
				first = _detail::UninitCopy(first, dest, dest + count, al);
			} );
		return first;
	}

	template<typename CntigusIter>
	OEL_FORCEINLINE CntigusIter _appendN(CntigusIter const first, size_type const count, std::true_type)
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

	template<typename MakeFuncAppend>
	OEL_FORCEINLINE void _appendImpl(size_type const count, MakeFuncAppend makeNew)
	{
		_assertNothrowMoveConstruct();

		if (_unusedCapacity() >= count)
		{
			makeNew(_m.end, count, _m);
		}
		else
		{
			_scopedPtr newData{*this, _calcCap(capacity(), size() + count)};

			size_type const oldSize = size();
			pointer pos = newData.ptr + oldSize;
			makeNew(pos, count, _m);
			// Exception free from here
			_relocateData(newData.ptr, pos, oldSize);

			_m.end = pos;
			_swapBuf(newData);
		}
		_m.end += count;
	}


	template<typename CalcCapFunc, typename MakeFuncInsert, typename... Args>
	T * _insertRealloc(T *const pos, size_type const nAfterPos, size_type const nToAdd,
					   CalcCapFunc calcNewCap, MakeFuncInsert makeNew, Args &&... args)
	{
		_scopedPtr newData{*this, calcNewCap(capacity(), size() + nToAdd)};

		size_type const nBeforePos = pos - data();
		T *const newPos = newData.ptr + nBeforePos;
		T *const next = makeNew(*this, newPos, nToAdd, std::forward<Args>(args)...);
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

	template<typename InputIter>
	iterator _insertImpl(const_iterator pos, InputIter first, size_type count)
	{
	#define OEL_DYNARR_INSERT_STEP0  \
		_detail::AssertTrivialRelocate<T>();  \
		\
		auto pPos = const_cast<T *>(to_pointer_contiguous(pos));  \
		OEL_ASSERT_MEM_BOUND(_m.data <= pPos && pPos <= _m.end);  \
		size_type const nAfterPos = _m.end - pPos;

		OEL_DYNARR_INSERT_STEP0
		using CanMemmove = can_memmove_with<T *, InputIter>;
		if (_unusedCapacity() >= count)
		{
			T *const dLast = pPos + count;
			// Relocate elements to make space, conceptually destroying [pos, pos + count)
			::memmove(dLast, pPos, sizeof(T) * nAfterPos);
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
					::memmove(dest, dLast, sizeof(T) * nAfterPos);
					_m.end -= (dLast - dest);
					OEL_WHEN_EXCEPTIONS_ON(throw);
				}
			}
		}
		else // not enough room, reallocate
		{
			pPos = _insertRealloc( pPos, nAfterPos, count, _calcCap,
					[first](dynarray & self, T * newPos, size_type count)
					{
						T *const next = newPos + count;
						self._uninitCopy(CanMemmove(), first, count, newPos, next);
						return next;
					} );
		}
		return _iterator{pPos, &_m};
	}
};

template<typename T, typename Alloc> template<typename... Args>
typename dynarray<T, Alloc>::iterator  dynarray<T, Alloc>::emplace(const_iterator pos, Args &&... args)
{
	OEL_DYNARR_INSERT_STEP0

	if (_m.end < _m.reservEnd) // then new element fits
	{
		// Temporary in case constructor throws or source is an element of this dynarray at pos or after
		aligned_union_t<T> tmp;
		_allocTrait::construct(_m, reinterpret_cast<T *>(&tmp), std::forward<Args>(args)...);
		// Relocate [pos, end) to [pos + 1, end + 1), conceptually destroying element at pos
		::memmove(pPos + 1, pPos, sizeof(T) * nAfterPos);
		++_m.end;

		::memcpy(pPos, &tmp, sizeof(T)); // relocate the new element to pos
	}
	else
	{	pPos = _insertRealloc(pPos, nAfterPos, {}, _calcCapAddOne,
							  _emplaceMakeElem{}, std::forward<Args>(args)...);
	}
	return _iterator{pPos, &_m};
}
#undef OEL_DYNARR_INSERT_STEP0

template<typename T, typename Alloc> template<typename... Args>
void dynarray<T, Alloc>::emplace_back(Args &&... args)
{
	if (_m.end < _m.reservEnd)
	{	_allocTrait::construct(_m, _m.end, std::forward<Args>(args)...);
	}
	else
	{
		_scopedPtr newData{*this, _calcCapAddOne(capacity())};

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
dynarray<T, Alloc>::dynarray(dynarray && other, const Alloc & a) noexcept
 :	_m(a)
{
	if (a != other._m && !std::is_empty<Alloc>::value)
	{	_moveUnequalAlloc(other);
	}
	else
	{
		static_cast<_dynarrBase &>(_m) = other._m;
		other._m.reservEnd = other._m.end = other._m.data = nullptr;
	}
}

template<typename T, typename Alloc>
dynarray<T, Alloc> & dynarray<T, Alloc>::operator =(dynarray && other) noexcept
{
	// TODO: check that this gets optimized out when propagate_on_container_move_assignment
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
		static_cast<_dynarrBase &>(_m) = other._m;
		_moveAssignAlloc(_allocTrait::propagate_on_container_move_assignment(), other._m);
		other._m.reservEnd = other._m.end = other._m.data = nullptr;
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
dynarray<T, Alloc>::dynarray(size_type size, default_init_tag, const Alloc & a)
 :	_m(a, size)
{
	_m.end = _m.reservEnd;
	_detail::UninitFillDefault(_m.data, _m.end, _m);
}

template<typename T, typename Alloc>
dynarray<T, Alloc>::dynarray(size_type size, const Alloc & a)
 :	_m(a, size)
{
	_m.end = _m.reservEnd;
	_detail::UninitFill(_m.data, _m.end, _m);
}

template<typename T, typename Alloc>
dynarray<T, Alloc>::dynarray(size_type size, const T & val, const Alloc & a)
 :	_m(a, size)
{
	_m.end = _m.reservEnd;
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
	std::swap(static_cast<_dynarrBase &>(_m),
			  static_cast<_dynarrBase &>(other._m));
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
inline void dynarray<T, Alloc>::append(size_type count, const T & val)
{
	_appendImpl( count,
		[&val](T * dest, size_type count, Alloc & a)
		{
			_detail::UninitFill(dest, dest + count, a, val);
		} );
}

template<typename T, typename Alloc> template<typename InputRange>
OEL_FORCEINLINE typename dynarray<T, Alloc>::iterator  dynarray<T, Alloc>::append(const InputRange & src)
{
	using IterSrc = decltype(::adl_begin(src));
	return _append<iterator>( ::adl_begin(src), _sizeOrEnd<IterSrc>(src),
							  [](iterator newPos, IterSrc) { return newPos; } );
}

template<typename T, typename Alloc> template<typename InputRange>
OEL_FORCEINLINE auto dynarray<T, Alloc>::append_ret_src(const InputRange & src) -> decltype(::adl_begin(src))
{
	using IterSrc = decltype(::adl_begin(src));
	return _append<IterSrc>( ::adl_begin(src), _sizeOrEnd<IterSrc>(src),
							 [](iterator, IterSrc sLast) { return sLast; } );
}

template<typename T, typename Alloc>
OEL_FORCEINLINE typename dynarray<T, Alloc>::iterator  dynarray<T, Alloc>::append(std::initializer_list<T> il)
{
	return append<>(il);
}

template<typename T, typename Alloc> template<typename ForwardRange>
inline typename dynarray<T, Alloc>::iterator  dynarray<T, Alloc>::
	insert_r(const_iterator pos, const ForwardRange & src)
{
	auto first = ::adl_begin(src);
	auto nElems = _sizeOrEnd<decltype(first)>(src);

	static_assert(std::is_same<decltype(nElems), size_type>::value,
				  "insert_r requires that source models Forward Range (Boost concept)");

	return _insertImpl(pos, first, nElems);
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
inline T & dynarray<T, Alloc>::at(size_type i)
{
	const auto & cSelf = *this;
	return const_cast<reference>(cSelf.at(i));
}
template<typename T, typename Alloc>
const T & dynarray<T, Alloc>::at(size_type i) const
{
	if (static_cast<size_t>(size()) > static_cast<size_t>(i))
		return _m.data[i];
	else
		OEL_THROW(std::out_of_range("Invalid index dynarray::at"));
}

template<typename T, typename Alloc>
inline T & dynarray<T, Alloc>::operator[](size_type i) noexcept
{
	OEL_ASSERT_MEM_BOUND(static_cast<size_t>(size()) > static_cast<size_t>(i));
	return _m.data[i];
}
template<typename T, typename Alloc>
inline const T & dynarray<T, Alloc>::operator[](size_type i) const noexcept
{
	OEL_ASSERT_MEM_BOUND(static_cast<size_t>(size()) > static_cast<size_t>(i));
	return _m.data[i];
}

#undef OEL_FORCEINLINE

} // namespace oel
