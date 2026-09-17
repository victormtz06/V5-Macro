#include "etherwarp_search.hpp"

#include "etherwarp_raymarch.hpp"
#include "pathfinder_heap.hpp"
#include "path_voxel_checks.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <limits>
#include <mutex>
#include <optional>
#include <thread>
#include <unordered_map>
#include <unordered_set>

namespace v5pf {
namespace {

constexpr double DEG_TO_RAD = 3.14159265358979323846 / 180.0;
constexpr double RAD_TO_DEG = 180.0 / 3.14159265358979323846;
constexpr double ADAPTIVE_COARSE_MULTIPLIER = 3.0;
constexpr double ROBUSTNESS_INITIAL_MARGIN = 0.05;
constexpr double ROBUSTNESS_MAX_MARGIN = 3.0;
constexpr int ROBUSTNESS_BINARY_SEARCH_STEPS = 5;
constexpr double ROBUSTNESS_EPSILON = 1e-6;
constexpr int MAX_REFINEMENT_SEEDS = 8;
constexpr int EXPANSION_BATCH_SIZE = 4;

struct NeighborCandidate {
  Int3 pos{};
  float yaw = std::numeric_limits<float>::quiet_NaN();
  float pitch = std::numeric_limits<float>::quiet_NaN();
  int depth = 0;
  double g = std::numeric_limits<double>::infinity();
  double h = 0.0;
  double f = std::numeric_limits<double>::infinity();
};

struct AngularSample {
  EtherwarpRayDirection direction{};
  int id = -1;
};

struct AngularRow {
  float pitch = 0.0f;
  std::vector<AngularSample> samples;
};

struct AngularLattice {
  std::vector<AngularSample> coarseSamples;
  std::vector<AngularRow> fineRows;
  double coarseYawWindow = 0.0;
  double coarsePitchWindow = 0.0;
  int fineSampleCount = 0;
};

struct RefinementSeed {
  float yaw = 0.0f;
  float pitch = 0.0f;
  double h = std::numeric_limits<double>::infinity();
};

struct WorkerScratch {
  std::vector<NeighborCandidate> neighbors;
  std::unordered_set<uint64_t> seenHits;
  std::vector<RefinementSeed> refinementSeeds;
  std::vector<uint32_t> fineVisitStamp;
  uint32_t fineVisitEpoch = 1;

  explicit WorkerScratch(const AngularLattice& lattice) {
    neighbors.reserve(512);
    seenHits.reserve(2048);
    refinementSeeds.reserve(MAX_REFINEMENT_SEEDS + 2);
    fineVisitStamp.assign(static_cast<size_t>(std::max(0, lattice.fineSampleCount)), 0U);
  }

  void beginNode() {
    neighbors.clear();
    seenHits.clear();
    refinementSeeds.clear();
    fineVisitEpoch++;
    if (fineVisitEpoch == 0) {
      std::fill(fineVisitStamp.begin(), fineVisitStamp.end(), 0U);
      fineVisitEpoch = 1;
    }
  }
};

struct BatchExpansion {
  int currIdx = -1;
  Int3 current{};
  double currG = 0.0;
  int currDepth = 0;
  bool isGoal = false;
  std::vector<NeighborCandidate> neighbors;
};

struct SimplifiedEtherwarpPath {
  std::vector<Int3> points;
  std::vector<float> angles;
};

struct SharedState {
  explicit SharedState(const int reserveTarget)
    : heap(nodeF, nodeH, nodeG, nodeHeapPos) {
    nodeX.reserve(static_cast<size_t>(reserveTarget));
    nodeY.reserve(static_cast<size_t>(reserveTarget));
    nodeZ.reserve(static_cast<size_t>(reserveTarget));
    nodeParent.reserve(static_cast<size_t>(reserveTarget));
    nodeDepth.reserve(static_cast<size_t>(reserveTarget));
    nodeG.reserve(static_cast<size_t>(reserveTarget));
    nodeH.reserve(static_cast<size_t>(reserveTarget));
    nodeF.reserve(static_cast<size_t>(reserveTarget));
    nodeYaw.reserve(static_cast<size_t>(reserveTarget));
    nodePitch.reserve(static_cast<size_t>(reserveTarget));
    nodeHeapPos.reserve(static_cast<size_t>(reserveTarget));
    coordToNode.reserve(static_cast<size_t>(reserveTarget));
    heap.reserve(reserveTarget);
  }

  std::mutex mutex;
  std::condition_variable cv;

  std::vector<int> nodeX;
  std::vector<int> nodeY;
  std::vector<int> nodeZ;
  std::vector<int> nodeParent;
  std::vector<int> nodeDepth;
  std::vector<double> nodeG;
  std::vector<double> nodeH;
  std::vector<double> nodeF;
  std::vector<float> nodeYaw;
  std::vector<float> nodePitch;
  std::vector<int> nodeHeapPos;
  std::unordered_map<uint64_t, int> coordToNode;
  detail::Heap<double> heap;

