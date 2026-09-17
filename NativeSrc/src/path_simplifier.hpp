#pragma once

#include "world_state.hpp"

#include <vector>

namespace v5pf {

std::vector<Int3> extractKeyPoints(
  const WorldSnapshot& world,
  const std::vector<Int3>& points,
  bool isFly,
  double epsilon = 1.7
);

} // namespace v5pf
