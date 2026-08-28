#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "BiomeMap.h"
#include "BlockType.h"
#include "Camera.h"
#include "ChunkManager.h"
#include "Frustum.h"
#include "Physics.h"
#include "Raycast.h"
#include "Shader.h"
#include "Texture.h"

/////////////////////// TODO //////////////////////
/// Maybe, MAYBE add a Logger system. I am thinking using that of graph_viz
/// ///

const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;

static const BlockType PLACEABLE_BLOCKS[36] = {
    BlockType::Grass,        BlockType::Dirt,         BlockType::Stone,
    BlockType::Sand,         BlockType::OakLog,       BlockType::OakLeaves,
    BlockType::Glass,        BlockType::Plank,        BlockType::Light,
    BlockType::Iron,         BlockType::PolishedIron,

    BlockType::Clay,         BlockType::Gravel,       BlockType::Ice,
    BlockType::Sandstone,    BlockType::Magma,

    BlockType::SpruceLog,    BlockType::SpruceLeaves, BlockType::BirchLog,
    BlockType::BirchLeaves,  BlockType::AcaciaLog,    BlockType::AcaciaLeaves,
    BlockType::JungleLog,    BlockType::JungleLeaves, BlockType::CherryLog,
    BlockType::CherryLeaves, BlockType::MangroveLog,  BlockType::MangroveLeaves,
    BlockType::CoalOre,      BlockType::IronOre,      BlockType::CopperOre,
    BlockType::GoldOre,      BlockType::DiamondOre,   BlockType::EmeraldOre,
    BlockType::LithiumOre,   BlockType::UraniumOre,
};

static const char *blockName(BlockType type) {
  switch (type) {
  case BlockType::Grass:
    return "Grass";
  case BlockType::Dirt:
    return "Dirt";
  case BlockType::Stone:
    return "Stone";
  case BlockType::Sand:
    return "Sand";
  case BlockType::OakLog:
    return "Oak Log";
  case BlockType::OakLeaves:
    return "Oak Leaves";
  case BlockType::Glass:
    return "Glass";
  case BlockType::Plank:
    return "Plank";
  case BlockType::Light:
    return "Light";
  case BlockType::Iron:
    return "Iron";
  case BlockType::PolishedIron:
    return "Smooth Iron";
  case BlockType::Clay:
    return "Clay";
  case BlockType::Gravel:
    return "Gravel";
  case BlockType::Ice:
    return "Ice";
  case BlockType::Sandstone:
    return "Sandstone";
  case BlockType::Magma:
    return "Magma";
  case BlockType::SpruceLog:
    return "Spruce Log";
  case BlockType::SpruceLeaves:
    return "Spruce Leaves";
  case BlockType::BirchLog:
    return "Birch Log";
  case BlockType::BirchLeaves:
    return "Birch Leaves";
  case BlockType::AcaciaLog:
    return "Acacia Log";
  case BlockType::AcaciaLeaves:
    return "Acacia Leaves";
  case BlockType::JungleLog:
    return "Jungle Log";
  case BlockType::JungleLeaves:
    return "Jungle Leaves";
  case BlockType::CherryLog:
    return "Cherry Log";
  case BlockType::CherryLeaves:
    return "Cherry Leaves";
  case BlockType::MangroveLog:
    return "Mangrove Log";
  case BlockType::MangroveLeaves:
    return "Mangrove Leaves";
  case BlockType::CoalOre:
    return "Coal Ore";
  case BlockType::IronOre:
    return "Iron Ore";
  case BlockType::CopperOre:
    return "Copper Ore";
  case BlockType::GoldOre:
    return "Gold Ore";
  case BlockType::DiamondOre:
    return "Diamond Ore";
  case BlockType::EmeraldOre:
    return "Emerald Ore";
  case BlockType::LithiumOre:
    return "Lithium Ore";
  case BlockType::UraniumOre:
    return "Uranium Ore";

  default:
    return "Air";
  }
}

static int iconTileForBlock(BlockType type) {
  return tileForBlockFace(type, FaceDir::NegZ);
}

static const char *biomeName(BiomeType type) {
  switch (type) {
  case BiomeType::Desert:
    return "Desert";
  case BiomeType::Savanna:
    return "Savanna";
  case BiomeType::Plains:
    return "Plains";
  case BiomeType::Forest:
    return "Forest";
  case BiomeType::Jungle:
    return "Jungle";
  case BiomeType::Swamp:
    return "Swamp";
  case BiomeType::Taiga:
    return "Taiga";
  case BiomeType::CherryGrove:
    return "Cherry Grove";
  case BiomeType::Tundra:
    return "Tundra";
  case BiomeType::Mountains:
    return "Mountains";
  default:
    return "Unknown";
  }
}

static const BiomeType BIOME_CYCLE[10] = {
    BiomeType::Desert,    BiomeType::Savanna,     BiomeType::Plains,
    BiomeType::Forest,    BiomeType::Jungle,      BiomeType::Swamp,
    BiomeType::Taiga,     BiomeType::CherryGrove, BiomeType::Tundra,
    BiomeType::Mountains,
};

