#pragma once

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "../user_traits.h"

#include <Eigen/Dense>


namespace oel
{

template<typename S, int R, int C, int O, int MR, int MC>
true_type specify_trivial_relocate(Eigen::Matrix<S, R, C, O, MR, MC>);

template<typename S, int O>
true_type specify_trivial_relocate(Eigen::Quaternion<S, O>);

template<typename S, int D, int M, int O>
true_type specify_trivial_relocate(Eigen::Transform<S, D, M, O>);

template<typename S>
true_type specify_trivial_relocate(Eigen::AngleAxis<S>);

template<typename S>
true_type specify_trivial_relocate(Eigen::Rotation2D<S>);

}