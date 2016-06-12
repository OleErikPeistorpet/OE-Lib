#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "core_util.h"

#ifndef OEL_NO_BOOST
	#include <boost/range/iterator_range_core.hpp>
	#include <boost/iterator/transform_iterator.hpp>
#endif


namespace oel
{

#ifndef OEL_NO_BOOST
	using boost::iterator_range;
#else
	template<typename Iterator>
	class iterator_range
	{
	public:
		Iterator first;
		Iterator last;

		iterator_range(Iterator f, Iterator l)  : first(f), last(l) {}

		Iterator begin() const  { return first; }
		Iterator end() const    { return last; }
	};
#endif

/// Create an iterator_range from two iterators, with type deduced from arguments
template<typename Iterator> inline
iterator_range<Iterator> make_view(Iterator first, Iterator last)  { return {first, last}; }


/// Wrapper for iterator and size. Similar to gsl::span, less safe, but not just for arrays
template< typename Iterator,
          bool = std::is_base_of<std::random_access_iterator_tag,
                                 typename iterator_traits<Iterator>::iterator_category>::value >
class counted_view
{
public:
	using iterator        = Iterator;
	using value_type      = typename iterator_traits<Iterator>::value_type;
	using difference_type = typename iterator_traits<Iterator>::difference_type;
	using size_type       = size_t;  // difference_type (signed) would be fine

	/// Initialize to empty
	counted_view() noexcept                              : _size() {}
	counted_view(iterator first, difference_type count)  : _begin(first), _size(count) { OEL_ASSERT_MEM_BOUND(count >= 0); }
	/// Construct from array or container with matching iterator type
	template<typename SizedRange>
	counted_view(SizedRange & r)  : _begin(::adl_begin(r)), _size(oel::ssize(r)) {}

	size_type size() const noexcept  { return _size; }

	iterator  begin() const          { return _begin; }

	/// Increment begin, decrementing size
	void      drop_front()  { OEL_ASSERT_MEM_BOUND(_size > 0);  ++_begin; --_size; }
	/// Decrement end
	void      drop_back()   { OEL_ASSERT_MEM_BOUND(_size > 0);  --_size; }

protected:
	Iterator  _begin;
	size_type _size;
};

/// Specialization for random-access Iterator
template<typename Iterator>
class counted_view<Iterator, true> : public counted_view<Iterator, false>
{
	using _base = counted_view<Iterator, false>;

public:
	using typename _base::iterator;
	using typename _base::value_type;
	using typename _base::difference_type;
	using typename _base::size_type;
	using reference = typename iterator_traits<Iterator>::reference;

	counted_view() noexcept                              {}
	counted_view(iterator first, difference_type count)  : _base(first, count) {}
	template<typename SizedRange>
	counted_view(SizedRange & r)                         : _base(::adl_begin(r), oel::ssize(r)) {}

	iterator  end() const   { return this->_begin + this->_size; }

	reference operator[](difference_type index) const  { return this->_begin[index]; }

	/// Return plain pointer to underlying array. Will only be found with contiguous Iterator (see to_pointer_contiguous)
	template<typename It1 = Iterator>
	auto data() const noexcept
	 -> decltype( to_pointer_contiguous(std::declval<It1>()) )  { return to_pointer_contiguous(this->_begin); }
};

/// Create a counted_view from iterator and count, with type deduced from first
template<typename Iterator> inline
counted_view<Iterator> make_view_n(Iterator first, typename iterator_traits<Iterator>::difference_type count)
	{ return {first, count}; }


namespace view
{

/// Create an iterator_range of std::move_iterator from two iterators
template<typename InputIterator> inline
iterator_range< std::move_iterator<InputIterator> >
	move(InputIterator first, InputIterator last)  { using MoveIt = std::move_iterator<InputIterator>;
	                                                 return {MoveIt{first}, MoveIt{last}}; }

/// Create a counted_view with move_iterator from iterator and count
template<typename InputIterator> inline
counted_view< std::move_iterator<InputIterator> >
	move_n(InputIterator first, typename iterator_traits<InputIterator>::difference_type count)
	{ return {std::make_move_iterator(first), count}; }

/// Create a counted_view with move_iterator from a range with size() member or an array
template<typename SizedRange> inline
auto move(SizedRange & r)
 -> counted_view< std::move_iterator<decltype(begin(r))> >  { return view::move_n(begin(r), oel::ssize(r)); }

/// Create an iterator_range of move_iterator from a range
template<typename InputRange> inline
auto move_iter_rng(InputRange & r)
 -> iterator_range< std::move_iterator<decltype(begin(r))> >  { return view::move(begin(r), end(r)); }


#ifndef OEL_NO_BOOST
	/** @brief Create a view with boost::transform_iterator from a range with size() member or an array
	*
	* Similar to boost::adaptors::transform, but more efficient with typical use  */
	template<typename UnaryFunc, typename SizedRange> inline
	auto transform(SizedRange & r, UnaryFunc f)
	 -> counted_view< boost::transform_iterator<UnaryFunc, decltype(begin(r))> >
		{ return {boost::make_transform_iterator(begin(r), std::move(f)), oel::ssize(r)}; }
#endif
}

} // namespace oel
