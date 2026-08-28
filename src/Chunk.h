#pragma once

#include "BiomeMap.h"
#include "BlockType.h"
#include <array>
#include <functional>
#include <glm/glm.hpp>
#include <vector>

constexpr int CHUNK_SIZE_XZ = 16;

constexpr int CHUNK_HEIGHT = 128;

constexpr int CHUNK_SIZE = CHUNK_SIZE_XZ;

class Chunk {
public:
  Chunk();

  BlockType getBlock(int x, int y, int z) const;
  void setBlock(int x, int y, int z, BlockType type);

  uint8_t getSkyLight(int x, int y, int z) const;
  void setSkyLight(int x, int y, int z, uint8_t value);
  glm::ivec3 getBlockLight(int x, int y, int z) const;
  void setBlockLight(int x, int y, int z, glm::ivec3 value);
  void clearBlockLight();

  void computeSkyLight();

  void generateFlatTerrain();

  void generateTerrain(int worldOffsetX, int worldOffsetZ,
                       const BiomeMap &biomes);

  void carveCaves(int worldOffsetX, int worldOffsetZ, const BiomeMap &biomes);

  BiomeType getColumnBiome(int x, int z) const;

  void buildMesh(
      std::vector<float> &outVertices, std::vector<unsigned int> &outIndices,
      std::vector<float> &outTransparentVertices,
      std::vector<unsigned int> &outTransparentIndices, int worldOffsetX = 0,
      int worldOffsetZ = 0,
      const std::function<BlockType(int, int, int)> &worldBlockLookup = nullptr,
      const std::function<uint8_t(int, int, int)> &worldSkyLightLookup =
          nullptr,
      const std::function<glm::ivec3(int, int, int)> &worldBlockLightLookup =
          nullptr) const;

  void buildGrassMesh(std::vector<float> &outVertices, const BiomeMap &biomes,
                      int worldOffsetX, int worldOffsetZ) const;

private:
  std::array<BlockType, CHUNK_SIZE_XZ * CHUNK_HEIGHT * CHUNK_SIZE_XZ> blocks;
  std::array<uint8_t, CHUNK_SIZE_XZ * CHUNK_HEIGHT * CHUNK_SIZE_XZ> skyLight;
  std::array<uint8_t, CHUNK_SIZE_XZ * CHUNK_HEIGHT * CHUNK_SIZE_XZ> blockLightR;
  std::array<uint8_t, CHUNK_SIZE_XZ * CHUNK_HEIGHT * CHUNK_SIZE_XZ> blockLightG;
  std::array<uint8_t, CHUNK_SIZE_XZ * CHUNK_HEIGHT * CHUNK_SIZE_XZ> blockLightB;

  std::array<BiomeType, CHUNK_SIZE_XZ * CHUNK_SIZE_XZ> columnBiome;

  std::array<int, CHUNK_SIZE_XZ * CHUNK_SIZE_XZ> surfaceHeightCache;

  static int index(int x, int y, int z);
  static int columnIndex(int x, int z);
  static bool inBounds(int x, int y, int z);
};
