#pragma once

#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace v5pf {

enum VoxelFlags : uint16_t {
  VF_PASSABLE = 1u << 0,
  VF_SOLID = 1u << 1,
  VF_PASSABLE_FLY = 1u << 2,
  VF_BLOCKING_WALL = 1u << 3,
  VF_FLUID = 1u << 4,
  VF_SLAB_BOTTOM = 1u << 5,
  VF_SLAB_TOP = 1u << 6,
  VF_FENCE_LIKE = 1u << 7,
  VF_STAIRS_BOTTOM = 1u << 8,
  VF_CARPET_LIKE = 1u << 9,
  VF_ETHER_PASSABLE = 1u << 10,
  VF_ETHER_TELEPORT_CLEAR = 1u << 11,
  VF_ETHER_FEET_BLOCKER = 1u << 12,
  VF_ETHER_FAKE_FULL_BLOCKER = 1u << 13,
};

constexpr uint16_t VF_AIR_DEFAULT = VF_PASSABLE | VF_PASSABLE_FLY | VF_ETHER_PASSABLE | VF_ETHER_TELEPORT_CLEAR;

struct Int3 {
  int x;
  int y;
  int z;
};

inline uint64_t coordKey(const int x, const int y, const int z) {
  const uint64_t px = ((static_cast<uint64_t>(x) + 33554432ULL) & 0x3FFFFFFULL);
  const uint64_t py = ((static_cast<uint64_t>(y) + 2048ULL) & 0xFFFULL);
  const uint64_t pz = ((static_cast<uint64_t>(z) + 33554432ULL) & 0x3FFFFFFULL);
  return (px << 38) | (py << 26) | pz;
}

inline uint64_t chunkKey(const int x, const int z) {
  const uint64_t px = static_cast<uint64_t>(static_cast<uint32_t>(x));
  const uint64_t pz = static_cast<uint64_t>(static_cast<uint32_t>(z));
  return (px << 32) | pz;
}

constexpr std::array<float, 257> makeFallTimes() {
  std::array<float, 257> fallTimes{};
  float currentDistance = 0.0f;
  int tick = 0;
  float velocity = 0.0f;

  for (int targetDistance = 1; targetDistance <= 256; targetDistance++) {
    while (currentDistance < targetDistance) {
      velocity = (velocity - 0.08f) * 0.98f;
      currentDistance -= velocity;
      tick++;
    }
    fallTimes[static_cast<size_t>(targetDistance)] = static_cast<float>(tick);
  }
  return fallTimes;
}

struct ActionCosts {
  static constexpr float INF_COST = 1e6f;
  static constexpr float FLY_ONE_BLOCK_TIME = 1.0f / 0.7f;
  static constexpr float SPRINT_ONE_BLOCK_TIME = 1.0f / 0.2806f;
  static constexpr float SPRINT_DIAGONAL_TIME = SPRINT_ONE_BLOCK_TIME * 1.41421356f;
  static constexpr float MOMENTUM_LOSS_PENALTY = 6.0f;
  static constexpr float JUMP_PENALTY = 2.0f;
  static constexpr float GAP_JUMP_REWARD_OFFSET = 1.5f;
  static constexpr float SLAB_ASCENT_TIME = SPRINT_ONE_BLOCK_TIME * 1.1f;
  static constexpr float WALK_OFF_EDGE_TIME = SPRINT_ONE_BLOCK_TIME * 0.5f;
  static constexpr float LAND_RECOVERY_TIME = 2.0f;
  static constexpr float JUMP_UP_ONE_BLOCK_TIME = 28.0f + MOMENTUM_LOSS_PENALTY + SPRINT_ONE_BLOCK_TIME;

  inline static constexpr auto FALL_TIMES = makeFallTimes();

  [[nodiscard]] static float getFallTime(const int blocks) {
    if (blocks <= 0) return 0.0f;
    if (blocks >= static_cast<int>(FALL_TIMES.size())) return INF_COST;
    return FALL_TIMES[static_cast<size_t>(blocks)] + LAND_RECOVERY_TIME;
  }
};

} // namespace v5pf
