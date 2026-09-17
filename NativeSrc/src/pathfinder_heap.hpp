#pragma once

#include <algorithm>
#include <vector>

namespace v5pf::detail {

template<typename Cost>
class Heap {
 public:
  explicit Heap(
    const std::vector<Cost>& fCost,
    const std::vector<Cost>& hCost,
    const std::vector<Cost>& gCost,
    std::vector<int>& heapPos
  ) : fCost_(fCost), hCost_(hCost), gCost_(gCost), heapPos_(heapPos) {
    items_.resize(1);
  }

  [[nodiscard]] bool empty() const {
    return size_ == 0;
  }

  void clear() {
    size_ = 0;
  }

  void reserve(const int capacity) {
    if (capacity <= 0) return;
    const size_t needed = static_cast<size_t>(capacity + 1);
    if (items_.size() < needed) {
      items_.resize(needed, -1);
    }
  }

  void add(const int nodeIdx) {
    if (static_cast<int>(items_.size()) <= size_ + 1) {
      items_.resize(items_.size() * 2 + 1, -1);
    }

    size_++;
    items_[static_cast<size_t>(size_)] = nodeIdx;
    heapPos_[static_cast<size_t>(nodeIdx)] = size_;
    siftUp(size_);
  }

  void relocate(const int nodeIdx) {
    const int pos = heapPos_[static_cast<size_t>(nodeIdx)];
    if (pos <= 0) return;
    siftUp(pos);
  }

  int poll() {
    if (size_ <= 0) return -1;

    const int result = items_[1];
    heapPos_[static_cast<size_t>(result)] = -1;

    if (size_ == 1) {
      size_ = 0;
      return result;
    }

    const int last = items_[static_cast<size_t>(size_)];
    size_--;

    items_[1] = last;
    heapPos_[static_cast<size_t>(last)] = 1;
    siftDown(1);

    return result;
  }

 private:
  std::vector<int> items_;
  int size_ = 0;

  const std::vector<Cost>& fCost_;
  const std::vector<Cost>& hCost_;
  const std::vector<Cost>& gCost_;
  std::vector<int>& heapPos_;

  [[nodiscard]] bool less(const int left, const int right) const {
    const size_t lhs = static_cast<size_t>(left);
    const size_t rhs = static_cast<size_t>(right);
    if (fCost_[lhs] != fCost_[rhs]) return fCost_[lhs] < fCost_[rhs];
    if (hCost_[lhs] != hCost_[rhs]) return hCost_[lhs] < hCost_[rhs];
    if (gCost_[lhs] != gCost_[rhs]) return gCost_[lhs] > gCost_[rhs];
    return left < right;
  }

  void siftUp(int pos) {
    const int node = items_[static_cast<size_t>(pos)];

    while (pos > 1) {
      const int parent = (pos - 2) / 4 + 1;
      const int parentNode = items_[static_cast<size_t>(parent)];
      if (!less(node, parentNode)) break;

      items_[static_cast<size_t>(pos)] = parentNode;
      heapPos_[static_cast<size_t>(parentNode)] = pos;
      pos = parent;
    }

    items_[static_cast<size_t>(pos)] = node;
    heapPos_[static_cast<size_t>(node)] = pos;
  }

  void siftDown(int pos) {
    const int node = items_[static_cast<size_t>(pos)];

    while (true) {
      const int firstChild = (pos - 1) * 4 + 2;
      if (firstChild > size_) break;

      int child = firstChild;
      const int lastChild = std::min(firstChild + 3, size_);
      for (int candidate = firstChild + 1; candidate <= lastChild; candidate++) {
        if (less(items_[static_cast<size_t>(candidate)], items_[static_cast<size_t>(child)])) {
          child = candidate;
        }
      }

      const int childNode = items_[static_cast<size_t>(child)];
      if (!less(childNode, node)) break;

      items_[static_cast<size_t>(pos)] = childNode;
      heapPos_[static_cast<size_t>(childNode)] = pos;
      pos = child;
    }

    items_[static_cast<size_t>(pos)] = node;
    heapPos_[static_cast<size_t>(node)] = pos;
  }
};


} // namespace v5pf::detail
