#pragma once

#include "BiomeMap.h"
#include "Chunk.h"
#include <GL/glew.h>
#include <cstdint>
#include <deque>
#include <glm/glm.hpp>
#include <limits>
#include <set>
#include <tuple>
#include <unordered_map>
#include <vector>

constexpr int RENDER_DISTANCE = 5;
constexpr int UNLOAD_DISTANCE = RENDER_DISTANCE + 2;

constexpr int CHUNKS_LOADED_PER_FRAME = 1;

struct ChunkMesh {
  GLuint vao = 0, vbo = 0, ebo = 0;
  GLsizei indexCount = 0;
};

struct GrassMesh {
  GLuint vao = 0, vbo = 0;
  GLsizei vertexCount = 0;
};

inline int64_t chunkKey(int cx, int cz) {
  return (int64_t)cx << 32 | (uint32_t)cz;
}

class ChunkManager {
public:
  explicit ChunkManager(uint32_t seed = 1337) : biomes(seed), worldSeed(seed) {}

  void update(const glm::vec3 &playerWorldPos);

  void loadAllPending();

  BlockType getWorldBlock(int worldX, int worldY, int worldZ) const;

  uint8_t getWorldSkyLight(int worldX, int worldY, int worldZ) const;
  glm::ivec3 getWorldBlockLight(int worldX, int worldY, int worldZ) const;
  void setWorldBlockLight(int worldX, int worldY, int worldZ, glm::ivec3 value);

  const std::set<std::tuple<int, int, int>> &getEmissiveBlockPositions() const {
    return emissiveBlockPositions;
  }

  void setBlock(int worldX, int worldY, int worldZ, BlockType type);

  const std::unordered_map<int64_t, ChunkMesh> &opaqueMeshes() const {
    return chunkMeshesOpaque;
  }
  const std::unordered_map<int64_t, ChunkMesh> &transparentMeshes() const {
    return chunkMeshesTransparent;
  }
  const std::unordered_map<int64_t, GrassMesh> &grassMeshes() const {
    return chunkGrassMeshes;
  }
  size_t loadedChunkCount() const { return chunks.size(); }

  BiomeType getBiomeAt(float worldX, float worldZ) const {
    return biomes.classify(worldX, worldZ);
  }

  int estimateSurfaceHeight(float worldX, float worldZ) const {
    return biomes.surfaceHeight(worldX, worldZ);
  }

private:
  BiomeMap biomes;
  uint32_t worldSeed;
  std::unordered_map<int64_t, Chunk> chunks;
  std::unordered_map<int64_t, ChunkMesh> chunkMeshesOpaque;
  std::unordered_map<int64_t, ChunkMesh> chunkMeshesTransparent;
  std::unordered_map<int64_t, GrassMesh> chunkGrassMeshes;

  int lastPlayerChunkX = std::numeric_limits<int>::min();
  int lastPlayerChunkZ = std::numeric_limits<int>::min();

  std::deque<std::pair<int, int>> pendingChunkLoads;
  std::set<int64_t> pendingChunkKeys;

  void loadChunk(int cx, int cz);

  void unloadChunk(int cx, int cz);

  void uploadChunkMesh(int cx, int cz);

  void uploadGrassMesh(int cx, int cz);

  void recomputeBlockLight();

  void recomputeBlockLightIncremental(
      const std::vector<std::tuple<int, int, int>> &newEmissivePositions,
      int chunkCX, int chunkCZ);

  std::set<std::tuple<int, int, int>> emissiveBlockPositions;
};
