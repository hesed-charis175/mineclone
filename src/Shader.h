#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <string>

class Shader {
public:
  Shader(const std::string &vertexPath, const std::string &fragmentPath);
  ~Shader();

  void use() const;

  void setMat4(const std::string &name, const glm::mat4 &value) const;
  void setVec2(const std::string &name, const glm::vec2 &value) const;
  void setVec3(const std::string &name, const glm::vec3 &value) const;
  void setVec4(const std::string &name, const glm::vec4 &value) const;
  void setInt(const std::string &name, int value) const;
  void setFloat(const std::string &name, float value) const;

  GLuint id() const { return programId; }

private:
  GLuint programId = 0;

  static std::string readFile(const std::string &path);
  static GLuint compileShader(GLenum type, const std::string &source,
                              const std::string &debugName);
  GLint uniformLocation(const std::string &name) const;
};