static bool isDeepInBiome(const ChunkManager &cm, float x, float z,
                          BiomeType target, float margin) {
  if (cm.getBiomeAt(x, z) != target)
    return false;
  static const float dirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
  for (const auto &d : dirs) {
    if (cm.getBiomeAt(x + d[0] * margin, z + d[1] * margin) != target)
      return false;
  }
  return true;
}

static bool findNearestBiome(const ChunkManager &cm, float fromX, float fromZ,
                             BiomeType target, float &outX, float &outZ) {
  constexpr float INTERIOR_MARGIN = 32.0f;
  if (isDeepInBiome(cm, fromX, fromZ, target, INTERIOR_MARGIN)) {
    outX = fromX;
    outZ = fromZ;
    return true;
  }

  constexpr float RING_STEP = 16.0f;
  constexpr float MAX_RADIUS = 6000.0f;
  constexpr int SAMPLES_PER_RING = 32;

  for (float r = RING_STEP; r <= MAX_RADIUS; r += RING_STEP) {
    for (int i = 0; i < SAMPLES_PER_RING; ++i) {
      float angle = (2.0f * 3.14159265f * (float)i) / (float)SAMPLES_PER_RING;
      float x = fromX + std::cos(angle) * r;
      float z = fromZ + std::sin(angle) * r;
      if (isDeepInBiome(cm, x, z, target, INTERIOR_MARGIN)) {
        outX = x;
        outZ = z;
        return true;
      }
    }
  }
  return false;
}

static void buildCrosshairMesh(GLuint &vao, GLuint &vbo) {
  const float armHalfLen = 0.02f;
  const float thickness = 0.0025f;

  float verts[] = {
      -armHalfLen, -thickness,  armHalfLen, -thickness,  armHalfLen,
      thickness,   -armHalfLen, -thickness, armHalfLen,  thickness,
      -armHalfLen, thickness,   -thickness, -armHalfLen, thickness,
      -armHalfLen, thickness,   armHalfLen, -thickness,  -armHalfLen,
      thickness,   armHalfLen,  -thickness, armHalfLen,
  };

  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  glBindVertexArray(0);
}

static void buildHighlightMesh(GLuint &vao, GLuint &vbo, GLuint &fillEbo,
                               GLuint &edgeEbo) {
  const float e = 0.0025f; // outward inset, in world units

  float verts[] = {
      -e,    -e,    -e,    // 0
      1 + e, -e,    -e,    // 1
      1 + e, 1 + e, -e,    // 2
      -e,    1 + e, -e,    // 3
      -e,    -e,    1 + e, // 4
      1 + e, -e,    1 + e, // 5
      1 + e, 1 + e, 1 + e, // 6
      -e,    1 + e, 1 + e, // 7
  };
  unsigned int fillIdx[] = {
      0, 1, 2, 2, 3, 0, // back  (-z)
      5, 4, 7, 7, 6, 5, // front (+z)
      4, 0, 3, 3, 7, 4, // left  (-x)
      1, 5, 6, 6, 2, 1, // right (+x)
      4, 5, 1, 1, 0, 4, // bottom (-y)
      3, 2, 6, 6, 7, 3, // top    (+y)
  };
  unsigned int edgeIdx[] = {
      0, 1, 1, 2, 2, 3, 3, 0, // bottom face edges
      4, 5, 5, 6, 6, 7, 7, 4, // top face edges
      0, 4, 1, 5, 2, 6, 3, 7, // vertical edges
  };

  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);
  glGenBuffers(1, &fillEbo);
  glGenBuffers(1, &edgeEbo);
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, fillEbo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(fillIdx), fillIdx,
               GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  glBindVertexArray(0);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, edgeEbo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(edgeIdx), edgeIdx,
               GL_STATIC_DRAW);
}

constexpr int SHADOW_MAP_SIZE = 2048;

static void initShadowMap(GLuint &shadowFBO, GLuint &shadowMapTexture) {
  glGenFramebuffers(1, &shadowFBO);

  glGenTextures(1, &shadowMapTexture);
  glBindTexture(GL_TEXTURE_2D, shadowMapTexture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_MAP_SIZE,
               SHADOW_MAP_SIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

  float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
  glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

  glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                         shadowMapTexture, 0);
  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    std::cerr << "Shadow map framebuffer incomplete!\n";
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void createGBuffer(int width, int height, GLuint &fbo, GLuint &posTex,
                          GLuint &normalTex, GLuint &albedoTex,
                          GLuint &blockLightTex, GLuint &depthRBO) {
  glGenFramebuffers(1, &fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);

  auto makeColorTarget = [&](GLuint &tex, GLint internalFormat, GLenum format,
                             GLenum type, GLenum attachment) {
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format,
                 type, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, tex, 0);
  };

  makeColorTarget(posTex, GL_RGBA16F, GL_RGBA, GL_FLOAT, GL_COLOR_ATTACHMENT0);
  makeColorTarget(normalTex, GL_RGBA16F, GL_RGBA, GL_FLOAT,
                  GL_COLOR_ATTACHMENT1);
  makeColorTarget(albedoTex, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE,
                  GL_COLOR_ATTACHMENT2);
  makeColorTarget(blockLightTex, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE,
                  GL_COLOR_ATTACHMENT3);

  GLenum drawBuffers[4] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1,
                           GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3};
  glDrawBuffers(4, drawBuffers);

  glGenRenderbuffers(1, &depthRBO);
  glBindRenderbuffer(GL_RENDERBUFFER, depthRBO);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                            GL_RENDERBUFFER, depthRBO);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    std::cerr << "G-buffer framebuffer incomplete!\n";
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void destroyGBuffer(GLuint &fbo, GLuint &posTex, GLuint &normalTex,
                           GLuint &albedoTex, GLuint &blockLightTex,
                           GLuint &depthRBO) {
  glDeleteFramebuffers(1, &fbo);
  glDeleteTextures(1, &posTex);
  glDeleteTextures(1, &normalTex);
  glDeleteTextures(1, &albedoTex);
  glDeleteTextures(1, &blockLightTex);
  glDeleteRenderbuffers(1, &depthRBO);
}

