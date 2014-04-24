#include "gl.h"
#include "pngimage.h"

#include <GL/gl.h>
#include <cinttypes>
#include <cstdlib>

Texture::Texture(PngImage& image)
{
    const size_t bufferSize = image.getWidth() * image.getHeight() * sizeof(pixel_t);
    u_int8_t *buffer = new u_int8_t(bufferSize);
    std::memset(buffer, 0x0, bufferSize);

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
}
