#pragma once

#include "common.hpp"

#include <memory>
#include <mutex>
#include <optional>

namespace v5pf {

struct BlockUpdate {
  int x;
  int y;
  int z;
  uint16_t flags;
};

struct ChunkData {
  int minY = -64;
  int maxY = 320; // exclusive
  std::vector<int32_t> sectionOffsets;
  std::vector<uint16_t> voxels;

  [[nodiscard]] int sectionCount() const {
    const int64_t minY64 = static_cast<int64_t>(minY);
    const int64_t maxY64 = static_cast<int64_t>(maxY);
    if (maxY64 <= minY64) {
      return 0;
    }

    const int64_t span = maxY64 - minY64;
    const int64_t sections = (span + 15) >> 4;
    if (sections > static_cast<int64_t>(std::numeric_limits<int>::max())) {
      return std::numeric_limits<int>::max();
    }
    return static_cast<int>(sections);
  }

  void ensureLayout();

  [[nodiscard]] bool hasSection(int sectionIdx) const;
  [[nodiscard]] const uint16_t* sectionData(int sectionIdx) const;
  [[nodiscard]] uint16_t* sectionData(int sectionIdx);
  void assignSection(int sectionIdx, const uint16_t* source);
  [[nodiscard]] uint16_t getFlags(int localX, int y, int localZ) const;
  void setFlags(int localX, int y, int localZ, uint16_t flags);
};

struct ChunkUpdate {
  int chunkX;
  int chunkZ;
  ChunkData chunk;
};

using SharedChunkData = std::shared_ptr<const ChunkData>;
using ChunkMap = std::unordered_map<uint64_t, SharedChunkData>;

struct WorldIdentity {};

struct WorldData {
  std::string worldKey = "runtime_memory";
  int minY = -64;
  int maxY = 320; // exclusive
  ChunkMap chunks;
  std::shared_ptr<const WorldIdentity> identity = std::make_shared<WorldIdentity>();
  uint32_t latestCacheGeneration = 1;
  std::unordered_map<uint64_t, uint32_t> chunkCacheGenerations;
};

struct WorldSnapshot {
  std::shared_ptr<const WorldData> data;
  std::string worldKey;
  int minY = -64;
  int maxY = 320; // exclusive

  [[nodiscard]] const ChunkMap& chunks() const;
  [[nodiscard]] uint16_t getFlags(int x, int y, int z) const;
  [[nodiscard]] uint32_t cacheGenerationForChunk(int chunkX, int chunkZ) const;
};

class WorldState {
 public:
  void setWorld(std::string worldKey, int minY, int maxY);
  void clear();

  void upsertChunks(std::vector<ChunkUpdate> updates);
  void removeChunks(const std::vector<uint64_t>& chunkKeys);

  void applyUpdates(const std::vector<BlockUpdate>& updates);

  [[nodiscard]] WorldSnapshot snapshot() const;

 private:
  mutable std::mutex mutex_;
  std::shared_ptr<const WorldData> data_ = std::make_shared<WorldData>();
};

} // namespace v5pf