static void createSingleChannelTarget(int width, int height, GLuint &fbo,
                                      GLuint &tex) {
  glGenFramebuffers(1, &fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_FLOAT,
               nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         tex, 0);
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    std::cerr << "SSAO framebuffer incomplete!\n";
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void destroySingleChannelTarget(GLuint &fbo, GLuint &tex) {
  glDeleteFramebuffers(1, &fbo);
  glDeleteTextures(1, &tex);
}

static std::vector<glm::vec3> generateSSAOKernel() {
  std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);
  std::default_random_engine generator;
  std::vector<glm::vec3> kernel;
  kernel.reserve(32);
  for (int i = 0; i < 32; ++i) {
    glm::vec3 sample(randomFloats(generator) * 2.0f - 1.0f,
                     randomFloats(generator) * 2.0f - 1.0f,
                     randomFloats(generator));
    sample = glm::normalize(sample) * randomFloats(generator);
    float scale = (float)i / 32.0f;
    scale = glm::mix(0.1f, 1.0f, scale * scale);
    kernel.push_back(sample * scale);
  }
  return kernel;
}

static GLuint createSSAONoiseTexture() {
  std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);
  std::default_random_engine generator;
  std::vector<glm::vec3> noise;
  noise.reserve(16);
  for (int i = 0; i < 16; ++i) {
    noise.emplace_back(randomFloats(generator) * 2.0f - 1.0f,
                       randomFloats(generator) * 2.0f - 1.0f, 0.0f);
  }
  GLuint tex;
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, 4, 4, 0, GL_RGB, GL_FLOAT,
               noise.data());
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  return tex;
}

struct DayNightState {
  glm::vec3 lightDir;
  glm::vec3 lightColor;
  float skyLightFactor;
  glm::vec3 skyColor;
};

static DayNightState computeDayNight(float dayTime, float dayLength) {
  float t = dayTime / dayLength;
  float sunAngle = t * 2.0f * 3.14159265f;
  glm::vec3 sunDir =
      glm::normalize(glm::vec3(std::cos(sunAngle), std::sin(sunAngle), 0.25f));
  float sunElevation = sunDir.y; // 1 = straight up, 0 = horizon, <0 = below

  float dayFactor = glm::clamp((sunElevation + 0.1f) / 0.4f, 0.0f, 1.0f);

  DayNightState s;
  s.lightDir = -sunDir;
  glm::vec3 moonColor(0.15f, 0.18f, 0.28f);
  glm::vec3 dayColor(0.95f, 0.9f, 0.82f);

  s.lightColor = glm::mix(moonColor, dayColor, dayFactor) *
                 glm::mix(0.15f, 1.0f, dayFactor);
  s.skyLightFactor = dayFactor;
  glm::vec3 nightSky(0.02f, 0.02f, 0.06f);
  glm::vec3 daySky(0.53f, 0.81f, 0.92f);
  s.skyColor = glm::mix(nightSky, daySky, dayFactor);
  return s;
}

static bool initSDLAndGL(SDL_Window *&window, SDL_GLContext &glContext) {
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
    return false;
  }

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

  window = SDL_CreateWindow("Minecraft Clone", SDL_WINDOWPOS_CENTERED,
                            SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT,
                            SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
  if (!window) {
    std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << "\n";
    return false;
  }

  glContext = SDL_GL_CreateContext(window);
  if (!glContext) {
    std::cerr << "SDL_GL_CreateContext failed: " << SDL_GetError() << "\n";
    return false;
  }

  SDL_GL_SetSwapInterval(1);

  glewExperimental = GL_TRUE;
  GLenum glewErr = glewInit();
  if (glewErr != GLEW_OK) {
    std::cerr << "glewInit failed: " << glewGetErrorString(glewErr) << "\n";
    return false;
  }
  glGetError();

  SDL_SetRelativeMouseMode(SDL_TRUE);

  std::cout << "OpenGL version: " << glGetString(GL_VERSION) << "\n";
  std::cout << "GLSL version:   " << glGetString(GL_SHADING_LANGUAGE_VERSION)
            << "\n";

  glEnable(GL_DEPTH_TEST);
  glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

  return true;
}

