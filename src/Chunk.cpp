#include "Chunk.h"
#include "MeshUtils.h"
#include <algorithm>
#include <cmath>
#include <queue>
#include <utility>

static float smoothStaircase(float h, float step, float rampFraction) {
  if (step <= 0.0f)
    return h;
  float t = h / step;
  float base = std::floor(t);
  float frac = t - base;
  float rampStart = 0.5f - rampFraction;
  float rampEnd = 0.5f + rampFraction;
  float shaped;
  if (frac <= rampStart) {
    shaped = 0.0f;
  } else if (frac >= rampEnd) {
    shaped = 1.0f;
  } else {
    float u = (frac - rampStart) / (rampEnd - rampStart);
    shaped = u * u * (3.0f - 2.0f * u);
  }
  return (base + shaped) * step;
}

Chunk::Chunk() {
  blocks.fill(BlockType::Air);
  skyLight.fill(0);
  blockLightR.fill(0);
  blockLightG.fill(0);
  blockLightB.fill(0);
  columnBiome.fill(BiomeType::Plains);
  surfaceHeightCache.fill(0);
}

int Chunk::index(int x, int y, int z) {
  return x + y * CHUNK_SIZE_XZ + z * CHUNK_SIZE_XZ * CHUNK_HEIGHT;
}

int Chunk::columnIndex(int x, int z) { return x + z * CHUNK_SIZE_XZ; }

bool Chunk::inBounds(int x, int y, int z) {
  return x >= 0 && x < CHUNK_SIZE_XZ && y >= 0 && y < CHUNK_HEIGHT && z >= 0 &&
         z < CHUNK_SIZE_XZ;
}

BlockType Chunk::getBlock(int x, int y, int z) const {
  if (!inBounds(x, y, z))
    return BlockType::Air;
  return blocks[index(x, y, z)];
}

void Chunk::setBlock(int x, int y, int z, BlockType type) {
  if (!inBounds(x, y, z))
    return;
  blocks[index(x, y, z)] = type;
}

uint8_t Chunk::getSkyLight(int x, int y, int z) const {
  if (!inBounds(x, y, z))
    return 15;
  return skyLight[index(x, y, z)];
}

void Chunk::setSkyLight(int x, int y, int z, uint8_t value) {
  if (!inBounds(x, y, z))
    return;
  skyLight[index(x, y, z)] = value;
}

glm::ivec3 Chunk::getBlockLight(int x, int y, int z) const {
  if (!inBounds(x, y, z))
    return glm::ivec3(0);
  int i = index(x, y, z);
  return glm::ivec3(blockLightR[i], blockLightG[i], blockLightB[i]);
}

void Chunk::setBlockLight(int x, int y, int z, glm::ivec3 value) {
  if (!inBounds(x, y, z))
    return;
  int i = index(x, y, z);
  blockLightR[i] = (uint8_t)value.r;
  blockLightG[i] = (uint8_t)value.g;
  blockLightB[i] = (uint8_t)value.b;
}

void Chunk::clearBlockLight() {
  blockLightR.fill(0);
  blockLightG.fill(0);
  blockLightB.fill(0);
}

BiomeType Chunk::getColumnBiome(int x, int z) const {
  if (x < 0 || x >= CHUNK_SIZE_XZ || z < 0 || z >= CHUNK_SIZE_XZ)
    return BiomeType::Plains;
  return columnBiome[columnIndex(x, z)];
}