  int iterations = 0;
  int activeExpanders = 0;
  bool solved = false;
  bool exhausted = false;
  int solutionIdx = -1;
};

struct AimSampleOffset {
  double x = 0.5;
  double z = 0.5;
};

struct AngleRobustnessScore {
  float yaw = std::numeric_limits<float>::quiet_NaN();
  float pitch = std::numeric_limits<float>::quiet_NaN();
  double margin = 0.0;
  double baselineDistance = std::numeric_limits<double>::infinity();
  double centerDistance = std::numeric_limits<double>::infinity();
};

struct EyeOrigin {
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
};

inline float normalizeYawDegrees(double yaw) {
  yaw = std::fmod(yaw, 360.0);
  if (yaw < 0.0) yaw += 360.0;
  return static_cast<float>(yaw);
}

inline EyeOrigin resolveEyeOriginFromLanding(
  const WorldSnapshot& world,
  const Int3& from,
  const double eyeHeight
) {
  const uint16_t fromSupportFlags = flagsAt(world, from.x, from.y, from.z);
  return EyeOrigin{
    static_cast<double>(from.x) + 0.5,
    static_cast<double>(from.y) + etherwarpEyeYOffset(fromSupportFlags, eyeHeight),
    static_cast<double>(from.z) + 0.5
  };
}

inline double circularAngleDistance(const float a, const float b) {
  double delta = std::abs(static_cast<double>(a) - static_cast<double>(b));
  if (delta > 180.0) {
    delta = 360.0 - delta;
  }
  return delta;
}

inline EtherwarpRayDirection makeRayDirectionFromAngles(const float yaw, const float pitch) {
  const double yawRad = static_cast<double>(yaw) * DEG_TO_RAD;
  const double pitchRad = static_cast<double>(pitch) * DEG_TO_RAD;
  const double cosPitch = std::cos(pitchRad);

  EtherwarpRayDirection direction;
  direction.yaw = normalizeYawDegrees(yaw);
  direction.pitch = pitch;
  direction.dirX = -std::sin(yawRad) * cosPitch;
  direction.dirY = -std::sin(pitchRad);
  direction.dirZ = std::cos(yawRad) * cosPitch;
  return direction;
}

inline std::optional<EtherwarpRayDirection> makeDirectionToPoint(
  const double originX,
  const double originY,
  const double originZ,
  const double targetX,
  const double targetY,
  const double targetZ
) {
  const double deltaX = targetX - originX;
  const double deltaY = targetY - originY;
  const double deltaZ = targetZ - originZ;
  const double lengthSq = deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ;
  if (lengthSq <= 0.0 || !std::isfinite(lengthSq)) {
    return std::nullopt;
  }

  const double length = std::sqrt(lengthSq);
  const double horizontal = std::sqrt(deltaX * deltaX + deltaZ * deltaZ);

  EtherwarpRayDirection direction;
  direction.yaw = normalizeYawDegrees(std::atan2(-deltaX, deltaZ) * RAD_TO_DEG);
  direction.pitch = static_cast<float>(std::atan2(-deltaY, horizontal) * RAD_TO_DEG);
  direction.dirX = deltaX / length;
  direction.dirY = deltaY / length;
  direction.dirZ = deltaZ / length;
  return direction;
}

inline std::optional<EtherwarpRayDirection> makeAimDirection(
  const WorldSnapshot& world,
  const double originX,
  const double originY,
  const double originZ,
  const Int3& to
) {
  const uint16_t toSupportFlags = flagsAt(world, to.x, to.y, to.z);

  const double targetX = static_cast<double>(to.x) + 0.5;
  const double targetY = static_cast<double>(to.y) + etherwarpLandingYOffset(toSupportFlags);
  const double targetZ = static_cast<double>(to.z) + 0.5;

  return makeDirectionToPoint(originX, originY, originZ, targetX, targetY, targetZ);
}

inline std::optional<EtherwarpRayDirection> makeAimDirection(
  const WorldSnapshot& world,
  const Int3& from,
  const Int3& to,
  const double eyeHeight
) {
  const auto origin = resolveEyeOriginFromLanding(world, from, eyeHeight);
  return makeAimDirection(world, origin.x, origin.y, origin.z, to);
}

inline std::optional<EtherwarpRayDirection> makeAimDirectionToOffset(
  const double originX,
  const double originY,
  const double originZ,
  const Int3& to,
  const double targetXOffset,
  const double targetYOffset,
  const double targetZOffset
) {
  return makeDirectionToPoint(
    originX,
    originY,
    originZ,
    static_cast<double>(to.x) + targetXOffset,
    static_cast<double>(to.y) + targetYOffset,
    static_cast<double>(to.z) + targetZOffset
  );
}

inline std::optional<EtherwarpRayDirection> makeAimDirectionToOffset(
  const WorldSnapshot& world,
  const Int3& from,
  const Int3& to,
  const double eyeHeight,
  const double targetXOffset,
  const double targetYOffset,
  const double targetZOffset
) {
  const auto origin = resolveEyeOriginFromLanding(world, from, eyeHeight);
  return makeAimDirectionToOffset(origin.x, origin.y, origin.z, to, targetXOffset, targetYOffset, targetZOffset);
}

inline bool hitsExactEtherwarpTarget(
  const WorldSnapshot& world,
  const Int3& target,
  const double rayLength,
  const EtherwarpRayDirection& direction,
  const double originX,
  const double originY,
  const double originZ
) {
  const auto hit = raymarchEtherwarp(world, originX, originY, originZ, direction, rayLength);
  return hit.has_value() && hit->x == target.x && hit->y == target.y && hit->z == target.z;
}

inline bool hitsExactEtherwarpTarget(
  const WorldSnapshot& world,
  const Int3& from,
  const Int3& target,
  const double eyeHeight,
  const double rayLength,
  const EtherwarpRayDirection& direction
) {
  const auto origin = resolveEyeOriginFromLanding(world, from, eyeHeight);
  return hitsExactEtherwarpTarget(world, target, rayLength, direction, origin.x, origin.y, origin.z);
}

inline bool squareMarginIsValid(
  const WorldSnapshot& world,
  const Int3& to,
  const double rayLength,
  const EtherwarpRayDirection& direction,
  const double margin,
  const double originX,
  const double originY,
  const double originZ
) {
  static constexpr double DELTAS[] = {-1.0, 0.0, 1.0};

  for (const double yawFactor : DELTAS) {
    for (const double pitchFactor : DELTAS) {
      const float sampledYaw = direction.yaw + static_cast<float>(yawFactor * margin);
      const float sampledPitch = static_cast<float>(std::clamp(
        static_cast<double>(direction.pitch) + (pitchFactor * margin),
        -90.0,
        90.0
      ));
      const auto sampledDirection = makeRayDirectionFromAngles(sampledYaw, sampledPitch);
      if (!hitsExactEtherwarpTarget(world, to, rayLength, sampledDirection, originX, originY, originZ)) {
        return false;
      }
    }
  }

  return true;
}

inline bool squareMarginIsValid(
  const WorldSnapshot& world,
  const Int3& from,
  const Int3& to,
  const double eyeHeight,
  const double rayLength,
  const EtherwarpRayDirection& direction,
  const double margin
) {
  const auto origin = resolveEyeOriginFromLanding(world, from, eyeHeight);
  return squareMarginIsValid(world, to, rayLength, direction, margin, origin.x, origin.y, origin.z);
}

inline double measureSquareMargin(
  const WorldSnapshot& world,
  const Int3& to,
  const double rayLength,
  const EtherwarpRayDirection& direction,
  const double originX,
  const double originY,
  const double originZ
) {
  if (!hitsExactEtherwarpTarget(world, to, rayLength, direction, originX, originY, originZ)) {
    return -1.0;
  }

  if (!squareMarginIsValid(world, to, rayLength, direction, ROBUSTNESS_INITIAL_MARGIN, originX, originY, originZ)) {
    return 0.0;
  }

  double low = ROBUSTNESS_INITIAL_MARGIN;
  double high = ROBUSTNESS_INITIAL_MARGIN;
  while (high < ROBUSTNESS_MAX_MARGIN) {
    const double next = std::min(high * 2.0, ROBUSTNESS_MAX_MARGIN);
    if (next <= high) {
      break;
    }
    if (!squareMarginIsValid(world, to, rayLength, direction, next, originX, originY, originZ)) {
      high = next;
      break;
    }
    low = next;
    high = next;
  }

  if (low >= ROBUSTNESS_MAX_MARGIN - ROBUSTNESS_EPSILON &&
      squareMarginIsValid(world, to, rayLength, direction, ROBUSTNESS_MAX_MARGIN, originX, originY, originZ)) {
    return ROBUSTNESS_MAX_MARGIN;
  }

  if (high <= low + ROBUSTNESS_EPSILON) {
    return low;
  }

  double good = low;
  double bad = high;
  for (int i = 0; i < ROBUSTNESS_BINARY_SEARCH_STEPS; i++) {
    const double mid = (good + bad) * 0.5;
    if (squareMarginIsValid(world, to, rayLength, direction, mid, originX, originY, originZ)) {
      good = mid;
    } else {
      bad = mid;
    }
  }

  return good;
}

inline double measureSquareMargin(
  const WorldSnapshot& world,
  const Int3& from,
  const Int3& to,
  const double eyeHeight,
  const double rayLength,
  const EtherwarpRayDirection& direction
) {
  const auto origin = resolveEyeOriginFromLanding(world, from, eyeHeight);
  return measureSquareMargin(world, to, rayLength, direction, origin.x, origin.y, origin.z);
}

inline std::optional<EtherwarpRayDirection> resolveStableAimDirection(
  const WorldSnapshot& world,
  const double originX,
  const double originY,
  const double originZ,
  const Int3& to,
  const double rayLength,
  const std::optional<EtherwarpRayDirection>& fallback = std::nullopt
) {
  const auto centerDirection = makeAimDirection(world, originX, originY, originZ, to);
  if (!centerDirection.has_value() ||
      !hitsExactEtherwarpTarget(world, to, rayLength, *centerDirection, originX, originY, originZ)) {
    return std::nullopt;
  }

  const uint16_t toSupportFlags = flagsAt(world, to.x, to.y, to.z);
  // ---------------------------------------------------------------------
  // TODO: figure out a way to fix the self-intersection issue:
  // where you're standing on a snow layer and even though your LOS ray is clear,
  // the game treats the layer as a full block for some reason so if your LOS intersects the fake fullblock,
  // you're tped into the block you're already on
  //
  // snow intersection was fixed, and yet the self-intersection issue still persists...
  // at least, i think intersection as a general issue was fixed. it seems to have been but idk.
  // either way, it needs fixing but i don't know how to fix it yet
  // ---------------------------------------------------------------------

  const double landingY = etherwarpLandingYOffset(toSupportFlags);
  const double verticalOffsets[] = {landingY, landingY - 0.2, landingY - 0.35, landingY - 0.5};
  const AimSampleOffset lateralOffsets[] = {
    {0.5, 0.5},
    {0.4375, 0.5},
    {0.5625, 0.5},
    {0.5, 0.4375},
    {0.5, 0.5625},
    {0.4375, 0.4375},
    {0.5625, 0.4375},
    {0.4375, 0.5625},
    {0.5625, 0.5625},
    {0.375, 0.5},
    {0.625, 0.5},
    {0.5, 0.375},
    {0.5, 0.625},
  };

  for (const double yOffset : verticalOffsets) {
    for (const auto& lateral : lateralOffsets) {
      const auto direction = makeDirectionToPoint(
        originX,
        originY,
        originZ,
        static_cast<double>(to.x) + lateral.x,
        static_cast<double>(to.y) + yOffset,
        static_cast<double>(to.z) + lateral.z
      );
      if (!direction.has_value()) {
        continue;
      }

      const auto hit = raymarchEtherwarp(world, originX, originY, originZ, *direction, rayLength);
      if (hit.has_value() && hit->x == to.x && hit->y == to.y && hit->z == to.z) {
        return direction;
      }
    }
  }

  if (fallback.has_value()) {
    const auto hit = raymarchEtherwarp(world, originX, originY, originZ, *fallback, rayLength);
    if (hit.has_value() && hit->x == to.x && hit->y == to.y && hit->z == to.z) {
      return fallback;
    }
  }

  return std::nullopt;
}

inline std::optional<EtherwarpRayDirection> resolveStableAimDirection(
  const WorldSnapshot& world,
  const Int3& from,
  const Int3& to,
  const double eyeHeight,
  const double rayLength,
  const std::optional<EtherwarpRayDirection>& fallback = std::nullopt
) {
  const auto origin = resolveEyeOriginFromLanding(world, from, eyeHeight);
  return resolveStableAimDirection(world, origin.x, origin.y, origin.z, to, rayLength, fallback);
}

inline std::vector<AngularRow> buildAngularRows(
  const double yawStep,
  const double pitchStep,
  int* nextId
) {
  std::vector<AngularRow> rows;

  for (double pitch = -90.0; pitch <= 90.0001;) {
    const float clampedPitch = static_cast<float>(std::clamp(pitch, -90.0, 90.0));
    const double cosPitch = std::cos(static_cast<double>(clampedPitch) * DEG_TO_RAD);

    AngularRow row;
    row.pitch = clampedPitch;

    if (std::abs(cosPitch) < 1e-4) {
      AngularSample sample;
      sample.direction = makeRayDirectionFromAngles(0.0f, clampedPitch);
      if (nextId != nullptr) {
        sample.id = (*nextId)++;
      }
      row.samples.push_back(sample);
    } else {
      const double effectiveYawStep = yawStep / std::abs(cosPitch);
      for (double yaw = 0.0; yaw < 360.0;) {
        AngularSample sample;
        sample.direction = makeRayDirectionFromAngles(static_cast<float>(yaw), clampedPitch);
        if (nextId != nullptr) {
          sample.id = (*nextId)++;
        }
        row.samples.push_back(sample);

        const double nextYaw = yaw + effectiveYawStep;
        if (nextYaw == yaw) break;
        yaw = nextYaw;
      }
    }

    rows.push_back(std::move(row));

    const double nextPitch = pitch + pitchStep;
    if (nextPitch == pitch) break;
    pitch = nextPitch;
  }

  return rows;
}

inline AngularLattice buildAngularLattice(const EtherwarpSearchParams& params) {
  AngularLattice lattice;

  int nextFineId = 0;
  lattice.fineRows = buildAngularRows(params.yawStep, params.pitchStep, &nextFineId);
  lattice.fineSampleCount = nextFineId;

  const double coarseYawStep = params.yawStep * ADAPTIVE_COARSE_MULTIPLIER;
  const double coarsePitchStep = params.pitchStep * ADAPTIVE_COARSE_MULTIPLIER;
  auto coarseRows = buildAngularRows(coarseYawStep, coarsePitchStep, nullptr);
  for (auto& row : coarseRows) {
    lattice.coarseSamples.insert(
      lattice.coarseSamples.end(),
      std::make_move_iterator(row.samples.begin()),
      std::make_move_iterator(row.samples.end())
    );
  }

  lattice.coarseYawWindow = std::max(coarseYawStep, params.yawStep * 2.0);
  lattice.coarsePitchWindow = std::max(coarsePitchStep, params.pitchStep * 2.0);
  return lattice;
}

inline double heuristicToGoal(const Int3& pos, const Int3& goal) {
  const long long dx = static_cast<long long>(pos.x) - goal.x;
  const long long dy = static_cast<long long>(pos.y) - goal.y;
  const long long dz = static_cast<long long>(pos.z) - goal.z;
  return static_cast<double>(dx * dx + dy * dy + dz * dz);
}

inline double weightedHeuristic(const double heuristic, const EtherwarpSearchParams& params) {
  return heuristic * params.heuristicWeight;
}

inline bool isGoal(const Int3& pos, const Int3& goal) {
  return pos.x == goal.x && pos.y == goal.y && pos.z == goal.z;
}

inline int createNode(SharedState& state, const Int3& pos, const double h) {
  const int idx = static_cast<int>(state.nodeX.size());
  state.nodeX.push_back(pos.x);
  state.nodeY.push_back(pos.y);
  state.nodeZ.push_back(pos.z);
  state.nodeParent.push_back(-1);
  state.nodeDepth.push_back(0);
  state.nodeG.push_back(std::numeric_limits<double>::infinity());
  state.nodeH.push_back(h);
  state.nodeF.push_back(std::numeric_limits<double>::infinity());
  state.nodeYaw.push_back(std::numeric_limits<float>::quiet_NaN());
  state.nodePitch.push_back(std::numeric_limits<float>::quiet_NaN());
  state.nodeHeapPos.push_back(-1);
  state.coordToNode.emplace(coordKey(pos.x, pos.y, pos.z), idx);
  return idx;
}

inline bool tryDirectShot(
  const WorldSnapshot& world,
  const Int3& target,
  const double rayLength,
  const double originX,
  const double originY,
  const double originZ,
  EtherwarpRayDirection* usedDirection = nullptr
) {
  const auto direction = resolveStableAimDirection(world, originX, originY, originZ, target, rayLength);
  if (!direction.has_value()) {
    return false;
  }

  if (usedDirection != nullptr) {
    *usedDirection = *direction;
  }
  return true;
}

inline bool tryDirectShot(
  const WorldSnapshot& world,
  const Int3& from,
  const Int3& target,
  const double eyeHeight,
  const double rayLength,
  EtherwarpRayDirection* usedDirection = nullptr
) {
  const auto direction = resolveStableAimDirection(world, from, target, eyeHeight, rayLength);
  if (!direction.has_value()) {
    return false;
  }

  if (usedDirection != nullptr) {
    *usedDirection = *direction;
  }
  return true;
}

inline bool isBetterRobustnessScore(
  const AngleRobustnessScore& candidate,
  const AngleRobustnessScore& best
) {
  if (candidate.margin > best.margin + ROBUSTNESS_EPSILON) {
    return true;
  }
  if (candidate.margin + ROBUSTNESS_EPSILON < best.margin) {
    return false;
  }
  if (candidate.baselineDistance + ROBUSTNESS_EPSILON < best.baselineDistance) {
    return true;
  }
  if (candidate.baselineDistance > best.baselineDistance + ROBUSTNESS_EPSILON) {
    return false;
  }
  return candidate.centerDistance + ROBUSTNESS_EPSILON < best.centerDistance;
}

inline std::optional<EtherwarpRayDirection> optimizeAimDirectionForHop(
  const WorldSnapshot& world,
  const Int3& to,
  const double rayLength,
  const double originX,
  const double originY,
  const double originZ,
  const std::optional<EtherwarpRayDirection>& fallback = std::nullopt
) {
  const auto baseline = resolveStableAimDirection(world, originX, originY, originZ, to, rayLength, fallback);
  if (!baseline.has_value()) {
    return std::nullopt;
  }

  const uint16_t toSupportFlags = flagsAt(world, to.x, to.y, to.z);
  const double landingY = etherwarpLandingYOffset(toSupportFlags);
  const double verticalOffsets[] = {landingY, landingY - 0.12, landingY - 0.24, landingY - 0.36, landingY - 0.50};
  const AimSampleOffset lateralOffsets[] = {
    {0.5, 0.5},
    {0.4375, 0.5},
    {0.5625, 0.5},
    {0.5, 0.4375},
    {0.5, 0.5625},
    {0.4375, 0.4375},
    {0.5625, 0.4375},
    {0.4375, 0.5625},
    {0.5625, 0.5625},
    {0.375, 0.5},
    {0.625, 0.5},
    {0.5, 0.375},
    {0.5, 0.625},
    {0.46875, 0.5},
    {0.53125, 0.5},
    {0.5, 0.46875},
    {0.5, 0.53125},
    {0.46875, 0.46875},
    {0.53125, 0.46875},
    {0.46875, 0.53125},
    {0.53125, 0.53125},
  };

  const double baselineMargin = measureSquareMargin(world, to, rayLength, *baseline, originX, originY, originZ);
  AngleRobustnessScore bestScore;
  bestScore.yaw = baseline->yaw;
  bestScore.pitch = baseline->pitch;
  bestScore.margin = std::max(0.0, baselineMargin);
  bestScore.baselineDistance = 0.0;
  bestScore.centerDistance = 0.0;
  EtherwarpRayDirection bestDirection = *baseline;

  for (const double targetYOffset : verticalOffsets) {
    for (const auto& lateral : lateralOffsets) {
      const auto candidate = makeAimDirectionToOffset(
        originX,
        originY,
        originZ,
        to,
        lateral.x,
        targetYOffset,
        lateral.z
      );
      if (!candidate.has_value() ||
          !hitsExactEtherwarpTarget(world, to, rayLength, *candidate, originX, originY, originZ)) {
        continue;
      }

      AngleRobustnessScore score;
      score.yaw = candidate->yaw;
      score.pitch = candidate->pitch;
      score.margin = measureSquareMargin(world, to, rayLength, *candidate, originX, originY, originZ);
      score.baselineDistance = std::hypot(
        circularAngleDistance(candidate->yaw, baseline->yaw),
        std::abs(static_cast<double>(candidate->pitch) - static_cast<double>(baseline->pitch))
      );
      score.centerDistance = std::sqrt(
        ((lateral.x - 0.5) * (lateral.x - 0.5)) +
        ((targetYOffset - landingY) * (targetYOffset - landingY)) +
        ((lateral.z - 0.5) * (lateral.z - 0.5))
      );

      if (isBetterRobustnessScore(score, bestScore)) {
        bestScore = score;
        bestDirection = *candidate;
      }
    }
  }

  if (bestScore.margin > baselineMargin + ROBUSTNESS_EPSILON) {
    return bestDirection;
  }
  return baseline;
}

inline std::optional<EtherwarpRayDirection> optimizeAimDirectionForHop(
  const WorldSnapshot& world,
  const Int3& from,
  const Int3& to,
  const double eyeHeight,
  const double rayLength,
  const std::optional<EtherwarpRayDirection>& fallback = std::nullopt
) {
  const auto origin = resolveEyeOriginFromLanding(world, from, eyeHeight);
  return optimizeAimDirectionForHop(world, to, rayLength, origin.x, origin.y, origin.z, fallback);
}

inline std::vector<float> optimizeAnglesForRoute(
  const WorldSnapshot& world,
  const std::vector<Int3>& points,
  const std::vector<float>& fallbackAngles,
  const EtherwarpSearchParams& params
) {
  std::vector<float> angles;
  if (points.empty()) {
    return angles;
  }

  angles.reserve(points.size() * 2);
  for (size_t i = 0; i < points.size(); i++) {
    std::optional<EtherwarpRayDirection> fallback;
    const size_t angleIdx = i * 2;
    if (angleIdx + 1 < fallbackAngles.size() &&
        std::isfinite(fallbackAngles[angleIdx]) &&
        std::isfinite(fallbackAngles[angleIdx + 1])) {
      fallback = makeRayDirectionFromAngles(fallbackAngles[angleIdx], fallbackAngles[angleIdx + 1]);
    }

    std::optional<EtherwarpRayDirection> optimized;
    if (i == 0) {
      optimized = optimizeAimDirectionForHop(
        world,
        points[i],
        params.rayLength,
        params.startEyeX,
        params.startEyeY,
        params.startEyeZ,
        fallback
      );
    } else {
      optimized = optimizeAimDirectionForHop(
        world,
        points[i - 1],
        points[i],
        params.eyeHeight,
        params.rayLength,
        fallback
      );
    }

    if (optimized.has_value()) {
      angles.push_back(optimized->yaw);
      angles.push_back(optimized->pitch);
      continue;
    }

    if (fallback.has_value()) {
      angles.push_back(fallback->yaw);
      angles.push_back(fallback->pitch);
      continue;
    }

    EtherwarpRayDirection directShotDirection;
    const bool hasDirectShot = i == 0
      ? tryDirectShot(world, points[i], params.rayLength, params.startEyeX, params.startEyeY, params.startEyeZ, &directShotDirection)
      : tryDirectShot(world, points[i - 1], points[i], params.eyeHeight, params.rayLength, &directShotDirection);
    if (hasDirectShot) {
      angles.push_back(directShotDirection.yaw);
      angles.push_back(directShotDirection.pitch);
    } else {
      angles.push_back(std::numeric_limits<float>::quiet_NaN());
      angles.push_back(std::numeric_limits<float>::quiet_NaN());
    }
  }

  return angles;
}

inline bool appendNeighborCandidate(
  const EtherwarpSearchParams& params,
  const int nextDepth,
  const double nextG,
  const Int3& hit,
  const EtherwarpRayDirection& direction,
  WorkerScratch& scratch
) {
  const uint64_t key = coordKey(hit.x, hit.y, hit.z);
  if (!scratch.seenHits.emplace(key).second) {
    return false;
  }

  const double h = heuristicToGoal(hit, params.goal);
  NeighborCandidate candidate;
  candidate.pos = hit;
  candidate.yaw = direction.yaw;
  candidate.pitch = direction.pitch;
  candidate.depth = nextDepth;
  candidate.g = nextG;
  candidate.h = h;
  candidate.f = nextG + weightedHeuristic(h, params);
  scratch.neighbors.push_back(candidate);
  return isGoal(hit, params.goal);
}

inline bool refineAroundSeed(
  const WorldSnapshot& world,
  const EtherwarpSearchParams& params,
  const AngularLattice& lattice,
  const Int3& current,
  const double originX,
  const double originY,
  const double originZ,
  const bool useExplicitOrigin,
  const int nextDepth,
  const double nextG,
  const RefinementSeed& seed,
  WorkerScratch& scratch
) {
  for (const auto& row : lattice.fineRows) {
    if (std::abs(static_cast<double>(row.pitch) - static_cast<double>(seed.pitch)) > lattice.coarsePitchWindow) {
      continue;
    }

    for (const auto& sample : row.samples) {
      if (sample.id < 0) {
        continue;
      }
      if (circularAngleDistance(sample.direction.yaw, seed.yaw) > lattice.coarseYawWindow) {
        continue;
      }
      if (scratch.fineVisitStamp[static_cast<size_t>(sample.id)] == scratch.fineVisitEpoch) {
        continue;
      }

      scratch.fineVisitStamp[static_cast<size_t>(sample.id)] = scratch.fineVisitEpoch;
      const auto hit = raymarchEtherwarp(world, originX, originY, originZ, sample.direction, params.rayLength);
      if (!hit.has_value()) {
        continue;
      }

      std::optional<EtherwarpRayDirection> candidateDirection;
      if (useExplicitOrigin) {
        candidateDirection = resolveStableAimDirection(
          world,
          originX,
          originY,
          originZ,
          *hit,
          params.rayLength,
          sample.direction
        );
      } else {
        candidateDirection = resolveStableAimDirection(
          world,
          current,
          *hit,
          params.eyeHeight,
          params.rayLength,
          sample.direction
        );
      }
      if (!candidateDirection.has_value()) {
        continue;
      }

      if (appendNeighborCandidate(params, nextDepth, nextG, *hit, *candidateDirection, scratch)) {
        return true;
      }
    }
  }

  return false;
}

inline void collectNeighbors(
  const WorldSnapshot& world,
  const EtherwarpSearchParams& params,
  const AngularLattice& lattice,
  const Int3& current,
  const double currG,
  const int currDepth,
  WorkerScratch& scratch,
  const bool useExplicitOrigin = false,
  const double explicitOriginX = 0.0,
  const double explicitOriginY = 0.0,
  const double explicitOriginZ = 0.0
) {
  scratch.beginNode();

  double originX = explicitOriginX;
  double originY = explicitOriginY;
  double originZ = explicitOriginZ;
  if (!useExplicitOrigin) {
    const auto landingOrigin = resolveEyeOriginFromLanding(world, current, params.eyeHeight);
    originX = landingOrigin.x;
    originY = landingOrigin.y;
    originZ = landingOrigin.z;
  }
  const int nextDepth = currDepth + 1;
  const double nextG = currG + params.newNodeCost;

  EtherwarpRayDirection goalDirection;
  const bool hasGoalDirectShot = useExplicitOrigin
    ? tryDirectShot(world, params.goal, params.rayLength, originX, originY, originZ, &goalDirection)
    : tryDirectShot(world, current, params.goal, params.eyeHeight, params.rayLength, &goalDirection);
  if (hasGoalDirectShot) {
    appendNeighborCandidate(params, nextDepth, nextG, params.goal, goalDirection, scratch);
    return;
  }

  const auto goalAim = useExplicitOrigin
    ? makeAimDirection(world, originX, originY, originZ, params.goal)
    : makeAimDirection(world, current, params.goal, params.eyeHeight);
  if (goalAim.has_value()) {
    scratch.refinementSeeds.push_back(RefinementSeed{
      goalAim->yaw,
      goalAim->pitch,
      0.0
    });
  }

  for (const auto& sample : lattice.coarseSamples) {
    const auto hit = raymarchEtherwarp(world, originX, originY, originZ, sample.direction, params.rayLength);
    if (!hit.has_value()) {
      continue;
    }

    std::optional<EtherwarpRayDirection> candidateDirection;
    if (useExplicitOrigin) {
      candidateDirection = resolveStableAimDirection(
        world,
        originX,
        originY,
        originZ,
        *hit,
        params.rayLength,
        sample.direction
      );
    } else {
      candidateDirection = resolveStableAimDirection(
        world,
        current,
        *hit,
        params.eyeHeight,
        params.rayLength,
        sample.direction
      );
    }
    if (!candidateDirection.has_value()) {
      continue;
    }

    const size_t previousCount = scratch.neighbors.size();
    if (appendNeighborCandidate(params, nextDepth, nextG, *hit, *candidateDirection, scratch)) {
      return;
    }
    if (scratch.neighbors.size() == previousCount) {
      continue;
    }

    scratch.refinementSeeds.push_back(RefinementSeed{
      candidateDirection->yaw,
      candidateDirection->pitch,
      scratch.neighbors.back().h
    });
  }

  if (scratch.refinementSeeds.size() > 1) {
    auto coarseStart = goalAim.has_value() ? scratch.refinementSeeds.begin() + 1 : scratch.refinementSeeds.begin();
    std::sort(coarseStart, scratch.refinementSeeds.end(), [](const RefinementSeed& lhs, const RefinementSeed& rhs) {
      return lhs.h < rhs.h;
    });

    const size_t baseSeeds = goalAim.has_value() ? 1U : 0U;
    const size_t maxSeeds = baseSeeds + static_cast<size_t>(MAX_REFINEMENT_SEEDS);
    if (scratch.refinementSeeds.size() > maxSeeds) {
      scratch.refinementSeeds.resize(maxSeeds);
    }
  }

  for (const auto& seed : scratch.refinementSeeds) {
    if (refineAroundSeed(world, params, lattice, current, originX, originY, originZ, useExplicitOrigin, nextDepth, nextG, seed, scratch)) {
      return;
    }
  }
}

inline SimplifiedEtherwarpPath simplifyEtherwarpPath(
  const WorldSnapshot& world,
  const std::vector<Int3>& chainPoints,
  const std::vector<float>& chainYaw,
  const std::vector<float>& chainPitch,
  const EtherwarpSearchParams& params
) {
  SimplifiedEtherwarpPath result;
  if (chainPoints.empty()) {
    return result;
  }

  result.points.reserve(chainPoints.size());
  result.angles.reserve(chainPoints.size() * 2);

  size_t from = static_cast<size_t>(-1);
  while (true) {
    const size_t firstCandidate = from == static_cast<size_t>(-1) ? 0 : (from + 1);
    if (firstCandidate >= chainPoints.size()) {
      break;
    }

    size_t next = firstCandidate;
    std::optional<EtherwarpRayDirection> simplifiedDirection;

    for (size_t candidate = chainPoints.size(); candidate-- > firstCandidate;) {
      EtherwarpRayDirection direction;
      const bool hasDirectShot = from == static_cast<size_t>(-1)
        ? tryDirectShot(
            world,
            chainPoints[candidate],
            params.rayLength,
            params.startEyeX,
            params.startEyeY,
            params.startEyeZ,
            &direction
          )
        : tryDirectShot(world, chainPoints[from], chainPoints[candidate], params.eyeHeight, params.rayLength, &direction);
      if (!hasDirectShot) {
        continue;
      }

      next = candidate;
      simplifiedDirection = direction;
      break;
    }

    result.points.push_back(chainPoints[next]);
    if (simplifiedDirection.has_value()) {
      result.angles.push_back(simplifiedDirection->yaw);
      result.angles.push_back(simplifiedDirection->pitch);
    } else {
      result.angles.push_back(chainYaw[next]);
      result.angles.push_back(chainPitch[next]);
    }

    if (next == chainPoints.size() - 1) {
      break;
    }
    from = next;
  }

  return result;
}

} // namespace

std::optional<EtherwarpSearchResult> findEtherwarpPath(
  const WorldSnapshot& world,
  const EtherwarpSearchParams& params,
  std::atomic_bool& cancelFlag
) {
  cancelFlag.store(false);

  if (!isEtherwarpLandingBlockVoxel(world, params.goal.x, params.goal.y, params.goal.z)) {
    return std::nullopt;
  }

  if (params.maxIterations <= 0 || params.threadCount <= 0 ||
      !std::isfinite(params.startEyeX) ||
      !std::isfinite(params.startEyeY) ||
      !std::isfinite(params.startEyeZ) ||
      !std::isfinite(params.yawStep) || params.yawStep <= 0.0 ||
      !std::isfinite(params.pitchStep) || params.pitchStep <= 0.0 ||
      !std::isfinite(params.newNodeCost) || params.newNodeCost <= 0.0 ||
      !std::isfinite(params.heuristicWeight) || params.heuristicWeight <= 0.0 ||
      !std::isfinite(params.rayLength) || params.rayLength <= 0.0 ||
      !std::isfinite(params.rewireEpsilon) || params.rewireEpsilon < 0.0 ||
      !std::isfinite(params.eyeHeight) || params.eyeHeight <= 0.0) {
    return std::nullopt;
  }

  const auto totalStartTime = std::chrono::steady_clock::now();
  EtherwarpRayDirection directShotDirection;
  if (tryDirectShot(
        world,
        params.goal,
        params.rayLength,
        params.startEyeX,
        params.startEyeY,
        params.startEyeZ,
        &directShotDirection
      )) {
    const auto elapsedNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::steady_clock::now() - totalStartTime
    ).count();

    EtherwarpSearchResult result;
    result.points = {params.goal};
    const auto optimizedDirectShot = optimizeAimDirectionForHop(
      world,
      params.goal,
      params.rayLength,
      params.startEyeX,
      params.startEyeY,
      params.startEyeZ,
      directShotDirection
    );
    const EtherwarpRayDirection& finalDirectShot = optimizedDirectShot.has_value()
      ? *optimizedDirectShot
      : directShotDirection;
    result.angles = {
      finalDirectShot.yaw,
      finalDirectShot.pitch
    };
    result.timeMs = elapsedNs / 1000000LL;
    result.nodesExplored = 1;
    result.nanosecondsPerNode = static_cast<double>(elapsedNs);
    return result;
  }

