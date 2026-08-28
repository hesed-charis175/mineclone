#include "ChunkManager.h"
#include "TreeGenerator.h"
#include <cmath>
#include <cstdlib>
#include <queue>
#include <vector>

static int floorDiv(int a, int b) {
  int q = a / b;
  int r = a % b;
  if (r != 0 && ((r < 0) != (b < 0)))
    q--;
  return q;
}

void ChunkManager::update(const glm::vec3 &playerWorldPos) {
  int pcx = floorDiv((int)std::floor(playerWorldPos.x), CHUNK_SIZE_XZ);
  int pcz = floorDiv((int)std::floor(playerWorldPos.z), CHUNK_SIZE_XZ);

  if (pcx != lastPlayerChunkX || pcz != lastPlayerChunkZ) {
    lastPlayerChunkX = pcx;
    lastPlayerChunkZ = pcz;

    for (int cx = pcx - RENDER_DISTANCE; cx <= pcx + RENDER_DISTANCE; ++cx) {
      for (int cz = pcz - RENDER_DISTANCE; cz <= pcz + RENDER_DISTANCE; ++cz) {
        int64_t key = chunkKey(cx, cz);
        if (chunks.find(key) == chunks.end() &&
            pendingChunkKeys.find(key) == pendingChunkKeys.end()) {
          pendingChunkLoads.push_back({cx, cz});
          pendingChunkKeys.insert(key);
        }
      }
    }

    std::vector<int64_t> toUnload;
    for (const auto &[key, chunk] : chunks) {
      int cx = (int)(key >> 32);
      int cz = (int)(key & 0xFFFFFFFF);
      if (std::abs(cx - pcx) > UNLOAD_DISTANCE ||
          std::abs(cz - pcz) > UNLOAD_DISTANCE) {
        toUnload.push_back(key);
      }
    }
    for (int64_t key : toUnload) {
      unloadChunk((int)(key >> 32), (int)(key & 0xFFFFFFFF));
    }
  }

  int budget = CHUNKS_LOADED_PER_FRAME;
  while (budget > 0 && !pendingChunkLoads.empty()) {
    auto [cx, cz] = pendingChunkLoads.front();
    pendingChunkLoads.pop_front();
    pendingChunkKeys.erase(chunkKey(cx, cz));

    if (chunks.find(chunkKey(cx, cz)) == chunks.end()) {
      loadChunk(cx, cz);
      budget--;
    }
  }
}

void ChunkManager::loadChunk(int cx, int cz) {
  Chunk chunk;
  chunk.generateTerrain(cx * CHUNK_SIZE_XZ, cz * CHUNK_SIZE_XZ, biomes);

  chunk.carveCaves(cx * CHUNK_SIZE_XZ, cz * CHUNK_SIZE_XZ, biomes);

  TreeGenerator::generate(chunk, cx, cz, worldSeed, biomes);

  chunk.computeSkyLight();

  std::vector<std::tuple<int, int, int>> newEmissivePositions;
  for (int x = 0; x < CHUNK_SIZE_XZ; ++x) {
    for (int y = 0; y < CHUNK_HEIGHT; ++y) {
      for (int z = 0; z < CHUNK_SIZE_XZ; ++z) {
        if (isEmissive(chunk.getBlock(x, y, z))) {
          auto pos = std::make_tuple(cx * CHUNK_SIZE_XZ + x, y,
                                     cz * CHUNK_SIZE_XZ + z);
          if (emissiveBlockPositions.insert(pos).second) {
            newEmissivePositions.push_back(pos);
          }
        }
      }
    }
  }

  chunks[chunkKey(cx, cz)] = std::move(chunk);

  recomputeBlockLightIncremental(newEmissivePositions, cx, cz);

  uploadChunkMesh(cx, cz);
  uploadGrassMesh(cx, cz);

  if (chunks.count(chunkKey(cx - 1, cz)))
    uploadChunkMesh(cx - 1, cz);
  if (chunks.count(chunkKey(cx + 1, cz)))
    uploadChunkMesh(cx + 1, cz);
  if (chunks.count(chunkKey(cx, cz - 1)))
    uploadChunkMesh(cx, cz - 1);
  if (chunks.count(chunkKey(cx, cz + 1)))
    uploadChunkMesh(cx, cz + 1);
}

