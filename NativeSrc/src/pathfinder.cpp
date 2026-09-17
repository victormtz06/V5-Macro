#include "pathfinder.hpp"

#include "path_annotations.hpp"
#include "lazy_section_array.hpp"
#include "path_simplifier.hpp"
#include "path_signature.hpp"
#include "pathfinder_heap.hpp"
#include "pathfinder_runtime.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <limits>

namespace v5pf {

namespace {

struct NodeIndexEntry {
  uint32_t generation = 0;
  int index = -1;
};

class NodeIndexTable {
 public:
  void reset() {
    if (++generation_ == 0) {
      entries_.clear();
      generation_ = 1;
    }
  }

  std::pair<int, bool> getOrInsert(const int x, const int y, const int z, const int index) {
    auto& entry = entries_.at(x, y, z);
    if (entry.generation == generation_) return {entry.index, false};
    entry = {generation_, index};
    return {index, true};
  }

 private:
  uint32_t generation_ = 0;
  detail::LazySectionArray<NodeIndexEntry> entries_;
};

struct SearchWorkspace {
  std::vector<int> nodeX;
  std::vector<int> nodeY;
  std::vector<int> nodeZ;
  std::vector<float> nodeG;
  std::vector<float> nodeH;
  std::vector<float> nodeF;
  std::vector<float> nodeAvoidPenalty;
  std::vector<int> nodeParent;
  std::vector<int> nodeStartIndex;
  std::vector<int> nodeHeapPos;
  NodeIndexTable coordToNode;
  detail::Heap<float> heap;

  SearchWorkspace()
    : heap(nodeF, nodeH, nodeG, nodeHeapPos) {
  }

