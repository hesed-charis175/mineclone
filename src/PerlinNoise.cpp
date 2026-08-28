#include "PerlinNoise.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>

PerlinNoise::PerlinNoise(uint32_t seed) {
  std::array<int, 256> p;
  std::iota(p.begin(), p.end(), 0);

  std::mt19937 rng(seed);
  std::shuffle(p.begin(), p.end(), rng);

  for (int i = 0; i < 256; ++i) {
    perm[i] = p[i];
    perm[i + 256] = p[i];
  }
}

float PerlinNoise::fade(float t) { return t * t * t * (t * (t * 6 - 15) + 10); }

float PerlinNoise::lerp(float t, float a, float b) { return a + t * (b - a); }

float PerlinNoise::grad(int hash, float x, float y) {
  switch (hash & 7) {
  case 0:
    return x + y;
  case 1:
    return x - y;
  case 2:
    return -x + y;
  case 3:
    return -x - y;
  case 4:
    return x;
  case 5:
    return -x;
  case 6:
    return y;
  default:
    return -y;
  }
}

float PerlinNoise::noise2D(float x, float y) const {
  int xi = (int)std::floor(x) & 255;
  int yi = (int)std::floor(y) & 255;

  float xf = x - std::floor(x);
  float yf = y - std::floor(y);

  float u = fade(xf);
  float v = fade(yf);

  int aa = perm[perm[xi] + yi];
  int ab = perm[perm[xi] + yi + 1];
  int ba = perm[perm[xi + 1] + yi];
  int bb = perm[perm[xi + 1] + yi + 1];

  float x1 = lerp(u, grad(aa, xf, yf), grad(ba, xf - 1, yf));
  float x2 = lerp(u, grad(ab, xf, yf - 1), grad(bb, xf - 1, yf - 1));

  return lerp(v, x1, x2);
}

float PerlinNoise::octaveNoise(float x, float y, int octaves,
                               float persistence) const {
  float total = 0.0f;
  float frequency = 1.0f;
  float amplitude = 1.0f;
  float maxAmplitude = 0.0f;

  for (int i = 0; i < octaves; ++i) {
    total += noise2D(x * frequency, y * frequency) * amplitude;
    maxAmplitude += amplitude;
    amplitude *= persistence;
    frequency *= 2.0f;
  }

  return total / maxAmplitude;
}

float PerlinNoise::grad3D(int hash, float x, float y, float z) {
  int h = hash & 15;
  float u = h < 8 ? x : y;
  float v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
  return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

float PerlinNoise::noise3D(float x, float y, float z) const {
  int xi = (int)std::floor(x) & 255;
  int yi = (int)std::floor(y) & 255;
  int zi = (int)std::floor(z) & 255;

  float xf = x - std::floor(x);
  float yf = y - std::floor(y);
  float zf = z - std::floor(z);

  float u = fade(xf);
  float v = fade(yf);
  float w = fade(zf);

  int aaa = perm[perm[perm[xi] + yi] + zi];
  int aba = perm[perm[perm[xi] + yi + 1] + zi];
  int aab = perm[perm[perm[xi] + yi] + zi + 1];
  int abb = perm[perm[perm[xi] + yi + 1] + zi + 1];
  int baa = perm[perm[perm[xi + 1] + yi] + zi];
  int bba = perm[perm[perm[xi + 1] + yi + 1] + zi];
  int bab = perm[perm[perm[xi + 1] + yi] + zi + 1];
  int bbb = perm[perm[perm[xi + 1] + yi + 1] + zi + 1];

  float x1 = lerp(u, grad3D(aaa, xf, yf, zf), grad3D(baa, xf - 1, yf, zf));
  float x2 =
      lerp(u, grad3D(aba, xf, yf - 1, zf), grad3D(bba, xf - 1, yf - 1, zf));
  float y1 = lerp(v, x1, x2);

  float x3 =
      lerp(u, grad3D(aab, xf, yf, zf - 1), grad3D(bab, xf - 1, yf, zf - 1));
  float x4 = lerp(u, grad3D(abb, xf, yf - 1, zf - 1),
                  grad3D(bbb, xf - 1, yf - 1, zf - 1));
  float y2 = lerp(v, x3, x4);

  return lerp(w, y1, y2);
}

float PerlinNoise::octaveNoise3D(float x, float y, float z, int octaves,
                                 float persistence) const {
  float total = 0.0f;
  float frequency = 1.0f;
  float amplitude = 1.0f;
  float maxAmplitude = 0.0f;

  for (int i = 0; i < octaves; ++i) {
    total += noise3D(x * frequency, y * frequency, z * frequency) * amplitude;
    maxAmplitude += amplitude;
    amplitude *= persistence;
    frequency *= 2.0f;
  }

  return total / maxAmplitude;
}
