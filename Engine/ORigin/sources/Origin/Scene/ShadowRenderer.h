// Copyright (c) 2022-present Evangelion Manuhutu | ORigin Engine

#ifndef SHADOW_RENDERER_H
#define SHADOW_RENDERER_H

#include "Components/Components.h"
#include "Origin/Renderer/Framebuffer.h"
#include "Origin/Renderer/UniformBuffer.h"
#include "Origin/Renderer/Frustum.h"

namespace origin 
{
	enum class OGN_API LightingType;

	class OGN_API ShadowRenderer
	{
	public:
		void OnCreate(LightingType type);
		void BindFBO();
		void UnbindFBO();
		static std::shared_ptr<ShadowRenderer> Create(const std::shared_ptr<Shader>& depthShader, LightingType type);
		glm::mat4 ViewProjection;
		glm::mat4 ShadowProjection;
		uint32_t FrameBufferID = 0, DepthMapID = 0;
		std::shared_ptr<Shader> &GetDepthShader() { return m_DepthMapShader; }

	private:
		LightingType m_LightingType;
		std::shared_ptr<Shader> m_DepthMapShader;
		friend class Lighting;
	};
}

#endif