void ChunkManager::unloadChunk(int cx, int cz) {
  int64_t key = chunkKey(cx, cz);

  auto opaqueIt = chunkMeshesOpaque.find(key);
  if (opaqueIt != chunkMeshesOpaque.end()) {
    glDeleteVertexArrays(1, &opaqueIt->second.vao);
    glDeleteBuffers(1, &opaqueIt->second.vbo);
    glDeleteBuffers(1, &opaqueIt->second.ebo);
    chunkMeshesOpaque.erase(opaqueIt);
  }
  auto transparentIt = chunkMeshesTransparent.find(key);
  if (transparentIt != chunkMeshesTransparent.end()) {
    glDeleteVertexArrays(1, &transparentIt->second.vao);
    glDeleteBuffers(1, &transparentIt->second.vbo);
    glDeleteBuffers(1, &transparentIt->second.ebo);
    chunkMeshesTransparent.erase(transparentIt);
  }
  auto grassIt = chunkGrassMeshes.find(key);
  if (grassIt != chunkGrassMeshes.end()) {
    glDeleteVertexArrays(1, &grassIt->second.vao);
    glDeleteBuffers(1, &grassIt->second.vbo);
    chunkGrassMeshes.erase(grassIt);
  }
  chunks.erase(key);
}

BlockType ChunkManager::getWorldBlock(int worldX, int worldY,
                                      int worldZ) const {
  int cx = floorDiv(worldX, CHUNK_SIZE_XZ);
  int cz = floorDiv(worldZ, CHUNK_SIZE_XZ);

  auto it = chunks.find(chunkKey(cx, cz));
  if (it == chunks.end())
    return BlockType::Air; // no chunk generated there yet

  int localX = worldX - cx * CHUNK_SIZE_XZ;
  int localZ = worldZ - cz * CHUNK_SIZE_XZ;
  return it->second.getBlock(localX, worldY, localZ);
}

uint8_t ChunkManager::getWorldSkyLight(int worldX, int worldY,
                                       int worldZ) const {
  int cx = floorDiv(worldX, CHUNK_SIZE_XZ);
  int cz = floorDiv(worldZ, CHUNK_SIZE_XZ);
  auto it = chunks.find(chunkKey(cx, cz));
  if (it == chunks.end())
    return 15;
  int localX = worldX - cx * CHUNK_SIZE_XZ;
  int localZ = worldZ - cz * CHUNK_SIZE_XZ;
  return it->second.getSkyLight(localX, worldY, localZ);
}

glm::ivec3 ChunkManager::getWorldBlockLight(int worldX, int worldY,
                                            int worldZ) const {
  int cx = floorDiv(worldX, CHUNK_SIZE_XZ);
  int cz = floorDiv(worldZ, CHUNK_SIZE_XZ);
  auto it = chunks.find(chunkKey(cx, cz));
  if (it == chunks.end())
    return glm::ivec3(0);
  int localX = worldX - cx * CHUNK_SIZE_XZ;
  int localZ = worldZ - cz * CHUNK_SIZE_XZ;
  return it->second.getBlockLight(localX, worldY, localZ);
}

void ChunkManager::setWorldBlockLight(int worldX, int worldY, int worldZ,
                                      glm::ivec3 value) {
  int cx = floorDiv(worldX, CHUNK_SIZE_XZ);
  int cz = floorDiv(worldZ, CHUNK_SIZE_XZ);
  auto it = chunks.find(chunkKey(cx, cz));
  if (it == chunks.end())
    return;
  int localX = worldX - cx * CHUNK_SIZE_XZ;
  int localZ = worldZ - cz * CHUNK_SIZE_XZ;
  it->second.setBlockLight(localX, worldY, localZ, value);
}

