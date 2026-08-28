#pragma once

#include "TextureAtlas.h"
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/round.hpp>

enum class BlockType : uint8_t {
  Air = 0,
  Grass,
  Dirt,
  Stone,
  Sand,
  OakLog,
  OakLeaves,
  Glass,
  Plank,
  Light,
  Iron,
  PolishedIron,

  Clay,
  Gravel,
  Ice,
  Sandstone,
  Magma,

  SpruceLog,
  SpruceLeaves,
  BirchLog,
  BirchLeaves,
  AcaciaLog,
  AcaciaLeaves,
  JungleLog,
  JungleLeaves,
  CherryLog,
  CherryLeaves,
  MangroveLog,
  MangroveLeaves,

  CoalOre,
  IronOre,
  CopperOre,
  GoldOre,
  DiamondOre,
  EmeraldOre,
  LithiumOre,
  UraniumOre,

};

enum class FaceDir {
  PosZ,
  NegZ,
  NegX,
  PosX,
  PosY,
  NegY,
};

inline bool isTransparent(BlockType type) {
  switch (type) {
  case BlockType::OakLeaves:
  case BlockType::SpruceLeaves:
  case BlockType::BirchLeaves:
  case BlockType::AcaciaLeaves:
  case BlockType::JungleLeaves:
  case BlockType::CherryLeaves:
  case BlockType::MangroveLeaves:
  case BlockType::Glass:
    return true;
  default:
    return false;
  }
}

inline bool isAlphaBlended(BlockType type) { return type == BlockType::Glass; }

inline bool isOreGlow(BlockType type) {
  switch (type) {
  case BlockType::CoalOre:
  case BlockType::IronOre:
  case BlockType::CopperOre:
  case BlockType::GoldOre:
  case BlockType::DiamondOre:
  case BlockType::EmeraldOre:
  case BlockType::LithiumOre:
  case BlockType::UraniumOre:
    return true;
  default:
    return false;
  }
}

inline bool isEmissive(BlockType type) {
  return type == BlockType::Light || type == BlockType::Magma ||
         isOreGlow(type);
}

constexpr int LIGHT_BLOCK_LEVEL = 10;

constexpr int ORE_GLOW_LEVEL = 5;

constexpr int MAGMA_LIGHT_LEVEL = 8;

inline int lightLevelForBlock(BlockType type) {
  if (type == BlockType::Light)
    return LIGHT_BLOCK_LEVEL;
  if (type == BlockType::Magma)
    return MAGMA_LIGHT_LEVEL;
  if (isOreGlow(type))
    return ORE_GLOW_LEVEL;
  return 0;
}

inline glm::vec3 lightTintForBlock(BlockType type) {
  switch (type) {
  case BlockType::Magma:
    return glm::vec3(1.0f, 0.35f, 0.15f);
  case BlockType::CoalOre:
    return glm::vec3(0.55f, 0.55f, 0.6f);
  case BlockType::IronOre:
    return glm::vec3(1.0f, 0.82f, 0.62f);
  case BlockType::CopperOre:
    return glm::vec3(1.0f, 0.55f, 0.3f);
  case BlockType::GoldOre:
    return glm::vec3(1.0f, 0.85f, 0.25f);
  case BlockType::DiamondOre:
    return glm::vec3(0.55f, 0.92f, 1.0f);
  case BlockType::EmeraldOre:
    return glm::vec3(0.2f, 1.0f, 0.5f);
  case BlockType::LithiumOre:
    return glm::vec3(0.68f, 0.48f, 1.0f);
  case BlockType::UraniumOre:
    return glm::vec3(0.65f, 1.0f, 0.25f);
  default:
    return glm::vec3(1.0f);
  }
}

inline glm::ivec3 lightColorForBlock(BlockType type) {
  glm::vec3 seed = lightTintForBlock(type) * (float)lightLevelForBlock(type);
  return glm::clamp(glm::ivec3(glm::round(seed)), glm::ivec3(0),
                    glm::ivec3(15));
}

