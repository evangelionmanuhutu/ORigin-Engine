// Copyright (c) Evangelion Manuhutu | ORigin Engine

#pragma once
#include "Origin/Core/Application.h"
#include "Origin/Animation/Animation.h"
#include "Origin/Animation/AnimationState.h"
#include "Origin/Audio/AudioSource.h"
#include "Origin/Audio/AudioListener.h"
#include "Origin/Profiler/Profiler.h"
#include "Origin/Core/ConsoleManager.h"
#include "Origin/Core/Input.h"
#include "Origin/Core/KeyCodes.h"
#include "Origin/Core/MouseCodes.h"
#include "Origin/Core/CommandManager.h"
#include "Origin/Core/Event.h"
#include "Origin/Core/KeyEvent.h"
#include "Origin/Core/MouseEvent.h"
#include "Origin/Math/Math.h"
#include "Origin/Math/AABB.h"
#include "Origin/Project/Project.h"
#include "Origin/Renderer/VertexArray.h"
#include "Origin/Renderer/Buffer.h"
#include "Origin/Renderer/Renderer.h"
#include "Origin/Renderer/RenderCommand.h"
#include "Origin/Renderer/Renderer2D.h"
#include "Origin/Renderer/Renderer3D.h"
#include "Origin/Renderer/ShaderLibrary.h"
#include "Origin/Renderer/MaterialLibrary.h"
#include "Origin/Renderer/Shader.h"
#include "Origin/Renderer/ParticleSystem.h"
#include "Origin/Renderer/Texture.h"
#include "Origin/Renderer/SubTexture2D.h"
#include "Origin/Renderer/Framebuffer.h"
#include "Origin/Renderer/MeshRenderer.h"
#include "Origin/Renderer/ModelLoader.h"
#include "Origin/Renderer/Frustum.h"
#include "Origin/Scene/Entity.h"
#include "Origin/Scene/SceneCommand.h"
#include "Origin/Scene/ScriptableEntity.h"
#include "Origin/Scene/Skybox.h"
#include "Origin/Scene/EditorCamera.h"
#include "Origin/Scene/SceneCamera.h"
#include "Origin/Scene/Camera.h"
#include "Origin/Scene/Components/Components.h"
#include "Origin/Scene/Components/ComponentsCommand.h"
#include "Origin/Scene/Components/PhysicsComponents.h"
#include "Origin/Physics/PhysicsEngine.h"
#include "Origin/Physics/2D/Physics2D.h"
#include "Origin/Physics/2D/Contact2DListener.h"
#include "Origin/Scene/Lighting.h"
#include "Origin/Scene/SpriteSheet.h"
#include "Origin/Serializer/ProjectSerializer.h"
#include "Origin/Serializer/Serializer.h"
#include "Origin/Serializer/SceneSerializer.h"
#include "Origin/Serializer/SpriteSheetSerializer.h"
#include "Origin/Serializer/MaterialSerializer.h"
#include "Origin/GUI/GuiLayer.h"
#include "Origin/Core/Time.h"
#include "Origin/Core/Assert.h"