#include <jni.h>

#include "etherwarp_search.hpp"
#include "pathfinder.hpp"

#include <atomic>
#include <cstdint>
#include <exception>
#include <string>
#include <vector>

namespace {

v5pf::WorldState g_worldState;
std::atomic_bool g_cancelSearch{false};

std::vector<int> toIntVector(JNIEnv* env, jintArray array) {
  if (array == nullptr) return {};

  const jsize len = env->GetArrayLength(array);
  if (len <= 0) return {};

  std::vector<int> out(static_cast<size_t>(len));
  env->GetIntArrayRegion(array, 0, len, reinterpret_cast<jint*>(out.data()));
  return out;
}

std::vector<double> toDoubleVector(JNIEnv* env, jdoubleArray array) {
  if (array == nullptr) return {};

  const jsize len = env->GetArrayLength(array);
  if (len <= 0) return {};

  std::vector<double> out(static_cast<size_t>(len));
  env->GetDoubleArrayRegion(array, 0, len, reinterpret_cast<jdouble*>(out.data()));
  return out;
}

std::vector<jlong> toLongVector(JNIEnv* env, jlongArray array) {
  if (array == nullptr) return {};

  const jsize len = env->GetArrayLength(array);
  if (len <= 0) return {};

  std::vector<jlong> out(static_cast<size_t>(len));
  env->GetLongArrayRegion(array, 0, len, out.data());
  return out;
}

std::vector<uint16_t> toShortVector(JNIEnv* env, jshortArray array) {
  if (array == nullptr) return {};

  const jsize len = env->GetArrayLength(array);
  if (len <= 0) return {};

  std::vector<uint16_t> out(static_cast<size_t>(len));
  env->GetShortArrayRegion(array, 0, len, reinterpret_cast<jshort*>(out.data()));
  return out;
}

class ScopedUtfChars {
 public:
  ScopedUtfChars(JNIEnv* env, jstring value)
    : env_(env), value_(value) {
    if (env_ != nullptr && value_ != nullptr) {
      chars_ = env_->GetStringUTFChars(value_, nullptr);
    }
  }

  ScopedUtfChars(const ScopedUtfChars&) = delete;
  ScopedUtfChars& operator=(const ScopedUtfChars&) = delete;

  ~ScopedUtfChars() {
    if (env_ != nullptr && value_ != nullptr && chars_ != nullptr) {
      env_->ReleaseStringUTFChars(value_, chars_);
    }
  }

  [[nodiscard]] const char* get() const {
    return chars_;
  }

