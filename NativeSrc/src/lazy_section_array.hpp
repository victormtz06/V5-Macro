#pragma once

#include "common.hpp"

#include <array>
#include <memory>
#include <unordered_map>

namespace v5pf::detail {

template<typename T>
class LazySectionArray {
 public:
  T& at(const int x, const int y, const int z) {
    const uint64_t key = coordKey(x >> 4, y >> 4, z >> 4);
    const size_t cursorIdx = static_cast<size_t>((key ^ (key >> 26) ^ (key >> 38)) & 3ULL);
    auto& cursor = cursors_[cursorIdx];
    if (cursor.section == nullptr || cursor.key != key) {
      auto [it, inserted] = sections_.try_emplace(key);
      if (inserted) it->second = std::make_unique<Section>();
      cursor = {key, it->second.get()};
    }

    const size_t index = static_cast<size_t>(((y & 15) << 8) | ((z & 15) << 4) | (x & 15));
    return (*cursor.section)[index];
  }

  void clear() {
    sections_.clear();
    cursors_ = {};
  }

 private:
  using Section = std::array<T, 4096>;

  struct Cursor {
    uint64_t key = 0;
    Section* section = nullptr;
  };

  std::unordered_map<uint64_t, std::unique_ptr<Section>> sections_;
  std::array<Cursor, 4> cursors_{};
};

} // namespace v5pf::detail
