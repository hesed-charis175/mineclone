#include "Shader.h"

#include <fstream>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <sstream>

std::string Shader::readFile(const std::string &path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    std::cerr << "Failed to open shader file: " << path << "\n";
    return "";
  }
  std::stringstream ss;
  ss << file.rdbuf();
  return ss.str();
}

GLuint Shader::compileShader(GLenum type, const std::string &source,
                             const std::string &debugName) {
  GLuint shader = glCreateShader(type);
  const char *src = source.c_str();
  glShaderSource(shader, 1, &src, nullptr);
  glCompileShader(shader);

  GLint success = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    char log[1024];
    glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
    std::cerr << "Shader compile error (" << debugName << "):\n" << log << "\n";
  }
  return shader;
}

Shader::Shader(const std::string &vertexPath, const std::string &fragmentPath) {
  std::string vertSrc = readFile(vertexPath);
  std::string fragSrc = readFile(fragmentPath);

  GLuint vertShader = compileShader(GL_VERTEX_SHADER, vertSrc, vertexPath);
  GLuint fragShader = compileShader(GL_FRAGMENT_SHADER, fragSrc, fragmentPath);

  programId = glCreateProgram();
  glAttachShader(programId, vertShader);
  glAttachShader(programId, fragShader);
  glLinkProgram(programId);

  GLint success = 0;
  glGetProgramiv(programId, GL_LINK_STATUS, &success);
  if (!success) {
    char log[1024];
    glGetProgramInfoLog(programId, sizeof(log), nullptr, log);
    std::cerr << "Shader link error:\n" << log << "\n";
  }

  glDeleteShader(vertShader);
  glDeleteShader(fragShader);
}

Shader::~Shader() { glDeleteProgram(programId); }

void Shader::use() const { glUseProgram(programId); }

GLint Shader::uniformLocation(const std::string &name) const {
  return glGetUniformLocation(programId, name.c_str());
}

void Shader::setMat4(const std::string &name, const glm::mat4 &value) const {
  glUniformMatrix4fv(uniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::setVec2(const std::string &name, const glm::vec2 &value) const {
  glUniform2fv(uniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::setVec3(const std::string &name, const glm::vec3 &value) const {
  glUniform3fv(uniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::setVec4(const std::string &name, const glm::vec4 &value) const {
  glUniform4fv(uniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::setInt(const std::string &name, int value) const {
  glUniform1i(uniformLocation(name), value);
}

void Shader::setFloat(const std::string &name, float value) const {
  glUniform1f(uniformLocation(name), value);
}
