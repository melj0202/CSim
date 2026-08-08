#pragma once

#include "GL/glew.h"
#include "ITexture.h"
#include "thirdparty/stb/stb_image.h"
#include <array>
#include <cstring>
#include <string>

class GLTexture : public ITexture
{
public:
  GLTexture()
    : m_path("")
    , m_id(0)
    , m_size({ 0, 0 })
    , m_channels(4)
    , m_pbo{ 0, 0 }
    , m_pboIndex(0)
    , m_pboBytes(0)
  {
  }

  // channels: 1 = R8, 3 = RGB, 4 = RGBA (default)
  GLTexture(const unsigned char* data,
            int width,
            int height,
            int channels = 4,
            TextureFilter filter = TextureFilter::Nearest)
    : m_path("")
    , m_id(0)
    , m_size({ width, height })
    , m_channels(normalizeChannels(channels))
    , m_pbo{ 0, 0 }
    , m_pboIndex(0)
    , m_pboBytes(0)
  {
    UploadToGPU(data, width, height, m_channels, filter);
  }

  GLTexture(const std::string& path)
    : ITexture(path)
    , m_path(path)
    , m_id(0)
    , m_size({ 0, 0 })
    , m_channels(4)
    , m_pbo{ 0, 0 }
    , m_pboIndex(0)
    , m_pboBytes(0)
  {
    int width = 0;
    int height = 0;
    int bpp = 0;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &bpp, 4);
    if (data) {
      m_size = { width, height };
      m_channels = 4;
      UploadToGPU(data, width, height, 4, TextureFilter::Nearest);
      stbi_image_free(data);
    }
  }

  ~GLTexture() override {}

  void Bind(unsigned int slot) const override
  {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, m_id);
  }

  unsigned int getID() const override { return m_id; }
  std::array<int, 2> getSize() const override { return m_size; }
  int getChannels() const { return m_channels; }

  // CPU → GPU subimage upload with optional row stride and PBO ping-pong (P4).
  // data is host pointer (not PBO). srcRowStride is in pixels (0 = tightly
  // packed width). Small dirty rects pack tightly into a partial PBO region
  // (D-P6) instead of re-orphaning a full-texture buffer every frame.
  void UpdateSubImage(int x,
                      int y,
                      int width,
                      int height,
                      int channels,
                      const void* data,
                      int srcRowStridePixels)
  {
    if (!data || width <= 0 || height <= 0 || m_id == 0) {
      return;
    }

    const int ch = (channels > 0) ? channels : m_channels;
    const GLenum format = formatForChannels(ch);
    const int rowStride = (srcRowStridePixels > 0) ? srcRowStridePixels : width;
    const size_t bytesPerPixel = static_cast<size_t>(ch);
    const size_t fullBytes = static_cast<size_t>(m_size[0]) *
                             static_cast<size_t>(m_size[1]) * bytesPerPixel;
    const size_t packedBytes =
      static_cast<size_t>(width) * static_cast<size_t>(height) * bytesPerPixel;
    const size_t srcRowBytes = static_cast<size_t>(rowStride) * bytesPerPixel;
    const size_t copyRowBytes = static_cast<size_t>(width) * bytesPerPixel;
    const unsigned char* srcBase = static_cast<const unsigned char*>(data);

    // Prefer tight packing for partial rects (avoids full-texture map cost).
    const bool usePacked = (width < m_size[0] || height < m_size[1]);
    const size_t stageBytes = usePacked ? packedBytes : fullBytes;

    ensurePBOs(fullBytes);

    m_pboIndex = 1 - m_pboIndex;
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_pbo[m_pboIndex]);

    // Map only the staging range; invalidate to avoid GPU readback stalls.
    // Reuses the existing PBO allocation (no glBufferData orphan each frame).
    void* mapped =
      glMapBufferRange(GL_PIXEL_UNPACK_BUFFER,
                       0,
                       static_cast<GLsizeiptr>(stageBytes),
                       GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);
    if (!mapped) {
      // Fallback: direct client upload without PBO.
      glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
      glBindTexture(GL_TEXTURE_2D, m_id);
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
      if (rowStride != width) {
        glPixelStorei(GL_UNPACK_ROW_LENGTH, rowStride);
      }
      glTexSubImage2D(
        GL_TEXTURE_2D, 0, x, y, width, height, format, GL_UNSIGNED_BYTE, data);
      if (rowStride != width) {
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
      }
      return;
    }

    unsigned char* dstBase = static_cast<unsigned char*>(mapped);
    if (usePacked) {
      for (int row = 0; row < height; ++row) {
        unsigned char* dst = dstBase + static_cast<size_t>(row) * copyRowBytes;
        const unsigned char* src =
          srcBase + static_cast<size_t>(row) * srcRowBytes;
        std::memcpy(dst, src, copyRowBytes);
      }
    } else {
      // Full-texture layout: copy dirty rows at their native offsets.
      const size_t dstRowBytes = static_cast<size_t>(m_size[0]) * bytesPerPixel;
      const size_t dstOrigin =
        (static_cast<size_t>(y) * static_cast<size_t>(m_size[0]) +
         static_cast<size_t>(x)) *
        bytesPerPixel;
      for (int row = 0; row < height; ++row) {
        unsigned char* dst =
          dstBase + dstOrigin + static_cast<size_t>(row) * dstRowBytes;
        const unsigned char* src =
          srcBase + static_cast<size_t>(row) * srcRowBytes;
        std::memcpy(dst, src, copyRowBytes);
      }
    }
    glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);

    glBindTexture(GL_TEXTURE_2D, m_id);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    if (usePacked) {
      glTexSubImage2D(GL_TEXTURE_2D,
                      0,
                      x,
                      y,
                      width,
                      height,
                      format,
                      GL_UNSIGNED_BYTE,
                      reinterpret_cast<const GLvoid*>(0));
    } else {
      const size_t dstOrigin =
        (static_cast<size_t>(y) * static_cast<size_t>(m_size[0]) +
         static_cast<size_t>(x)) *
        bytesPerPixel;
      const GLvoid* pboOffset = reinterpret_cast<const GLvoid*>(dstOrigin);
      if (m_size[0] != width) {
        glPixelStorei(GL_UNPACK_ROW_LENGTH, m_size[0]);
      }
      glTexSubImage2D(GL_TEXTURE_2D,
                      0,
                      x,
                      y,
                      width,
                      height,
                      format,
                      GL_UNSIGNED_BYTE,
                      pboOffset);
      if (m_size[0] != width) {
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
      }
    }
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
  }

  void Destroy() override
  {
    if (m_pbo[0] != 0) {
      glDeleteBuffers(2, m_pbo);
      m_pbo[0] = 0;
      m_pbo[1] = 0;
      m_pboBytes = 0;
    }
    if (m_id != 0) {
      glDeleteTextures(1, &m_id);
      m_id = 0;
    }
  }