  const int reserveTarget = std::clamp(params.maxIterations / 2, 4096, 262144);

  const AngularLattice lattice = buildAngularLattice(params);
  SharedState state(reserveTarget);

  {
    WorkerScratch bootstrapScratch(lattice);
    collectNeighbors(
      world,
      params,
      lattice,
      Int3{},
      0.0,
      0,
      bootstrapScratch,
      true,
      params.startEyeX,
      params.startEyeY,
      params.startEyeZ
    );

    if (bootstrapScratch.neighbors.empty()) {
      return std::nullopt;
    }

    std::lock_guard lock(state.mutex);
    for (const auto& neighbor : bootstrapScratch.neighbors) {
      const int nodeIdx = createNode(state, neighbor.pos, neighbor.h);
      state.nodeParent[static_cast<size_t>(nodeIdx)] = -1;
      state.nodeDepth[static_cast<size_t>(nodeIdx)] = neighbor.depth;
      state.nodeG[static_cast<size_t>(nodeIdx)] = neighbor.g;
      state.nodeF[static_cast<size_t>(nodeIdx)] = neighbor.f;
      state.nodeYaw[static_cast<size_t>(nodeIdx)] = neighbor.yaw;
      state.nodePitch[static_cast<size_t>(nodeIdx)] = neighbor.pitch;
      state.heap.add(nodeIdx);

      if (isGoal(neighbor.pos, params.goal) && !state.solved) {
        state.solved = true;
        state.solutionIdx = nodeIdx;
      }
    }
  }

