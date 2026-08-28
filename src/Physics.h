#pragma once

#include "ChunkManager.h"
#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>

struct PlayerBounds {
  float halfWidth = 0.3f;
  float height = 1.8f;
  float eyeHeight = 1.62f;
};

inline void playerAABB(const glm::vec3 &feetPos, const PlayerBounds &bounds,
                       glm::vec3 &outMin, glm::vec3 &outMax) {
  outMin = feetPos - glm::vec3(bounds.halfWidth, 0.0f, bounds.halfWidth);
  outMax =
      feetPos + glm::vec3(bounds.halfWidth, bounds.height, bounds.halfWidth);
}

inline bool aabbIntersectsWorld(const ChunkManager &cm, const glm::vec3 &min,
                                const glm::vec3 &max) {
  int minX = (int)std::floor(min.x), maxX = (int)std::floor(max.x - 0.0001f);
  int minY = (int)std::floor(min.y), maxY = (int)std::floor(max.y - 0.0001f);
  int minZ = (int)std::floor(min.z), maxZ = (int)std::floor(max.z - 0.0001f);

  for (int x = minX; x <= maxX; ++x)
    for (int y = minY; y <= maxY; ++y)
      for (int z = minZ; z <= maxZ; ++z)
        if (cm.getWorldBlock(x, y, z) != BlockType::Air)
          return true;
  return false;
}

inline bool moveAxisWithCollision(const ChunkManager &cm, glm::vec3 &feetPos,
                                  const PlayerBounds &bounds, int axis,
                                  float delta) {
  if (delta == 0.0f)
    return false;

  const float stepSize = 0.02f;
  float remaining = delta;
  float sign = (delta > 0.0f) ? 1.0f : -1.0f;

  while (std::abs(remaining) > 0.0f) {
    float step = sign * std::min(stepSize, std::abs(remaining));
    glm::vec3 trial = feetPos;
    trial[axis] += step;

    glm::vec3 aabbMin, aabbMax;
    playerAABB(trial, bounds, aabbMin, aabbMax);
    if (aabbIntersectsWorld(cm, aabbMin, aabbMax)) {
      return true;
    }
    feetPos = trial;
    remaining -= step;
  }
  return false;
}
