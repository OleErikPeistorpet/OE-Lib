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


/** @file
*/

namespace oel
{

#ifndef OEL_NO_BOOST
	using boost::iterator_range;
	using boost::make_iterator_range;
#else
	template<typename Iterator>
	class iterator_range
	{
	public:
		using iterator        = Iterator;
		using difference_type = typename iterator_traits<Iterator>::difference_type;

		iterator_range(Iterator f, Iterator l)  : _begin(f), _end(l) {}

		iterator begin() const  { return _begin; }
		iterator end() const    { return _end; }

		template<typename It = Iterator,
		         enable_if<std::is_base_of< random_access_traversal_tag, iterator_traversal_t<It> >::value> = 0>
		difference_type size() const  { return _end - _begin; }

	protected:
		Iterator _begin;
		Iterator _end;
	};

	/// Create an iterator_range from two iterators, with type deduced from arguments
	template<typename Iterator> inline
	iterator_range<Iterator> make_iterator_range(Iterator first, Iterator last)  { return {first, last}; }
#endif


/// Wrapper for iterator and size. Similar to gsl::span, less safe, but not just for arrays
template<typename Iterator,
         bool = std::is_base_of< random_access_traversal_tag, iterator_traversal_t<Iterator> >::value>
class counted_view
{
public:
	using iterator        = Iterator;
	using value_type      = typename iterator_traits<Iterator>::value_type;
	using difference_type = typename iterator_traits<Iterator>::difference_type;
	using size_type       = size_t;  // difference_type (signed) would be fine

	/// Initialize to empty
	counted_view() noexcept                      : _size() {}
	counted_view(Iterator f, difference_type n)  : _begin(f), _size(n) { OEL_ASSERT_MEM_BOUND(n >= 0); }
	/// Construct from array or container with matching iterator type
	template<typename SizedRange>
	counted_view(SizedRange & r)  : _begin(::adl_begin(r)), _size(oel::ssize(r)) {}

	iterator  begin() const           { return _begin; }

	size_type size() const noexcept   { return _size; }

	bool      empty() const noexcept  { return 0 == _size; }

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

	counted_view() noexcept                      {}
	counted_view(Iterator f, difference_type n)  : _base(f, n) {}
	template<typename SizedRange>
	counted_view(SizedRange & r)                 : _base(::adl_begin(r), oel::ssize(r)) {}

	iterator  end() const   { return this->_begin + this->_size; }

	reference operator[](difference_type index) const  { return this->_begin[index]; }

	/// Return plain pointer to underlying array. Will only be found with contiguous Iterator (see to_pointer_contiguous)
	template<typename It = Iterator>
	auto data() const noexcept
	 -> decltype( to_pointer_contiguous(std::declval<It>()) )  { return to_pointer_contiguous(this->_begin); }
};

/// Make views. View is a concept described in the documentation for the Range v3 library
namespace view
{

/// Create a counted_view from iterator and count, with type deduced from first
template<typename Iterator> inline
counted_view<Iterator> counted(Iterator first, typename iterator_traits<Iterator>::difference_type count)
	{ return {first, count}; }


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

#include "auxi/view_detail_move.inc"

/** @brief Wrap a range such that the elements can be moved from when passed to a container or algorithm
* @return counted_view<std::move_iterator> if r.size() exists or r is an array, else iterator_range<std::move_iterator>  */
template<typename InputRange> inline
auto move(InputRange & r)
 -> decltype( _detail::Move(r, int{}) )
     { return _detail::Move(r, int{}); }


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
