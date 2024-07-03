// Copyright (c) 2022 Evangelion Manuhutu | ORigin Engine
#pragma once
#include "Origin/Scene/EditorCamera.h"
#include "Origin/Scene/SceneCamera.h"

#include <memory>

namespace origin
{
	class Skybox
	{
	public:
		virtual void Draw(const glm::mat4& viewProjection) = 0;
		virtual void Draw(const SceneCamera& camera) = 0;
		virtual void Draw(const EditorCamera& camera) = 0;
		virtual void SetBlur(float blur) = 0;
		static std::shared_ptr<Skybox> Create(const std::string& filepath, const std::string& format);
	};

}

