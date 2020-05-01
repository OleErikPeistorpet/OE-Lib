// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


namespace _detail
{
	template< typename SizedRange >
	inline auto Move(
		SizedRange & r,
		decltype( oel::ssize(r), int() )) // best match for int if ssize(r) is well-formed (SFINAE)
	{
		return view::counted( std::make_move_iterator(begin(r)), oel::ssize(r) );
	}

	template< typename InputRange >
	inline auto Move(InputRange & r, long)
	{
		return view::move(begin(r), end(r));
	}


	template< typename SizedRange, typename UnaryFunc >
	auto Transform(
		SizedRange & r, UnaryFunc f,
		decltype(oel::ssize(r), int()) )
	{
		return counted_view< transform_iterator<UnaryFunc, decltype( begin(r) )> >
			{ {f, begin(r)}, oel::ssize(r) };
	}

	template< typename Range, typename UnaryFunc >
	inline auto Transform(Range & r, UnaryFunc f, long)
	{
		using TraI = transform_iterator<UnaryFunc, decltype( begin(r) )>;
		return view::subrange( TraI{f, begin(r)}, end(r) );
	}
}