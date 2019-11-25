// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


namespace _detail
{
	template< typename SizedRange >
	auto Move(SizedRange & r, int)
	->	decltype( view::counted(std::make_move_iterator(begin(r)), oel::ssize(r)) )
		 { return view::counted(std::make_move_iterator(begin(r)), oel::ssize(r)); }

	template< typename InputRange >
	OEL_ALWAYS_INLINE inline auto Move(InputRange & r, long)
	{
		return view::move(begin(r), end(r));
	}
}