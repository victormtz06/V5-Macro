#pragma once

#include "world_state.hpp"

#include <vector>

namespace v5pf {

std::vector<int> encodeNodeFlags(
  const WorldSnapshot& world,
  const std::vector<Int3>& nodes,
  bool isFly,
  bool includeProximity
);

std::vector<int> encodeKeyMetrics(const WorldSnapshot& world, const std::vector<Int3>& nodes);

} // namespace v5pf
