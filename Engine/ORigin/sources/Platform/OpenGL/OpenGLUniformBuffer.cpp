// Copyright (c) Evangelion Manuhutu | ORigin Engine

#include "pch.h"

#include "OpenGLUniformBuffer.h"
#include "Origin/Core/Assert.h"
#include "Origin/Profiler/Profiler.h"

#include <glad/glad.h>

namespace origin
{
	OpenGLUniformBuffer::OpenGLUniformBuffer(u32 bufferSize, u32 bindingPoint)
		: m_BindingPoint(bindingPoint)
	{
		OGN_PROFILER_RENDERING();

		glCreateBuffers(1, &m_RendererID);
		glBindBuffer(GL_UNIFORM_BUFFER, m_RendererID);
		glBufferData(GL_UNIFORM_BUFFER, bufferSize, NULL, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, m_RendererID);
	}

	OpenGLUniformBuffer::~OpenGLUniformBuffer()
	{
		OGN_PROFILER_RENDERING();

		glDeleteBuffers(1, &m_RendererID);
	}

	void OpenGLUniformBuffer::Bind()
	{
		OGN_PROFILER_RENDERING();

		glBindBufferBase(GL_UNIFORM_BUFFER, m_BindingPoint, m_RendererID);
	}

	void OpenGLUniformBuffer::Unbind()
	{
		OGN_PROFILER_RENDERING();

		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}

	void OpenGLUniformBuffer::SetData(const void* data, u32 size, u32 offset)
	{
		OGN_PROFILER_RENDERING();

		// call this had any changes
		
		// try to use glBindBufferBase (Bind function) also
		// if the uniform does not retrieve any changes
		
		glBindBuffer(GL_UNIFORM_BUFFER, m_RendererID);
		glBufferSubData(GL_UNIFORM_BUFFER, offset, size, data);
	}

	u32 OpenGLUniformBuffer::Get()
	{
		return m_RendererID;
	}

}
