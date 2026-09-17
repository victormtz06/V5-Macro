#pragma once

#include "world_state.hpp"

#include <atomic>
#include <optional>

namespace v5pf {

struct AvoidZone {
  int x;
  int y;
  int z;
  int radiusSq;
  int maxYDiff;
  float penalty;
};

struct SearchParams {
  std::vector<Int3> starts;
  std::vector<Int3> goals;
  bool isFly = false;
  int maxIterations = 500000;
  float heuristicWeight = 1.05f;
  float nonPrimaryStartPenalty = 0.0f;
  int moveOrderOffset = 0;
  std::vector<AvoidZone> avoidZones;
};

struct SearchResult {
  std::vector<Int3> points;
  std::vector<Int3> keyPoints;
  std::vector<int> pathFlags;
  std::vector<int> keyNodeFlags;
  std::vector<int> keyNodeMetrics;
  std::string signatureHex;
  long long timeMs = 0;
  int nodesExplored = 0;
  double nanosecondsPerNode = 0.0;
  int selectedStartIndex = -1;
};

std::optional<SearchResult> findPath(
  const WorldSnapshot& world,
  const SearchParams& params,
  std::atomic_bool& cancelFlag
);

} // namespace v5pf
