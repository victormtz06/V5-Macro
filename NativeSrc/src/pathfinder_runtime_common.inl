#pragma once

#include <algorithm>
#include <cmath>
#include <limits>

namespace v5pf::detail {

inline Runtime::Runtime(const WorldSnapshot& world, const SearchParams& params)
  : world_(world),
    params_(params),
    voxelCursor_(world),
    cache_(runtimeCache()) {
  cache_.begin(world);

  flyMinY_ = world_.minY;
  flyMaxY_ = world_.maxY - 2;

  if (params_.isFly && !params_.starts.empty() && !params_.goals.empty()) {
    startFly_ = params_.starts.front();
  }
}

inline bool Runtime::isAtGoal(const int x, const int y, const int z) const {
  for (const auto& goal : params_.goals) {
    if (goal.x == x && goal.y == y && goal.z == z) {
      return true;
    }
  }
  return false;
}

inline float Runtime::heuristic(const int x, const int y, const int z) const {
  return params_.isFly ? flyHeuristic(x, y, z) : walkHeuristic(x, y, z);
}

inline float Runtime::transientAvoidPenalty(const int x, const int y, const int z) const {
  float penalty = 0.0f;
  for (const auto& zone : params_.avoidZones) {
    if (std::abs(y - zone.y) > zone.maxYDiff) continue;

    const int dx = x - zone.x;
    const int dz = z - zone.z;
    const long long distSq = static_cast<long long>(dx) * dx + static_cast<long long>(dz) * dz;
    if (distSq > zone.radiusSq) continue;

    const float normalized = zone.radiusSq <= 1 ? 0.0f : static_cast<float>(distSq) / static_cast<float>(zone.radiusSq);
    const float falloff = std::max(0.2f, 1.0f - normalized);
    penalty += zone.penalty * falloff;
  }
  return penalty;
}

inline float Runtime::flyHorizontalProgress(const int x, const int y, const int z) const {
  return calculateProgress(x, z, closestFlyGoal(x, y, z));
}

inline bool Runtime::walkMove(const Int3& current, const Int3& delta, MoveOut& out) {
  if (delta.y == 0) {
    if (delta.x == 0 || delta.z == 0) {
      return moveTraverse(current, delta.x, delta.z, out);
    }
    return moveDiagonal(current, delta.x, delta.z, out);
  }

  if (delta.y > 0) {
    return moveAscend(current, delta.x, delta.z, out);
  }

  return moveDescend(current, delta.x, delta.z, out);
}

inline bool Runtime::flyMove(const Int3& current, const Int3& delta, MoveOut& out) {
  const auto& goal = closestFlyGoal(current.x, current.y, current.z);
  return moveFly(current, delta.x, delta.y, delta.z, calculateProgress(current.x, current.z, goal), out);
}

inline bool Runtime::flyMove(const Int3& current, const Int3& delta, const float progress, MoveOut& out) {
  return moveFly(current, delta.x, delta.y, delta.z, progress, out);
}

inline uint16_t Runtime::flagsAt(const int x, const int y, const int z) const {
  return voxelCursor_.getFlags(x, y, z);
}

inline uint32_t Runtime::cacheGenerationAt(const int x, const int z) const {
  const int chunkX = x >> 4;
  const int chunkZ = z >> 4;
  const uint32_t hash = static_cast<uint32_t>(chunkX) ^ (static_cast<uint32_t>(chunkZ) << 1);
  auto& cursor = chunkGenerationCursors_[static_cast<size_t>(hash & 3u)];
  if (cursor.chunkX != chunkX || cursor.chunkZ != chunkZ) {
    cursor = {chunkX, chunkZ, world_.cacheGenerationForChunk(chunkX, chunkZ)};
  }
  return cursor.generation;
}

inline bool Runtime::isPassable(const int x, const int y, const int z) const {
  return isPassableFlags(flagsAt(x, y, z));
}

inline bool Runtime::isPassableForFlying(const int x, const int y, const int z) const {
  return isFlyPassableFlags(flagsAt(x, y, z));
}

inline bool Runtime::isSafe(const int x, const int y, const int z) {
  constexpr uint8_t bit = 1u << 0;
  auto& cached = cache_.classifications.at(x, y, z);
  const uint32_t generation = cacheGenerationAt(x, z);
  if (cached.generation != generation) cached = {generation, {}};
  if ((cached.value.computed & bit) != 0) return (cached.value.values & bit) != 0;

  const bool safe = hasFlag(flagsAt(x, y - 1, z), VF_SOLID) &&
    isPassableFlags(flagsAt(x, y, z)) &&
    isPassableFlags(flagsAt(x, y + 1, z));
  cached.value.computed |= bit;
  if (safe) cached.value.values |= bit;
  return safe;
}

inline bool Runtime::isFlyColumnClear(const int x, const int y, const int z) {
  constexpr uint8_t bit = 1u << 3;
  auto& cached = cache_.classifications.at(x, y, z);
  const uint32_t generation = cacheGenerationAt(x, z);
  if (cached.generation != generation) cached = {generation, {}};
  if ((cached.value.computed & bit) != 0) return (cached.value.values & bit) != 0;

  const uint16_t feet = flagsAt(x, y, z);
  const uint16_t head = flagsAt(x, y + 1, z);
  const bool clear = isFlyPassableFlags(feet) &&
    !hasFlag(head, VF_SLAB_TOP) &&
    isFlyPassableFlags(head);
  cached.value.computed |= bit;
  if (clear) cached.value.values |= bit;
  return clear;
}

inline float Runtime::walkHeuristic(const int x, const int y, const int z) const {
  if (params_.goals.empty()) return 0.0f;

  const float sprintCost = ActionCosts::SPRINT_ONE_BLOCK_TIME;
  const float diagonalCost = ActionCosts::SPRINT_DIAGONAL_TIME;
  const float fallCostPerBlock = ActionCosts::getFallTime(2) * 0.5f;
  const float jumpCostPerBlock = ActionCosts::JUMP_UP_ONE_BLOCK_TIME;
  const float verticalReluctance = sprintCost * 0.35f;

  float best = std::numeric_limits<float>::infinity();
  for (const auto& goal : params_.goals) {
    const long long dx = std::llabs(static_cast<long long>(x) - goal.x);
    const long long dz = std::llabs(static_cast<long long>(z) - goal.z);
    const long long dy = static_cast<long long>(y) - goal.y;

    const long long minHoriz = std::min(dx, dz);
    const long long maxHoriz = std::max(dx, dz);

    float horizontal = static_cast<float>(minHoriz) * diagonalCost +
      static_cast<float>(maxHoriz - minHoriz) * sprintCost;

    if (dy != 0) {
      const float absDy = static_cast<float>(std::llabs(dy));
      horizontal += (dy > 0 ? static_cast<float>(dy) * fallCostPerBlock : absDy * jumpCostPerBlock);
      horizontal += absDy * verticalReluctance;
    }

    if (horizontal < best) {
      best = horizontal;
    }
  }

  return std::isfinite(best) ? best : 0.0f;
}

inline int Runtime::directionMask(const int x, const int y, const int z) {
  int mask = 0;
  for (int dir = 0; dir < 8; dir++) {
    if (!isStepDirection(x, y, z, DX[static_cast<size_t>(dir)], DZ[static_cast<size_t>(dir)])) {
      mask |= (1 << dir);
    }
  }
  return mask == 0 ? ((1 << 8) - 1) : mask;
}

inline bool Runtime::isStepDirection(const int x, const int y, const int z, const int dx, const int dz) {
  const int nx = x + dx;
  const int nz = z + dz;

  const uint16_t upperSurface = flagsAt(nx, y, nz);
  if ((hasFlag(upperSurface, VF_STAIRS_BOTTOM) || hasFlag(upperSurface, VF_SLAB_BOTTOM)) &&
      hasFlag(upperSurface, VF_SOLID) &&
      isPassableFlags(flagsAt(nx, y + 1, nz)) &&
      isPassableFlags(flagsAt(nx, y + 2, nz))) {
    return true;
  }

  const uint16_t lowerSurface = flagsAt(nx, y - 2, nz);
  if ((hasFlag(lowerSurface, VF_STAIRS_BOTTOM) || hasFlag(lowerSurface, VF_SLAB_BOTTOM)) &&
      hasFlag(lowerSurface, VF_SOLID) &&
      isPassableFlags(flagsAt(nx, y - 1, nz)) &&
      isPassableFlags(upperSurface)) {
    return true;
  }

  return false;
}

inline bool Runtime::isEdge(const int x, const int y, const int z) {
  constexpr uint8_t bit = 1u << 1;
  auto& cached = cache_.classifications.at(x, y, z);
  const uint32_t generation = cacheGenerationAt(x, z);
  if (cached.generation != generation) cached = {generation, {}};
  if ((cached.value.computed & bit) != 0) return (cached.value.values & bit) != 0;

  const bool edge = !hasFlag(flagsAt(x, y, z), VF_SOLID) &&
    !hasFlag(flagsAt(x, y - 1, z), VF_SOLID) &&
    !hasFlag(flagsAt(x, y - 2, z), VF_SOLID);
  cached.value.computed |= bit;
  if (edge) cached.value.values |= bit;
  return edge;
}

inline bool Runtime::isWall(const int x, const int y, const int z) {
  constexpr uint8_t bit = 1u << 2;
  auto& cached = cache_.classifications.at(x, y, z);
  const uint32_t generation = cacheGenerationAt(x, z);
  if (cached.generation != generation) cached = {generation, {}};
  if ((cached.value.computed & bit) != 0) return (cached.value.values & bit) != 0;

  const uint16_t head = flagsAt(x, y + 1, z);
  bool wall = hasFlag(head, VF_BLOCKING_WALL);
  if (!wall && hasFlag(head, VF_SOLID)) {
    const uint16_t feet = flagsAt(x, y, z);
    wall = hasFlag(head, VF_SLAB_TOP) ||
      (!hasFlag(feet, VF_SLAB_BOTTOM) && !hasFlag(feet, VF_STAIRS_BOTTOM));
  } else if (!wall) {
    wall = hasFlag(flagsAt(x, y, z), VF_BLOCKING_WALL);
  }

  cached.value.computed |= bit;
  if (wall) cached.value.values |= bit;
  return wall;
}

inline void Runtime::directionalDistances(
  const int x,
  const int y,
  const int z,
  const int mask,
  int& edgeDist,
  int& wallDist
) {
  edgeDist = MAX_DIST;
  wallDist = MAX_DIST;
  for (int dir = 0; dir < 8; dir++) {
    if ((mask & (1 << dir)) == 0) continue;

    int rayEdge = MAX_DIST;
    int rayWall = MAX_DIST;
    int cx = x;
    int cz = z;
    for (int d = 1; d <= MAX_DIST && (rayEdge == MAX_DIST || rayWall == MAX_DIST); d++) {
      cx += DX[static_cast<size_t>(dir)];
      cz += DZ[static_cast<size_t>(dir)];
      if (rayEdge == MAX_DIST && isEdge(cx, y, cz)) rayEdge = d - 1;
      if (rayWall == MAX_DIST && isWall(cx, y, cz)) rayWall = d - 1;
    }
    edgeDist = std::min(edgeDist, rayEdge);
    wallDist = std::min(wallDist, rayWall);
    if (edgeDist == 0 && wallDist == 0) break;
  }
}

inline float Runtime::combinedPenalty(const int edgeDist, const int wallDist) const {
  const int edgeIdx = std::clamp(edgeDist, 0, OPEN_SPACE_SOFT_CAP);
  const int wallIdx = std::clamp(wallDist, 0, OPEN_SPACE_SOFT_CAP);
  return EDGE_PENALTIES[static_cast<size_t>(edgeIdx)] + WALL_PENALTIES[static_cast<size_t>(wallIdx)];
}

inline float Runtime::pathPenalty(const int x, const int y, const int z) {
  auto& cached = cache_.penalties.at(x, y, z);
  const uint32_t generation = cacheGenerationAt(x, z);
  if (cached.generation == generation) return cached.value;

  const int mask = directionMask(x, y, z);
  int edgeDist;
  int wallDist;
  directionalDistances(x, y, z, mask, edgeDist, wallDist);
  float value = combinedPenalty(edgeDist, wallDist);
  if (hasFlag(flagsAt(x, y, z), VF_FLUID)) value += 20.0f;
  if (hasFlag(flagsAt(x, y + 1, z), VF_FLUID)) value += 20.0f;
  cached = {generation, value};
  return value;
}

} // namespace v5pf::detail