 private:
  JNIEnv* env_ = nullptr;
  jstring value_ = nullptr;
  const char* chars_ = nullptr;
};

bool hasPendingJavaException(JNIEnv* env) {
  return env != nullptr && env->ExceptionCheck() == JNI_TRUE;
}

void throwJavaException(JNIEnv* env, const char* className, const std::string& message) {
  if (env == nullptr || hasPendingJavaException(env)) {
    return;
  }

  jclass exceptionClass = env->FindClass(className);
  if (exceptionClass == nullptr) {
    env->ExceptionClear();
    exceptionClass = env->FindClass("java/lang/RuntimeException");
    if (exceptionClass == nullptr) {
      return;
    }
  }

  env->ThrowNew(exceptionClass, message.c_str());
}

constexpr int kMinAllowedWorldY = -2048;
constexpr int kMaxAllowedWorldY = 2048;
constexpr int64_t kMaxWorldSpanBlocks = 4096;

bool isValidHeightRange(const jint minY, const jint maxY) {
  return minY < maxY &&
    minY >= kMinAllowedWorldY &&
    maxY <= kMaxAllowedWorldY &&
    static_cast<int64_t>(maxY) - static_cast<int64_t>(minY) <= kMaxWorldSpanBlocks;
}

std::string buildInvalidHeightMessage(const jint minY, const jint maxY) {
  return "Invalid minY/maxY: minY=" + std::to_string(minY) +
    ", maxY=" + std::to_string(maxY) +
    ", required minY < maxY, minY >= " + std::to_string(kMinAllowedWorldY) +
    ", maxY <= " + std::to_string(kMaxAllowedWorldY) +
    ", and span <= " + std::to_string(kMaxWorldSpanBlocks);
}

void throwRuntimeFromException(JNIEnv* env, const char* entrypoint, const std::exception& ex) {
  throwJavaException(
    env,
    "java/lang/RuntimeException",
    std::string(entrypoint) + " failed with native exception: " + ex.what()
  );
}

void throwRuntimeUnknown(JNIEnv* env, const char* entrypoint) {
  throwJavaException(
    env,
    "java/lang/RuntimeException",
    std::string(entrypoint) + " failed with unknown native exception"
  );
}

std::vector<v5pf::Int3> parsePoints(const std::vector<int>& flat) {
  std::vector<v5pf::Int3> out;
  if (flat.empty() || flat.size() % 3 != 0) return out;

  out.reserve(flat.size() / 3);
  for (size_t i = 0; i + 2 < flat.size(); i += 3) {
    out.push_back(v5pf::Int3{flat[i], flat[i + 1], flat[i + 2]});
  }

  return out;
}

std::vector<jint> packPoints(const std::vector<v5pf::Int3>& points) {
  std::vector<jint> packed;
  packed.reserve(points.size() * 3);
  for (const auto& p : points) {
    packed.push_back(static_cast<jint>(p.x));
    packed.push_back(static_cast<jint>(p.y));
    packed.push_back(static_cast<jint>(p.z));
  }
  return packed;
}

v5pf::ChunkUpdate makeChunkUpdate(
  const int chunkX,
  const int chunkZ,
  const int minY,
  const int maxY,
  const uint64_t sectionMask,
  const std::vector<uint16_t>& sectionFlags
) {
  v5pf::ChunkUpdate update;
  update.chunkX = chunkX;
  update.chunkZ = chunkZ;
  update.chunk.minY = minY;
  update.chunk.maxY = maxY;
  update.chunk.ensureLayout();

  size_t readOffset = 0;
  const int maskedSectionCount = std::min(update.chunk.sectionCount(), 64);
  for (int i = 0; i < maskedSectionCount; i++) {
    if ((sectionMask & (1ULL << i)) == 0ULL) continue;
    if (readOffset + 4096 > sectionFlags.size()) break;

    update.chunk.assignSection(i, sectionFlags.data() + readOffset);
    readOffset += 4096;
  }

  return update;
}

} // namespace

