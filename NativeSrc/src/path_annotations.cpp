#include "path_annotations.hpp"

#include "path_directional_scan.hpp"
#include "path_voxel_checks.hpp"

namespace v5pf {
namespace {

constexpr int FLAG_FLUID_FEET = 1 << 0;
constexpr int FLAG_FLUID_HEAD = 1 << 1;
constexpr int FLAG_LOW_HEADROOM = 1 << 2;
constexpr int FLAG_NEAR_EDGE = 1 << 3;
constexpr int FLAG_NEAR_WALL = 1 << 4;
constexpr int FLAG_STEP_UP_NEXT = 1 << 5;
constexpr int FLAG_DROP_NEXT = 1 << 6;
constexpr int FLAG_TIGHT_CORRIDOR = 1 << 7;

inline int edgeDistance(const WorldSnapshot& world, const int x, const int y, const int z) {
  return directionalDistance(world, x, y, z, isEdgeVoxel);
}

inline int wallDistance(const WorldSnapshot& world, const int x, const int y, const int z) {
  return directionalDistance(world, x, y, z, isWallVoxel);
}

} // namespace

std::vector<int> encodeNodeFlags(
  const WorldSnapshot& world,
  const std::vector<Int3>& nodes,
  const bool isFly,
  const bool includeProximity
) {
  if (nodes.empty()) return {};

  std::vector<int> out(nodes.size(), 0);
  const int yOffset = isFly ? 0 : -1;

  for (size_t i = 0; i < nodes.size(); i++) {
    const Int3& p = nodes[i];
    const int yOut = p.y + yOffset;
    const int walkY = yOut + (isFly ? 0 : 1);

    int flags = 0;
    if (isFluidVoxel(world, p.x, walkY, p.z)) flags |= FLAG_FLUID_FEET;
    if (isFluidVoxel(world, p.x, walkY + 1, p.z)) flags |= FLAG_FLUID_HEAD;

    if (!isFly) {
      if (!isWalkPassableVoxel(world, p.x, walkY + 2, p.z)) {
        flags |= FLAG_LOW_HEADROOM;
      }
    }

    if (includeProximity) {
      const int edgeDist = edgeDistance(world, p.x, walkY, p.z);
      const int wallDist = wallDistance(world, p.x, walkY, p.z);
      if (edgeDist <= 1) flags |= FLAG_NEAR_EDGE;
      if (wallDist <= 1) flags |= FLAG_NEAR_WALL;
      if (edgeDist <= 1 && wallDist <= 1) flags |= FLAG_TIGHT_CORRIDOR;
    }

    if (i + 1 < nodes.size()) {
      const int dy = nodes[i + 1].y - p.y;
      if (dy > 0) flags |= FLAG_STEP_UP_NEXT;
      if (dy < 0) flags |= FLAG_DROP_NEXT;
    }

    out[i] = flags;
  }

  return out;
}

std::vector<int> encodeKeyMetrics(const WorldSnapshot& world, const std::vector<Int3>& nodes) {
  if (nodes.empty()) return {};

  std::vector<int> out(nodes.size() * 3, 0);
  size_t idx = 0;
  for (size_t i = 0; i < nodes.size(); i++) {
    const Int3& p = nodes[i];
    out[idx++] = edgeDistance(world, p.x, p.y, p.z);
    out[idx++] = wallDistance(world, p.x, p.y, p.z);
    out[idx++] = i + 1 < nodes.size() ? nodes[i + 1].y - p.y : 0;
  }
  return out;
}

} // namespace v5pf
