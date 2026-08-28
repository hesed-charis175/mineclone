#pragma once

#include <glm/glm.hpp>

constexpr int ATLAS_COLS = 8;
constexpr int ATLAS_ROWS = 8;

enum TileIndex {
  TILE_GRASS_TOP = 0,
  TILE_GRASS_SIDE = 1,
  TILE_DIRT = 2,
  TILE_STONE = 3,
  TILE_SAND = 4,
  TILE_OAK_LOG_TOP = 5,
  TILE_OAK_LOG_SIDE = 6,
  TILE_OAK_LEAVES = 7,
  TILE_GLASS = 8,
  TILE_PLANK = 9,
  TILE_LIGHT = 10,
  TILE_IRON = 11,
  TILE_POLISHED_IRON = 12,

  TILE_CLAY = 13,
  TILE_GRAVEL = 14,
  TILE_ICE = 15,
  TILE_SANDSTONE = 16,
  TILE_MAGMA = 17,

  TILE_SPRUCE_LOG_TOP = 18,
  TILE_SPRUCE_LOG_SIDE = 19,
  TILE_SPRUCE_LEAVES = 20,

  TILE_BIRCH_LOG_TOP = 21,
  TILE_BIRCH_LOG_SIDE = 22,
  TILE_BIRCH_LEAVES = 23,

  TILE_ACACIA_LOG_TOP = 24,
  TILE_ACACIA_LOG_SIDE = 25,
  TILE_ACACIA_LEAVES = 26,

  TILE_JUNGLE_LOG_TOP = 27,
  TILE_JUNGLE_LOG_SIDE = 28,
  TILE_JUNGLE_LEAVES = 29,

  TILE_CHERRY_LOG_TOP = 30,
  TILE_CHERRY_LOG_SIDE = 31,
  TILE_CHERRY_LEAVES = 32,

  TILE_MANGROVE_LOG_TOP = 33,
  TILE_MANGROVE_LOG_SIDE = 34,
  TILE_MANGROVE_LEAVES = 35,

  TILE_COAL_ORE = 36,
  TILE_IRON_ORE = 37,
  TILE_COPPER_ORE = 38,
  TILE_GOLD_ORE = 39,
  TILE_DIAMOND_ORE = 40,
  TILE_EMERALD_ORE = 41,
  TILE_LITHIUM_ORE = 42,
  TILE_URANIUM_ORE = 43,

  TILE_GRASS_TUFT = 44,
};

inline glm::vec4 tileUV(int tileIndex) {
  int col = tileIndex % ATLAS_COLS;
  int rowFromTop = tileIndex / ATLAS_COLS;
  int rowFromBottom = (ATLAS_ROWS - 1) - rowFromTop;

  float tileW = 1.0f / ATLAS_COLS;
  float tileH = 1.0f / ATLAS_ROWS;

  float uMin = col * tileW;
  float vMin = rowFromBottom * tileH;
  return glm::vec4(uMin, vMin, uMin + tileW, vMin + tileH);
}
