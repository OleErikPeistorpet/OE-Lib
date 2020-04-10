// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


namespace _detail
{
	template< typename Sentinel, typename Iterator, typename Range, typename... None >
	constexpr basic_view<Iterator, Sentinel>  all(Iterator first, Range & r, None...)
	{
		return{ first, Sentinel{end(r)} };
	}

	template< typename, typename Iterator, typename SizedRange >
	constexpr auto all(Iterator first, SizedRange & r)
	->	decltype( counted_view<Iterator>{first, oel::ssize(r)} )
		 { return counted_view<Iterator>{first, oel::ssize(r)}; }
}