inline float specularStrength(BlockType type) {
  switch (type) {
  case BlockType::PolishedIron:
    return 0.65f;
  case BlockType::Ice:
    return 0.45f;
  case BlockType::Glass:
    return 0.20f;
  case BlockType::Iron:
    return 0.30f;
  default:
    return 0.0f;
  }
}

inline int tileForBlockFace(BlockType type, FaceDir face) {
  bool topOrBottom = (face == FaceDir::PosY || face == FaceDir::NegY);

  switch (type) {
  case BlockType::Grass:
    if (face == FaceDir::PosY)
      return TILE_GRASS_TOP;
    if (face == FaceDir::NegY)
      return TILE_DIRT;
    return TILE_GRASS_SIDE;
  case BlockType::Dirt:
    return TILE_DIRT;
  case BlockType::Stone:
    return TILE_STONE;
  case BlockType::Sand:
    return TILE_SAND;

  case BlockType::OakLog:
    return topOrBottom ? TILE_OAK_LOG_TOP : TILE_OAK_LOG_SIDE;
  case BlockType::OakLeaves:
    return TILE_OAK_LEAVES;

  case BlockType::Glass:
    return TILE_GLASS;
  case BlockType::Plank:
    return TILE_PLANK;
  case BlockType::Light:
    return TILE_LIGHT;
  case BlockType::Iron:
    return TILE_IRON;
  case BlockType::PolishedIron:
    return TILE_POLISHED_IRON;

  case BlockType::Clay:
    return TILE_CLAY;
  case BlockType::Gravel:
    return TILE_GRAVEL;
  case BlockType::Ice:
    return TILE_ICE;
  case BlockType::Sandstone:
    return TILE_SANDSTONE;
  case BlockType::Magma:
    return TILE_MAGMA;

  case BlockType::SpruceLog:
    return topOrBottom ? TILE_SPRUCE_LOG_TOP : TILE_SPRUCE_LOG_SIDE;
  case BlockType::SpruceLeaves:
    return TILE_SPRUCE_LEAVES;

  case BlockType::BirchLog:
    return topOrBottom ? TILE_BIRCH_LOG_TOP : TILE_BIRCH_LOG_SIDE;
  case BlockType::BirchLeaves:
    return TILE_BIRCH_LEAVES;

  case BlockType::AcaciaLog:
    return topOrBottom ? TILE_ACACIA_LOG_TOP : TILE_ACACIA_LOG_SIDE;
  case BlockType::AcaciaLeaves:
    return TILE_ACACIA_LEAVES;

  case BlockType::JungleLog:
    return topOrBottom ? TILE_JUNGLE_LOG_TOP : TILE_JUNGLE_LOG_SIDE;
  case BlockType::JungleLeaves:
    return TILE_JUNGLE_LEAVES;

  case BlockType::CherryLog:
    return topOrBottom ? TILE_CHERRY_LOG_TOP : TILE_CHERRY_LOG_SIDE;
  case BlockType::CherryLeaves:
    return TILE_CHERRY_LEAVES;

  case BlockType::MangroveLog:
    return topOrBottom ? TILE_MANGROVE_LOG_TOP : TILE_MANGROVE_LOG_SIDE;
  case BlockType::MangroveLeaves:
    return TILE_MANGROVE_LEAVES;
  case BlockType::CoalOre:
    return TILE_COAL_ORE;
  case BlockType::IronOre:
    return TILE_IRON_ORE;
  case BlockType::CopperOre:
    return TILE_COPPER_ORE;
  case BlockType::GoldOre:
    return TILE_GOLD_ORE;
  case BlockType::DiamondOre:
    return TILE_DIAMOND_ORE;
  case BlockType::EmeraldOre:
    return TILE_EMERALD_ORE;
  case BlockType::LithiumOre:
    return TILE_LITHIUM_ORE;
  case BlockType::UraniumOre:
    return TILE_URANIUM_ORE;
  case BlockType::Air:
  default:
    return TILE_STONE;
  }
}
