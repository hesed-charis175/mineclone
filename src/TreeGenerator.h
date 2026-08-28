#pragma once

#include "BiomeMap.h"
#include "Chunk.h"
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <random>
#include <utility>
#include <vector>

class TreeGenerator {
public:
  static void generate(Chunk &chunk, int cx, int cz, uint32_t worldSeed,
                       const BiomeMap &biomes) {
    BiomeType dominant = chunk.getColumnBiome(8, 8);
    const BiomeSurfaceParams &sp = biomes.surfaceParams(dominant);
    if (!sp.hasTrees || sp.treeMinSpacing <= 0.0f)
      return;

    uint32_t seed = worldSeed ^ (uint32_t)(cx * 73856093) ^
                    (uint32_t)(cz * 19349663) ^ 0x9E3779B9u;
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> coord(3, CHUNK_SIZE_XZ - 4);
    std::uniform_real_distribution<float> speciesRoll(0.0f, 1.0f);

    int attempts = (int)((CHUNK_SIZE_XZ * CHUNK_SIZE_XZ) /
                         (sp.treeMinSpacing * sp.treeMinSpacing));

    struct Placed {
      int x, z;
    };
    std::vector<Placed> accepted;

    for (int i = 0; i < attempts; ++i) {
      int x = coord(rng), z = coord(rng);

      bool tooClose = false;
      for (const auto &p : accepted) {
        float dx = (float)(p.x - x), dz = (float)(p.z - z);
        if (dx * dx + dz * dz < sp.treeMinSpacing * sp.treeMinSpacing) {
          tooClose = true;
          break;
        }
      }
      if (tooClose)
        continue;

      if (chunk.getColumnBiome(x, z) != dominant)
        continue;

      int surfaceY = -1;
      for (int y = CHUNK_HEIGHT - 1; y >= 0; --y) {
        if (chunk.getBlock(x, y, z) != BlockType::Air) {
          surfaceY = y;
          break;
        }
      }
      if (surfaceY < 0 || chunk.getBlock(x, surfaceY, z) != BlockType::Grass)
        continue;
      if (chunk.getBlock(x, surfaceY + 1, z) != BlockType::Air)
        continue;

      accepted.push_back({x, z});

      bool useAlt = sp.altChance > 0.0f && speciesRoll(rng) < sp.altChance;
      BlockType logType = useAlt ? sp.treeLogAlt : sp.treeLog;
      BlockType leafType = useAlt ? sp.treeLeavesAlt : sp.treeLeaves;
      TreeShape shape = useAlt ? sp.treeShapeAlt : sp.treeShape;

      int minTrunk, maxTrunk;
      trunkHeightRange(shape, minTrunk, maxTrunk);
      std::uniform_int_distribution<int> trunkHeightDist(minTrunk, maxTrunk);

      int trunkHeight = trunkHeightDist(rng);
      int topY = surfaceY + trunkHeight;
      int trunkTopLogY = topY - 1;
      for (int y = surfaceY + 1; y <= trunkTopLogY; ++y)
        chunk.setBlock(x, y, z, logType);

      stampCanopy(chunk, x, topY, z, leafType, shape, rng);
    }
  }

private:
  static void trunkHeightRange(TreeShape shape, int &minTrunk, int &maxTrunk) {
    switch (shape) {
    case TreeShape::Conical:
      minTrunk = 5;
      maxTrunk = 8;
      break;
    case TreeShape::Umbrella:
    case TreeShape::Weeping:
      minTrunk = 3;
      maxTrunk = 5;
      break;
    case TreeShape::Round:
    default:
      minTrunk = 4;
      maxTrunk = 7;
      break;
    }
  }

  struct CanopyLayer {
    int dy;
    int radius;
    float edgeKeepChance;
    bool plusOnly;
  };
  static void stampCanopy(Chunk &chunk, int x, int topY, int z,
                          BlockType leafType, TreeShape shape,
                          std::mt19937 &rng) {
    std::uniform_real_distribution<float> roll(0.0f, 1.0f);

    static const CanopyLayer roundFull[] = {
        {-2, 2, 0.55f, false},
        {-1, 2, 0.75f, false},
        {0, 1, 1.0f, false},
        {1, 1, 0.0f, true},
    };
    static const CanopyLayer roundSmall[] = {
        {-1, 1, 1.0f, false},
        {0, 1, 0.6f, false},
        {1, 1, 0.0f, true},
    };
    static const CanopyLayer conical[] = {
        {-4, 2, 0.45f, false}, {-3, 2, 0.7f, false}, {-2, 1, 1.0f, false},
        {-1, 1, 0.6f, false},  {0, 0, 1.0f, false},

        {1, 0, 1.0f, false},
    };
    static const CanopyLayer umbrella[] = {
        {0, 2, 0.55f, false},
        {1, 1, 0.35f, false},
    };
    static const CanopyLayer weeping[] = {
        {-1, 2, 0.5f, false},
        {0, 2, 0.7f, false},
        {1, 1, 0.4f, false},
    };

    const CanopyLayer *layers = roundFull;
    int layerCount = (int)(sizeof(roundFull) / sizeof(roundFull[0]));

    switch (shape) {
    case TreeShape::Conical:
      layers = conical;
      layerCount = (int)(sizeof(conical) / sizeof(conical[0]));
      break;
    case TreeShape::Umbrella:
      layers = umbrella;
      layerCount = (int)(sizeof(umbrella) / sizeof(umbrella[0]));
      break;
    case TreeShape::Weeping:
      layers = weeping;
      layerCount = (int)(sizeof(weeping) / sizeof(weeping[0]));
      break;
    case TreeShape::Round:
    default:
      if (roll(rng) < 0.3f) {
        layers = roundSmall;
        layerCount = (int)(sizeof(roundSmall) / sizeof(roundSmall[0]));
      }
      break;
    }

    for (int li = 0; li < layerCount; ++li) {
      const CanopyLayer &layer = layers[li];
      int ly = topY + layer.dy;
      for (int dx = -layer.radius; dx <= layer.radius; ++dx) {
        for (int dz = -layer.radius; dz <= layer.radius; ++dz) {
          if (layer.plusOnly && dx != 0 && dz != 0)
            continue;
          bool onOuterRing =
              layer.radius > 0 &&
              std::max(std::abs(dx), std::abs(dz)) == layer.radius;
          if (onOuterRing && roll(rng) > layer.edgeKeepChance)
            continue;
          int lx = x + dx, lz = z + dz;
          if (chunk.getBlock(lx, ly, lz) == BlockType::Air)
            chunk.setBlock(lx, ly, lz, leafType);
        }
      }
    }
  }
};