void Chunk::computeSkyLight() {
  skyLight.fill(0);

  struct Cell {
    int x, y, z;
  };
  std::queue<Cell> queue;

  for (int x = 0; x < CHUNK_SIZE_XZ; ++x) {
    for (int z = 0; z < CHUNK_SIZE_XZ; ++z) {
      for (int y = CHUNK_HEIGHT - 1; y >= 0; --y) {
        BlockType t = getBlock(x, y, z);
        if (t == BlockType::Air || isTransparent(t)) {
          setSkyLight(x, y, z, 15);
          queue.push({x, y, z});
        } else {
          break;
        }
      }
    }
  }

  static const int offsets[6][3] = {
      {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1},
  };

  while (!queue.empty()) {
    Cell c = queue.front();
    queue.pop();
    uint8_t level = getSkyLight(c.x, c.y, c.z);
    if (level == 0)
      continue;

    for (const auto &off : offsets) {
      int nx = c.x + off[0], ny = c.y + off[1], nz = c.z + off[2];
      if (!inBounds(nx, ny, nz))
        continue;
      BlockType nt = getBlock(nx, ny, nz);
      if (!(nt == BlockType::Air || isTransparent(nt)))
        continue;

      uint8_t newLevel = (uint8_t)(level - 1);
      if (newLevel > getSkyLight(nx, ny, nz)) {
        setSkyLight(nx, ny, nz, newLevel);
        queue.push({nx, ny, nz});
      }
    }
  }
}

void Chunk::generateFlatTerrain() {
  for (int x = 0; x < CHUNK_SIZE_XZ; ++x) {
    for (int z = 0; z < CHUNK_SIZE_XZ; ++z) {
      for (int y = 0; y < 16; ++y) {
        BlockType type;
        if (y < 4)
          type = BlockType::Stone;
        else if (y < 7)
          type = BlockType::Dirt;
        else if (y == 7)
          type = BlockType::Grass;
        else
          type = BlockType::Air;
        setBlock(x, y, z, type);
      }
    }
  }
}

void Chunk::generateTerrain(int worldOffsetX, int worldOffsetZ,
                            const BiomeMap &biomes) {

  constexpr int SAND_LEVEL = 5;

  for (int x = 0; x < CHUNK_SIZE_XZ; ++x) {
    for (int z = 0; z < CHUNK_SIZE_XZ; ++z) {
      float worldX = (float)(worldOffsetX + x);
      float worldZ = (float)(worldOffsetZ + z);

      BiomeType biome = biomes.classify(worldX, worldZ);
      columnBiome[columnIndex(x, z)] = biome;

      const BiomeTerrainParams &tp = biomes.terrainParams(biome);
      const BiomeSurfaceParams &sp = biomes.surfaceParams(biome);

      float n = biomes.heightNoise().octaveNoise(worldX * tp.noiseScale,
                                                 worldZ * tp.noiseScale,
                                                 tp.octaves, tp.persistence);

      float rawHeight;
      if (tp.reliefSensitivity > 0.0f) {
        float reliefMask = biomes.terrainRelief(worldX, worldZ);
        BiomeMap::ZoneWeights zw = biomes.zoneWeights(reliefMask);

        constexpr float valleyBase = 16.0f, plainBase = 20.0f;
        constexpr float valleyAmp = 2.5f, plainAmp = 6.0f,
                        mountainRollingAmp = 14.0f;
        constexpr float valleyStep = 2.0f, plainStep = 3.0f,
                        mountainStep = 8.0f;

        float base = valleyBase * zw.valley + plainBase * zw.plain +
                     tp.baseHeight * zw.mountain;
        float amp = valleyAmp * zw.valley + plainAmp * zw.plain +
                    mountainRollingAmp * zw.mountain;
        float stepSize = valleyStep * zw.valley + plainStep * zw.plain +
                         mountainStep * zw.mountain;

        float coarseN = biomes.heightNoise().octaveNoise(
            worldX * 0.012f, worldZ * 0.012f, 3, 0.5f);
        float coarseHeight = base + coarseN * amp;

        if (zw.mountain > 0.05f) {
          float domeHeight = biomes.mountainPeakHeight(worldX, worldZ);
          if (domeHeight > 0.0f) {
            coarseHeight += domeHeight * zw.mountain;
          }
        }

        rawHeight = smoothStaircase(coarseHeight, stepSize, 0.28f);

        rawHeight += n * amp * 0.15f;
      } else {
        rawHeight = tp.baseHeight + n * tp.amplitude;
      }

      int surfaceY = (int)std::lround(rawHeight);
      surfaceY = std::clamp(surfaceY, 1, CHUNK_HEIGHT - 1);
      surfaceHeightCache[columnIndex(x, z)] = surfaceY;

      int terrainFillHeight = std::min(surfaceY + 2, CHUNK_HEIGHT);

      bool beach = surfaceY <= SAND_LEVEL;
      BlockType surfaceBlock = beach ? BlockType::Sand : sp.surfaceBlock;
      BlockType subsurfaceBlock = beach ? BlockType::Sand : sp.subsurfaceBlock;
      int subsurfaceDepthForColumn = tp.subsurfaceDepth;
      if (!beach && biome == BiomeType::Mountains) {
        int relativeHeight = surfaceY - tp.baseHeight;
        if (relativeHeight < 22) {

          float scree = biomes.screePatch(worldX, worldZ);
          if (scree > 0.65f) {
            surfaceBlock = BlockType::Gravel;
            subsurfaceBlock = BlockType::Gravel;
          } else {
            surfaceBlock = BlockType::Grass;
            subsurfaceBlock = BlockType::Dirt;
          }

          subsurfaceDepthForColumn = 6;
        } else if (relativeHeight > 40) {
          surfaceBlock = BlockType::Ice;
          subsurfaceBlock = BlockType::Dirt;
        } else {
          subsurfaceBlock = BlockType::Dirt;
        }
      }

      for (int y = 0; y < terrainFillHeight; ++y) {
        BlockType type;
        if (y > surfaceY) {
          type = BlockType::Air;
        } else if (y == surfaceY) {
          type = surfaceBlock;
        } else if (y >= surfaceY - subsurfaceDepthForColumn) {
          type = subsurfaceBlock;
        } else {
          type = BlockType::Stone;
        }
        setBlock(x, y, z, type);
      }
    }
  }
}