int main() {
  SDL_Window *window = nullptr;
  SDL_GLContext glContext = nullptr;

  if (!initSDLAndGL(window, glContext)) {
    return 1;
  }

  Camera camera(glm::vec3(8.0f, 30.0f, 8.0f));

  ChunkManager chunkManager;
  chunkManager.update(camera.position);
  chunkManager.loadAllPending();

  PlayerBounds playerBounds;
  glm::vec3 feetPos(8.5f, 0.0f, 8.5f);
  {
    int surfaceY = 0;
    for (int y = CHUNK_HEIGHT - 1; y >= 0; --y) {
      if (chunkManager.getWorldBlock(8, y, 8) != BlockType::Air) {
        surfaceY = y;
        break;
      }
    }
    feetPos.y = (float)(surfaceY + 1);
  }
  camera.position = feetPos + glm::vec3(0.0f, playerBounds.eyeHeight, 0.0f);

  float velocityY = 0.0f;
  bool grounded = false;
  bool flyMode = false;
  const float GRAVITY = 25.0f;

  const float JUMP_SPEED = 8.0f;
  const float TERMINAL_VELOCITY = -50.0f;
  std::cout << "Walk mode. Press F to toggle fly mode.\n";
  std::cout << "[DEBUG] Press B to teleport to the next biome.\n";

  GLsizei totalIndices = 0;
  for (const auto &[key, mesh] : chunkManager.opaqueMeshes())
    totalIndices += mesh.indexCount;
  for (const auto &[key, mesh] : chunkManager.transparentMeshes())
    totalIndices += mesh.indexCount;
  std::cout << "World: render distance " << RENDER_DISTANCE << " chunks, "
            << chunkManager.loadedChunkCount() << " chunks loaded, "
            << (totalIndices / 6) << " total visible faces\n";

  Shader transparentShader("shaders/basic.vert", "shaders/basic.frag");
  Shader grassShader("shaders/grass.vert", "shaders/grass.frag");
  Texture atlas("assets/atlas.png");

  Shader shadowShader("shaders/shadow.vert", "shaders/shadow.frag");
  GLuint shadowFBO, shadowMapTexture;
  initShadowMap(shadowFBO, shadowMapTexture);

  Shader gbufferShader("shaders/gbuffer.vert", "shaders/gbuffer.frag");
  Shader ssaoShader("shaders/quad.vert", "shaders/ssao.frag");
  Shader ssaoBlurShader("shaders/quad.vert", "shaders/ssao_blur.frag");
  Shader deferredLightingShader("shaders/quad.vert",
                                "shaders/deferred_lighting.frag");

  int fbWidth = WINDOW_WIDTH, fbHeight = WINDOW_HEIGHT;

  GLuint gBufferFBO, gPositionTex, gNormalTex, gAlbedoTex, gBlockLightTex,
      gDepthRBO;
  createGBuffer(fbWidth, fbHeight, gBufferFBO, gPositionTex, gNormalTex,
                gAlbedoTex, gBlockLightTex, gDepthRBO);
  GLuint ssaoFBO, ssaoColorTex;
  createSingleChannelTarget(fbWidth, fbHeight, ssaoFBO, ssaoColorTex);
  GLuint ssaoBlurFBO, ssaoBlurTex;
  createSingleChannelTarget(fbWidth, fbHeight, ssaoBlurFBO, ssaoBlurTex);

  std::vector<glm::vec3> ssaoKernel = generateSSAOKernel();
  GLuint ssaoNoiseTex = createSSAONoiseTexture();

  GLuint quadVAO;
  glGenVertexArrays(1, &quadVAO);

  gbufferShader.use();
  gbufferShader.setInt("uTexture", 0);

  ssaoShader.use();
  ssaoShader.setInt("gPosition", 0);
  ssaoShader.setInt("gNormal", 1);
  ssaoShader.setInt("uNoiseTex", 2);
  for (int i = 0; i < (int)ssaoKernel.size(); ++i) {
    ssaoShader.setVec3("uSamples[" + std::to_string(i) + "]", ssaoKernel[i]);
  }

  ssaoBlurShader.use();
  ssaoBlurShader.setInt("uSSAOInput", 0);

  deferredLightingShader.use();
  deferredLightingShader.setInt("gPosition", 0);
  deferredLightingShader.setInt("gNormal", 1);
  deferredLightingShader.setInt("gAlbedoEmissive", 2);
  deferredLightingShader.setInt("uSSAO", 3);
  deferredLightingShader.setInt("uShadowMap", 4);
  deferredLightingShader.setInt("gBlockLight", 5);

  float dayTime = 0.0f;
  const float DAY_LENGTH = 600.0f;

  Shader crosshairShader("shaders/crosshair.vert", "shaders/crosshair.frag");
  GLuint crosshairVao, crosshairVbo;
  buildCrosshairMesh(crosshairVao, crosshairVbo);

  Shader highlightShader("shaders/highlight.vert", "shaders/highlight.frag");
  GLuint highlightVao, highlightVbo, highlightFillEbo, highlightEdgeEbo;
  buildHighlightMesh(highlightVao, highlightVbo, highlightFillEbo,
                     highlightEdgeEbo);

  Shader uiIconShader("shaders/ui_icon.vert", "shaders/ui_icon.frag");
  GLuint uiIconVao, uiIconVbo;
  {
    float verts[] = {
        // pos.x, pos.y, uv.x, uv.y
        0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
        0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
    };
    glGenVertexArrays(1, &uiIconVao);
    glGenBuffers(1, &uiIconVbo);
    glBindVertexArray(uiIconVao);
    glBindBuffer(GL_ARRAY_BUFFER, uiIconVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          (void *)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
  }

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_BLEND);

  int selectedBlockIndex = 2; // Stone
  std::cout << "Selected block: "
            << blockName(PLACEABLE_BLOCKS[selectedBlockIndex]) << "\n";

  bool running = true;
  SDL_Event event;
  Uint32 lastTicks = SDL_GetTicks();

  SDL_GetRelativeMouseState(nullptr, nullptr);

  while (running) {
    Uint32 nowTicks = SDL_GetTicks();
    float dt = (nowTicks - lastTicks) / 1000.0f;
    lastTicks = nowTicks;

    dayTime += dt;
    if (dayTime > DAY_LENGTH)
      dayTime -= DAY_LENGTH;
    DayNightState dayNight = computeDayNight(dayTime, DAY_LENGTH);

    bool leftClicked = false, rightClicked = false, jumpPressed = false;
    bool biomeJumpPressed = false;

    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        running = false;
      } else if (event.type == SDL_KEYDOWN &&
                 event.key.keysym.sym == SDLK_ESCAPE) {
        running = false;
      } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_f) {
        flyMode = !flyMode;
        velocityY = 0.0f;
        std::cout << (flyMode ? "Fly mode ON\n" : "Fly mode OFF (walking)\n");
      } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_b) {
        biomeJumpPressed = true;
      } else if (event.type == SDL_KEYDOWN &&
                 event.key.keysym.sym == SDLK_SPACE) {
        jumpPressed = true;
      } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym >= SDLK_1 &&
                 event.key.keysym.sym <= SDLK_9) {
        selectedBlockIndex = event.key.keysym.sym - SDLK_1;
        std::cout << "Selected block: "
                  << blockName(PLACEABLE_BLOCKS[selectedBlockIndex]) << "\n";
      } else if (event.type == SDL_MOUSEWHEEL) {
        constexpr int numPlaceable =
            (int)(sizeof(PLACEABLE_BLOCKS) / sizeof(PLACEABLE_BLOCKS[0]));
        selectedBlockIndex =
            ((selectedBlockIndex - event.wheel.y) % numPlaceable +
             numPlaceable) %
            numPlaceable;
        std::cout << "Selected block: "
                  << blockName(PLACEABLE_BLOCKS[selectedBlockIndex]) << "\n";
      } else if (event.type == SDL_WINDOWEVENT &&
                 event.window.event == SDL_WINDOWEVENT_RESIZED) {
        fbWidth = event.window.data1;
        fbHeight = event.window.data2;
        glViewport(0, 0, fbWidth, fbHeight);
        destroyGBuffer(gBufferFBO, gPositionTex, gNormalTex, gAlbedoTex,
                       gBlockLightTex, gDepthRBO);
        createGBuffer(fbWidth, fbHeight, gBufferFBO, gPositionTex, gNormalTex,
                      gAlbedoTex, gBlockLightTex, gDepthRBO);
        destroySingleChannelTarget(ssaoFBO, ssaoColorTex);
        createSingleChannelTarget(fbWidth, fbHeight, ssaoFBO, ssaoColorTex);
        destroySingleChannelTarget(ssaoBlurFBO, ssaoBlurTex);
        createSingleChannelTarget(fbWidth, fbHeight, ssaoBlurFBO, ssaoBlurTex);
      } else if (event.type == SDL_MOUSEBUTTONDOWN) {
        if (event.button.button == SDL_BUTTON_LEFT)
          leftClicked = true;
        else if (event.button.button == SDL_BUTTON_RIGHT)
          rightClicked = true;
      }
    }

    int mouseDx = 0, mouseDy = 0;
    SDL_GetRelativeMouseState(&mouseDx, &mouseDy);
    camera.processMouseMovement((float)mouseDx, (float)mouseDy);

    const Uint8 *keys = SDL_GetKeyboardState(nullptr);

    if (flyMode) {
      camera.processKeyboard(keys[SDL_SCANCODE_W], keys[SDL_SCANCODE_S],
                             keys[SDL_SCANCODE_A], keys[SDL_SCANCODE_D],
                             keys[SDL_SCANCODE_SPACE],
                             keys[SDL_SCANCODE_LSHIFT], dt);
      feetPos = camera.position - glm::vec3(0.0f, playerBounds.eyeHeight, 0.0f);
    } else {
      glm::vec3 flatFront =
          glm::normalize(glm::vec3(camera.front.x, 0.0f, camera.front.z));
      glm::vec3 flatRight =
          glm::normalize(glm::vec3(camera.right.x, 0.0f, camera.right.z));

      glm::vec3 wishDir(0.0f);
      if (keys[SDL_SCANCODE_W])
        wishDir += flatFront;
      if (keys[SDL_SCANCODE_S])
        wishDir -= flatFront;
      if (keys[SDL_SCANCODE_D])
        wishDir += flatRight;
      if (keys[SDL_SCANCODE_A])
        wishDir -= flatRight;
      if (glm::length(wishDir) > 0.0001f)
        wishDir = glm::normalize(wishDir);

      glm::vec3 horizontalDelta = wishDir * camera.movementSpeed * dt;
      moveAxisWithCollision(chunkManager, feetPos, playerBounds, 0,
                            horizontalDelta.x);
      moveAxisWithCollision(chunkManager, feetPos, playerBounds, 2,
                            horizontalDelta.z);

      if (grounded && jumpPressed) {
        velocityY = JUMP_SPEED;
        grounded = false;
      }

      velocityY = std::max(velocityY - GRAVITY * dt, TERMINAL_VELOCITY);
      bool verticalCollision = moveAxisWithCollision(
          chunkManager, feetPos, playerBounds, 1, velocityY * dt);
      if (verticalCollision) {
        if (velocityY < 0.0f)
          grounded = true;
        velocityY = 0.0f;

      } else {
        grounded = false;
      }

      camera.position = feetPos + glm::vec3(0.0f, playerBounds.eyeHeight, 0.0f);
    }
    if (biomeJumpPressed) {
      BiomeType current = chunkManager.getBiomeAt(feetPos.x, feetPos.z);
      int curIdx = 0;
      for (int i = 0; i < 10; ++i) {
        if (BIOME_CYCLE[i] == current) {
          curIdx = i;
          break;
        }
      }
      bool teleported = false;
      for (int offset = 1; offset <= 9 && !teleported; ++offset) {
        BiomeType target = BIOME_CYCLE[(curIdx + offset) % 10];
        float destX, destZ;
        if (findNearestBiome(chunkManager, feetPos.x, feetPos.z, target, destX,
                             destZ)) {
          int surfaceY = chunkManager.estimateSurfaceHeight(destX, destZ);
          feetPos = glm::vec3(destX, (float)(surfaceY + 1), destZ);
          camera.position =
              feetPos + glm::vec3(0.0f, playerBounds.eyeHeight, 0.0f);
          velocityY = 0.0f;
          grounded = false;
          std::cout << "Teleported to " << biomeName(target) << " at ("
                    << (int)destX << ", " << (int)destZ << ")\n";
          teleported = true;
        }
      }
      if (!teleported) {
        std::cout << "Couldn't find any other biome within search radius "
                     "— this seed may be unusually uniform nearby.\n";
      }
    }

    chunkManager.update(camera.position);

    if (biomeJumpPressed) {
      chunkManager.loadAllPending();
    }

    RaycastHit currentHit =
        raycastBlocks(chunkManager, camera.position, camera.front);

    if (currentHit.hit && leftClicked) {
      chunkManager.setBlock(currentHit.blockPos.x, currentHit.blockPos.y,
                            currentHit.blockPos.z, BlockType::Air);
      currentHit.hit = false;
    } else if (currentHit.hit && rightClicked) {
      chunkManager.setBlock(currentHit.placePos.x, currentHit.placePos.y,
                            currentHit.placePos.z,
                            PLACEABLE_BLOCKS[selectedBlockIndex]);
    }

    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 projection = glm::perspective(
        glm::radians(70.0f), (float)fbWidth / (float)fbHeight, 0.1f, 400.0f);

    Frustum frustum;
    frustum.update(projection * view);

    const float shadowOrthoHalfSize = 40.0f;
    glm::vec3 lightTarget = camera.position;
    glm::vec3 lightPos = lightTarget - dayNight.lightDir * 100.0f;
    glm::mat4 lightView =
        glm::lookAt(lightPos, lightTarget, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 lightProjection =
        glm::ortho(-shadowOrthoHalfSize, shadowOrthoHalfSize,
                   -shadowOrthoHalfSize, shadowOrthoHalfSize, 1.0f, 300.0f);
    glm::mat4 lightSpaceMatrix = lightProjection * lightView;

    glViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    shadowShader.use();
    shadowShader.setMat4("uLightSpaceMatrix", lightSpaceMatrix);
    shadowShader.setMat4("uModel", glm::mat4(1.0f));
    for (const auto &[key, mesh] : chunkManager.opaqueMeshes()) {
      glBindVertexArray(mesh.vao);
      glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0);
    }
    for (const auto &[key, mesh] : chunkManager.transparentMeshes()) {
      glBindVertexArray(mesh.vao);
      glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0);
    }
    glBindVertexArray(0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, fbWidth, fbHeight);
    glDisable(GL_BLEND);
    glBindFramebuffer(GL_FRAMEBUFFER, gBufferFBO);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    gbufferShader.use();
    gbufferShader.setMat4("uView", view);
    gbufferShader.setMat4("uProjection", projection);
    gbufferShader.setMat4("uModel", glm::mat4(1.0f));
    atlas.bind(0);

    int chunksDrawn = 0, chunksCulled = 0;
    for (const auto &[key, mesh] : chunkManager.opaqueMeshes()) {
      int cx = (int)(key >> 32);
      int cz = (int)(key & 0xFFFFFFFF);
      glm::vec3 aabbMin(cx * CHUNK_SIZE_XZ, 0.0f, cz * CHUNK_SIZE_XZ);
      glm::vec3 aabbMax(aabbMin.x + CHUNK_SIZE_XZ, (float)CHUNK_HEIGHT,
                        aabbMin.z + CHUNK_SIZE_XZ);
      if (!frustum.intersectsAABB(aabbMin, aabbMax)) {
        chunksCulled++;
        continue;
      }
      chunksDrawn++;
      glBindVertexArray(mesh.vao);
      glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0);
    }
    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
    glClear(GL_COLOR_BUFFER_BIT);
    ssaoShader.use();
    ssaoShader.setMat4("uProjection", projection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gPositionTex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gNormalTex);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, ssaoNoiseTex);
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
    glClear(GL_COLOR_BUFFER_BIT);
    ssaoBlurShader.use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ssaoColorTex);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glClearColor(dayNight.skyColor.r, dayNight.skyColor.g, dayNight.skyColor.b,
                 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    deferredLightingShader.use();
    deferredLightingShader.setMat4("uInvView", glm::inverse(view));
    deferredLightingShader.setMat4("uLightSpaceMatrix", lightSpaceMatrix);
    deferredLightingShader.setVec3("uLightDir", dayNight.lightDir);
    glm::vec3 lightDirView =
        glm::vec3(view * glm::vec4(dayNight.lightDir, 0.0f));
    deferredLightingShader.setVec3("uLightDirView", lightDirView);
    deferredLightingShader.setVec3("uLightColor", dayNight.lightColor);
    deferredLightingShader.setFloat("uSkyLightFactor", dayNight.skyLightFactor);

    {
      const auto &lights = chunkManager.getEmissiveBlockPositions();
      std::vector<std::pair<float, glm::vec3>> byDistance;
      byDistance.reserve(lights.size());
      for (const auto &[lx, ly, lz] : lights) {
        glm::vec3 worldPos((float)lx + 0.5f, (float)ly + 0.5f,
                           (float)lz + 0.5f);
        glm::vec3 diff = worldPos - camera.position;
        byDistance.push_back({glm::dot(diff, diff), worldPos});
      }
      std::sort(byDistance.begin(), byDistance.end(),
                [](const auto &a, const auto &b) { return a.first < b.first; });

      constexpr int MAX_POINT_LIGHTS = 4;
      int numPointLights = std::min((int)byDistance.size(), MAX_POINT_LIGHTS);
      for (int i = 0; i < numPointLights; ++i) {
        glm::vec3 posView =
            glm::vec3(view * glm::vec4(byDistance[i].second, 1.0f));
        deferredLightingShader.setVec3(
            "uPointLightPosView[" + std::to_string(i) + "]", posView);
      }
      deferredLightingShader.setInt("uNumPointLights", numPointLights);
    }
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gPositionTex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gNormalTex);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gAlbedoTex);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, ssaoBlurTex);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, shadowMapTexture);
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, gBlockLightTex);

    glDisable(GL_DEPTH_TEST);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, gBufferFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, fbWidth, fbHeight, 0, 0, fbWidth, fbHeight,
                      GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    grassShader.use();
    grassShader.setMat4("uView", view);
    grassShader.setMat4("uProjection", projection);
    grassShader.setMat4("uModel", glm::mat4(1.0f));
    grassShader.setInt("uTexture", 0);
    atlas.bind(0);
    glm::vec3 grassAmbient = glm::mix(glm::vec3(0.22f, 0.24f, 0.28f),
                                      glm::vec3(1.0f), dayNight.skyLightFactor);
    grassShader.setVec3("uAmbientColor", grassAmbient);
    for (const auto &[key, mesh] : chunkManager.grassMeshes()) {
      if (mesh.vertexCount == 0)
        continue;
      int cx = (int)(key >> 32);
      int cz = (int)(key & 0xFFFFFFFF);
      glm::vec3 aabbMin(cx * CHUNK_SIZE_XZ, 0.0f, cz * CHUNK_SIZE_XZ);
      glm::vec3 aabbMax(aabbMin.x + CHUNK_SIZE_XZ, (float)CHUNK_HEIGHT,
                        aabbMin.z + CHUNK_SIZE_XZ);
      if (!frustum.intersectsAABB(aabbMin, aabbMax))
        continue;
      glBindVertexArray(mesh.vao);
      glDrawArrays(GL_TRIANGLES, 0, mesh.vertexCount);
    }
    glBindVertexArray(0);

    glEnable(GL_BLEND);
    transparentShader.use();
    transparentShader.setMat4("uView", view);
    transparentShader.setMat4("uProjection", projection);
    transparentShader.setMat4("uLightSpaceMatrix", lightSpaceMatrix);
    transparentShader.setVec3("uLightDir", dayNight.lightDir);
    transparentShader.setVec3("uLightColor", dayNight.lightColor);
    transparentShader.setFloat("uSkyLightFactor", dayNight.skyLightFactor);
    transparentShader.setInt("uTexture", 0);
    transparentShader.setInt("uShadowMap", 1);
    transparentShader.setMat4("uModel", glm::mat4(1.0f));
    atlas.bind(0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, shadowMapTexture);

    glDepthMask(GL_FALSE);
    for (const auto &[key, mesh] : chunkManager.transparentMeshes()) {
      if (mesh.indexCount == 0)
        continue;
      int cx = (int)(key >> 32);
      int cz = (int)(key & 0xFFFFFFFF);
      glm::vec3 aabbMin(cx * CHUNK_SIZE_XZ, 0.0f, cz * CHUNK_SIZE_XZ);
      glm::vec3 aabbMax(aabbMin.x + CHUNK_SIZE_XZ, (float)CHUNK_HEIGHT,
                        aabbMin.z + CHUNK_SIZE_XZ);
      if (!frustum.intersectsAABB(aabbMin, aabbMax))
        continue;
      glBindVertexArray(mesh.vao);
      glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0);
    }
    glDepthMask(GL_TRUE);
    glBindVertexArray(0);

    static Uint32 lastStatsPrint = 0;
    if (nowTicks - lastStatsPrint > 2000) {
      lastStatsPrint = nowTicks;
      std::cout << "Chunks loaded: " << chunkManager.loadedChunkCount()
                << " | drawn: " << chunksDrawn
                << " | frustum-culled: " << chunksCulled << "\n";
    }

    if (currentHit.hit) {
      highlightShader.use();
      glm::mat4 highlightModel =
          glm::translate(glm::mat4(1.0f), glm::vec3(currentHit.blockPos));
      highlightShader.setMat4("uModel", highlightModel);
      highlightShader.setMat4("uView", view);
      highlightShader.setMat4("uProjection", projection);
      glDepthMask(GL_FALSE);
      glBindVertexArray(highlightVao);

      highlightShader.setVec3("uColor", glm::vec3(1.0f, 1.0f, 1.0f));
      highlightShader.setFloat("uAlpha", 0.28f);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, highlightFillEbo);
      glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

      highlightShader.setVec3("uColor", glm::vec3(0.05f, 0.05f, 0.05f));
      highlightShader.setFloat("uAlpha", 0.85f);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, highlightEdgeEbo);
      glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);

      glBindVertexArray(0);
      glDepthMask(GL_TRUE);
    }

    glDisable(GL_DEPTH_TEST);
    crosshairShader.use();
    glBindVertexArray(crosshairVao);
    glDrawArrays(GL_TRIANGLES, 0, 12);
    glBindVertexArray(0);

    {
      uiIconShader.use();
      float aspectCorrection = (float)fbHeight / (float)fbWidth;
      glm::vec2 center(-0.85f, 0.82f);
      glm::vec2 borderHalf(0.105f * aspectCorrection, 0.105f);
      glm::vec2 iconHalf(0.09f * aspectCorrection, 0.09f);

      glBindVertexArray(uiIconVao);
      atlas.bind(0);
      uiIconShader.setInt("uTexture", 0);
      uiIconShader.setVec2("uCenter", center);

      uiIconShader.setVec2("uHalfSize", borderHalf);
      uiIconShader.setInt("uUseSolidColor", 1);
      uiIconShader.setVec3("uSolidColor", glm::vec3(0.05f, 0.05f, 0.05f));
      glDrawArrays(GL_TRIANGLES, 0, 6);

      glm::vec4 tile =
          tileUV(iconTileForBlock(PLACEABLE_BLOCKS[selectedBlockIndex]));
      uiIconShader.setVec2("uHalfSize", iconHalf);
      uiIconShader.setInt("uUseSolidColor", 0);
      uiIconShader.setVec4("uTileUV", tile);
      glDrawArrays(GL_TRIANGLES, 0, 6);

      glBindVertexArray(0);
    }
    glEnable(GL_DEPTH_TEST);

    SDL_GL_SwapWindow(window);
  }

  for (const auto &[key, mesh] : chunkManager.opaqueMeshes()) {
    glDeleteVertexArrays(1, &mesh.vao);
    glDeleteBuffers(1, &mesh.vbo);
    glDeleteBuffers(1, &mesh.ebo);
  }
  for (const auto &[key, mesh] : chunkManager.transparentMeshes()) {
    glDeleteVertexArrays(1, &mesh.vao);
    glDeleteBuffers(1, &mesh.vbo);
    glDeleteBuffers(1, &mesh.ebo);
  }
  glDeleteVertexArrays(1, &crosshairVao);
  glDeleteBuffers(1, &crosshairVbo);
  glDeleteVertexArrays(1, &highlightVao);
  glDeleteBuffers(1, &highlightVbo);
  glDeleteBuffers(1, &highlightFillEbo);
  glDeleteBuffers(1, &highlightEdgeEbo);
  glDeleteVertexArrays(1, &uiIconVao);
  glDeleteBuffers(1, &uiIconVbo);
  glDeleteFramebuffers(1, &shadowFBO);
  glDeleteTextures(1, &shadowMapTexture);
  destroyGBuffer(gBufferFBO, gPositionTex, gNormalTex, gAlbedoTex,
                 gBlockLightTex, gDepthRBO);
  destroySingleChannelTarget(ssaoFBO, ssaoColorTex);
  destroySingleChannelTarget(ssaoBlurFBO, ssaoBlurTex);
  glDeleteTextures(1, &ssaoNoiseTex);
  glDeleteVertexArrays(1, &quadVAO);

  SDL_GL_DeleteContext(glContext);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
