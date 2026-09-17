#pragma once

#include "world_state.hpp"

namespace v5pf {

inline bool hasVoxelFlag(const uint16_t flags, const uint16_t bit) {
  return (flags & bit) != 0;
}

inline bool isEtherPassableFlags(const uint16_t flags) {
  return hasVoxelFlag(flags, VF_ETHER_PASSABLE);
}

inline bool isEtherwarpTeleportSpaceClearFlags(const uint16_t flags) {
  return hasVoxelFlag(flags, VF_ETHER_TELEPORT_CLEAR) &&
    !hasVoxelFlag(flags, VF_ETHER_FEET_BLOCKER);
}

inline bool isEtherwarpStandableFlags(const uint16_t flags) {
  return hasVoxelFlag(flags, VF_SOLID);
}

inline bool isEtherwarpFakeFullBlockerFlags(const uint16_t flags) {
  return hasVoxelFlag(flags, VF_ETHER_FAKE_FULL_BLOCKER);
}

inline int etherwarpStandOffset(const uint16_t supportFlags) {
  return hasVoxelFlag(supportFlags, VF_FENCE_LIKE) ? 2 : 1;
}

inline double etherwarpLandingYOffset(const uint16_t supportFlags) {
  return hasVoxelFlag(supportFlags, VF_FENCE_LIKE) ? 2.05 : 1.05;
}

inline double etherwarpEyeYOffset(const uint16_t supportFlags, const double baseEyeHeight) {
  // Etherwarp chains can fire before the player settles from +0.05 down onto the block.
  // Keep the origin at the transient teleport height so fast consecutive casts do not aim high.
  return baseEyeHeight + (etherwarpLandingYOffset(supportFlags) - 1.0);
}

inline uint16_t flagsAt(const WorldSnapshot& world, const int x, const int y, const int z) {
  return world.getFlags(x, y, z);
}

inline bool isSolidVoxel(const WorldSnapshot& world, const int x, const int y, const int z) {
  return hasVoxelFlag(flagsAt(world, x, y, z), VF_SOLID);
}

inline bool isTopSlabVoxel(const WorldSnapshot& world, const int x, const int y, const int z) {
  return hasVoxelFlag(flagsAt(world, x, y, z), VF_SLAB_TOP);
}

inline bool isFluidVoxel(const WorldSnapshot& world, const int x, const int y, const int z) {
  return hasVoxelFlag(flagsAt(world, x, y, z), VF_FLUID);
}

inline bool isWalkPassableVoxel(const WorldSnapshot& world, const int x, const int y, const int z) {
  const uint16_t flags = flagsAt(world, x, y, z);
  return hasVoxelFlag(flags, VF_PASSABLE) || hasVoxelFlag(flags, VF_CARPET_LIKE);
}

inline bool isFlyPassableVoxel(const WorldSnapshot& world, const int x, const int y, const int z) {
  const uint16_t flags = flagsAt(world, x, y, z);
  return hasVoxelFlag(flags, VF_PASSABLE_FLY) ||
    hasVoxelFlag(flags, VF_PASSABLE) ||
    hasVoxelFlag(flags, VF_CARPET_LIKE);
}

inline bool isEtherPassableVoxel(const WorldSnapshot& world, const int x, const int y, const int z) {
  return isEtherPassableFlags(flagsAt(world, x, y, z));
}

inline bool isEtherwarpTeleportSpaceClearVoxel(
  const WorldSnapshot& world,
  const int x,
  const int y,
  const int z
) {
  return isEtherwarpTeleportSpaceClearFlags(flagsAt(world, x, y, z));
}

inline bool isEtherwarpStandableVoxel(const WorldSnapshot& world, const int x, const int y, const int z) {
  return isEtherwarpStandableFlags(flagsAt(world, x, y, z));
}

inline bool isEtherwarpLandingBlockVoxel(const WorldSnapshot& world, const int x, const int y, const int z) {
  const uint16_t supportFlags = flagsAt(world, x, y, z);
  if (!isEtherwarpStandableFlags(supportFlags)) return false;

  const int standOffset = etherwarpStandOffset(supportFlags);
  const int feetY = y + standOffset;
  const int headY = feetY + 1;
  if (y < world.minY || headY >= world.maxY) return false;

  return isEtherwarpTeleportSpaceClearVoxel(world, x, feetY, z) &&
    isEtherwarpTeleportSpaceClearVoxel(world, x, headY, z);
}

inline bool isWalkSafeVoxel(const WorldSnapshot& world, const int x, const int y, const int z) {
  return isSolidVoxel(world, x, y - 1, z) &&
    isWalkPassableVoxel(world, x, y, z) &&
    isWalkPassableVoxel(world, x, y + 1, z);
}

inline bool isFlyColumnClearVoxel(const WorldSnapshot& world, const int x, const int y, const int z) {
  return isFlyPassableVoxel(world, x, y, z) &&
    isFlyPassableVoxel(world, x, y + 1, z) &&
    !isTopSlabVoxel(world, x, y + 1, z);
}

inline bool isEdgeVoxel(const WorldSnapshot& world, const int x, const int y, const int z) {
  return !isSolidVoxel(world, x, y - 1, z);
}

inline bool isWallVoxel(const WorldSnapshot& world, const int x, const int y, const int z) {
  if (!isWalkPassableVoxel(world, x, y, z)) return true;
  if (!isWalkPassableVoxel(world, x, y + 1, z)) return true;

  const uint16_t feetFlags = flagsAt(world, x, y, z);
  const uint16_t headFlags = flagsAt(world, x, y + 1, z);

  return hasVoxelFlag(feetFlags, VF_BLOCKING_WALL) ||
    hasVoxelFlag(feetFlags, VF_FENCE_LIKE) ||
    hasVoxelFlag(headFlags, VF_BLOCKING_WALL) ||
    hasVoxelFlag(headFlags, VF_FENCE_LIKE);
}

} // namespace v5pf
