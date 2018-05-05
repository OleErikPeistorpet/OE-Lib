#pragma once

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


// std:: unique_ptr, shared_ptr, weak_ptr, basic_string, array
// boost:: intrusive_ptr, circular_buffer, polymorphic_allocator
// is_trivially_copyable std:: reference_wrapper, pair, tuple

// This file is included by dynarray.h, so should not be needed in user code

#include "../auxi/type_traits.h"

#include <memory>
#include <array>
#include <tuple>

#if defined __has_include
#if __has_include(<memory_resource>) and (!defined _MSC_VER or _HAS_CXX17)
	#include <memory_resource>

	#define OEL_HAS_STD_PMR  1
#endif
#endif

#ifndef OEL_NO_BOOST

#include <boost/circular_buffer_fwd.hpp>
#include <boost/container/container_fwd.hpp>

namespace boost
{
	template<typename T> class intrusive_ptr;
}
#endif


// std::string in GNU library with _GLIBCXX_USE_CXX11_ABI is not trivially relocatable (uses pointer to internal buffer)
#if (defined _CPPLIB_VER or defined _LIBCPP_VERSION or defined __GLIBCXX__) and !_GLIBCXX_USE_CXX11_ABI

#include <string>

namespace oel
{
	template<typename C, typename Tr, typename Alloc>
	struct is_trivially_relocatable< std::basic_string<C, Tr, Alloc> >
	 :	is_trivially_relocatable<Alloc> {};
}
#endif

#if defined __GLIBCXX__ and __GNUC__ == 4  // MSVC, GCC 5, libc++ all good

#include <functional>

namespace oel
{	template<typename T>
	struct is_trivially_copyable< std::reference_wrapper<T> > : true_type{};
}
#endif

namespace oel
{

//! std::unique_ptr assumed trivially relocatable if the deleter is
template<typename T, typename Del>
struct is_trivially_relocatable< std::unique_ptr<T, Del> >
 :	is_trivially_relocatable<Del> {};

template<typename T>
struct is_trivially_relocatable< std::shared_ptr<T> > : true_type {};

template<typename T>
struct is_trivially_relocatable< std::weak_ptr<T> > : true_type {};

template<typename T, std::size_t S>
struct is_trivially_relocatable< std::array<T, S> >
 :	is_trivially_relocatable<T> {};

#ifdef OEL_HAS_STD_PMR
	//! Should work with any reasonable implementation, but can't be sure without testing
	template<typename T>
	struct is_trivially_relocatable< std::pmr::polymorphic_allocator<T> > : true_type {};
#endif

#ifndef OEL_NO_BOOST
	#if BOOST_VERSION >= 106000
	template<typename T>
	struct is_trivially_relocatable< boost::container::pmr::polymorphic_allocator<T> > : true_type {};
	#endif

	template<typename T>
	struct is_trivially_relocatable< boost::intrusive_ptr<T> > : true_type {};

	template<typename T, typename Alloc>
	struct is_trivially_relocatable< boost::circular_buffer<T, Alloc> >
	 :	is_trivially_relocatable<Alloc> {};
#endif


template<typename T, typename U>
struct is_trivially_copyable< std::pair<T, U> >
 :	all_< is_trivially_copyable<T>, is_trivially_copyable<U> > {};

template<typename... Ts>
struct is_trivially_copyable< std::tuple<Ts...> >
 :	all_< is_trivially_copyable<Ts>... > {};

}