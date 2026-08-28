#pragma once

#include <glm/glm.hpp>

class Camera {
public:
  Camera(glm::vec3 startPosition);

  glm::mat4 getViewMatrix() const;

  void processMouseMovement(float dx, float dy);

  void processKeyboard(bool forward, bool backward, bool left, bool right,
                       bool up, bool down, float dt);

  void setYawPitch(float newYaw, float newPitch);

  glm::vec3 position;
  float yaw;
  float pitch;

  float movementSpeed = 15.0f;
  float mouseSensitivity = 0.1f;

  glm::vec3 front;
  glm::vec3 up;
  glm::vec3 right;
  static constexpr glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);

private:
  void updateVectors();
};
