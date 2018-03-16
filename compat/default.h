#pragma once

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


// std:: unique_ptr, shared_ptr, weak_ptr, basic_string (pair is_trivially_copyable)
// boost:: intrusive_ptr, circular_buffer

// This file is included by dynarray.h, so should not be needed in user code

#include "../user_traits.h"

#include <memory>

#if defined __has_include
#if __has_include(<memory_resource>)
	#include <memory_resource>

	#define OEL_HAS_STD_PMR 1
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


// std::string in GCC 5 with _GLIBCXX_USE_CXX11_ABI is not trivially relocatable (uses pointer to internal buffer)
#if (_MSC_VER || _LIBCPP_VERSION || __GLIBCXX__) && !_GLIBCXX_USE_CXX11_ABI
	#include <string>

	namespace oel
	{
	template<typename C, typename Tr, typename Alloc>
	struct is_trivially_relocatable< std::basic_string<C, Tr, Alloc> >
	 :	is_trivially_relocatable<Alloc> {};
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

template<typename T, typename U>
struct is_trivially_copyable< std::pair<T, U> >
 :	all_< is_trivially_copyable<T>, is_trivially_copyable<U> > {};

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

}