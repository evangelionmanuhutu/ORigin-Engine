// Copyright (c) 2022 Evangelion Manuhutu | ORigin Engine

#include "pch.h"
#include "stb_image.h"
#include "OpenGL_Texture.h"
#include "Origin\Core\OriginCore.h"

namespace Origin
{

  OpenGLTexture3D::OpenGLTexture3D(uint32_t width, uint32_t height)
		: m_Width(width), m_Height(height)
	{
	}

  OpenGLTexture3D::OpenGLTexture3D(const std::string& filepath)
		: m_RendererID(0), m_FilePath(filepath), m_Index(0)
	{
		std::string faces[6] =
		{
			"/right.jpg",
			"/left.jpg",
			"/top.jpg",
			"/bottom.jpg",
			"/back.jpg",
			"/front.jpg",
		};

		int width, height, bpp;
		stbi_uc* data = nullptr;

		stbi_set_flip_vertically_on_load(false);
		GLenum dataFormat = 0, internalFormat = 0;

		// set the file path
		for (uint32_t i = 0; i < 6; i++)
		{
			data = stbi_load(std::string(filepath + faces[i]).c_str(), &height, &width, &bpp, 0);

			switch (bpp)
			{
			case 3:
				internalFormat = GL_RGB8;
				dataFormat = GL_RGB;

				break;

			case 4:
				internalFormat = GL_RGBA8;
				dataFormat = GL_RGBA;
				break;
			}

			if (data)
			{
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
				stbi_image_free(data);
				OGN_CORE_TRACE("Texture \"{0}\" Successfully Loaded", faces[i]);
			}
			else
			{
				OGN_CORE_ERROR("Failed to load Texture: {0}", faces[i]);
				stbi_image_free(data);
			}
		}
		OGN_CORE_TRACE("Bits Per Pixel : {0}", bpp);
		OGN_CORE_TRACE("Internal Format: {0}, Data Format: {1}", internalFormat, dataFormat);

		// generate texture
		glGenTextures(1, &m_RendererID);
		glBindTexture(GL_TEXTURE_2D, m_RendererID);

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	}

	void OpenGLTexture3D::Delete()
	{
	}

  OpenGLTexture3D::~OpenGLTexture3D()
	{

	}

	void OpenGLTexture3D::SetData(void* data, uint32_t size)
	{

	}

	void OpenGLTexture3D::Bind(uint32_t slot)
	{

	}

	OpenGLTexture2D::OpenGLTexture2D(uint32_t width, uint32_t height)
    : m_Width(width), m_Height(height)
	{
		m_InternalFormat = GL_RGBA8;
		m_DataFormat = GL_RGBA;

		glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);
		glTextureStorage2D(m_RendererID, 1, m_InternalFormat, m_Width, m_Height);
		glTexParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(m_RendererID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		m_IsLoaded = true;
	}

  OpenGLTexture2D::OpenGLTexture2D(const std::string& path)
    : m_RendererID(0), m_FilePath(path), m_Index(0)
  {
		int width, height, bpp;
		stbi_set_flip_vertically_on_load(1);

		stbi_uc* data = nullptr;
		data = stbi_load(path.c_str(), &width, &height, &bpp, 0);

		if (!data)
		{
			OGN_CORE_ERROR("Texture2D: Failed to load texture! {}", path);
			stbi_image_free(data);
			return;
		}

		m_Width = width;
		m_Height = height;
		m_BPP = bpp;

		GLenum internalFormat = 0, dataFormat = 0;
		if (m_BPP == 4)
		{
			internalFormat = GL_RGBA8;
			dataFormat = GL_RGBA;
		}
		else if (m_BPP == 3)
		{
			internalFormat = GL_RGB8;
			dataFormat = GL_RGB;
		}

		m_InternalFormat = internalFormat;
		m_DataFormat = dataFormat;

		glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);
		glTextureStorage2D(m_RendererID, 1, m_InternalFormat, m_Width, m_Height);

		glTexParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTexParameteri(m_RendererID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glTextureSubImage2D(m_RendererID, 0, 0, 0, m_Width, m_Height, m_DataFormat, GL_UNSIGNED_BYTE, data);
		stbi_image_free(data);
		m_IsLoaded = true;
  }

	OpenGLTexture2D::~OpenGLTexture2D()
	{
    glDeleteTextures(1, &m_RendererID);
  }

	void OpenGLTexture2D::SetData(void* data, uint32_t size)
	{
    uint32_t bpp = m_DataFormat == GL_RGBA ? 4 : 3;
    OGN_CORE_ASSERT(size == m_Width * m_Height * bpp, "data must be entire texture!");
    glTextureSubImage2D(m_RendererID, 0, 0, 0, m_Width, m_Height, m_DataFormat, GL_UNSIGNED_BYTE, data);
	}

	void OpenGLTexture2D::Bind(uint32_t index)
	{
		m_Index = index;
		glBindTextureUnit(index, m_RendererID); // bind texture index to renderID
  }

	void OpenGLTexture2D::Delete()
	{
		glBindTextureUnit(m_Index, 0); // bind texture index to nothing (0)
		OGN_CORE_WARN("Texture \"{}\" at index {} has been deleted", m_FilePath, m_Index);
	}
}