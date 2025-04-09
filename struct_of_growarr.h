#pragma once

// Copyright 2023 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "allocator.h"
#include "auxi/impl_algo.h"
#include "auxi/struct_of_growarr_detail.h"
#include "optimize_ext/default.h"

#include <algorithm>

/** @file
*/

namespace oel
{

//! struct_of_growarr is trivially relocatable if Alloc is
template< template<typename> typename E, typename Alloc >
is_trivially_relocatable<Alloc> specify_trivial_relocate(struct_of_growarr<E, Alloc>);


//! Structure of resizable arrays, dynamically allocated.
/**
* In general, only that which differs from std::vector is documented.
*
* There is a general requirement that template argument Ts are trivially relocatable or noexcept move
* constructible (checked when compiling). Most types can be relocated trivially, but it often needs to be
* declared manually. See is_trivially_relocatable (fwd.h). Performance is better for trivially relocatable types.
*
* Note that the allocator model is not quite standard: `destroy` is never used,
* `construct` may not be called if a type is trivially constructible and is not called when relocating elements.
*/
template
<	template< typename > typename ElemStruct,
	typename Alloc
>
class struct_of_growarr
{
	using _alloTrait = typename std::allocator_traits<Alloc>;

public:
	using allocator_type  = Alloc;
	using difference_type = ptrdiff_t;
	using size_type       = size_t;

	template< F, typename... I >
	using zip_iterator = _zipTransformIterator< F, I... >;


	constexpr struct_of_growarr() noexcept(noexcept(Alloc{}))   : _m(Alloc{}) {}
	constexpr explicit struct_of_growarr(Alloc a) noexcept      : _m(a) {}

	//! Construct empty struct_of_growarr with space reserved for exactly capacity elements
	struct_of_growarr(reserve_tag, size_type capacity, Alloc a = Alloc{})   : _m(a) { _initReserve(capacity); }

	struct_of_growarr(size_type size, for_overwrite_t, Alloc a = Alloc{});

	explicit struct_of_growarr(size_type size, Alloc a = Alloc{});

	struct_of_growarr(struct_of_growarr && other) noexcept              : _m(std::move(other._m)) {}
	struct_of_growarr(struct_of_growarr && other, Alloc a);
	explicit struct_of_growarr(const struct_of_growarr & other)
		:	struct_of_growarr(other, _alloTrait::select_on_container_copy_construction(other._m) ) {}
	explicit struct_of_growarr(const struct_of_growarr & other, Alloc a)   : _m(a) { append(other); }

	~struct_of_growarr() noexcept;

	struct_of_growarr & operator =(struct_of_growarr && other) &
		noexcept(_alloTrait::propagate_on_container_move_assignment::value or _alloTrait::is_always_equal::value);
	struct_of_growarr & operator =(const struct_of_growarr &) = delete;

	friend void swap(struct_of_growarr & a, struct_of_growarr & b) noexcept
		{
			using std::swap;

			[[maybe_unused]] allocator_type & a0 = a._m;
			[[maybe_unused]] allocator_type & a1 = b._m;
			if constexpr (_alloTrait::propagate_on_container_swap::value)
				swap(a0, a1);
			else // Standard says this is undefined if allocators compare unequal
				OEL_ASSERT(a0 == a1);
		}

	template< typename... Ranges >
	auto append(Ranges &&... sources) -> std::tuple< borrowed_iterator_t<Ranges>... >;

	template< typename StructOfGrowarrViews >
	void append_fields(StructOfGrowarrViews && source);

	void resize_for_overwrite(size_type n)   { _doResize< _detail::DefaultInit<allocator_type> >(n); }
	void resize(size_type n)                 { _doResize<_uninitFill>(n); }

	/** @brief Beware, passing an element of same array is often unsafe
	* @pre args shall not refer to any element of this struct_of_growarr, unless `size() < capacity()` */
	template< typename... Ts >
	void push_back(Ts &&... args);

	// TODO
	template< typename... NullaryFuncs >
	void emplace_back(NullaryFuncs... makers);

	void pop_back() noexcept
		{
			OEL_ASSERT(_m.size > 0);
			--_m.size;
			_m.end-> ~T();
		}
	void pop_back(size_type count) noexcept   { erase_to_end(end() - count); }