extern "C" {

JNIEXPORT jboolean JNICALL Java_com_chattriggers_ctjs_api_world_pathfinding_NativePathfinderJNI_initNative(JNIEnv* env, jclass) {
  try {
    return JNI_TRUE;
  } catch (const std::exception& ex) {
    throwRuntimeFromException(env, "initNative", ex);
    return JNI_FALSE;
  } catch (...) {
    throwRuntimeUnknown(env, "initNative");
    return JNI_FALSE;
  }
}

JNIEXPORT void JNICALL Java_com_chattriggers_ctjs_api_world_pathfinding_NativePathfinderJNI_setWorld(
  JNIEnv* env,
  jclass,
  jstring worldKey,
  jint minY,
  jint maxY
) {
  try {
    if (!isValidHeightRange(minY, maxY)) {
      throwJavaException(env, "java/lang/IllegalArgumentException", buildInvalidHeightMessage(minY, maxY));
      return;
    }

    const ScopedUtfChars keyChars(env, worldKey);
    if (hasPendingJavaException(env)) {
      return;
    }

    std::string key = keyChars.get() != nullptr ? keyChars.get() : "runtime_memory";

    g_worldState.setWorld(std::move(key), static_cast<int>(minY), static_cast<int>(maxY));
  } catch (const std::exception& ex) {
    throwRuntimeFromException(env, "setWorld", ex);
  } catch (...) {
    throwRuntimeUnknown(env, "setWorld");
  }
}

JNIEXPORT void JNICALL Java_com_chattriggers_ctjs_api_world_pathfinding_NativePathfinderJNI_clearWorld(JNIEnv* env, jclass) {
  try {
    g_worldState.clear();
  } catch (const std::exception& ex) {
    throwRuntimeFromException(env, "clearWorld", ex);
  } catch (...) {
    throwRuntimeUnknown(env, "clearWorld");
  }
}

JNIEXPORT void JNICALL Java_com_chattriggers_ctjs_api_world_pathfinding_NativePathfinderJNI_upsertChunk(
  JNIEnv* env,
  jclass,
  jint chunkX,
  jint chunkZ,
  jint minY,
  jint maxY,
  jlong sectionMask,
  jshortArray sectionFlags
) {
  try {
    if (!isValidHeightRange(minY, maxY)) {
      throwJavaException(env, "java/lang/IllegalArgumentException", buildInvalidHeightMessage(minY, maxY));
      return;
    }

    const auto flags = toShortVector(env, sectionFlags);
    if (hasPendingJavaException(env)) {
      return;
    }

    std::vector<v5pf::ChunkUpdate> updates;
    updates.push_back(makeChunkUpdate(
      static_cast<int>(chunkX),
      static_cast<int>(chunkZ),
      static_cast<int>(minY),
      static_cast<int>(maxY),
      static_cast<uint64_t>(sectionMask),
      flags
    ));
    g_worldState.upsertChunks(std::move(updates));
  } catch (const std::exception& ex) {
    throwRuntimeFromException(env, "upsertChunk", ex);
  } catch (...) {
    throwRuntimeUnknown(env, "upsertChunk");
  }
}

JNIEXPORT void JNICALL Java_com_chattriggers_ctjs_api_world_pathfinding_NativePathfinderJNI_upsertChunks(
  JNIEnv* env,
  jclass,
  jintArray metadata,
  jlongArray sectionMasks,
  jobjectArray sectionFlags
) {
  try {
    const auto flatMetadata = toIntVector(env, metadata);
    const auto masks = toLongVector(env, sectionMasks);
    if (hasPendingJavaException(env)) {
      return;
    }

    const size_t chunkCount = masks.size();
    if (sectionFlags == nullptr || flatMetadata.size() != chunkCount * 4 ||
        static_cast<size_t>(env->GetArrayLength(sectionFlags)) != chunkCount) {
      throwJavaException(env, "java/lang/IllegalArgumentException", "Invalid chunk batch arrays");
      return;
    }

    std::vector<v5pf::ChunkUpdate> updates;
    updates.reserve(chunkCount);
    for (size_t i = 0; i < chunkCount; i++) {
      const size_t offset = i * 4;
      const jint minY = flatMetadata[offset + 2];
      const jint maxY = flatMetadata[offset + 3];
      if (!isValidHeightRange(minY, maxY)) {
        throwJavaException(env, "java/lang/IllegalArgumentException", buildInvalidHeightMessage(minY, maxY));
        return;
      }

      auto flagsArray = static_cast<jshortArray>(env->GetObjectArrayElement(sectionFlags, static_cast<jsize>(i)));
      if (hasPendingJavaException(env)) {
        return;
      }
      const auto flags = toShortVector(env, flagsArray);
      if (flagsArray != nullptr) {
        env->DeleteLocalRef(flagsArray);
      }
      if (hasPendingJavaException(env)) {
        return;
      }

      updates.push_back(makeChunkUpdate(
        flatMetadata[offset],
        flatMetadata[offset + 1],
        minY,
        maxY,
        static_cast<uint64_t>(masks[i]),
        flags
      ));
    }

    g_worldState.upsertChunks(std::move(updates));
  } catch (const std::exception& ex) {
    throwRuntimeFromException(env, "upsertChunks", ex);
  } catch (...) {
    throwRuntimeUnknown(env, "upsertChunks");
  }
}

JNIEXPORT void JNICALL Java_com_chattriggers_ctjs_api_world_pathfinding_NativePathfinderJNI_removeChunks(
  JNIEnv* env,
  jclass,
  jlongArray chunkKeys
) {
  try {
    const auto keys = toLongVector(env, chunkKeys);
    if (hasPendingJavaException(env)) return;

    std::vector<uint64_t> parsed;
    parsed.reserve(keys.size());
    for (const jlong key : keys) {
      parsed.push_back(static_cast<uint64_t>(key));
    }
    g_worldState.removeChunks(parsed);
  } catch (const std::exception& ex) {
    throwRuntimeFromException(env, "removeChunks", ex);
  } catch (...) {
    throwRuntimeUnknown(env, "removeChunks");
  }
}

JNIEXPORT void JNICALL Java_com_chattriggers_ctjs_api_world_pathfinding_NativePathfinderJNI_applyBlockUpdates(
  JNIEnv* env,
  jclass,
  jintArray updates
) {
  try {
    const auto flat = toIntVector(env, updates);
    if (hasPendingJavaException(env)) {
      return;
    }
    if (flat.empty() || flat.size() % 4 != 0) {
      return;
    }

    std::vector<v5pf::BlockUpdate> parsed;
    parsed.reserve(flat.size() / 4);

    for (size_t i = 0; i + 3 < flat.size(); i += 4) {
      parsed.push_back(v5pf::BlockUpdate{
        flat[i],
        flat[i + 1],
        flat[i + 2],
        static_cast<uint16_t>(flat[i + 3])
      });
    }

    g_worldState.applyUpdates(parsed);
  } catch (const std::exception& ex) {
    throwRuntimeFromException(env, "applyBlockUpdates", ex);
  } catch (...) {
    throwRuntimeUnknown(env, "applyBlockUpdates");
  }
}

JNIEXPORT jobject JNICALL Java_com_chattriggers_ctjs_api_world_pathfinding_NativePathfinderJNI_findPath(
  JNIEnv* env,
  jclass,
  jintArray startPoints,
  jintArray endPoints,
  jboolean isFly,
  jint maxIterations,
  jdouble heuristicWeight,
  jdouble nonPrimaryStartPenalty,
  jint moveOrderOffset,
  jintArray avoidMeta,
  jdoubleArray avoidPenalty
) {
  try {
    const auto startFlat = toIntVector(env, startPoints);
    const auto endFlat = toIntVector(env, endPoints);
    if (hasPendingJavaException(env)) {
      return nullptr;
    }

    const auto starts = parsePoints(startFlat);
    const auto goals = parsePoints(endFlat);

    if (starts.empty() || goals.empty()) {
      return nullptr;
    }

    const auto avoidMetaFlat = toIntVector(env, avoidMeta);
    const auto avoidPenaltyFlat = toDoubleVector(env, avoidPenalty);
    if (hasPendingJavaException(env)) {
      return nullptr;
    }

    std::vector<v5pf::AvoidZone> avoidZones;
    if (!avoidMetaFlat.empty() && avoidMetaFlat.size() % 5 == 0) {
      const size_t count = avoidMetaFlat.size() / 5;
      if (avoidPenaltyFlat.size() == count) {
        avoidZones.reserve(count);
        for (size_t i = 0; i < count; i++) {
          const size_t base = i * 5;
          avoidZones.push_back(v5pf::AvoidZone{
            avoidMetaFlat[base],
            avoidMetaFlat[base + 1],
            avoidMetaFlat[base + 2],
            avoidMetaFlat[base + 3],
            avoidMetaFlat[base + 4],
            static_cast<float>(avoidPenaltyFlat[i]),
          });
        }
      }
    }

    v5pf::SearchParams params;
    params.starts = starts;
    params.goals = goals;
    params.isFly = isFly == JNI_TRUE;
    params.maxIterations = static_cast<int>(maxIterations);
    params.heuristicWeight = static_cast<float>(heuristicWeight);
    params.nonPrimaryStartPenalty = static_cast<float>(nonPrimaryStartPenalty);
    params.moveOrderOffset = static_cast<int>(moveOrderOffset);
    params.avoidZones = std::move(avoidZones);

    g_cancelSearch.store(false);
    const auto worldSnapshot = g_worldState.snapshot();
    auto result = v5pf::findPath(worldSnapshot, params, g_cancelSearch);

    if (!result.has_value() || result->points.empty()) {
      return nullptr;
    }

    const auto packedPath = packPoints(result->points);
    const auto packedKeyPath = packPoints(result->keyPoints);
    const std::vector<jint> packedPathFlags(result->pathFlags.begin(), result->pathFlags.end());
    const std::vector<jint> packedKeyNodeFlags(result->keyNodeFlags.begin(), result->keyNodeFlags.end());
    const std::vector<jint> packedKeyNodeMetrics(result->keyNodeMetrics.begin(), result->keyNodeMetrics.end());

    jintArray pathArray = env->NewIntArray(static_cast<jsize>(packedPath.size()));
    if (pathArray == nullptr) {
      return nullptr;
    }
    env->SetIntArrayRegion(pathArray, 0, static_cast<jsize>(packedPath.size()), packedPath.data());

    jintArray keyPathArray = env->NewIntArray(static_cast<jsize>(packedKeyPath.size()));
    if (keyPathArray == nullptr) {
      return nullptr;
    }
    env->SetIntArrayRegion(keyPathArray, 0, static_cast<jsize>(packedKeyPath.size()), packedKeyPath.data());

    jintArray pathFlagsArray = env->NewIntArray(static_cast<jsize>(packedPathFlags.size()));
    if (pathFlagsArray == nullptr) {
      return nullptr;
    }
    if (!packedPathFlags.empty()) {
      env->SetIntArrayRegion(
        pathFlagsArray,
        0,
        static_cast<jsize>(packedPathFlags.size()),
        packedPathFlags.data()
      );
    }

    jintArray keyNodeFlagsArray = env->NewIntArray(static_cast<jsize>(packedKeyNodeFlags.size()));
    if (keyNodeFlagsArray == nullptr) {
      return nullptr;
    }
    if (!packedKeyNodeFlags.empty()) {
      env->SetIntArrayRegion(
        keyNodeFlagsArray,
        0,
        static_cast<jsize>(packedKeyNodeFlags.size()),
        packedKeyNodeFlags.data()
      );
    }

    jintArray keyNodeMetricsArray = env->NewIntArray(static_cast<jsize>(packedKeyNodeMetrics.size()));
    if (keyNodeMetricsArray == nullptr) {
      return nullptr;
    }
    if (!packedKeyNodeMetrics.empty()) {
      env->SetIntArrayRegion(
        keyNodeMetricsArray,
        0,
        static_cast<jsize>(packedKeyNodeMetrics.size()),
        packedKeyNodeMetrics.data()
      );
    }

    jstring signature = env->NewStringUTF(result->signatureHex.c_str());
    if (signature == nullptr) {
      return nullptr;
    }

    jclass resultClass = env->FindClass("com/chattriggers/ctjs/api/world/pathfinding/NativePathResult");
    if (resultClass == nullptr) {
      return nullptr;
    }

    jmethodID ctor = env->GetMethodID(
      resultClass,
      "<init>",
      "([I[IJIDI[I[I[ILjava/lang/String;)V"
    );
    if (ctor == nullptr) {
      return nullptr;
    }

    return env->NewObject(
      resultClass,
      ctor,
      pathArray,
      keyPathArray,
      static_cast<jlong>(result->timeMs),
      static_cast<jint>(result->nodesExplored),
      static_cast<jdouble>(result->nanosecondsPerNode),
      static_cast<jint>(result->selectedStartIndex),
      pathFlagsArray,
      keyNodeFlagsArray,
      keyNodeMetricsArray,
      signature
    );
  } catch (const std::exception& ex) {
    throwRuntimeFromException(env, "findPath", ex);
    return nullptr;
  } catch (...) {
    throwRuntimeUnknown(env, "findPath");
    return nullptr;
  }
}

JNIEXPORT jobject JNICALL Java_com_chattriggers_ctjs_api_world_pathfinding_NativePathfinderJNI_findEtherwarpPath(
  JNIEnv* env,
  jclass,
  jint goalX,
  jint goalY,
  jint goalZ,
  jdouble startEyeX,
  jdouble startEyeY,
  jdouble startEyeZ,
  jint maxIterations,
  jint threadCount,
  jdouble yawStep,
  jdouble pitchStep,
  jdouble newNodeCost,
  jdouble heuristicWeight,
  jdouble rayLength,
  jdouble rewireEpsilon,
  jdouble eyeHeight
) {
  try {
    v5pf::EtherwarpSearchParams params;
    params.goal = v5pf::Int3{
      static_cast<int>(goalX),
      static_cast<int>(goalY),
      static_cast<int>(goalZ),
    };
    params.startEyeX = static_cast<double>(startEyeX);
    params.startEyeY = static_cast<double>(startEyeY);
    params.startEyeZ = static_cast<double>(startEyeZ);
    params.maxIterations = static_cast<int>(maxIterations);
    params.threadCount = static_cast<int>(threadCount);
    params.yawStep = static_cast<double>(yawStep);
    params.pitchStep = static_cast<double>(pitchStep);
    params.newNodeCost = static_cast<double>(newNodeCost);
    params.heuristicWeight = static_cast<double>(heuristicWeight);
    params.rayLength = static_cast<double>(rayLength);
    params.rewireEpsilon = static_cast<double>(rewireEpsilon);
    params.eyeHeight = static_cast<double>(eyeHeight);

    g_cancelSearch.store(false);
    const auto worldSnapshot = g_worldState.snapshot();
    auto result = v5pf::findEtherwarpPath(worldSnapshot, params, g_cancelSearch);

    if (!result.has_value() || result->points.empty()) {
      return nullptr;
    }

    const auto packedPath = packPoints(result->points);
    const std::vector<jfloat> packedAngles(result->angles.begin(), result->angles.end());

    jintArray pathArray = env->NewIntArray(static_cast<jsize>(packedPath.size()));
    if (pathArray == nullptr) {
      return nullptr;
    }
    env->SetIntArrayRegion(pathArray, 0, static_cast<jsize>(packedPath.size()), packedPath.data());

    jfloatArray angleArray = env->NewFloatArray(static_cast<jsize>(packedAngles.size()));
    if (angleArray == nullptr) {
      return nullptr;
    }
    if (!packedAngles.empty()) {
      env->SetFloatArrayRegion(angleArray, 0, static_cast<jsize>(packedAngles.size()), packedAngles.data());
    }

    jclass resultClass = env->FindClass("com/chattriggers/ctjs/api/world/pathfinding/NativeEtherwarpResult");
    if (resultClass == nullptr) {
      return nullptr;
    }

    jmethodID ctor = env->GetMethodID(resultClass, "<init>", "([I[FJID)V");
    if (ctor == nullptr) {
      return nullptr;
    }

    return env->NewObject(
      resultClass,
      ctor,
      pathArray,
      angleArray,
      static_cast<jlong>(result->timeMs),
      static_cast<jint>(result->nodesExplored),
      static_cast<jdouble>(result->nanosecondsPerNode)
    );
  } catch (const std::exception& ex) {
    throwRuntimeFromException(env, "findEtherwarpPath", ex);
    return nullptr;
  } catch (...) {
    throwRuntimeUnknown(env, "findEtherwarpPath");
    return nullptr;
  }
}

JNIEXPORT void JNICALL Java_com_chattriggers_ctjs_api_world_pathfinding_NativePathfinderJNI_cancelSearch(JNIEnv* env, jclass) {
  try {
    g_cancelSearch.store(true);
  } catch (const std::exception& ex) {
    throwRuntimeFromException(env, "cancelSearch", ex);
  } catch (...) {
    throwRuntimeUnknown(env, "cancelSearch");
  }
}

} // extern "C"
