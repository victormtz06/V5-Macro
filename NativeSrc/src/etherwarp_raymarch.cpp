#include "etherwarp_raymarch.hpp"

#include "path_voxel_checks.hpp"
#include "world_voxel_cursor.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace v5pf {
namespace {

inline double boundaryDistance(const double position, const int step) {
  const double floored = std::floor(position);
  if (step > 0) return (floored + 1.0) - position;
  if (step < 0) return position - floored;
  return std::numeric_limits<double>::infinity();
}

inline bool isLandingBlockAt(
  WorldVoxelCursor& cursor,
  const int x,
  const int y,
  const int z,
  const uint16_t supportFlags,
  const WorldSnapshot& world
) {
  if (!isEtherwarpStandableFlags(supportFlags)) {
    return false;
  }

  const int standOffset = etherwarpStandOffset(supportFlags);
  const int feetY = y + standOffset;
  const int headY = feetY + 1;
  if (y < world.minY || headY >= world.maxY) {
    return false;
  }

  const uint16_t feetFlags = cursor.getFlags(x, feetY, z);
  if (!isEtherwarpTeleportSpaceClearFlags(feetFlags)) {
    return false;
  }

  return isEtherwarpTeleportSpaceClearFlags(cursor.getFlags(x, headY, z));
}

} // namespace

std::optional<Int3> raymarchEtherwarp(
  const WorldSnapshot& world,
  const double originX,
  const double originY,
  const double originZ,
  const EtherwarpRayDirection& direction,
  const double maxDistance
) {
  const double dirX = direction.dirX;
  const double dirY = direction.dirY;
  const double dirZ = direction.dirZ;

  const double lengthSq = dirX * dirX + dirY * dirY + dirZ * dirZ;
  if (lengthSq <= 0.0 || !std::isfinite(lengthSq)) {
    return std::nullopt;
  }

  int cellX = static_cast<int>(std::floor(originX));
  int cellY = static_cast<int>(std::floor(originY));
  int cellZ = static_cast<int>(std::floor(originZ));
  WorldVoxelCursor cursor(world);
  uint16_t cellFlags = cursor.getFlags(cellX, cellY, cellZ);

  const int stepX = dirX > 0.0 ? 1 : (dirX < 0.0 ? -1 : 0);
  const int stepY = dirY > 0.0 ? 1 : (dirY < 0.0 ? -1 : 0);
  const int stepZ = dirZ > 0.0 ? 1 : (dirZ < 0.0 ? -1 : 0);

  const double invAbsX = stepX == 0 ? std::numeric_limits<double>::infinity() : 1.0 / std::abs(dirX);
  const double invAbsY = stepY == 0 ? std::numeric_limits<double>::infinity() : 1.0 / std::abs(dirY);
  const double invAbsZ = stepZ == 0 ? std::numeric_limits<double>::infinity() : 1.0 / std::abs(dirZ);

  double nextX = boundaryDistance(originX, stepX) * invAbsX;
  double nextY = boundaryDistance(originY, stepY) * invAbsY;
  double nextZ = boundaryDistance(originZ, stepZ) * invAbsZ;

  if (isEtherwarpFakeFullBlockerFlags(cellFlags)) return std::nullopt;
  if (!isEtherPassableFlags(cellFlags)) {
    if (isLandingBlockAt(cursor, cellX, cellY, cellZ, cellFlags, world)) {
      return Int3{cellX, cellY, cellZ};
    }
    return std::nullopt;
  }

  double distance = 0.0;
  int traversedCells = 0;

  while (distance <= maxDistance && traversedCells++ < 1000) {
    int minAxis = 0;
    double minNext = nextX;
    if (nextX < nextY) {
      if (nextX < nextZ) {
        minAxis = 0;
        minNext = nextX;
      } else {
        minAxis = 2;
        minNext = nextZ;
      }
    } else if (nextY < nextZ) {
      minAxis = 1;
      minNext = nextY;
    } else {
      minAxis = 2;
      minNext = nextZ;
    }

    distance = minNext;

    if (minAxis == 0) {
      cellX += stepX;
      nextX += invAbsX;
    } else if (minAxis == 1) {
      cellY += stepY;
      nextY += invAbsY;
    } else {
      cellZ += stepZ;
      nextZ += invAbsZ;
    }

    if (distance > maxDistance) {
      break;
    }

    cellFlags = cursor.getFlags(cellX, cellY, cellZ);
    if (isEtherwarpFakeFullBlockerFlags(cellFlags)) return std::nullopt;
    if (!isEtherPassableFlags(cellFlags)) {
      if (isLandingBlockAt(cursor, cellX, cellY, cellZ, cellFlags, world)) {
        return Int3{cellX, cellY, cellZ};
      }
      return std::nullopt;
    }
  }

  return std::nullopt;
}

} // namespace v5pf
