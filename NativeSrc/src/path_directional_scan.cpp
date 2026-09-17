#include "path_directional_scan.hpp"

#include <array>

namespace v5pf {
namespace {

constexpr int MAX_PROXIMITY_SCAN = 6;
constexpr std::array<int, 8> DIR_X = {0, 0, 1, -1, 1, -1, 1, -1};
constexpr std::array<int, 8> DIR_Z = {-1, 1, 0, 0, -1, -1, 1, 1};

} // namespace

int directionalDistance(
  const WorldSnapshot& world,
  const int x,
  const int y,
  const int z,
  DirectionMatcher matcher
) {
  int minDist = MAX_PROXIMITY_SCAN;

  for (size_t dir = 0; dir < DIR_X.size(); dir++) {
    const int dx = DIR_X[dir];
    const int dz = DIR_Z[dir];
    int cx = x + dx;
    int cz = z + dz;

    int dist = MAX_PROXIMITY_SCAN;
    for (int d = 1; d <= MAX_PROXIMITY_SCAN; d++) {
      if (matcher(world, cx, y, cz)) {
        dist = d - 1;
        break;
      }
      cx += dx;
      cz += dz;
    }

    if (dist < minDist) {
      minDist = dist;
    }

    if (minDist == 0) {
      break;
    }
  }

  return minDist;
}

} // namespace v5pf