void Chunk::carveCaves(int worldOffsetX, int worldOffsetZ,
                       const BiomeMap &biomes) {
  constexpr int SURFACE_MARGIN = 6;
  constexpr int ENTRANCE_SURFACE_MARGIN = 1;
  constexpr int CAVE_FLOOR = 4;

  constexpr int MAX_CAVE_SPAN = 45;
  constexpr float CAVE_THRESHOLD = 0.065f;
  constexpr float MAGMA_THRESHOLD = 0.75f;
  constexpr int MAGMA_BAND = 6;
  constexpr float GRAVEL_FLOOR_CHANCE = 0.6f;

  for (int x = 0; x < CHUNK_SIZE_XZ; ++x) {
    for (int z = 0; z < CHUNK_SIZE_XZ; ++z) {
      BiomeType biome = columnBiome[columnIndex(x, z)];
      const BiomeTerrainParams &tp = biomes.terrainParams(biome);
      if (!tp.hasCaves)
        continue;

      int surfaceY = surfaceHeightCache[columnIndex(x, z)];

      float worldX = (float)(worldOffsetX + x);
      float worldZ = (float)(worldOffsetZ + z);

      int surfaceMargin = SURFACE_MARGIN;
      if (biomes.terrainRelief(worldX, worldZ) > 0.45f &&
          biomes.caveEntranceMask(worldX, worldZ) > 0.7f) {
        surfaceMargin = ENTRANCE_SURFACE_MARGIN;
      }

      int caveTop = surfaceY - surfaceMargin;
      if (caveTop < CAVE_FLOOR)
        continue; // this column's terrain is too thin for a cave at all
      int caveBottom = std::max(CAVE_FLOOR, caveTop - MAX_CAVE_SPAN);

      for (int y = caveBottom; y <= caveTop; ++y) {
        if (getBlock(x, y, z) != BlockType::Stone)
          continue;

        float n = biomes.heightNoise().noise3D(
            worldX * 0.045f, (float)y * 0.06f, worldZ * 0.045f);
        if (std::abs(n) >= CAVE_THRESHOLD)
          continue;

        if (y <= caveBottom + MAGMA_BAND) {
          float magmaRoll = biomes.heightNoise().noise2D(
              worldX * 0.2f + 4242.0f, worldZ * 0.2f - 4242.0f);
          if (magmaRoll > MAGMA_THRESHOLD) {
            setBlock(x, y, z, BlockType::Magma);
            continue;
          }
        }

        setBlock(x, y, z, BlockType::Air);

        if (y > caveBottom && getBlock(x, y - 1, z) == BlockType::Stone) {
          float gravelRoll = biomes.heightNoise().noise2D(
              worldX * 0.3f - 1234.0f, (worldZ + (float)y) * 0.3f + 1234.0f);
          if (gravelRoll > GRAVEL_FLOOR_CHANCE) {
            setBlock(x, y - 1, z, BlockType::Gravel);
          }
        }
      }
    }
  }
}

