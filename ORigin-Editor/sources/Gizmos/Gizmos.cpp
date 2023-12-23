#include "Gizmos.h"

#include "Origin\Renderer\Renderer2D.h"
#include "..\EditorLayer.h"

namespace origin {

#define GRID2D_ZOFFSET 0.9f
#define SELECTED2D_ZOFFSET 1.0f
#define ICON_ZOFFSET 1.1f
#define COLLIDER2D_ZOFFSET 1.2f

	void Gizmos::OnUpdate(const EditorCamera& camera)
	{
		Renderer::BeginScene(camera);

		DrawVerticalGrid(camera);
		DrawOverlay(camera);

	}

	void Gizmos::DrawVerticalGrid(const EditorCamera& camera)
	{
		float orthoSize = camera.GetOrthoSize();
		glm::vec2 cameraPosition = glm::vec2(camera.GetPosition());

		float lineSpacing = 1.0f;
		if (orthoSize >= 20.0f)
			lineSpacing = pow(5.0f, floor(log10(orthoSize)));

		float offset = -0.5f * lineSpacing;

		float minX = cameraPosition.x - orthoSize - camera.GetWidth() / 10.0f;
		float maxX = cameraPosition.x + orthoSize + camera.GetWidth() / 10.0f;
		float minY = cameraPosition.y - orthoSize - camera.GetHeight() / 10.0f;
		float maxY = cameraPosition.y + orthoSize + camera.GetHeight() / 10.0f;

		auto nx = floor(minX / lineSpacing) * lineSpacing;
		auto ny = floor(minY / lineSpacing) * lineSpacing;

		glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 0.5f);

		for (float x = nx; x <= maxX; x += lineSpacing)
			Renderer2D::DrawLine(glm::vec3(x + offset, minY, GRID2D_ZOFFSET), glm::vec3(x + offset, maxY, GRID2D_ZOFFSET), color);

		for (float y = ny; y <= maxY; y += lineSpacing)
			Renderer2D::DrawLine(glm::vec3(minX, y + offset, GRID2D_ZOFFSET), glm::vec3(maxX, y + offset, GRID2D_ZOFFSET), color);