void ChunkManager::recomputeBlockLight() {
  for (auto &[key, chunk] : chunks)
    chunk.clearBlockLight();

  struct Cell {
    int x, y, z;
  };
  std::queue<Cell> queue;

  std::vector<Cell> litCells;

  for (const auto &[wx, wy, wz] : emissiveBlockPositions) {
    glm::ivec3 level = lightColorForBlock(getWorldBlock(wx, wy, wz));
    setWorldBlockLight(wx, wy, wz, level);
    queue.push({wx, wy, wz});
    litCells.push_back({wx, wy, wz});
  }

  static const int offsets[6][3] = {
      {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1},
  };

  while (!queue.empty()) {
    Cell c = queue.front();
    queue.pop();
    glm::ivec3 level = getWorldBlockLight(c.x, c.y, c.z);
    if (level.x <= 1 && level.y <= 1 && level.z <= 1)
      continue;

    glm::ivec3 newLevel = glm::max(level - glm::ivec3(1), glm::ivec3(0));

    for (const auto &off : offsets) {
      int nx = c.x + off[0], ny = c.y + off[1], nz = c.z + off[2];
      if (ny < 0 || ny >= CHUNK_HEIGHT)
        continue;
      int ncx = floorDiv(nx, CHUNK_SIZE_XZ);
      int ncz = floorDiv(nz, CHUNK_SIZE_XZ);
      if (chunks.find(chunkKey(ncx, ncz)) == chunks.end())
        continue;

      BlockType nt = getWorldBlock(nx, ny, nz);
      if (!(nt == BlockType::Air || isTransparent(nt)))
        continue;

      glm::ivec3 existing = getWorldBlockLight(nx, ny, nz);
      glm::ivec3 combined = glm::max(existing, newLevel);
      if (combined != existing) {
        setWorldBlockLight(nx, ny, nz, combined);
        queue.push({nx, ny, nz});
        litCells.push_back({nx, ny, nz});
      }
    }
  }

  std::vector<glm::ivec3> blended(litCells.size());
  for (size_t i = 0; i < litCells.size(); ++i) {
    const Cell &c = litCells[i];
    glm::ivec3 sum = getWorldBlockLight(c.x, c.y, c.z);
    int count = 1;
    for (const auto &off : offsets) {
      int nx = c.x + off[0], ny = c.y + off[1], nz = c.z + off[2];
      if (ny < 0 || ny >= CHUNK_HEIGHT)
        continue;
      if (chunks.find(chunkKey(floorDiv(nx, CHUNK_SIZE_XZ),
                               floorDiv(nz, CHUNK_SIZE_XZ))) == chunks.end())
        continue;
      sum += getWorldBlockLight(nx, ny, nz);
      count++;
    }
    blended[i] = sum / count;
  }
  for (size_t i = 0; i < litCells.size(); ++i) {
    const Cell &c = litCells[i];
    glm::ivec3 current = getWorldBlockLight(c.x, c.y, c.z);
    glm::ivec3 brightened = glm::max(blended[i], current);
    if (brightened != current) {
      setWorldBlockLight(c.x, c.y, c.z, brightened);
    }
  }
}