private:
  static int normalizeChannels(int channels)
  {
    if (channels == 1) {
      return 1;
    }
    if (channels == 3) {
      return 3;
    }
    return 4;
  }

  static GLenum formatForChannels(int channels)
  {
    if (channels == 1) {
      return GL_RED;
    }
    if (channels == 3) {
      return GL_RGB;
    }
    return GL_RGBA;
  }

  static GLenum internalFormatForChannels(int channels)
  {
    if (channels == 1) {
      return GL_R8;
    }
    if (channels == 3) {
      return GL_RGB8;
    }
    return GL_RGBA8;
  }

  void ensurePBOs(size_t bytes)
  {
    if (bytes == 0) {
      return;
    }
    if (m_pbo[0] != 0 && m_pboBytes >= bytes) {
      return;
    }
    if (m_pbo[0] != 0) {
      glDeleteBuffers(2, m_pbo);
      m_pbo[0] = 0;
      m_pbo[1] = 0;
    }
    glGenBuffers(2, m_pbo);
    m_pboBytes = bytes;
    for (int i = 0; i < 2; ++i) {
      glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_pbo[i]);
      glBufferData(GL_PIXEL_UNPACK_BUFFER,
                   static_cast<GLsizeiptr>(bytes),
                   nullptr,
                   GL_STREAM_DRAW);
    }
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    m_pboIndex = 0;
  }

  void UploadToGPU(const unsigned char* data,
                   int width,
                   int height,
                   int channels,
                   TextureFilter filter)
  {
    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);

    const GLenum format = formatForChannels(channels);
    const GLenum internalFmt = internalFormatForChannels(channels);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 static_cast<GLint>(internalFmt),
                 width,
                 height,
                 0,
                 format,
                 GL_UNSIGNED_BYTE,
                 data);

    const GLint glFilter =
      (filter == TextureFilter::Linear) ? GL_LINEAR : GL_NEAREST;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, glFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, glFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // R8 samples only .r; make .gba = r for any accidental RGB sampling.
    if (channels == 1) {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_RED);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_RED);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_ONE);
    }
  }

  std::string m_path;
  unsigned int m_id;
  std::array<int, 2> m_size;
  int m_channels;

  // Async upload: double-buffered pixel unpack buffers.
  GLuint m_pbo[2];
  int m_pboIndex;
  size_t m_pboBytes;
};