void Chunk::buildMesh(
    std::vector<float> &outVertices, std::vector<unsigned int> &outIndices,
    std::vector<float> &outTransparentVertices,
    std::vector<unsigned int> &outTransparentIndices, int worldOffsetX,
    int worldOffsetZ,
    const std::function<BlockType(int, int, int)> &worldBlockLookup,
    const std::function<uint8_t(int, int, int)> &worldSkyLightLookup,
    const std::function<glm::ivec3(int, int, int)> &worldBlockLightLookup)
    const {
  outVertices.clear();
  outIndices.clear();
  outTransparentVertices.clear();
  outTransparentIndices.clear();

  auto queryLight = [&](int lx, int ly,
                        int lz) -> std::pair<uint8_t, glm::ivec3> {
    if (inBounds(lx, ly, lz)) {
      return {getSkyLight(lx, ly, lz), getBlockLight(lx, ly, lz)};
    } else if (worldSkyLightLookup && worldBlockLightLookup) {
      return {worldSkyLightLookup(worldOffsetX + lx, ly, worldOffsetZ + lz),
              worldBlockLightLookup(worldOffsetX + lx, ly, worldOffsetZ + lz)};
    } else {
      return {15, glm::ivec3(0)};
    }
  };

  struct FaceLightInfo {
    int normalAxis, tangent1Axis, tangent2Axis;
    int cornerT1[4], cornerT2[4];
  };
  static const FaceLightInfo faceLightInfo[6] = {
      {2, 0, 1, {0, 1, 1, 0}, {0, 0, 1, 1}}, // PosZ
      {2, 0, 1, {1, 0, 0, 1}, {0, 0, 1, 1}}, // NegZ
      {0, 1, 2, {0, 0, 1, 1}, {0, 1, 1, 0}}, // NegX
      {0, 1, 2, {0, 0, 1, 1}, {1, 0, 0, 1}}, // PosX
      {1, 0, 2, {0, 1, 1, 0}, {1, 1, 0, 0}}, // PosY
      {1, 0, 2, {0, 1, 1, 0}, {0, 0, 1, 1}}, // NegY
  };

  static const int faceOffsets[6][3] = {
      {0, 0, 1},  // PosZ
      {0, 0, -1}, // NegZ
      {-1, 0, 0}, // NegX
      {1, 0, 0},  // PosX
      {0, 1, 0},  // PosY
      {0, -1, 0}, // NegY
  };

  for (int x = 0; x < CHUNK_SIZE_XZ; ++x) {
    for (int y = 0; y < CHUNK_HEIGHT; ++y) {
      for (int z = 0; z < CHUNK_SIZE_XZ; ++z) {
        BlockType type = getBlock(x, y, z);
        if (type == BlockType::Air)
          continue;

        float fx = (float)(x + worldOffsetX);
        float fy = (float)y;
        float fz = (float)(z + worldOffsetZ);

        for (int f = 0; f < 6; ++f) {
          FaceDir dir = (FaceDir)f;
          int nx = x + faceOffsets[f][0];
          int ny = y + faceOffsets[f][1];
          int nz = z + faceOffsets[f][2];

          BlockType neighbor;
          if (inBounds(nx, ny, nz)) {
            neighbor = getBlock(nx, ny, nz);
          } else if (worldBlockLookup) {
            neighbor =
                worldBlockLookup(worldOffsetX + nx, ny, worldOffsetZ + nz);
          } else {
            neighbor = BlockType::Air;
          }
          bool emitFace = (neighbor == BlockType::Air) ||
                          (isTransparent(neighbor) &&
                           (!isTransparent(type) || neighbor != type));
          if (!emitFace)
            continue;

          glm::vec3 corners[4];
          switch (dir) {
          case FaceDir::PosZ:
            corners[0] = {fx, fy, fz + 1};
            corners[1] = {fx + 1, fy, fz + 1};
            corners[2] = {fx + 1, fy + 1, fz + 1};
            corners[3] = {fx, fy + 1, fz + 1};
            break;
          case FaceDir::NegZ:
            corners[0] = {fx + 1, fy, fz};
            corners[1] = {fx, fy, fz};
            corners[2] = {fx, fy + 1, fz};
            corners[3] = {fx + 1, fy + 1, fz};
            break;
          case FaceDir::NegX:
            corners[0] = {fx, fy, fz};
            corners[1] = {fx, fy, fz + 1};
            corners[2] = {fx, fy + 1, fz + 1};
            corners[3] = {fx, fy + 1, fz};
            break;
          case FaceDir::PosX:
            corners[0] = {fx + 1, fy, fz + 1};
            corners[1] = {fx + 1, fy, fz};
            corners[2] = {fx + 1, fy + 1, fz};
            corners[3] = {fx + 1, fy + 1, fz + 1};
            break;
          case FaceDir::PosY:
            corners[0] = {fx, fy + 1, fz + 1};
            corners[1] = {fx + 1, fy + 1, fz + 1};
            corners[2] = {fx + 1, fy + 1, fz};
            corners[3] = {fx, fy + 1, fz};
            break;
          case FaceDir::NegY:
            corners[0] = {fx, fy, fz};
            corners[1] = {fx + 1, fy, fz};
            corners[2] = {fx + 1, fy, fz + 1};
            corners[3] = {fx, fy, fz + 1};
            break;
          }

          int tile = tileForBlockFace(type, dir);
          glm::vec3 normal((float)faceOffsets[f][0], (float)faceOffsets[f][1],
                           (float)faceOffsets[f][2]);

          const FaceLightInfo &info = faceLightInfo[f];
          int blockCoord[3] = {x, y, z};
          int depthCoord[3] = {nx, ny, nz};
          float skyLight01[4];
          glm::vec3 blockLight01[4];
          for (int ci = 0; ci < 4; ++ci) {
            int t1 = blockCoord[info.tangent1Axis] + info.cornerT1[ci];
            int t2 = blockCoord[info.tangent2Axis] + info.cornerT2[ci];
            int skySum = 0;
            glm::ivec3 blockSum(0);
            for (int dt1 = -1; dt1 <= 0; ++dt1) {
              for (int dt2 = -1; dt2 <= 0; ++dt2) {
                int coord[3];
                coord[info.normalAxis] = depthCoord[info.normalAxis];
                coord[info.tangent1Axis] = t1 + dt1;
                coord[info.tangent2Axis] = t2 + dt2;
                auto [sky, block] = queryLight(coord[0], coord[1], coord[2]);
                skySum += sky;
                blockSum += block;
              }
            }
            skyLight01[ci] = (skySum / 4.0f) / 15.0f;
            blockLight01[ci] = glm::vec3(blockSum) / 4.0f / 15.0f;
          }

          float material = isEmissive(type) ? 1.0f : specularStrength(type);

          if (isAlphaBlended(type)) {
            addFace(outTransparentVertices, outTransparentIndices, corners,
                    tile, normal, material, skyLight01, blockLight01);
          } else {
            addFace(outVertices, outIndices, corners, tile, normal, material,
                    skyLight01, blockLight01);
          }
        }
      }
    }
  }
}