  auto worker = [&]() {
    WorkerScratch scratch(lattice);
    std::vector<BatchExpansion> batch;
    batch.reserve(EXPANSION_BATCH_SIZE);

    while (!cancelFlag.load()) {
      batch.clear();

      {
        std::unique_lock lock(state.mutex);
        while (true) {
          if (cancelFlag.load() || state.solved || state.exhausted) {
            return;
          }

          if (state.iterations >= params.maxIterations) {
            state.exhausted = true;
            state.cv.notify_all();
            return;
          }

          if (!state.heap.empty()) {
            while (static_cast<int>(batch.size()) < EXPANSION_BATCH_SIZE &&
                   !state.heap.empty() &&
                   state.iterations < params.maxIterations) {
              const int currIdx = state.heap.poll();
              state.activeExpanders++;
              state.iterations++;

              BatchExpansion expansion;
              expansion.currIdx = currIdx;
              expansion.current = Int3{
                state.nodeX[static_cast<size_t>(currIdx)],
                state.nodeY[static_cast<size_t>(currIdx)],
                state.nodeZ[static_cast<size_t>(currIdx)]
              };
              expansion.currG = state.nodeG[static_cast<size_t>(currIdx)];
              expansion.currDepth = state.nodeDepth[static_cast<size_t>(currIdx)];
              expansion.neighbors.reserve(512);
              batch.push_back(std::move(expansion));
            }
            break;
          }

          if (state.activeExpanders == 0) {
            state.exhausted = true;
            state.cv.notify_all();
            return;
          }

          state.cv.wait(lock);
        }
      }

      for (auto& expansion : batch) {
        expansion.neighbors.clear();
        expansion.isGoal = isGoal(expansion.current, params.goal);
        if (expansion.isGoal) {
          continue;
        }
        collectNeighbors(
          world,
          params,
          lattice,
          expansion.current,
          expansion.currG,
          expansion.currDepth,
          scratch
        );
        expansion.neighbors = scratch.neighbors;
      }

      {
        std::lock_guard lock(state.mutex);
        for (const auto& expansion : batch) {
          if (expansion.isGoal && !state.solved) {
            state.solved = true;
            state.solutionIdx = expansion.currIdx;
          }

          if (!state.solved && !state.exhausted && !cancelFlag.load()) {
            for (const auto& neighbor : expansion.neighbors) {
              const uint64_t key = coordKey(neighbor.pos.x, neighbor.pos.y, neighbor.pos.z);
              const auto found = state.coordToNode.find(key);

              int nodeIdx = -1;
              if (found == state.coordToNode.end()) {
                nodeIdx = createNode(state, neighbor.pos, neighbor.h);
                state.nodeParent[static_cast<size_t>(nodeIdx)] = expansion.currIdx;
                state.nodeDepth[static_cast<size_t>(nodeIdx)] = neighbor.depth;
                state.nodeG[static_cast<size_t>(nodeIdx)] = neighbor.g;
                state.nodeF[static_cast<size_t>(nodeIdx)] = neighbor.f;
                state.nodeYaw[static_cast<size_t>(nodeIdx)] = neighbor.yaw;
                state.nodePitch[static_cast<size_t>(nodeIdx)] = neighbor.pitch;
                state.heap.add(nodeIdx);
                continue;
              }

              nodeIdx = found->second;
              if (neighbor.g + params.rewireEpsilon < state.nodeG[static_cast<size_t>(nodeIdx)]) {
                state.nodeParent[static_cast<size_t>(nodeIdx)] = expansion.currIdx;
                state.nodeDepth[static_cast<size_t>(nodeIdx)] = neighbor.depth;
                state.nodeG[static_cast<size_t>(nodeIdx)] = neighbor.g;
                state.nodeF[static_cast<size_t>(nodeIdx)] = neighbor.g + weightedHeuristic(state.nodeH[static_cast<size_t>(nodeIdx)], params);
                state.nodeYaw[static_cast<size_t>(nodeIdx)] = neighbor.yaw;
                state.nodePitch[static_cast<size_t>(nodeIdx)] = neighbor.pitch;

                if (state.nodeHeapPos[static_cast<size_t>(nodeIdx)] != -1) {
                  state.heap.relocate(nodeIdx);
                } else {
                  state.heap.add(nodeIdx);
                }
              }
            }
          }

          state.activeExpanders--;
        }

        if (!state.solved && state.activeExpanders == 0 && state.heap.empty()) {
          state.exhausted = true;
        }
        state.cv.notify_all();
      }
    }
  };

