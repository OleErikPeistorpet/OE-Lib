#pragma once

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include "../container_core.h"

#include <Eigen/Dense>


namespace oetl
{
template<typename S, int R, int C, int O, int MR, int MC>
struct is_trivially_relocatable< Eigen::Matrix<S, R, C, O, MR, MC> > : std::true_type {};
}