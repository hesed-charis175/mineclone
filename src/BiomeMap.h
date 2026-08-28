#pragma once

#include "BlockType.h"
#include "PerlinNoise.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <glm/glm.hpp>
#include <unordered_map>

enum class BiomeType : uint8_t {
  Desert,
  Savanna,
  Plains,
  Forest,
  Jungle,
  Swamp,
  Taiga,
  CherryGrove,
  Tundra,
  Mountains,
};

enum class TreeShape {
  Round,
  Conical,
  Umbrella,
  Weeping,
};

struct BiomeTerrainParams {
  float noiseScale;
  int baseHeight;
  int amplitude;
  int octaves;
  float persistence;

  float reliefSensitivity;

  bool hasCaves;

  int subsurfaceDepth;
};

struct BiomeSurfaceParams {
  BlockType surfaceBlock;
  BlockType subsurfaceBlock;

  bool hasTrees;
  BlockType treeLog;
  BlockType treeLeaves;
  float treeMinSpacing;
  TreeShape treeShape;
  BlockType treeLogAlt;
  BlockType treeLeavesAlt;
  TreeShape treeShapeAlt;
  float altChance;

  float grassTuftChance;
  glm::vec3 grassTint;
};

class BiomeMap {
public:
  explicit BiomeMap(uint32_t seed) : noise(seed), worldSeed(seed) {}

  static float equalizeNoise(float v) {
    float centered = v - 0.5f;
    float sign = centered < 0.0f ? -1.0f : 1.0f;
    float mag = std::min(std::abs(centered) * 2.0f, 1.0f);
    float expanded = std::pow(mag, 0.6f);
    return 0.5f + sign * expanded * 0.5f;
  }

  float temperature(float worldX, float worldZ) const {
    float raw =
        0.5f + 0.5f * noise.octaveNoise((worldX + 10000.0f) * 0.01f,
                                        (worldZ + 10000.0f) * 0.01f, 3, 0.5f);
    return equalizeNoise(raw);
  }

  float moisture(float worldX, float worldZ) const {
    float raw =
        0.5f + 0.5f * noise.octaveNoise((worldX - 20000.0f) * 0.01f,
                                        (worldZ - 20000.0f) * 0.01f, 3, 0.5f);
    return equalizeNoise(raw);
  }

  float rarity(float worldX, float worldZ) const {
    return 0.5f + 0.5f * noise.octaveNoise((worldX + 55555.0f) * 0.01f,
                                           (worldZ + 55555.0f) * 0.01f, 2,
                                           0.5f);
  }

  float grassPatch(float worldX, float worldZ) const {
    return 0.5f + 0.5f * noise.octaveNoise((worldX + 33333.0f) * 0.05f,
                                           (worldZ + 33333.0f) * 0.05f, 2,
                                           0.5f);
  }

  float terrainRelief(float worldX, float worldZ) const {
    float raw =
        0.5f + 0.5f * noise.octaveNoise((worldX + 77777.0f) * 0.006f,
                                        (worldZ + 77777.0f) * 0.006f, 3, 0.55f);
    return equalizeNoise(raw);
  }

  static constexpr float PEAK_CELL_SIZE = 160.0f;
  static constexpr float PEAK_MIN_RADIUS = 55.0f;
  static constexpr float PEAK_MAX_RADIUS = 95.0f;

  static constexpr float PEAK_MIN_HEIGHT = 26.0f;
  static constexpr float PEAK_MAX_HEIGHT = 52.0f;

  struct MountainPeak {
    bool exists = false;
    float x = 0.0f, z = 0.0f;
    float radius = 0.0f;
    float height = 0.0f;
  };

  MountainPeak peakForCell(int cellX, int cellZ) const {
    uint32_t h = worldSeed ^ (uint32_t)(cellX * 668265263) ^
                 (uint32_t)(cellZ * 2246822519u);
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= (h >> 16);

    MountainPeak peak;
    peak.exists = (h % 100u) < 55u;
    if (!peak.exists)
      return peak;

    uint32_t hx = h * 2246822519u;
    uint32_t hz = h * 3266489917u;
    float jitterX = ((hx >> 8) & 0xFFFFu) / 65535.0f;
    float jitterZ = ((hz >> 8) & 0xFFFFu) / 65535.0f;
    peak.x = ((float)cellX + jitterX) * PEAK_CELL_SIZE;
    peak.z = ((float)cellZ + jitterZ) * PEAK_CELL_SIZE;

    float radiusT = ((h >> 3) & 0xFFu) / 255.0f;
    peak.radius =
        PEAK_MIN_RADIUS + radiusT * (PEAK_MAX_RADIUS - PEAK_MIN_RADIUS);
    uint32_t hh = h * 2654435761u;
    hh = (hh ^ (hh >> 15)) * 0x27d4eb2fu;
    float heightT = ((hh >> 7) & 0xFFu) / 255.0f;
    peak.height =
        PEAK_MIN_HEIGHT + heightT * (PEAK_MAX_HEIGHT - PEAK_MIN_HEIGHT);
    return peak;
  }

