#pragma once

#include "ITexture.h"
#include "GL/glew.h"
#include "thirdparty/stb/stb_image.h"
#include <string>
#include <array>

class GLTexture : public ITexture {
public:
	GLTexture() : m_path(""), m_id(0), m_size({0, 0}), m_channels(4) {}

	// channels: 3 = RGB, 4 = RGBA (default)
	GLTexture(const unsigned char* data, int width, int height, int channels = 4)
	{
		m_path = "";
		m_size = {width, height};
		m_channels = (channels == 3) ? 3 : 4;
		if (data)
		{
			UploadToGPU(data, width, height, m_channels);
		}
		else
		{
			m_id = 0;
		}
	}

	GLTexture(const std::string& path) : ITexture(path), m_path(path), m_id(0), m_size({0, 0}), m_channels(4)
	{
		int width = 0;
		int height = 0;
		int bpp = 0;
		unsigned char* data = stbi_load(path.c_str(), &width, &height, &bpp, 4);
		if (data)
		{
			m_size = {width, height};
			m_channels = 4;
			UploadToGPU(data, width, height, 4);
			stbi_image_free(data);
		}
	}

	~GLTexture() override
	{
	}

	void Bind(unsigned int slot) const override
	{
		glActiveTexture(GL_TEXTURE0 + slot);
		glBindTexture(GL_TEXTURE_2D, m_id);
	}

	unsigned int getID() const override { return m_id; }
	std::array<int, 2> getSize() const override { return m_size; }
	int getChannels() const { return m_channels; }

	void Destroy() override
	{
		if (m_id != 0)
		{
			glDeleteTextures(1, &m_id);
			m_id = 0;
		}
	}

private:
	void UploadToGPU(const unsigned char* data, int width, int height, int channels)
	{
		glGenTextures(1, &m_id);
		glBindTexture(GL_TEXTURE_2D, m_id);

		GLenum format = (channels == 3) ? GL_RGB : GL_RGBA;
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}

	std::string m_path;
	unsigned int m_id;
	std::array<int, 2> m_size;
	int m_channels;
};