	//! Erase the element at pos without maintaining order of elements after pos.
	/**
	* Constant complexity (compared to linear in the distance between pos and end() for normal erase). */
	template< typename F, typename... I >
	void unordered_erase(zip_iterator<F, I...> pos)
		{
			_detail::unordered_erase(static_cast<_internBase_7KQw &>(_m), _asMut(pos)); }
		}

	template< typename F, typename... I >
	void erase(zip_iterator<F, I...> pos)
		{
			_detail::erase(static_cast<_internBase_7KQw &>(_m), _asMut(pos)); }
		}

	void erase(size_type index)   { erase(begin() + index); }

	template< typename F, typename... I >
	void erase(zip_iterator<F, I...> first, zip_iterator<F, I...> last)
		{
			_detail::erase(static_cast<_internBase_7KQw &>(_m), _asMut(first), _asMut(last)); }
		}

	void erase(size_type first, size_type last)
		{
			auto const b = begin();  erase(b + first, b + last);
		}

	template< typename F, typename... I >
	void erase_to_end(zip_iterator<F, I...> first) noexcept;

	void clear() noexcept                        { erase_to_end(begin()); }

	void reserve(size_type minCap)
		{
			if (_m.capacity < minCap)
				_realloc(_calcCapChecked(minCap));
		}
	//! It's probably a good idea to check that size < capacity before calling, maybe add some treshold to size
	void shrink_to_fit();

	[[nodiscard]] bool empty() const noexcept  { return 0 == _m.size; }

	size_type size() const noexcept   OEL_ALWAYS_INLINE { return _m.size; }

	size_type capacity() const noexcept        { return _m.capacity; }

	constexpr size_type max_size() const noexcept   { return _alloTrait::max_size(_m); } // TODO

	allocator_type get_allocator() const noexcept   { return _m; }

	auto operator->()
		{
			using T = _detail::ViewTag;
			return _arrowProxy<T>{ _m.data._apply(_views<T>{_m.size}) };
		}
	auto operator->() const
		{
			using T = _detail::ConstViewTag;
			return _arrowProxy<T>{ _m.data._apply(_views<T>{_m.size}) };
		}

	auto mut_fields()           { return *operator->().operator->(); }

	auto const_fields() const   { return *operator->().operator->(); }

	auto begin()
		{
			return _m.data._apply(_zipBegin<_detail::ElementTag, _detail::RvalueElementTag>{});
		}
	auto begin() const
		{
			return _m.data._apply(_zipBegin<_detail::ConstElementTag, void>{});
		}
	auto cbegin() const  { return begin(); }

	auto end()          { return begin() + _m.size; }
	auto end() const    { return begin() + _m.size; }
	auto cend() const   { return begin() + _m.size; }

	decltype(auto) operator[](size_type index)         { return begin()[index]; }
	decltype(auto) operator[](size_type index) const   { return begin()[index]; }

	decltype(auto) back()         { return begin()[_m.size - 1]; }
	decltype(auto) back() const   { return begin()[_m.size - 1]; }

	template< typename Func >
	auto zip_transform(Func f)
		{
			return _m.data._apply( _detail::ZipTransform<false, Func>{std::move(f), _m.size} );
		}
	template< typename Func >
	auto zip_transform(Func f) const
		{
			return _m.data._apply( _detail::ZipTransform<true, Func>{std::move(f), _m.size} );
		}



////////////////////////////////////////////////////////////////////////////////
//
// Implementation only in rest of the file