  void reset(const size_t reserveSize, const int heapCapacity) {
    nodeX.clear();
    nodeY.clear();
    nodeZ.clear();
    nodeG.clear();
    nodeH.clear();
    nodeF.clear();
    nodeAvoidPenalty.clear();
    nodeParent.clear();
    nodeStartIndex.clear();
    nodeHeapPos.clear();
    coordToNode.reset();
    heap.clear();

    nodeX.reserve(reserveSize);
    nodeY.reserve(reserveSize);
    nodeZ.reserve(reserveSize);
    nodeG.reserve(reserveSize);
    nodeH.reserve(reserveSize);
    nodeF.reserve(reserveSize);
    nodeAvoidPenalty.reserve(reserveSize);
    nodeParent.reserve(reserveSize);
    nodeStartIndex.reserve(reserveSize);
    nodeHeapPos.reserve(reserveSize);
    heap.reserve(heapCapacity);
  }
};

SearchWorkspace& searchWorkspace() {
  static thread_local SearchWorkspace workspace;
  return workspace;
}

} // namespace

std::optional<SearchResult> findPath(
  const WorldSnapshot& world,
  const SearchParams& params,
  std::atomic_bool& cancelFlag
) {
  cancelFlag.store(false);

  if (params.starts.empty() || params.goals.empty()) {
    return std::nullopt;
  }

  if (params.maxIterations <= 0) {
    return std::nullopt;
  }

  if (params.isFly && params.starts.size() != 1) {
    return std::nullopt;
  }

  const auto startTime = std::chrono::steady_clock::now();
  detail::Runtime runtime(world, params);

  const int reserveTarget = std::clamp(params.maxIterations / 2, 16384, 262144);
  const size_t reserveSize = static_cast<size_t>(reserveTarget);
  auto& workspace = searchWorkspace();
  workspace.reset(reserveSize, reserveTarget);
  auto& nodeX = workspace.nodeX;
  auto& nodeY = workspace.nodeY;
  auto& nodeZ = workspace.nodeZ;
  auto& nodeG = workspace.nodeG;
  auto& nodeH = workspace.nodeH;
  auto& nodeF = workspace.nodeF;
  auto& nodeAvoidPenalty = workspace.nodeAvoidPenalty;
  auto& nodeParent = workspace.nodeParent;
  auto& nodeStartIndex = workspace.nodeStartIndex;
  auto& nodeHeapPos = workspace.nodeHeapPos;
  auto& coordToNode = workspace.coordToNode;
  auto& heap = workspace.heap;

  const float weight = (std::isfinite(params.heuristicWeight) && params.heuristicWeight > 0.0f)
    ? params.heuristicWeight
    : 1.0f;
  const bool isFly = params.isFly;

  auto getOrCreateNode = [&](const int x, const int y, const int z) {
    const int idx = static_cast<int>(nodeX.size());
    const auto [nodeIdx, inserted] = coordToNode.getOrInsert(x, y, z, idx);
    if (!inserted) return std::pair{nodeIdx, false};

    nodeX.push_back(x);
    nodeY.push_back(y);
    nodeZ.push_back(z);
    nodeG.push_back(std::numeric_limits<float>::infinity());
    nodeH.push_back(runtime.heuristic(x, y, z) * weight);
    nodeF.push_back(nodeH.back());
    nodeAvoidPenalty.push_back(runtime.transientAvoidPenalty(x, y, z));
    nodeParent.push_back(-1);
    nodeStartIndex.push_back(-1);
    nodeHeapPos.push_back(-1);
    return std::pair{idx, true};
  };

  auto setNodeCost = [&](const int nodeIdx, const float cost) {
    const size_t idx = static_cast<size_t>(nodeIdx);
    nodeG[idx] = cost;
    nodeF[idx] = cost + nodeH[idx];
  };

  std::array<Int3, 16> walkMovesOrdered = detail::WALK_MOVES;
  const int walkOffset = params.moveOrderOffset >= 0 ? (params.moveOrderOffset % static_cast<int>(walkMovesOrdered.size())) : 0;
  if (walkOffset != 0) {
    for (int i = 0; i < static_cast<int>(walkMovesOrdered.size()); i++) {
      walkMovesOrdered[static_cast<size_t>(i)] =
        detail::WALK_MOVES[static_cast<size_t>((i + walkOffset) % static_cast<int>(walkMovesOrdered.size()))];
    }
  }

  for (size_t i = 0; i < params.starts.size(); i++) {
    const auto& start = params.starts[i];
    const float startPenalty = i == 0 ? 0.0f : std::max(0.0f, params.nonPrimaryStartPenalty);

    const int nodeIdx = getOrCreateNode(start.x, start.y, start.z).first;

    if (startPenalty < nodeG[static_cast<size_t>(nodeIdx)]) {
      nodeParent[static_cast<size_t>(nodeIdx)] = -1;
      nodeStartIndex[static_cast<size_t>(nodeIdx)] = static_cast<int>(i);
      setNodeCost(nodeIdx, startPenalty);
    }

    if (nodeHeapPos[static_cast<size_t>(nodeIdx)] == -1) {
      heap.add(nodeIdx);
    } else {
      heap.relocate(nodeIdx);
    }
  }

  int iterations = 0;
  while (!heap.empty() && iterations < params.maxIterations) {
    if ((iterations & 63) == 0 && cancelFlag.load(std::memory_order_relaxed)) {
      return std::nullopt;
    }

    iterations++;

    const int currIdx = heap.poll();
    if (currIdx < 0) break;

    const Int3 curr{nodeX[static_cast<size_t>(currIdx)], nodeY[static_cast<size_t>(currIdx)], nodeZ[static_cast<size_t>(currIdx)]};

    if (runtime.isAtGoal(curr.x, curr.y, curr.z)) {
      std::vector<Int3> path;
      int walk = currIdx;
      while (walk != -1) {
        path.push_back(Int3{nodeX[static_cast<size_t>(walk)], nodeY[static_cast<size_t>(walk)], nodeZ[static_cast<size_t>(walk)]});
        walk = nodeParent[static_cast<size_t>(walk)];
      }
      std::reverse(path.begin(), path.end());
      auto keyPoints = extractKeyPoints(world, path, params.isFly);
      auto pathFlags = encodeNodeFlags(world, path, params.isFly, false);
      auto keyNodeFlags = encodeNodeFlags(world, keyPoints, params.isFly, true);
      auto keyNodeMetrics = encodeKeyMetrics(world, keyPoints);
      auto signatureHex = buildPathSignatureHex(path);

      const auto elapsedNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now() - startTime
      ).count();

      SearchResult result;
      result.points = std::move(path);
      result.keyPoints = std::move(keyPoints);
      result.pathFlags = std::move(pathFlags);
      result.keyNodeFlags = std::move(keyNodeFlags);
      result.keyNodeMetrics = std::move(keyNodeMetrics);
      result.signatureHex = std::move(signatureHex);
      result.timeMs = elapsedNs / 1000000LL;
      result.nodesExplored = iterations;
      result.nanosecondsPerNode = iterations > 0
        ? static_cast<double>(elapsedNs) / static_cast<double>(iterations)
        : 0.0;
      result.selectedStartIndex = nodeStartIndex[static_cast<size_t>(currIdx)];
      return result;
    }

    const float currCost = nodeG[static_cast<size_t>(currIdx)];

    const int currStartIdx = nodeStartIndex[static_cast<size_t>(currIdx)];

    if (isFly) {
      const float currFlyProgress = runtime.flyHorizontalProgress(curr.x, curr.y, curr.z);
      for (const auto& move : detail::FLY_MOVES) {
        detail::MoveOut out;
        if (!runtime.flyMove(curr, move, currFlyProgress, out)) continue;
        if (out.cost >= ActionCosts::INF_COST) continue;

        const auto [nIdx, inserted] = getOrCreateNode(out.pos.x, out.pos.y, out.pos.z);
        const float newCost = currCost + out.cost + nodeAvoidPenalty[static_cast<size_t>(nIdx)];
        if (inserted) {
          nodeParent[static_cast<size_t>(nIdx)] = currIdx;
          nodeStartIndex[static_cast<size_t>(nIdx)] = currStartIdx;
          setNodeCost(nIdx, newCost);
          heap.add(nIdx);
        } else {
          if (newCost < nodeG[static_cast<size_t>(nIdx)]) {
            nodeParent[static_cast<size_t>(nIdx)] = currIdx;
            nodeStartIndex[static_cast<size_t>(nIdx)] = currStartIdx;
            setNodeCost(nIdx, newCost);

            if (nodeHeapPos[static_cast<size_t>(nIdx)] != -1) {
              heap.relocate(nIdx);
            } else {
              heap.add(nIdx);
            }
          }
        }
      }
      continue;
    }

    for (const auto& move : walkMovesOrdered) {
      detail::MoveOut out;
      if (!runtime.walkMove(curr, move, out)) continue;
      if (out.cost >= ActionCosts::INF_COST) continue;

      const auto [nIdx, inserted] = getOrCreateNode(out.pos.x, out.pos.y, out.pos.z);
      const float newCost = currCost + out.cost + nodeAvoidPenalty[static_cast<size_t>(nIdx)];
      if (inserted) {
        nodeParent[static_cast<size_t>(nIdx)] = currIdx;
        nodeStartIndex[static_cast<size_t>(nIdx)] = currStartIdx;
        setNodeCost(nIdx, newCost);
        heap.add(nIdx);
      } else {
        if (newCost < nodeG[static_cast<size_t>(nIdx)]) {
          nodeParent[static_cast<size_t>(nIdx)] = currIdx;
          nodeStartIndex[static_cast<size_t>(nIdx)] = currStartIdx;
          setNodeCost(nIdx, newCost);

          if (nodeHeapPos[static_cast<size_t>(nIdx)] != -1) {
            heap.relocate(nIdx);
          } else {
            heap.add(nIdx);
          }
        }
      }
    }
  }

  return std::nullopt;
}

} // namespace v5pf
