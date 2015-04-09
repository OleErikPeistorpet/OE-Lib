#pragma once

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "../container_core.h"

#include <Eigen/Dense>


namespace oel
{

template<typename S, int R, int C, int O, int MR, int MC>
struct is_trivially_relocatable< Eigen::Matrix<S, R, C, O, MR, MC> >
 :	std::true_type {};

template<typename S, int O>
struct is_trivially_relocatable< Eigen::Quaternion<S, O> > : std::true_type {};

template<typename S, int D, int M, int O>
struct is_trivially_relocatable< Eigen::Transform<S, D, M, O> >
 :	std::true_type {};

template<typename S>
struct is_trivially_relocatable< Eigen::AngleAxis<S> > : std::true_type {};

template<typename S>
struct is_trivially_relocatable< Eigen::Rotation2D<S> > : std::true_type {};

}