private:
	struct _internBase_7KQw
	{
		ElemStruct<_detail::InternalTag> data;
		size_type size;
		size_type capacity;
	};

	using _uninitFill = _detail::UninitFill<allocator_type>;
	using _alloc_7KQw = Alloc; // guarding against name collision due to inheritance (MSVC)

	struct _memOwner : public _internBase_7KQw, public _alloc_7KQw
	{
		using _internBase_7KQw::data;
		using _internBase_7KQw::size;
		using _internBase_7KQw::capacity;

		constexpr _memOwner(_alloc_7KQw & a) noexcept
		 :	_internBase_7KQw{}, _alloc_7KQw{std::move(a)}
		{}

		constexpr _memOwner(_memOwner && other) noexcept
		 :	_internBase_7KQw{other}, _alloc_7KQw{std::move(other)}
		{
			other.data = nullptr;
			other.capacity = other.size = 0;
		}

		~_memOwner()
		{
			if (data)
				::oel::_detail::StructGrowarrAllocateWrap<_alloc_7KQw, Ts...>::dealloc(*this, data, capacity);
		}
	}
	_m; // the only non-static data member

	void _resetData(T *const newData, size_type const newCap)
	{
		if (_m.data)
			_allocateWrap::dealloc(_m, _m.data, _m.capacity);

		_m.data     = newData;
		_m.capacity = newCap;
	}

	void _initReserve(size_type const capToCheck)
	{
		boost::pfr::get<0>(_m.data).p = _allocateChecked(capToCheck);
		// TODO
		_m.capacity = capToCheck;
	}

	void _moveInternBase(_internBase & src) noexcept
	{
		_internBase & dest = _m;
		dest = src;
		src  = {};
	}


	template< typename Tag >
	struct _arrowProxy
	{
		ElemStruct<Tag> r;

		auto operator->() noexcept
		{
			return &r;
		}
	};


	template< typename Tag >
	struct _views
	{
		size_type size;

		template< typename... Ts >
		ElemStruct<Tag> operator()(const Ts &... fields) const
		{
			return{ {{fields.p, static_cast<difference_type>(size)}}... };
		}
	};

	template< typename ElemTag, typename RvalueTag >
	struct _zipBegin
	{
		template< typename... Ts >
		auto operator()(const Ts &... fields) const
		{
			using F =
				_detail::Zip
				<	ElemStruct<ElemTag>,
					ElemStruct<RvalueTag>
				>;
			constexpr bool isConst{std::is_same_v<ElemTag, _detail::ConstElementTag>};
			return _zipTransformIterator
			{	F{},
				_detail::PtrAsConst< isConst, decltype(fields.p) >(fields.p)...
			};
		}
	};


	size_type _spareCapacity() const
	{
		return _m.capacity - _m.size;
	}

	size_type _calcCapUnchecked(size_type const newSize) const
	{
		return std::max(2 * _m.capacity, newSize);
	}

	size_type _calcCapChecked(size_type const newSize) const
	{
		if (newSize <= max_size())
			return _calcCapUnchecked(newSize);
		else
			_detail::LengthError::raise();
	}

	size_type _calcCapAdd(size_type const nAdd) const
	{
		if (nAdd <= SIZE_MAX / 2 / sizeof(T)) // assumes that allocating greater than SIZE_MAX / 2 always fails
			return _calcCapUnchecked(_m.size + nAdd);
		else
			_detail::LengthError::raise();
	}

	size_type _calcCapAddOne() const
	{
		constexpr auto startBytesGood = std::max(3 * sizeof(void *), 4 * sizeof(int));
		constexpr auto minGrow = (startBytesGood + sizeof(T) - 1) / sizeof(T);
		return _m.capacity + std::max(_m.capacity, minGrow); // growth factor is 2
	}

	T * _allocateChecked(size_type const n)
	{
		if (n <= max_size())
			return _allocateWrap::allocate(_m, n);
		else
			_detail::LengthError::raise();
	}


	void _realloc(size_type const newCap)
	{
		auto const newData = _allocateWrap::allocate(_m, newCap);
		_detail::Relocate(_m.data, oldSize, newData);
		_resetData(newData, newCap);
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

		_m.end = newEnd;
	}
};


template< template<typename> typename ElemStruct, typename Alloc >
void struct_of_growarr<ElemStruct, Alloc>::_growByOne()
{
	_realloc(_calcCapAddOne());
}


template< template<typename> typename ElemStruct, typename Alloc >
#if defined _MSC_VER
	#error OEL_UNLIKELY
	__declspec(noinline) // to get the compiler to inline calling function
#endif
void struct_of_growarr<ElemStruct, Alloc>::_growBy(size_type const count)
{
	_realloc(_calcCapAdd(count));
}


template< template<typename> typename ElemStruct, typename Alloc >
template< typename... Ts >
void struct_of_growarr<ElemStruct, Alloc>::push_back(Ts &&... args)
{
	if (_m.size == _m.capacity)
		_growByOne();

	 // TODO
	_alloTrait::construct(_m, _m.end, static_cast<Ts &&>(args)...);

	return *(_m.end++);
}


