#pragma once

// Copyright 2023 Ole Erik Peistorpet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "util.h" // for OutOfRange

#include <algorithm>
#include <iterator>


namespace oel
{

template< typename D >
struct _arrayInterface
{
	[[nodiscard]] bool empty() const noexcept   { return static_cast<const D &>(*this).size() == 0; }

	auto cbegin() const   OEL_ALWAYS_INLINE { return static_cast<const D &>(*this).begin(); }
	auto cend() const     OEL_ALWAYS_INLINE { return static_cast<const D &>(*this).end(); }

	auto rbegin()         OEL_ALWAYS_INLINE { return std::reverse_iterator{static_cast<D &>      (*this).end()}; }
	auto rbegin() const   OEL_ALWAYS_INLINE { return std::reverse_iterator{static_cast<const D &>(*this).end()}; }
	auto crbegin() const  OEL_ALWAYS_INLINE { return std::reverse_iterator{static_cast<const D &>(*this).end()}; }

	auto rend()         OEL_ALWAYS_INLINE { return std::reverse_iterator{static_cast<D &>      (*this).begin()}; }
	auto rend() const   OEL_ALWAYS_INLINE { return std::reverse_iterator{static_cast<const D &>(*this).begin()}; }
	auto crend() const  OEL_ALWAYS_INLINE { return std::reverse_iterator{static_cast<const D &>(*this).begin()}; }

	auto & front() noexcept        { return static_cast<D &>      (*this)[0]; }
	auto & front() const noexcept  { return static_cast<const D &>(*this)[0]; }

	auto & back() noexcept         { return static_cast<D &>      (*this).end()[-1]; }
	auto & back() const noexcept   { return static_cast<const D &>(*this).end()[-1]; }

	auto & at(size_t index)   OEL_ALWAYS_INLINE
		{
			const auto & cSelf = *this;
			using T = std::remove_const_t< std::remove_reference_t<decltype( cSelf.at(index) )> >;
			return const_cast<T &>(cSelf.at(index));
		}
	auto & at(size_t index) const
		{
			auto & derived = static_cast<const D &>(*this);
			if (index < static_cast<size_t>( derived.size() ))
				return derived.data()[index];
			else
				_detail::OutOfRange::raise("oel: at() bad index");
		}

	friend bool operator==(const D & left, const D & right)
		{
			return left.size() == right.size()
			       and std::equal(left.begin(), left.end(), right.begin());
		}
	friend bool operator!=(const D & left, const D & right)  { return !(left == right); }

	friend bool operator <(const D & left, const D & right)
		{
			return std::lexicographical_compare(left.begin(), left.end(), right.begin(), right.end());
		}
	friend bool operator >(const D & left, const D & right)  { return right < left; }
};

} // oel
