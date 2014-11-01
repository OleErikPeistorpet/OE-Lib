#pragma once

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include <boost/move/iterator.hpp>
#include <boost/range/iterator_range_core.hpp>


namespace oetl
{

/// Create a boost::move_iterator from InputIterator
template<typename InputIterator> inline
boost::move_iterator<InputIterator> make_move_iter(InputIterator it)  { return boost::make_move_iterator(it); }


/// Create an iterator_range from two iterators
template<typename Iterator> inline
boost::iterator_range<Iterator> make_range(Iterator first, Iterator last)  { return {first, last}; }

/// Create an iterator_range of boost::move_iterator from range reference
template<typename InputRange> inline
auto move_range(InputRange && range) -> boost::iterator_range< boost::move_iterator<decltype(begin(range))> >
{
	return oetl::make_range( make_move_iter(begin(range)), make_move_iter(end(range)) );
}

/// Create an iterator_range of boost::move_iterator from two iterators
template<typename InputIterator> inline
boost::iterator_range< boost::move_iterator<InputIterator> >  move_range(InputIterator first, InputIterator last)
{
	return oetl::make_range(make_move_iter(first), make_move_iter(last));
}


////////////////////////////////////////////////////////////////////////////////


template<typename Iterator> inline
auto to_ptr(std::move_iterator<Iterator> it) NOEXCEPT
 -> decltype( to_ptr(it.base()) )  { return to_ptr(it.base()); }

} // namespace oetl
