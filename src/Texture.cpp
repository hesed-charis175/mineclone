#include "Texture.h"

#include "vendor/stb_image.h"
#include <iostream>

Texture::Texture(const std::string &path) {
  stbi_set_flip_vertically_on_load(true);

  int channels;
  unsigned char *data =
      stbi_load(path.c_str(), &texWidth, &texHeight, &channels, 0);
  if (!data) {
    std::cerr << "Failed to load texture: " << path << " ("
              << stbi_failure_reason() << ")\n";
    return;
  }

  GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;

  glGenTextures(1, &textureId);
  glBindTexture(GL_TEXTURE_2D, textureId);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_NEAREST_MIPMAP_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glTexImage2D(GL_TEXTURE_2D, 0, format, texWidth, texHeight, 0, format,
               GL_UNSIGNED_BYTE, data);
  glGenerateMipmap(GL_TEXTURE_2D);

  stbi_image_free(data);

  std::cout << "Loaded texture: " << path << " (" << texWidth << "x"
            << texHeight << ")\n";
}

Texture::~Texture() {
  if (textureId)
    glDeleteTextures(1, &textureId);
}

void Texture::bind(GLuint unit) const {
  glActiveTexture(GL_TEXTURE0 + unit);
  glBindTexture(GL_TEXTURE_2D, textureId);
}
