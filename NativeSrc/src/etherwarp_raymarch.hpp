#pragma once

#include "world_state.hpp"

#include <optional>

namespace v5pf {

struct EtherwarpRayDirection {
  float yaw = 0.0f;
  float pitch = 0.0f;
  double dirX = 0.0;
  double dirY = 0.0;
  double dirZ = 0.0;
};

std::optional<Int3> raymarchEtherwarp(
  const WorldSnapshot& world,
  double originX,
  double originY,
  double originZ,
  const EtherwarpRayDirection& direction,
  double maxDistance
);

} // namespace v5pf
