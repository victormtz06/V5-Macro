#pragma once

#include "world_state.hpp"

#include <atomic>
#include <optional>

namespace v5pf {

inline constexpr double ETHERWARP_STANDING_EYE_HEIGHT = 2.62;
inline constexpr double ETHERWARP_LEGACY_SNEAK_OFFSET = 0.08;
inline constexpr double ETHERWARP_MODERN_SNEAK_OFFSET = 0.35;
inline constexpr double ETHERWARP_LEGACY_EYE_HEIGHT =
  ETHERWARP_STANDING_EYE_HEIGHT - ETHERWARP_LEGACY_SNEAK_OFFSET;
inline constexpr double ETHERWARP_MODERN_EYE_HEIGHT =
  ETHERWARP_STANDING_EYE_HEIGHT - ETHERWARP_MODERN_SNEAK_OFFSET;

struct EtherwarpSearchParams {
  Int3 goal{0, 0, 0};
  double startEyeX = 0.0;
  double startEyeY = 0.0;
  double startEyeZ = 0.0;
  int maxIterations = 100000;
  int threadCount = 1;
  double yawStep = 5.0;
  double pitchStep = 5.0;
  double newNodeCost = 1.0;
  double heuristicWeight = 1.0;
  double rayLength = 61.0;
  double rewireEpsilon = 1.0;
  double eyeHeight = ETHERWARP_LEGACY_EYE_HEIGHT;
};

struct EtherwarpSearchResult {
  std::vector<Int3> points;
  std::vector<float> angles;
  long long timeMs = 0;
  int nodesExplored = 0;
  double nanosecondsPerNode = 0.0;
};

std::optional<EtherwarpSearchResult> findEtherwarpPath(
  const WorldSnapshot& world,
  const EtherwarpSearchParams& params,
  std::atomic_bool& cancelFlag
);

} // namespace v5pf