		Renderer2D::EndScene();
	}

	void Gizmos::DrawOverlay(const EditorCamera& camera)
	{
		
		auto textures = EditorLayer::Get().m_UITextures;
		auto& reg = EditorLayer::Get().m_ActiveScene->m_Registry;

		auto drawIcon = [&](TransformComponent tc, std::shared_ptr<Texture2D> texture, int entity)
			{
				glm::mat4 transform = glm::mat4(1.0f);
				switch (camera.GetProjectionType())
				{
				case ProjectionType::Perspective:
					transform = glm::translate(glm::mat4(1.0f), tc.Translation)
						* glm::rotate(glm::mat4(1.0f), -camera.GetYaw(), glm::vec3(0, 1, 0))
						* glm::rotate(glm::mat4(1.0f), -camera.GetPitch(), glm::vec3(1, 0, 0))
						* glm::scale(glm::mat4(1.0f), glm::vec3(1.0f));
					break;

				case ProjectionType::Orthographic:
					transform = translate(glm::mat4(1.0f), glm::vec3(tc.Translation.x, tc.Translation.y, ICON_ZOFFSET));
					break;

				}

				Renderer2D::DrawQuad(transform, texture, glm::vec2(1.0f), glm::vec4(1.0f), (int)entity);
			};

		auto& cam = reg.view<TransformComponent, CameraComponent>();
		for (auto& entity : cam)
		{
			auto& [tc, cc] = cam.get<TransformComponent, CameraComponent>(entity);
			auto& sceneCam= cc.Camera;

			if (camera.GetOrthoSize() > 10.0f || camera.GetProjectionType() == ProjectionType::Perspective)
			{
				drawIcon(tc, textures.at("camera"), (int)entity);

				float sizeX = 0.0f;
				float sizeY = 0.0f;

				if (cc.Camera.GetAspectRatioType() == SceneCamera::AspectRatioType::SixteenByNine)
				{
					sizeX = sceneCam.GetOrthographicSize() * sceneCam.GetAspectRatioSize();
					sizeY = sizeX / 16.0f * 9.0f;

					glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(sizeX, sizeY, 1.0f));

					glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(tc.Translation.x, tc.Translation.y, 1.3f))
						* glm::toMat4(glm::qua(tc.Rotation)) * scale;

					Renderer2D::DrawRect(transform, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
				}
			}
		}

		auto& audio = reg.view<TransformComponent, AudioComponent>();
		for (auto& entity : audio)
		{
			auto& tc = audio.get<TransformComponent>(entity);
			drawIcon(tc, textures.at("audio"), (int)entity);
		}

		auto& lights = reg.view<TransformComponent, AudioComponent>();
		for (auto& entity : lights)
		{
			auto& tc = lights.get<TransformComponent>(entity);
			drawIcon(tc, textures.at("audio"), (int)entity);
		}

		Renderer2D::EndScene();

		if (EditorLayer::Get().m_VisualizeCollider)
		{
			auto& scene = EditorLayer::Get().m_ActiveScene;

			const auto& circle = scene->GetAllEntitiesWith<TransformComponent, CircleCollider2DComponent>();
			for (auto entity : circle)
			{
				const auto& [tc, cc2d] = circle.get<TransformComponent, CircleCollider2DComponent>(entity);

				glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(glm::vec2(tc.Translation) + cc2d.Offset, tc.Translation.z + COLLIDER2D_ZOFFSET))
					* glm::scale(glm::mat4(1.0f), glm::vec3(glm::vec2(tc.Scale * cc2d.Radius * 2.0f), 1.0f));

				Renderer2D::DrawCircle(transform, glm::vec4(0, 1, 0, 1), 0.05f, (int)entity);
			}

			const auto& quad = scene->GetAllEntitiesWith<TransformComponent, BoxCollider2DComponent>();
			for (auto entity : quad)
			{
				const auto& [tc, bc2d] = quad.get<TransformComponent, BoxCollider2DComponent>(entity);

				glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(glm::vec2(tc.Translation) + bc2d.Offset, COLLIDER2D_ZOFFSET))
					* glm::rotate(glm::mat4(1.0f), tc.Rotation.z, glm::vec3(0.0f, 0.0f, 1.0f))
					* glm::scale(glm::mat4(1.0f), glm::vec3(glm::vec2(tc.Scale) * bc2d.Size * 2.0f, 1.0f));

				Renderer2D::DrawRect(transform, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), (int)entity);
			}

			const auto& box = scene->GetAllEntitiesWith<TransformComponent, BoxColliderComponent>();
			for (auto entity : box)
			{
				const auto& [tc, bc] = box.get<TransformComponent, BoxColliderComponent>(entity);

				glm::vec3 scale = tc.Scale * glm::vec3(bc.Size * 2.0f);

				glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(tc.Translation + bc.Offset))
					* glm::toMat4(glm::quat(tc.Rotation))
					* glm::scale(glm::mat4(1.0f), scale * 2.0f);
				Renderer3D::DrawRect(transform, glm::vec4(1.0f, 0.0f, 1.0f, 1.0f), (int)entity);
			}

			const auto& sphere = scene->GetAllEntitiesWith<TransformComponent, SphereColliderComponent>();
			for (auto entity : sphere)
			{
				const auto& [tc, cc] = sphere.get<TransformComponent, SphereColliderComponent>(entity);
				glm::vec3 scale = tc.Scale * glm::vec3(cc.Radius * 2.0f);

				glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(tc.Translation + cc.Offset))
					* glm::rotate(glm::mat4(1.0f), tc.Rotation.x, glm::vec3(1, 0, 0))
					* glm::rotate(glm::mat4(1.0f), 0.0f, glm::vec3(0, 1, 0))
					* glm::rotate(glm::mat4(1.0f), tc.Rotation.z, glm::vec3(0, 0, 1))
					* glm::scale(glm::mat4(1.0f), glm::vec3(scale * 2.1f));

				Renderer2D::DrawCircle(transform, glm::vec4(0.7f, 0.0f, 1.0f, 1.0f), 1.0f);

				transform = glm::translate(glm::mat4(1.0f), glm::vec3(tc.Translation + cc.Offset))
					* glm::rotate(glm::mat4(1.0f), tc.Rotation.x + glm::radians(90.0f), glm::vec3(1, 0, 0))
					* glm::rotate(glm::mat4(1.0f), tc.Rotation.z, glm::vec3(0, 1, 0))
					* glm::rotate(glm::mat4(1.0f), 0.0f, glm::vec3(0, 0, 1))
					* glm::scale(glm::mat4(1.0f), glm::vec3(scale * 2.1f));

				Renderer2D::DrawCircle(transform, glm::vec4(0.7f, 0.0f, 1.0f, 1.0f), 1.0f, (int)entity);
			}
		}

		if (Entity selectedEntity = EditorLayer::Get().m_SceneHierarchy.GetSelectedEntity())
		{
			const auto& tc = selectedEntity.GetComponent<TransformComponent>();
			glm::mat4 rotation = glm::toMat4(glm::quat(tc.Rotation));

			if (selectedEntity.HasComponent<SpriteRenderer2DComponent>())
			{
				glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(tc.Translation.x, tc.Translation.y, tc.Translation.z + SELECTED2D_ZOFFSET))
					* rotation * glm::scale(glm::mat4(1.0f), tc.Scale);

				Renderer2D::DrawQuad(transform, glm::vec4(1.0f, 1.0f, 1.0f, 0.3f));
				Renderer2D::DrawRect(transform, glm::vec4(1.0f, 0.5f, 0.0f, 1.0f));
			}

			if (selectedEntity.HasComponent<CircleRendererComponent>())
			{
				glm::vec3 translation = tc.Translation + glm::vec3(0.0f, 0.0f, 0.5f);
				glm::vec3 scale = tc.Scale * glm::vec3(1.0f);

				glm::mat4 transform = glm::translate(glm::mat4(1.0f), translation)
					* rotation * glm::scale(glm::mat4(1.0f), scale);

				Renderer2D::DrawCircle(transform, glm::vec4(1.0f, 0.5f, 0.0f, 1.0f), 0.05f);
			}

			if (selectedEntity.HasComponent<SpriteRendererComponent>())
			{
				glm::vec3 translation = tc.Translation + glm::vec3(0.0f, 0.0f, 0.0f);
				glm::mat4 transform = glm::translate(glm::mat4(1.0f), translation)
					* rotation * glm::scale(glm::mat4(1.0f), tc.Scale);

				Renderer3D::DrawRect(transform, glm::vec4(1.0f, 0.5f, 0.0f, 1.0f));
			}
		}
		Renderer2D::EndScene();
	}
}