void ChunkManager::recomputeBlockLightIncremental(
    const std::vector<std::tuple<int, int, int>> &newEmissivePositions,
    int chunkCX, int chunkCZ) {
  struct Cell {
    int x, y, z;
  };
  std::queue<Cell> queue;

  std::vector<Cell> litCells;

  for (const auto &[wx, wy, wz] : newEmissivePositions) {
    glm::ivec3 level = lightColorForBlock(getWorldBlock(wx, wy, wz));
    glm::ivec3 existing = getWorldBlockLight(wx, wy, wz);
    glm::ivec3 combined = glm::max(existing, level);
    if (combined != existing) {
      setWorldBlockLight(wx, wy, wz, combined);
    }
    queue.push({wx, wy, wz});
    litCells.push_back({wx, wy, wz});
  }

  int baseX = chunkCX * CHUNK_SIZE_XZ;
  int baseZ = chunkCZ * CHUNK_SIZE_XZ;
  auto seedBoundaryColumn = [&](int wx, int wz) {
    for (int y = 0; y < CHUNK_HEIGHT; ++y) {
      glm::ivec3 lvl = getWorldBlockLight(wx, y, wz);
      if (lvl.x > 0 || lvl.y > 0 || lvl.z > 0) {
        queue.push({wx, y, wz});
      }
    }
  };
  for (int i = 0; i < CHUNK_SIZE_XZ; ++i) {
    seedBoundaryColumn(baseX - 1, baseZ + i);             // west neighbor
    seedBoundaryColumn(baseX + CHUNK_SIZE_XZ, baseZ + i); // east neighbor
    seedBoundaryColumn(baseX + i, baseZ - 1);             // south neighbor
    seedBoundaryColumn(baseX + i, baseZ + CHUNK_SIZE_XZ); // north neighbor
  }

  static const int offsets[6][3] = {
      {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1},
  };

  while (!queue.empty()) {
    Cell c = queue.front();
    queue.pop();
    glm::ivec3 level = getWorldBlockLight(c.x, c.y, c.z);
    if (level.x <= 1 && level.y <= 1 && level.z <= 1)
      continue;

    glm::ivec3 newLevel = glm::max(level - glm::ivec3(1), glm::ivec3(0));

    for (const auto &off : offsets) {
      int nx = c.x + off[0], ny = c.y + off[1], nz = c.z + off[2];
      if (ny < 0 || ny >= CHUNK_HEIGHT)
        continue;
      int ncx = floorDiv(nx, CHUNK_SIZE_XZ);
      int ncz = floorDiv(nz, CHUNK_SIZE_XZ);
      if (chunks.find(chunkKey(ncx, ncz)) == chunks.end())
        continue;

      BlockType nt = getWorldBlock(nx, ny, nz);
      if (!(nt == BlockType::Air || isTransparent(nt)))
        continue;

      glm::ivec3 existing = getWorldBlockLight(nx, ny, nz);
      glm::ivec3 combined = glm::max(existing, newLevel);
      if (combined != existing) {
        setWorldBlockLight(nx, ny, nz, combined);
        queue.push({nx, ny, nz});
        litCells.push_back({nx, ny, nz});
      }
    }
  }

  std::vector<glm::ivec3> blended(litCells.size());
  for (size_t i = 0; i < litCells.size(); ++i) {
    const Cell &c = litCells[i];
    glm::ivec3 sum = getWorldBlockLight(c.x, c.y, c.z);
    int count = 1;
    for (const auto &off : offsets) {
      int nx = c.x + off[0], ny = c.y + off[1], nz = c.z + off[2];
      if (ny < 0 || ny >= CHUNK_HEIGHT)
        continue;
      if (chunks.find(chunkKey(floorDiv(nx, CHUNK_SIZE_XZ),
                               floorDiv(nz, CHUNK_SIZE_XZ))) == chunks.end())
        continue;
      sum += getWorldBlockLight(nx, ny, nz);
      count++;
    }
    blended[i] = sum / count;
  }
  for (size_t i = 0; i < litCells.size(); ++i) {
    const Cell &c = litCells[i];
    glm::ivec3 current = getWorldBlockLight(c.x, c.y, c.z);
    glm::ivec3 brightened = glm::max(blended[i], current);
    if (brightened != current) {
      setWorldBlockLight(c.x, c.y, c.z, brightened);
    }
  }
}

static void uploadOneMesh(ChunkMesh &mesh, const std::vector<float> &vertices,
                          const std::vector<unsigned int> &indices) {
  bool firstUpload = (mesh.vao == 0);
  if (firstUpload) {
    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    glGenBuffers(1, &mesh.ebo);
  }

  mesh.indexCount = (GLsizei)indices.size();

  glBindVertexArray(mesh.vao);

  glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float),
               vertices.data(), GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
               indices.data(), GL_STATIC_DRAW);

  if (firstUpload) {
    const GLsizei stride = 13 * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride,
                          (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride,
                          (void *)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, stride,
                          (void *)(8 * sizeof(float)));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, stride,
                          (void *)(9 * sizeof(float)));
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, stride,
                          (void *)(10 * sizeof(float)));
    glEnableVertexAttribArray(5);
  }

  glBindVertexArray(0);
}

