﻿// Copyright (c) 2022 Evangelion Manuhutu | ORigin Engine

#include "pch.h"
#include "OpenGL_Framebuffer.h"

#include <glad/glad.h>

namespace origin {

	static const uint32_t s_MaxFramebufferSize = 8192;

	namespace Utils	{


		// ================ Texture 2D ================

		static GLenum Texture2DTarget(bool multisampled) 
		{ 
			return multisampled ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D; 
		}

		static void CreateTextures2D(bool multisampled, uint32_t* outTextureID, uint32_t count)
		{
			glCreateTextures(Texture2DTarget(multisampled), count, outTextureID);
		}

		static void BindTexture2D(bool multisampled, uint32_t textureID)
		{ 
			glBindTexture(Texture2DTarget(multisampled), textureID);
		}

		static void AttachColorTexture2D(uint32_t textureID, int samples, GLenum internalFormat, GLenum format, uint32_t width, uint32_t height, GLenum filterMode, GLenum wrapMode, int index)
		{
			bool multisampled = samples > 1;
			if (multisampled)
			{
				glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, internalFormat, width, height, GL_FALSE);
			}
			else
			{
				glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, nullptr);
			}

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filterMode);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filterMode);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);
			
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, Texture2DTarget(multisampled), textureID, NULL);
		}

		static void AttachDepthTexture2D(uint32_t textureID, int samples, GLenum textureFormat, GLenum attachmentType, uint32_t width, uint32_t height, GLenum filterMode, GLenum wrapMode)
		{
			bool multisampled = samples > 1;
			if (multisampled)
			{
				glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, textureFormat, width, height, GL_FALSE);
			}
			else
			{
				if(attachmentType == GL_DEPTH_STENCIL_ATTACHMENT)
					glTexStorage2D(GL_TEXTURE_2D, 1, textureFormat, width, height);
				else if(attachmentType == GL_DEPTH_ATTACHMENT)
					glTexImage2D(GL_TEXTURE_2D, 0, textureFormat, width, height, 0, textureFormat, GL_FLOAT, nullptr);
			}

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filterMode);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filterMode);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);

			float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
			glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

			glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentType, Texture2DTarget(multisampled), textureID, NULL);
		}

		// ============ CUBE MAP ============

		static void CreateTextureCubeMap(uint32_t* outTextureID, uint32_t count)
		{
			glCreateTextures(GL_TEXTURE_CUBE_MAP, count, outTextureID);
		}

		static void BindTextureCubeMap(uint32_t textureID)
		{
			glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
		}

		static void AttachDepthTextureCubeMap(uint32_t textureID, GLenum textureFormat, GLenum attachmentType, uint32_t width, uint32_t height, GLenum filterMode, GLenum wrapMode)
		{
			for (int i = 0; i < 6; i++)
			{
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
					0, textureFormat, width, height, 0, textureFormat, GL_FLOAT, nullptr);
			}

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filterMode);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filterMode);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, wrapMode);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);

			glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentType, GL_TEXTURE_CUBE_MAP, textureID, NULL);
		}

		static bool IsDepthFormat(FramebufferTextureFormat format)
		{
			switch (format)
			{
				case FramebufferTextureFormat::DEPTH:
				case FramebufferTextureFormat::DEPTH24STENCIL8:
				case FramebufferTextureFormat::DEPTH_CUBE: return true;
			}

			return false;
		}

		static GLenum ORiginFBTextureFormatToGL(FramebufferTextureFormat format)
		{
			switch (format)
			{
				case origin::FramebufferTextureFormat::RGBA8: return GL_RGBA8;
				case origin::FramebufferTextureFormat::RED_INTEGER: return GL_RED_INTEGER;
			}

			OGN_CORE_ASSERT(false, "Unkown Framebuffer Format");
			return 0;
		}
	}

	OpenGL_Framebuffer::OpenGL_Framebuffer(const FramebufferSpecification& specification)
		: m_Spec(specification)
	{
		for (auto spec : m_Spec.Attachments.TextureAttachments)
		{
			if (!Utils::IsDepthFormat(spec.TextureFormat))
				m_ColorAttachmentSpecifications.emplace_back(spec);
			else
				m_DepthAttachmentSpecification = spec;
		}

		Invalidate();
	}

	OpenGL_Framebuffer::~OpenGL_Framebuffer()
	{
		glDeleteFramebuffers(1, &m_RendererID);
		glDeleteFramebuffers(m_ColorAttachments.size(), m_ColorAttachments.data());
		glDeleteFramebuffers(1, &m_DepthAttachment);
	}

	void OpenGL_Framebuffer::Invalidate()
	{
		if (m_RendererID)
		{
			glDeleteFramebuffers(1, &m_RendererID);
			glDeleteFramebuffers(m_ColorAttachments.size(), m_ColorAttachments.data());
			glDeleteFramebuffers(1, &m_DepthAttachment);

			m_ColorAttachments.clear();
			m_DepthAttachment = 0;
		}

		glCreateFramebuffers(1, &m_RendererID);
		glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);

		bool multisample = m_Spec.Samples > 1;

		// Color Attachments
		if (m_ColorAttachmentSpecifications.size())
		{
			m_ColorAttachments.resize(m_ColorAttachmentSpecifications.size());

			// Creating Texture First
			Utils::CreateTextures2D(multisample, m_ColorAttachments.data(), m_ColorAttachments.size());

			for (size_t i = 0; i < m_ColorAttachments.size(); i++)
			{
				Utils::BindTexture2D(multisample, m_ColorAttachments[i]);
				switch (m_ColorAttachmentSpecifications[i].TextureFormat)
				{
				case FramebufferTextureFormat::RGBA8:
					Utils::AttachColorTexture2D(m_ColorAttachments[i], m_Spec.Samples, GL_RGBA8, GL_RGBA, m_Spec.Width, m_Spec.Height, m_Spec.FilterMode, m_Spec.WrapMode, i);
					break;
				case FramebufferTextureFormat::RED_INTEGER:
					Utils::AttachColorTexture2D(m_ColorAttachments[i], m_Spec.Samples, GL_R32I, GL_RED_INTEGER, m_Spec.Width, m_Spec.Height, m_Spec.FilterMode, m_Spec.WrapMode, i);
					break;
				}
			}
		}

		// Depth Color Attachments
		if (m_DepthAttachmentSpecification.TextureFormat != FramebufferTextureFormat::None)
		{
			// Creating Texture First
			switch (m_DepthAttachmentSpecification.TextureFormat)
			{
			case FramebufferTextureFormat::DEPTH:
			case FramebufferTextureFormat::DEPTH24STENCIL8:
				Utils::CreateTextures2D(multisample, &m_DepthAttachment, 1);
				Utils::BindTexture2D(multisample, m_DepthAttachment);
				break;

			case FramebufferTextureFormat::DEPTH_CUBE:
				Utils::CreateTextureCubeMap(&m_DepthCubeAttachment, 1);
				Utils::BindTextureCubeMap(m_DepthCubeAttachment);
				break;
			}
			
			// Then, Attach the texture
			switch (m_DepthAttachmentSpecification.TextureFormat)
			{
			// ID, Samples, Internal Format, Attachment Type, Width, Height, Filter, Wrap
			case FramebufferTextureFormat::DEPTH:
				Utils::AttachDepthTexture2D(m_DepthAttachment, m_Spec.Samples, GL_DEPTH_COMPONENT, GL_DEPTH_ATTACHMENT, m_Spec.Width, m_Spec.Height, m_Spec.FilterMode, m_Spec.WrapMode);
				break;
			case FramebufferTextureFormat::DEPTH24STENCIL8:
				Utils::AttachDepthTexture2D(m_DepthAttachment, m_Spec.Samples, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL_ATTACHMENT, m_Spec.Width, m_Spec.Height, m_Spec.FilterMode, m_Spec.WrapMode);
				break;
			case FramebufferTextureFormat::DEPTH_CUBE:
				Utils::AttachDepthTextureCubeMap(m_DepthCubeAttachment, GL_DEPTH_COMPONENT, GL_DEPTH_ATTACHMENT, m_Spec.Width, m_Spec.Height, m_Spec.FilterMode, m_Spec.WrapMode);
				break;
			}
		}

		if (m_ColorAttachments.size() > 1)
		{
			GLenum buffers[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2,  GL_COLOR_ATTACHMENT3 };
			glDrawBuffers(m_ColorAttachments.size(), buffers);
		}

		if(m_ColorAttachments.empty())
			glDrawBuffer(GL_NONE);
		if (m_Spec.ReadBuffer == false);
			glReadBuffer(GL_NONE);

		OGN_CORE_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Framebuffer incomplete");
		glBindFramebuffer(GL_FRAMEBUFFER, NULL);
	}

	void OpenGL_Framebuffer::Bind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);
		glViewport(0, 0, m_Spec.Width, m_Spec.Height);
	}

	void OpenGL_Framebuffer::Unbind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, NULL);
	}

	void OpenGL_Framebuffer::Resize(uint32_t width, uint32_t height)
	{
		if (width == 0 || height == 0 || width > s_MaxFramebufferSize || height > s_MaxFramebufferSize)
		{
			OGN_CORE_WARN("Attempted to resize framebuffer to {0}, {1}", width, height);
			return;
		}

		m_Spec.Width = width;
		m_Spec.Height = height;
		
		Invalidate();
	}

	int OpenGL_Framebuffer::ReadPixel(uint32_t attachmentIndex, int x, int y)
	{
		bool check = attachmentIndex < m_ColorAttachments.size();

		if (!check)
		{
			OGN_CORE_WARN("attachment index : {}", attachmentIndex);
			OGN_CORE_WARN("color attachments size : {}", m_ColorAttachments.size());
			OGN_CORE_ASSERT(false, "Framebuffer attachment index is lower than Color Attachments size");
		}

		glReadBuffer(GL_COLOR_ATTACHMENT0 + attachmentIndex);
		int pixelData;
		glReadPixels(x, y, 1, 1, GL_RED_INTEGER, GL_INT, &pixelData);
		return pixelData;
	}

	void OpenGL_Framebuffer::ClearAttachment(uint32_t attachmentIndex, int value)
	{
		auto& spec = m_ColorAttachmentSpecifications[attachmentIndex];
		glClearTexImage(m_ColorAttachments[attachmentIndex], 0,
			Utils::ORiginFBTextureFormatToGL(spec.TextureFormat), GL_INT, &value);
	}
}
