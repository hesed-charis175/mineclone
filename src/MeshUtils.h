#pragma once

#include "TextureAtlas.h"
#include <glm/glm.hpp>
#include <vector>

inline void addFace(std::vector<float> &verts,
                    std::vector<unsigned int> &indices,
                    const glm::vec3 corners[4], int tileIndex,
                    const glm::vec3 &normal, float material,
                    const float skyLight01[4],
                    const glm::vec3 blockLight01[4]) {
  glm::vec4 uv = tileUV(tileIndex);
  glm::vec2 cornerUV[4] = {
      {uv.x, uv.y}, {uv.z, uv.y}, {uv.z, uv.w}, {uv.x, uv.w}};

  unsigned int base = (unsigned int)(verts.size() / 13);
  for (int i = 0; i < 4; ++i) {
    verts.push_back(corners[i].x);
    verts.push_back(corners[i].y);
    verts.push_back(corners[i].z);
    verts.push_back(cornerUV[i].x);
    verts.push_back(cornerUV[i].y);
    verts.push_back(normal.x);
    verts.push_back(normal.y);
    verts.push_back(normal.z);
    verts.push_back(material);
    verts.push_back(skyLight01[i]);
    verts.push_back(blockLight01[i].r);
    verts.push_back(blockLight01[i].g);
    verts.push_back(blockLight01[i].b);
  }
  unsigned int quadIndices[6] = {0, 1, 2, 2, 3, 0};
  for (unsigned int qi : quadIndices)
    indices.push_back(base + qi);
}
