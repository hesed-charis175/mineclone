#pragma once

#include <array>
#include <glm/glm.hpp>

struct Plane {
  glm::vec3 normal{0.0f};
  float d = 0.0f;
};

class Frustum {
public:
  void update(const glm::mat4 &viewProj) {
    auto row = [&](int i) {
      return glm::vec4(viewProj[0][i], viewProj[1][i], viewProj[2][i],
                       viewProj[3][i]);
    };
    glm::vec4 r0 = row(0), r1 = row(1), r2 = row(2), r3 = row(3);

    setPlane(0, r3 + r0); // left
    setPlane(1, r3 - r0); // right
    setPlane(2, r3 + r1); // bottom
    setPlane(3, r3 - r1); // top
    setPlane(4, r3 + r2); // near
    setPlane(5, r3 - r2); // far
  }

  bool intersectsAABB(const glm::vec3 &min, const glm::vec3 &max) const {
    for (const Plane &p : planes) {
      glm::vec3 positiveVertex(p.normal.x >= 0 ? max.x : min.x,
                               p.normal.y >= 0 ? max.y : min.y,
                               p.normal.z >= 0 ? max.z : min.z);
      if (glm::dot(p.normal, positiveVertex) + p.d < 0.0f) {
        return false;
      }
    }
    return true;
  }

private:
  std::array<Plane, 6> planes;

  void setPlane(int i, const glm::vec4 &v) {
    glm::vec3 n(v.x, v.y, v.z);
    float len = glm::length(n);
    planes[i].normal = n / len;
    planes[i].d = v.w / len;
  }
};
