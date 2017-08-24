#pragma once

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


// is_trivially_relocatable std::array
// is_trivially_copyable std:: reference_wrapper, tuple

#include "../user_traits.h"

#include <array>
#include <tuple>
#include <functional> // for reference_wrapper

namespace oel
{

template<typename T, std::size_t S>
struct is_trivially_relocatable< std::array<T, S> >
 :	is_trivially_relocatable<T> {};

#if _MSC_VER < 1900 && __GNUC__ < 5 && !_LIBCPP_VERSION  // VC++ 2015, GCC 5, libc++ all good
	template<typename T>
	struct is_trivially_copyable< std::reference_wrapper<T> > : true_type{};
#endif

template<typename... Ts>
struct is_trivially_copyable< std::tuple<Ts...> >
 :	all_< is_trivially_copyable<Ts>... > {};

}