template< template<typename> typename ElemStruct, typename Alloc >
template< typename... Ranges >
inline auto struct_of_growarr<ElemStruct, Alloc>::append(Ranges &&... sources)
->	std::tuple< borrowed_iterator_t<Ranges>... >
{
	static_assert(... and _detail::rangeIsForwardOrSized<Ranges>);

	count = std::min({UDist(sources)...});

	if (_spareCapacity() < count) OEL_UNLIKELY
		_growBy(count);

	// TODO
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
			_detail::Destroy(_m.end, dest);
			OEL_RETHROW;
		}
		_m.end = dLast;
	}
	return src;
}


template< template<typename> typename ElemStruct, typename Alloc >
struct_of_growarr<ElemStruct, Alloc>::struct_of_growarr(size_type n, for_overwrite_t, Alloc a)
 :	_m(a)
{
	_initReserve(n);
	_m.size = _m.capacity;
	_detail::DefaultInit<allocator_type>::call(_m.data, _m.reservEnd, _m);
}


template< template<typename> typename ElemStruct, typename Alloc >
struct_of_growarr<ElemStruct, Alloc>::struct_of_growarr(size_type n, Alloc a)
 :	_m(a)
{
	_initReserve(n);
	_m.size = _m.capacity;
	_uninitFill::call(_m.data, _m.reservEnd, _m);
}


template< template<typename> typename ElemStruct, typename Alloc >
struct_of_growarr<ElemStruct, Alloc>::struct_of_growarr(struct_of_growarr && other, Alloc a)
 :	_m(a) // moves from a
{
	const allocator_type & myA = _m;
	OEL_CONST_COND if (!_alloTrait::is_always_equal::value and myA != other._m)
		append(other | view::move);
	else
		_moveInternBase(other._m);
}


template< template<typename> typename ElemStruct, typename Alloc >
struct_of_growarr<ElemStruct, Alloc> &
	struct_of_growarr<ElemStruct, Alloc>::operator =(struct_of_growarr && other) &
	noexcept(_alloTrait::propagate_on_container_move_assignment::value or _alloTrait::is_always_equal::value)
{
	allocator_type & myA = _m;
	OEL_CONST_COND if (!_alloTrait::propagate_on_container_move_assignment::value and myA != other._m)
	{
		assign(other | view::move);
	}
	else // take allocated memory from other
	{
		if (_m.data)
		{
			_detail::Destroy(_m.data, _m.end);
			_allocateWrap::dealloc(_m, _m.data, _m.capacity);
		}
		_moveInternBase(other._m);
		if constexpr (_alloTrait::propagate_on_container_move_assignment::value)
			myA = static_cast<allocator_type &&>(other._m);
	}
	return *this;
}


template< template<typename> typename ElemStruct, typename Alloc >
void struct_of_growarr<ElemStruct, Alloc>::shrink_to_fit()
{
	if (_m.size != 0)
	{
		_realloc(_m.size);
	}
	else
	{	_m.size = 0;
		_resetData(nullptr, 0);
	}
}


template< template<typename> typename ElemStruct, typename Alloc >
void struct_of_growarr<ElemStruct, Alloc>::erase_to_end(size_type first) noexcept
{
	T *const newEnd{to_pointer_contiguous(first)};
	OEL_ASSERT(_m.data <= newEnd and newEnd <= _m.end);

	_detail::Destroy(newEnd, _m.end);
	_m.end = newEnd;
}


template< template<typename> typename ElemStruct, typename Alloc >
void struct_of_growarr<ElemStruct, Alloc>::unordered_erase(size_type pos)
{
	if constexpr (is_trivially_relocatable<T>::value)
	{
		T & elem = *pos;
		elem.~T();

		--_m.end;

		auto & mem = reinterpret_cast< storage_for<T> & >(elem);
		mem     = *reinterpret_cast< storage_for<T> * >(_m.end); // relocate last element to pos
	}
	else
	{	*pos = std::move(back());
		pop_back();
	}
	return pos;
}


template< template<typename> typename ElemStruct, typename Alloc >
void struct_of_growarr<ElemStruct, Alloc>::erase(size_type pos)
{
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


template< template<typename> typename ElemStruct, typename Alloc >
void struct_of_growarr<ElemStruct, Alloc>::erase(size_type first, size_type last)
{
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


#if defined __GNUC__ and __GNUC__ < 12
	template< template<typename> typename E, typename A >
	explicit struct_of_growarr(const struct_of_growarr<E, A> &) -> struct_of_growarr<E, A>;
#endif

} // namespace oel
