#pragma once

#include "ITexture.h"
#include "GL/glew.h"
#include "thirdparty/stb/stb_image.h"
#include <string>
#include <array>

class GLTexture : public ITexture {
public:
    GLTexture() : m_path(""), m_id(0), m_size({0, 0}) {}

    GLTexture(const unsigned char* data, int width, int height) {
        m_path = "";
        m_size = {width, height};
        
        if (data) {
            UploadToGPU(data, width, height);
        } else {
            m_id = 0;
        }
    }

    GLTexture(const std::string& path) : ITexture(path), m_path(path), m_id(0), m_size({0, 0}) {
        int width, height, bpp;
        // Force 4 channels (RGBA) to match GL_RGBA allocation safely
        unsigned char* data = stbi_load(path.c_str(), &width, &height, &bpp, 4);
        
        if (data) {
            m_size = {width, height};
            UploadToGPU(data, width, height);
            stbi_image_free(data);
        }
    }

    ~GLTexture() override {
        // Resource destruction MUST be explicit via Destroy()!
    }

    void Bind(unsigned int slot) const override {
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, m_id);
    }

    unsigned int getID() const override { return m_id; }
    std::array<int, 2> getSize() const override { return m_size; }

    void Destroy() override {
        if (m_id != 0) {
            glDeleteTextures(1, &m_id);
            m_id = 0; // Prevent double-deletion bugs
        }
    }

private:
    void UploadToGPU(const unsigned char* data, int width, int height) {
        glGenTextures(1, &m_id);
        glBindTexture(GL_TEXTURE_2D, m_id);
        
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        
        // Sampling parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    std::string m_path;
    unsigned int m_id;
    std::array<int, 2> m_size;
};