  const int workerCount = std::max(1, params.threadCount);
  std::vector<std::thread> workers;
  workers.reserve(static_cast<size_t>(std::max(0, workerCount - 1)));
  for (int i = 1; i < workerCount; i++) {
    workers.emplace_back(worker);
  }

  worker();

  for (auto& thread : workers) {
    if (thread.joinable()) {
      thread.join();
    }
  }

  if (cancelFlag.load()) {
    return std::nullopt;
  }

  int solutionIdx = -1;
  int explored = 0;
  {
    std::lock_guard lock(state.mutex);
    if (!state.solved || state.solutionIdx < 0) {
      return std::nullopt;
    }
    solutionIdx = state.solutionIdx;
    explored = state.iterations;
  }

  std::vector<int> chain;
  std::vector<Int3> chainPoints;
  std::vector<float> chainYaw;
  std::vector<float> chainPitch;
  {
    std::lock_guard lock(state.mutex);
    int walk = solutionIdx;
    while (walk != -1) {
      chain.push_back(walk);
      walk = state.nodeParent[static_cast<size_t>(walk)];
    }
  }
  std::reverse(chain.begin(), chain.end());

  {
    std::lock_guard lock(state.mutex);
    chainPoints.reserve(chain.size());
    chainYaw.reserve(chain.size());
    chainPitch.reserve(chain.size());
    for (const int idx : chain) {
      chainPoints.push_back(Int3{
        state.nodeX[static_cast<size_t>(idx)],
        state.nodeY[static_cast<size_t>(idx)],
        state.nodeZ[static_cast<size_t>(idx)]
      });
      chainYaw.push_back(state.nodeYaw[static_cast<size_t>(idx)]);
      chainPitch.push_back(state.nodePitch[static_cast<size_t>(idx)]);
    }
  }

  const auto simplified = simplifyEtherwarpPath(world, chainPoints, chainYaw, chainPitch, params);

  EtherwarpSearchResult result;
  result.points = simplified.points;
  result.angles = optimizeAnglesForRoute(world, simplified.points, simplified.angles, params);

  const auto elapsedNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
    std::chrono::steady_clock::now() - totalStartTime
  ).count();

  result.timeMs = elapsedNs / 1000000LL;
  result.nodesExplored = explored;
  result.nanosecondsPerNode = explored > 0
    ? static_cast<double>(elapsedNs) / static_cast<double>(explored)
    : 0.0;

  return result;
}

} // namespace v5pf
