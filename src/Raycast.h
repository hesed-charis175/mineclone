#pragma once

#include "ChunkManager.h"
#include <cmath>
#include <glm/glm.hpp>

struct RaycastHit {
  bool hit = false;
  glm::ivec3 blockPos{0};
  glm::ivec3 placePos{0};
};

inline RaycastHit raycastBlocks(const ChunkManager &manager, glm::vec3 origin,
                                glm::vec3 direction, float maxDistance = 6.0f,
                                float step = 0.05f) {
  RaycastHit result;
  glm::vec3 dir = glm::normalize(direction);

  glm::ivec3 lastEmpty((int)std::floor(origin.x), (int)std::floor(origin.y),
                       (int)std::floor(origin.z));

  for (float t = 0.0f; t < maxDistance; t += step) {
    glm::vec3 p = origin + dir * t;
    glm::ivec3 blockPos((int)std::floor(p.x), (int)std::floor(p.y),
                        (int)std::floor(p.z));

    BlockType block = manager.getWorldBlock(blockPos.x, blockPos.y, blockPos.z);
    if (block != BlockType::Air) {
      result.hit = true;
      result.blockPos = blockPos;
      result.placePos = lastEmpty;
      return result;
    }
    lastEmpty = blockPos;
  }

  return result;
}