  float mountainPeakHeight(float worldX, float worldZ) const {
    int cellX = (int)std::floor(worldX / PEAK_CELL_SIZE);
    int cellZ = (int)std::floor(worldZ / PEAK_CELL_SIZE);

    float best = 0.0f;
    for (int dcz = -1; dcz <= 1; ++dcz) {
      for (int dcx = -1; dcx <= 1; ++dcx) {
        MountainPeak peak = peakForCell(cellX + dcx, cellZ + dcz);
        if (!peak.exists)
          continue;
        float dx = worldX - peak.x;
        float dz = worldZ - peak.z;
        float planarDistSq = dx * dx + dz * dz;
        if (planarDistSq > peak.radius * peak.radius)
          continue;
        float domeShape = std::sqrt(
            std::max(0.0f, 1.0f - planarDistSq / (peak.radius * peak.radius)));
        float domeHeight = domeShape * peak.height;
        best = std::max(best, domeHeight);
      }
    }
    return best;
  }

  BiomeType classify(float worldX, float worldZ) const {
    float t = temperature(worldX, worldZ);
    float m = moisture(worldX, worldZ);
    if (t > 0.65f && m < 0.3f) {
      return BiomeType::Desert;
    }
    return BiomeType::Mountains;

#if 0
    if (t > 0.65f) {
      if (m < 0.3f)
        return BiomeType::Desert;
      if (m < 0.6f)
        return BiomeType::Savanna;
      return BiomeType::Jungle;
    }

    if (t < 0.3f) {
      return m < 0.4f ? BiomeType::Tundra : BiomeType::Taiga;
    }

    // Mid-temperature band.
    if (m > 0.75f)
      return BiomeType::Swamp;

    if (m > 0.4f && m < 0.6f && rarity(worldX, worldZ) > 0.85f)
      return BiomeType::CherryGrove; // rare pocket, checked before the
                                     // ordinary Forest/Plains split below

    if (m > 0.55f)
      return BiomeType::Forest;
    if (m < 0.2f)
      return BiomeType::Mountains; // dry + mid-temperature reads as rocky
                                   // highlands rather than plains
    return BiomeType::Plains;
#endif
  }

  struct ZoneWeights {
    float valley, plain, mountain;
  };

  ZoneWeights zoneWeights(float relief) const {
    float toPlain = glm::smoothstep(0.24f, 0.34f, relief);
    float toMountain = glm::smoothstep(0.56f, 0.66f, relief);
    float mountain = toMountain;
    float plain = toPlain * (1.0f - toMountain);
    float valley = 1.0f - toPlain;
    return {valley, plain, mountain};
  }

  float screePatch(float worldX, float worldZ) const {
    return 0.5f + 0.5f * noise.octaveNoise((worldX - 44444.0f) * 0.035f,
                                           (worldZ - 44444.0f) * 0.035f, 2,
                                           0.5f);
  }

  float caveEntranceMask(float worldX, float worldZ) const {
    float raw =
        0.5f + 0.5f * noise.octaveNoise((worldX + 88888.0f) * 0.015f,
                                        (worldZ + 88888.0f) * 0.015f, 2, 0.5f);
    return equalizeNoise(raw);
  }

  const BiomeTerrainParams &terrainParams(BiomeType biome) const;
  const BiomeSurfaceParams &surfaceParams(BiomeType biome) const;

  const PerlinNoise &heightNoise() const { return noise; }

  int surfaceHeight(float worldX, float worldZ) const {
    BiomeType biome = classify(worldX, worldZ);
    const BiomeTerrainParams &tp = terrainParams(biome);
    float n = noise.octaveNoise(worldX * tp.noiseScale, worldZ * tp.noiseScale,
                                tp.octaves, tp.persistence);
    int surfaceY = tp.baseHeight + (int)std::lround(n * tp.amplitude);
    return std::max(surfaceY, 1);
  }

private:
  PerlinNoise noise;
  uint32_t worldSeed;
};
