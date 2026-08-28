#pragma once

#include <array>
#include <cstdint>

class PerlinNoise {
public:
  explicit PerlinNoise(uint32_t seed);

  float noise2D(float x, float y) const;
  float noise3D(float x, float y, float z) const;

  float octaveNoise(float x, float y, int octaves, float persistence) const;
  float octaveNoise3D(float x, float y, float z, int octaves,
                      float persistence) const;

private:
  std::array<int, 512> perm;

  static float fade(float t);
  static float lerp(float t, float a, float b);
  static float grad(int hash, float x, float y);
  static float grad3D(int hash, float x, float y, float z);
};
