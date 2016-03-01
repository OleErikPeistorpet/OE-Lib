#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "core_util.h"

#ifndef OEL_NO_BOOST
	#include <boost/range/iterator_range_core.hpp>
#endif


namespace oel
{

#ifndef OEL_NO_BOOST
	using boost::iterator_range;
#else
	template<typename Iterator>
	struct iterator_range
	{
		Iterator first;
		Iterator last;

		Iterator begin() const  { return first; }
		Iterator end() const    { return last; }
	};
#endif

/// Create an iterator_range from two iterators, with type deduced from arguments
template<typename Iterator> inline
iterator_range<Iterator> as_view(Iterator first, Iterator last)  { return {first, last}; }

/// Create an iterator_range from range reference, adding advanceBegin to begin(r) and advanceEnd to end(r)
template<typename RandomAccessRange> inline
auto as_view(RandomAccessRange & r,
				difference_type<decltype(begin(r))> advanceBegin, difference_type<decltype(begin(r))> advanceEnd)
 -> iterator_range<decltype(begin(r))>
{
	return {begin(r) + advanceBegin, end(r) + advanceEnd};
}
/// Returns iterator_range with advanceBegin added to begin(r) and advanceEnd added to end(r)
template<typename RandomAccessIterator> inline
iterator_range<RandomAccessIterator> as_view(const iterator_range<RandomAccessIterator> & r,
			difference_type<RandomAccessIterator> advanceBegin, difference_type<RandomAccessIterator> advanceEnd)
{
	return {r.begin() + advanceBegin, r.end() + advanceEnd};
}

/// Create an iterator_range from range reference with begin(r) incremented
template<typename InputRange> inline
auto as_view_increment_begin(InputRange & r)
 -> iterator_range<decltype(begin(r))>  { return {++begin(r), end(r)}; }
/// Returns iterator_range with begin(r) incremented
template<typename InputIterator> inline
iterator_range<InputIterator> as_view_increment_begin(const iterator_range<InputIterator> & r)
	{ return {++r.begin(), r.end()}; }

/// Create an iterator_range from range reference with end(r) decremented
template<typename BidirectionTravRange> inline
auto as_view_decrement_end(BidirectionTravRange & r)
 -> iterator_range<decltype(begin(r))>  { return {begin(r), --end(r)}; }
/// Returns iterator_range with end(r) decremented
template<typename BidirectionTravIterator> inline
iterator_range<BidirectionTravIterator> as_view_decrement_end(const iterator_range<BidirectionTravIterator> & r)
	{ return {r.begin(), --r.end()}; }


/// Wrapper for iterator and size. Similar to gsl::span, less safe, but not just for arrays
template<typename Iterator>
class counted_view
{
public:
	using iterator        = Iterator;
	using value_type      = typename std::iterator_traits<Iterator>::value_type;
	using reference       = typename std::iterator_traits<Iterator>::reference;
	using difference_type = typename std::iterator_traits<Iterator>::difference_type;
	using size_type       = size_t;  // difference_type would be OK

	/// Initialize to empty
	counted_view()                                : _count() {}
	counted_view(iterator first, size_type size)  : _begin(first), _count(size) {}
	/// Construct from container with matching iterator type
	template<typename Container>
	counted_view(Container & c)  : _begin(::adl_begin(c)), _count(oel::ssize(c)) {}

	size_type size() const   { return _count; }

	bool      empty() const  { return 0 == _count; }

/// Create a std::move_iterator from InputIterator
template<typename InputIterator> inline
std::move_iterator<InputIterator> make_move_iter(InputIterator it)  { return std::move_iterator<InputIterator>(it); }


/// Create an iterator_range from two iterators
template<typename Iterator> inline
iterator_range<Iterator> make_range(Iterator first, Iterator last)  { return {first, last}; }

/// Create an iterator_range of move_iterator from range reference (array or STL container)
template<typename InputRange> inline
auto move_range(InputRange & r) -> iterator_range< std::move_iterator<decltype(begin(r))> >
{
	return oel::make_range( make_move_iter(begin(r)), make_move_iter(end(r)) );
}
/// Create an iterator_range of move_iterator from iterator_range
template<typename InputIterator> inline
iterator_range< std::move_iterator<InputIterator> >  move_range(const iterator_range<InputIterator> & r)
{
	return oel::make_range( make_move_iter(r.begin()), make_move_iter(r.end()) );
}
/// Create an iterator_range of move_iterator from two iterators
template<typename InputIterator> inline
iterator_range< std::move_iterator<InputIterator> >  move_range(InputIterator first, InputIterator last)
{
	return oel::make_range(make_move_iter(first), make_move_iter(last));
}

} // namespace oel
