// Copyright 2020 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


namespace _detail
{
	template< typename ViewIter, typename Range, typename... None >
	basic_view<ViewIter> all(Range & r, None...)
	{
		return{ ViewIter{begin(r)}, ViewIter{end(r)} };
	}

	template< typename ViewIter, typename SizedRange >
	auto all(SizedRange & r)
	->	decltype(counted_view<ViewIter>( ViewIter{begin(r)}, oel::ssize(r) ))
	{	return   counted_view<ViewIter>( ViewIter{begin(r)}, oel::ssize(r) ); }



	template< typename Func, typename Iterator >
	struct Transf
	{
		using TI = transform_iterator<Func, Iterator>;

		template< typename F_ = Func,
		          enable_if< std::is_empty<F_>::value > = 0
		>
		static TI _sentinel(Func f, Iterator last)
		{
			return {f, last};
		}

		template< typename... None >
		static Iterator _sentinel(Func, Iterator last, None...)
		{
			return last;
		}


		static auto call(basic_view<Iterator> v, Func f)
		{
			return view::subrange( TI{f, v.begin()}, _sentinel(f, v.end()) );
		}

		static auto call(counted_view<Iterator> v, Func f)
		{
			return counted_view<TI>( {f, v.begin()}, v.size() );
		}
	};
}