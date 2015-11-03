#pragma once

// Copyright 2014, 2015 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "basic_util.h"

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

template<typename Iterator, typename Size = std::size_t>
struct counted_view
{
	using value_type      = typename std::iterator_traits<Iterator>::value_type;
	using reference       = typename std::iterator_traits<Iterator>::reference;
	using const_reference = reference;
	using iterator        = Iterator;
	using const_iterator  = Iterator;
	using difference_type = typename std::iterator_traits<Iterator>::difference_type;
	using size_type       = Size;

	Iterator first;
	Size     count;

	counted_view()  : first(), count() {}

	counted_view(iterator first, size_type count)  : first(first), count(count) {}

	template<typename Container>
	counted_view(Container & ctr)  : first(begin(ctr)), count(oel::ssize(ctr)) {}

	Size     size() const   { return count; }
	bool     empty() const  { return 0 == count; }

	Iterator begin() const                           { return first; }
	// -- Rest only for random-access Iterator -- //
	template<typename /*dummy*/ = Iterator>
	auto     end() const -> decltype(first + count)  { return first + count; }

	template<typename Integer>
	auto operator[](Integer index) const -> decltype(first[index])  { return first[index]; }
};

/// Create an iterator_range from two iterators
template<typename Iterator> inline
iterator_range<Iterator> make_range(Iterator first, Iterator last)  { return {first, last}; }
/// Create a counted_view from iterator and size
template<typename Iterator> inline
counted_view<Iterator>   as_counted_view(Iterator first, size_t count)  { return {first, count}; }

/// Create an iterator_range of move_iterator from two iterators
template<typename InputIterator>
iterator_range< std::move_iterator<InputIterator> >   move_range(InputIterator first, InputIterator last);
/// Create an iterator_range of move_iterator from reference to a range (array or container)
template<typename InputRange> inline
auto move_range(InputRange & r)
 -> iterator_range< std::move_iterator<decltype(begin(r))> >  { return oel::move_range(begin(r), end(r)); }
/// Create an iterator_range of move_iterator from iterator_range
template<typename InputIterator> inline
iterator_range< std::move_iterator<InputIterator> >   move_range(const iterator_range<InputIterator> & r)
	{ return oel::move_range(r.begin(), r.end()); }
/// Create a counted_view of move_iterator from counted_view
template<typename InputIterator, typename S> inline
counted_view< std::move_iterator<InputIterator>, S >  move_range(const counted_view<InputIterator, S> & r)
	{ return {std::make_move_iterator(r.begin()), count}; }
/// Create a counted_view of move_iterator from iterator and size
template<typename InputIterator>
counted_view< std::move_iterator<InputIterator> >     move_range_n(InputIterator first, size_t count)
	{ return oel::as_counted_view(std::make_move_iterator(first), count); }

///
template<typename T>
counted_view<unsigned char *> as_binary(T & obj)
{
	static_assert(is_trivially_copyable<T>::value, "Unsafe");
	return {reinterpret_cast<unsigned char *>(&obj, sizeof obj)};
}
///
template<typename T>
counted_view<const unsigned char *> as_binary(const T & obj)
{
	static_assert(is_trivially_copyable<T>::value, "Unsafe");
	return {reinterpret_cast<const unsigned char *>(&obj, sizeof obj)};
}

} // namespace oel



////////////////////////////////////////////////////////////////////////////////
//
// Implementation


template<typename InputIterator>
inline oel::iterator_range< std::move_iterator<InputIterator> >  oel::move_range(InputIterator first, InputIterator last)
{
	using MoveIt = std::move_iterator<InputIterator>;
	return oel::make_range(MoveIt{first}, MoveIt{last});
}