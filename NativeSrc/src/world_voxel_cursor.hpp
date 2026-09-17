#pragma once

#include "world_state.hpp"

#include <algorithm>
#include <array>
#include <limits>

namespace v5pf {

class WorldVoxelCursor {
 public:
  explicit WorldVoxelCursor(const WorldSnapshot& world)
    : world_(world) {
  }

  [[nodiscard]] uint16_t getFlags(const int x, const int y, const int z) const {
    if (y < world_.minY || y >= world_.maxY) {
      return VF_AIR_DEFAULT;
    }

    const int chunkX = x >> 4;
    const int chunkZ = z >> 4;
    const int sectionY = (y - world_.minY) >> 4;
    const uint32_t cursorHash = static_cast<uint32_t>(chunkX) ^
      (static_cast<uint32_t>(chunkZ) << 1) ^ static_cast<uint32_t>(sectionY);
    auto& entry = entries_[static_cast<size_t>(cursorHash & 3u)];
    if (chunkX != entry.chunkX || chunkZ != entry.chunkZ || y < entry.sectionMinY || y >= entry.sectionMaxY) {
      entry = {};
      entry.chunkX = chunkX;
      entry.chunkZ = chunkZ;

      const auto& chunks = world_.chunks();
      const auto it = chunks.find(chunkKey(chunkX, chunkZ));
      if (it == chunks.end() || it->second == nullptr) {
        entry.sectionMinY = world_.minY;
        entry.sectionMaxY = world_.maxY;
        entry.fallback = VF_SOLID | VF_BLOCKING_WALL;
        return entry.fallback;
      }

      const ChunkData* chunk = it->second.get();
      if (y < chunk->minY || y >= chunk->maxY) return VF_AIR_DEFAULT;
      const int sectionIdx = (y - chunk->minY) >> 4;
      entry.sectionMinY = chunk->minY + (sectionIdx << 4);
      entry.sectionMaxY = std::min(entry.sectionMinY + 16, chunk->maxY);
      entry.section = chunk->sectionData(sectionIdx);
    }

    if (entry.section == nullptr) {
      return entry.fallback;
    }

    const int index = ((y & 15) << 8) | ((z & 15) << 4) | (x & 15);
    return entry.section[static_cast<size_t>(index)];
  }

 private:
  struct Entry {
    int chunkX = std::numeric_limits<int>::min();
    int chunkZ = std::numeric_limits<int>::min();
    int sectionMinY = std::numeric_limits<int>::max();
    int sectionMaxY = std::numeric_limits<int>::min();
    const uint16_t* section = nullptr;
    uint16_t fallback = VF_AIR_DEFAULT;
  };

  const WorldSnapshot& world_;
  mutable std::array<Entry, 4> entries_{};
};

} // namespace v5pf