static void uploadGrassMeshGPU(GrassMesh &mesh,
                               const std::vector<float> &vertices) {
  bool firstUpload = (mesh.vao == 0);
  if (firstUpload) {
    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);
  }

  mesh.vertexCount = (GLsizei)(vertices.size() / 8);

  glBindVertexArray(mesh.vao);
  glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float),
               vertices.data(), GL_STATIC_DRAW);

  if (firstUpload) {
    const GLsizei stride = 8 * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride,
                          (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride,
                          (void *)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);
  }

  glBindVertexArray(0);
}

void ChunkManager::uploadChunkMesh(int cx, int cz) {
  int64_t key = chunkKey(cx, cz);
  auto chunkIt = chunks.find(key);
  if (chunkIt == chunks.end())
    return;

  std::vector<float> vertices, transparentVertices;
  std::vector<unsigned int> indices, transparentIndices;

  chunkIt->second.buildMesh(
      vertices, indices, transparentVertices, transparentIndices,
      cx * CHUNK_SIZE_XZ, cz * CHUNK_SIZE_XZ,
      [this](int wx, int wy, int wz) { return getWorldBlock(wx, wy, wz); },
      [this](int wx, int wy, int wz) { return getWorldSkyLight(wx, wy, wz); },
      [this](int wx, int wy, int wz) {
        return getWorldBlockLight(wx, wy, wz);
      });

  uploadOneMesh(chunkMeshesOpaque[key], vertices, indices);
  uploadOneMesh(chunkMeshesTransparent[key], transparentVertices,
                transparentIndices);
}

void ChunkManager::uploadGrassMesh(int cx, int cz) {
  int64_t key = chunkKey(cx, cz);
  auto chunkIt = chunks.find(key);
  if (chunkIt == chunks.end())
    return;

  std::vector<float> vertices;
  chunkIt->second.buildGrassMesh(vertices, biomes, cx * CHUNK_SIZE_XZ,
                                 cz * CHUNK_SIZE_XZ);
  uploadGrassMeshGPU(chunkGrassMeshes[key], vertices);
}

void ChunkManager::setBlock(int worldX, int worldY, int worldZ,
                            BlockType type) {
  int cx = floorDiv(worldX, CHUNK_SIZE_XZ);
  int cz = floorDiv(worldZ, CHUNK_SIZE_XZ);

  auto it = chunks.find(chunkKey(cx, cz));
  if (it == chunks.end())
    return;

  int localX = worldX - cx * CHUNK_SIZE_XZ;
  int localZ = worldZ - cz * CHUNK_SIZE_XZ;
  BlockType oldType = it->second.getBlock(localX, worldY, localZ);
  it->second.setBlock(localX, worldY, localZ, type);

  bool hadLights = !emissiveBlockPositions.empty();
  if (isEmissive(oldType) && !isEmissive(type)) {
    emissiveBlockPositions.erase({worldX, worldY, worldZ});
  } else if (!isEmissive(oldType) && isEmissive(type)) {
    emissiveBlockPositions.insert({worldX, worldY, worldZ});
  }

  it->second.computeSkyLight();
  if (hadLights || !emissiveBlockPositions.empty()) {
    recomputeBlockLight();
  }

  uploadChunkMesh(cx, cz);
  uploadGrassMesh(cx, cz);

  if (localX == 0)
    uploadChunkMesh(cx - 1, cz);
  if (localX == CHUNK_SIZE_XZ - 1)
    uploadChunkMesh(cx + 1, cz);
  if (localZ == 0)
    uploadChunkMesh(cx, cz - 1);
  if (localZ == CHUNK_SIZE_XZ - 1)
    uploadChunkMesh(cx, cz + 1);
}

void ChunkManager::loadAllPending() {
  while (!pendingChunkLoads.empty()) {
    auto [cx, cz] = pendingChunkLoads.front();
    pendingChunkLoads.pop_front();
    pendingChunkKeys.erase(chunkKey(cx, cz));
    if (chunks.find(chunkKey(cx, cz)) == chunks.end()) {
      loadChunk(cx, cz);
    }
  }
}
