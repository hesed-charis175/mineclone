#pragma once

#include <GL/glew.h>
#include <string>

class Texture {
public:
  explicit Texture(const std::string &path);
  ~Texture();

  void bind(GLuint unit = 0) const;

  int width() const { return texWidth; }
  int height() const { return texHeight; }

private:
  GLuint textureId = 0;
  int texWidth = 0;
  int texHeight = 0;
};
