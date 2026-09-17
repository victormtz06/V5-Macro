#include "path_simplifier.hpp"

#include "path_voxel_checks.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>

namespace v5pf {
namespace {

bool canFlyDirectly(const WorldSnapshot& world, const Int3& from, const Int3& to) {
  const int dx = to.x - from.x;
  const int dy = to.y - from.y;
  const int dz = to.z - from.z;

  const int steps = std::max({std::abs(dx), std::abs(dy), std::abs(dz)});
  if (steps <= 1) return true;

  int lastX = std::numeric_limits<int>::min();
  int lastY = std::numeric_limits<int>::min();
  int lastZ = std::numeric_limits<int>::min();

  for (int i = 1; i <= steps; i++) {
    const double t = static_cast<double>(i) / static_cast<double>(steps);
    const int x = static_cast<int>(std::floor(static_cast<double>(from.x) + static_cast<double>(dx) * t));
    const int y = static_cast<int>(std::floor(static_cast<double>(from.y) + static_cast<double>(dy) * t));
    const int z = static_cast<int>(std::floor(static_cast<double>(from.z) + static_cast<double>(dz) * t));

    if (x == lastX && y == lastY && z == lastZ) continue;
    if (!isFlyColumnClearVoxel(world, x, y, z)) return false;

    lastX = x;
    lastY = y;
    lastZ = z;
  }

  return true;
}

} // namespace

std::vector<Int3> extractKeyPoints(
  const WorldSnapshot& world,
  const std::vector<Int3>& points,
  const bool isFly,
  const double epsilon
) {
  if (points.size() <= 2) return points;

  std::vector<Int3> result;
  result.reserve(points.size());

  std::function<void(int, int)> simplify = [&](const int from, const int to) {
    if (to - from <= 1) {
      result.push_back(points[static_cast<size_t>(from)]);
      return;
    }

    const Int3& start = points[static_cast<size_t>(from)];
    const Int3& end = points[static_cast<size_t>(to)];

    const double dx = static_cast<double>(end.x - start.x);
    const double dy = static_cast<double>(end.y - start.y);
    const double dz = static_cast<double>(end.z - start.z);
    const double lenSq = dx * dx + dy * dy + dz * dz;

    double maxDistSq = 0.0;
    int maxIndex = -1;

    for (int i = from + 1; i < to; i++) {
      const Int3& p = points[static_cast<size_t>(i)];

      double distSq = 0.0;
      if (lenSq == 0.0) {
        const double ddx = static_cast<double>(p.x - start.x);
        const double ddy = static_cast<double>(p.y - start.y);
        const double ddz = static_cast<double>(p.z - start.z);
        distSq = ddx * ddx + ddy * ddy + ddz * ddz;
      } else {
        const double px = static_cast<double>(p.x - start.x);
        const double py = static_cast<double>(p.y - start.y);
        const double pz = static_cast<double>(p.z - start.z);

        const double t = std::clamp((px * dx + py * dy + pz * dz) / lenSq, 0.0, 1.0);

        const double cx = static_cast<double>(start.x) + dx * t;
        const double cy = static_cast<double>(start.y) + dy * t;
        const double cz = static_cast<double>(start.z) + dz * t;

        const double ox = static_cast<double>(p.x) - cx;
        const double oy = static_cast<double>(p.y) - cy;
        const double oz = static_cast<double>(p.z) - cz;
        distSq = ox * ox + oy * oy + oz * oz;
      }

      if (distSq > maxDistSq) {
        maxDistSq = distSq;
        maxIndex = i;
      }
    }

    bool canSimplify = maxDistSq <= epsilon * epsilon;

    if (canSimplify) {
      if (isFly) {
        canSimplify = canFlyDirectly(world, start, end);
      }
    }

    if (!canSimplify) {
      const int splitIndex = maxIndex == -1 ? (from + to) / 2 : maxIndex;
      simplify(from, splitIndex);
      simplify(splitIndex, to);
    } else {
      result.push_back(start);
    }
  };

  simplify(0, static_cast<int>(points.size() - 1));
  result.push_back(points.back());

  return result;
}

} // namespace v5pf