void Chunk::buildGrassMesh(std::vector<float> &outVertices,
                           const BiomeMap &biomes, int worldOffsetX,
                           int worldOffsetZ) const {
  outVertices.clear();

  for (int x = 0; x < CHUNK_SIZE_XZ; ++x) {
    for (int z = 0; z < CHUNK_SIZE_XZ; ++z) {
      BiomeType biome = columnBiome[columnIndex(x, z)];
      const BiomeSurfaceParams &sp = biomes.surfaceParams(biome);
      if (sp.grassTuftChance <= 0.0f)
        continue;

      int surfaceY = -1;
      for (int y = CHUNK_HEIGHT - 1; y >= 0; --y) {
        if (getBlock(x, y, z) != BlockType::Air) {
          surfaceY = y;
          break;
        }
      }
      if (surfaceY < 0 || surfaceY + 1 >= CHUNK_HEIGHT)
        continue;
      if (getBlock(x, surfaceY, z) != BlockType::Grass)
        continue;
      if (getBlock(x, surfaceY + 1, z) != BlockType::Air)
        continue;

      uint32_t wx = (uint32_t)(worldOffsetX + x);
      uint32_t wz = (uint32_t)(worldOffsetZ + z);
      uint32_t h = wx * 374761393u + wz * 668265263u;
      h = (h ^ (h >> 13)) * 1274126177u;
      h ^= (h >> 16);
      float roll = (h & 0xFFFFu) / 65535.0f;

      constexpr float PATCH_THRESHOLD = 0.55f;
      float patch = biomes.grassPatch((float)(worldOffsetX + x),
                                      (float)(worldOffsetZ + z));
      if (patch < PATCH_THRESHOLD)
        continue;
      float patchIntensity =
          (patch - PATCH_THRESHOLD) / (1.0f - PATCH_THRESHOLD);
      float effectiveChance =
          sp.grassTuftChance * (0.3f + 0.7f * patchIntensity);
      if (roll > effectiveChance)
        continue;

      float fx = (float)(worldOffsetX + x) + 0.5f;
      float fy = (float)(surfaceY + 1);
      float fz = (float)(worldOffsetZ + z) + 0.5f;
      const float halfWidth = 0.4f;
      const float tuftHeight = 0.8f;

      glm::vec4 uv = tileUV(TILE_GRASS_TUFT);
      glm::vec2 cornerUV[4] = {
          {uv.x, uv.y}, {uv.z, uv.y}, {uv.z, uv.w}, {uv.x, uv.w}};
      glm::vec3 tint = sp.grassTint;

      auto emitQuad = [&](const glm::vec3 corners[4]) {
        static const int quadOrder[6] = {0, 1, 2, 2, 3, 0};
        for (int qi : quadOrder) {
          outVertices.push_back(corners[qi].x);
          outVertices.push_back(corners[qi].y);
          outVertices.push_back(corners[qi].z);
          outVertices.push_back(cornerUV[qi].x);
          outVertices.push_back(cornerUV[qi].y);
          outVertices.push_back(tint.r);
          outVertices.push_back(tint.g);
          outVertices.push_back(tint.b);
        }
      };

      glm::vec3 planeA[4] = {
          {fx - halfWidth, fy, fz - halfWidth},
          {fx + halfWidth, fy, fz + halfWidth},
          {fx + halfWidth, fy + tuftHeight, fz + halfWidth},
          {fx - halfWidth, fy + tuftHeight, fz - halfWidth},
      };
      glm::vec3 planeB[4] = {
          {fx - halfWidth, fy, fz + halfWidth},
          {fx + halfWidth, fy, fz - halfWidth},
          {fx + halfWidth, fy + tuftHeight, fz - halfWidth},
          {fx - halfWidth, fy + tuftHeight, fz + halfWidth},
      };
      emitQuad(planeA);
      emitQuad(planeB);
    }
  }
}
