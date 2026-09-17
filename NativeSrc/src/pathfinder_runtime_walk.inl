#pragma once

namespace v5pf::detail {

inline bool Runtime::moveTraverse(const Int3& current, const int dx, const int dz, MoveOut& out) {
  const int destX = current.x + dx;
  const int destZ = current.z + dz;
  if (!isSafe(destX, current.y, destZ)) return false;

  out.pos = {destX, current.y, destZ};
  out.cost = ActionCosts::SPRINT_ONE_BLOCK_TIME +
    pathPenalty(destX, current.y, destZ);
  return true;
}

inline bool Runtime::moveDiagonal(const Int3& current, const int dx, const int dz, MoveOut& out) {
  const int destX = current.x + dx;
  const int destZ = current.z + dz;

  if (!isPassable(destX, current.y, current.z)) return false;
  if (!isPassable(current.x, current.y, destZ)) return false;
  if (!isPassable(destX, current.y + 1, current.z)) return false;
  if (!isPassable(current.x, current.y + 1, destZ)) return false;
  if (!isSafe(destX, current.y, destZ)) return false;

  out.pos = {destX, current.y, destZ};
  out.cost = ActionCosts::SPRINT_DIAGONAL_TIME +
    pathPenalty(destX, current.y, destZ);
  return true;
}

inline bool Runtime::moveAscend(const Int3& current, const int dx, const int dz, MoveOut& out) {
  const int destX = current.x + dx;
  const int destZ = current.z + dz;

  if (!isPassableFlags(flagsAt(current.x, current.y + 2, current.z))) return false;

  const uint16_t destSupport = flagsAt(destX, current.y, destZ);
  if (!hasFlag(destSupport, VF_SOLID) || hasFlag(destSupport, VF_FENCE_LIKE)) return false;
  if (!isPassableFlags(flagsAt(destX, current.y + 1, destZ))) return false;
  if (!isPassableFlags(flagsAt(destX, current.y + 2, destZ))) return false;

  const uint16_t srcSupport = flagsAt(current.x, current.y - 1, current.z);
  const bool srcBottom = hasFlag(srcSupport, VF_SLAB_BOTTOM);
  const bool destBottom = hasFlag(destSupport, VF_SLAB_BOTTOM);

  if (srcBottom && !destBottom) return false;

  const bool srcStair = hasFlag(srcSupport, VF_STAIRS_BOTTOM);
  const bool destStair = hasFlag(destSupport, VF_STAIRS_BOTTOM);

  out.pos = {destX, current.y + 1, destZ};
  float baseCost = ActionCosts::JUMP_UP_ONE_BLOCK_TIME;
  if (destBottom || srcStair || destStair) {
    baseCost = ActionCosts::SLAB_ASCENT_TIME;
  }

  out.cost = baseCost +
    pathPenalty(destX, current.y + 1, destZ);
  return true;
}

inline bool Runtime::moveDescend(const Int3& current, const int dx, const int dz, MoveOut& out) {
  const int destX = current.x + dx;
  const int destZ = current.z + dz;

  if (!isPassable(destX, current.y + 1, destZ)) return false;
  if (!isPassable(destX, current.y, destZ)) return false;
  if (!isPassable(destX, current.y - 1, destZ)) return false;

  constexpr int maxFallHeight = 20;

  for (int dropBlocks = 1; dropBlocks <= maxFallHeight; dropBlocks++) {
    const int floorY = current.y - dropBlocks - 1;

    const uint16_t floorFlags = flagsAt(destX, floorY, destZ);
    if (isPassableFlags(floorFlags)) {
      continue;
    }

    if (!hasFlag(floorFlags, VF_SOLID)) {
      return false;
    }

    const int destY = floorY + 1;
    float totalCost = ActionCosts::WALK_OFF_EDGE_TIME + ActionCosts::getFallTime(dropBlocks);
    if (dropBlocks > 3) {
      const int excess = dropBlocks - 3;
      totalCost += static_cast<float>(excess * excess) * 2.0f;
    }

    totalCost += pathPenalty(destX, destY, destZ);

    out.pos = {destX, destY, destZ};
    out.cost = totalCost;
    return true;
  }

  return false;
}

} // namespace v5pf::detail
