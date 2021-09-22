#pragma once

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include <functional>


namespace oel
{

//* @brief Wrap a function pointer into an empty function object type
*
* http://www.open-std.org/jtc1//sc22//wg21/docs/papers/2016/p0477r0.pdf  <br>
* https://en.cppreference.com/w/cpp/utility/functional/invoke  */
template< auto Callable >
struct monostate_function
{
	using is_transparent = void;

	template< typename... Args >
	constexpr auto operator()(Args &&... args) const
		noexcept(noexcept( std::invoke(Callable, static_cast<Args &&>(args)...) ))
	->	decltype(          std::invoke(Callable, static_cast<Args &&>(args)...) )
		         { return  std::invoke(Callable, static_cast<Args &&>(args)...); }
};

}