#include "Camera.h"

#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

Camera::Camera(glm::vec3 startPosition)
    : position(startPosition), yaw(-90.0f), pitch(0.0f) {
  updateVectors();
}

void Camera::updateVectors() {
  glm::vec3 newFront;
  newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
  newFront.y = sin(glm::radians(pitch));
  newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
  front = glm::normalize(newFront);

  right = glm::normalize(glm::cross(front, worldUp));
  up = glm::normalize(glm::cross(right, front));
}

glm::mat4 Camera::getViewMatrix() const {
  return glm::lookAt(position, position + front, up);
}

void Camera::processMouseMovement(float dx, float dy) {
  yaw += dx * mouseSensitivity;
  pitch -= dy * mouseSensitivity;

  pitch = std::clamp(pitch, -89.0f, 89.0f);

  updateVectors();
}

void Camera::setYawPitch(float newYaw, float newPitch) {
  yaw = newYaw;
  pitch = std::clamp(newPitch, -89.0f, 89.0f);
  updateVectors();
}

void Camera::processKeyboard(bool forward, bool backward, bool left,
                             bool right_, bool up_, bool down, float dt) {
  float velocity = movementSpeed * dt;

  if (forward)
    position += front * velocity;
  if (backward)
    position -= front * velocity;
  if (left)
    position -= right * velocity;
  if (right_)
    position += right * velocity;
  if (up_)
    position += worldUp * velocity;
  if (down)
    position -= worldUp * velocity;
}
