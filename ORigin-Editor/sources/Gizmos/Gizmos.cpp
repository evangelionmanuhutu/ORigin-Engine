#include "Gizmos.h"

#include "Origin\Scene\Components.h"
#include "Origin\Renderer\Renderer2D.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/compatibility.hpp>
#include "..\EditorLayer.h"

namespace origin {

#define GRID2D_ZOFFSET -5.0f
#define ICON_ZOFFSET 1.0f
#define COLLIDER2D_ZOFFSET 1.05f
#define SELECTED2D_ZOFFSET 0.1f

#define BOUNDARY2D_ID -2

	void Gizmos::Draw2DVerticalGrid(const EditorCamera &camera)
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

		Renderer2D::End();
	}

	void Gizmos::Draw2DOverlay()
	{
		auto &editor = EditorLayer::Get();

		if (editor.m_VisualizeCollider)
		{
			auto& scene = EditorLayer::Get().m_ActiveScene;

			const auto& quad = scene->GetAllEntitiesWith<TransformComponent, BoxCollider2DComponent>();
			for (auto entity : quad)
			{
				const auto& [tc, bc2d] = quad.get<TransformComponent, BoxCollider2DComponent>(entity);

				glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(glm::vec2(tc.Translation) + bc2d.Offset, COLLIDER2D_ZOFFSET))
					* glm::rotate(glm::mat4(1.0f), tc.Rotation.z, glm::vec3(0.0f, 0.0f, 1.0f))
					* glm::scale(glm::mat4(1.0f), glm::vec3(glm::vec2(tc.Scale) * bc2d.Size * 2.0f, 1.0f));

				Renderer2D::DrawRect(transform, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), (int)entity);
			}

			const auto& circle = scene->GetAllEntitiesWith<TransformComponent, CircleCollider2DComponent>();
			for (auto entity : circle)
			{
				const auto& [tc, cc2d] = circle.get<TransformComponent, CircleCollider2DComponent>(entity);

				glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(glm::vec2(tc.Translation) + cc2d.Offset, tc.Translation.z + COLLIDER2D_ZOFFSET))
					* glm::scale(glm::mat4(1.0f), glm::vec3(glm::vec2(tc.Scale * cc2d.Radius * 2.0f), 1.0f));

				Renderer2D::DrawCircle(transform, glm::vec4(0, 1, 0, 1), 0.05f, 0.0f, (int)entity);
			}
		}

		if (Entity selectedEntity = editor.m_SceneHierarchy.GetSelectedEntity())
		{
			const auto& tc = selectedEntity.GetComponent<TransformComponent>();
			glm::mat4 rotation = glm::toMat4(glm::quat(tc.Rotation));

			if (selectedEntity.HasComponent<SpriteRenderer2DComponent>())
			{
				glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(tc.Translation.x, tc.Translation.y, tc.Translation.z + SELECTED2D_ZOFFSET))
					* rotation * glm::scale(glm::mat4(1.0f), tc.Scale);
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
		}
		Renderer2D::End();
	}

	void Gizmos::DrawOverlay(const EditorCamera &camera)
	{
		auto& scene = EditorLayer::Get().m_ActiveScene;

		if (EditorLayer::Get().m_VisualizeCollider)
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

			const auto& box = scene->GetAllEntitiesWith<TransformComponent, BoxColliderComponent>();
			for (auto entity : box)
			{
				const auto& [tc, bc] = box.get<TransformComponent, BoxColliderComponent>(entity);

				glm::vec3 scale = tc.Scale * glm::vec3(bc.Size * 2.0f);

				glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(tc.Translation + bc.Offset))
					* glm::toMat4(glm::quat(tc.Rotation))
					* glm::scale(glm::mat4(1.0f), scale * 2.0f);

				Renderer3D::DrawCube(transform, glm::vec4(1.0f, 0.0f, 1.0f, 1.0f), (int)entity);
			}

			const auto& sphere = scene->GetAllEntitiesWith<TransformComponent, SphereColliderComponent>();
			for (auto entity : sphere)
			{
				const auto& [tc, sc] = sphere.get<TransformComponent, SphereColliderComponent>(entity);

				glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(tc.Translation + sc.Offset))
					* glm::toMat4(glm::quat(tc.Rotation))
					* glm::scale(glm::mat4(1.0f), tc.Scale);

				Renderer3D::DrawSphere(transform,
					glm::vec4(1.0f, 0.0f, 1.0f, 1.0f),
					(sc.Radius + 0.1f) * 2.0f,
					(int)entity);
			}

			const auto& capsule = scene->GetAllEntitiesWith<TransformComponent, CapsuleColliderComponent>();
			for (auto entity : capsule)
			{
				const auto& [tc, cc] = capsule.get<TransformComponent, CapsuleColliderComponent>(entity);
				glm::vec3 rot = tc.Rotation + glm::vec3(0.0f, 0.0f, glm::radians(90.0f));

				glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(tc.Translation + cc.Offset))
					* glm::toMat4(glm::quat(rot))
					* glm::scale(glm::mat4(1.0f), tc.Scale);

				Renderer3D::DrawCapsule(transform, glm::vec4(1.0f, 0.0f, 1.0f, 1.0f), cc.Radius, cc.Height * 2.0f, (int)entity);
			}

			Renderer3D::End();
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

			const auto& revoluteJoint2DView = scene->m_Registry.view<TransformComponent, RevoluteJoint2DComponent>();
			for (auto e : revoluteJoint2DView)
			{
				auto& [tc, rjc] = revoluteJoint2DView.get<TransformComponent, RevoluteJoint2DComponent>(e);
				glm::vec2 anchorPoint = glm::vec2(tc.Translation.x + rjc.AnchorPoint.x, tc.Translation.y + rjc.AnchorPoint.y);
				glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(anchorPoint, tc.Translation.z + COLLIDER2D_ZOFFSET))
					* glm::scale(glm::mat4(1.0f), glm::vec3(0.1f));

				Renderer2D::DrawCircle(transform, glm::vec4(0.4f, 1.0f, 0.4f, 1.0f), 100.0f);
			}
			Renderer2D::End();
		}
	}

	void Gizmos::DrawIcons(const EditorCamera &camera)
	{
		auto &textures = EditorLayer::Get().m_UITextures;
		auto &reg = EditorLayer::Get().m_ActiveScene->m_Registry;

		auto drawIcon = [&](TransformComponent tc, const std::shared_ptr<Texture2D> &texture, int entity)
			{
				glm::mat4 transform = glm::mat4(1.0f);
				float scale = glm::clamp(glm::length(camera.GetPosition() - tc.Translation) * 0.05f, 1.0f, 10.0f);

				switch (camera.GetProjectionType())
				{
				case ProjectionType::Perspective:
					transform = glm::translate(glm::mat4(1.0f), tc.Translation)
						* glm::rotate(glm::mat4(1.0f), -camera.GetYaw(), glm::vec3(0, 1, 0))
						* glm::rotate(glm::mat4(1.0f), -camera.GetPitch(), glm::vec3(1, 0, 0))
						* glm::scale(glm::mat4(1.0f), glm::vec3(scale));
					break;

				case ProjectionType::Orthographic:
					transform = translate(glm::mat4(1.0f), glm::vec3(tc.Translation.x, tc.Translation.y, ICON_ZOFFSET));
					break;
				}

				Renderer2D::DrawQuad(transform, texture, (int)entity, glm::vec2(1.0f), glm::vec4(1.0f));
			};

		auto& cam = reg.view<TransformComponent, CameraComponent>();
		for (auto& entity : cam)
		{
			auto& [tc, cc] = cam.get<TransformComponent, CameraComponent>(entity);
			auto& sceneCam = cc.Camera;

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

					glm::vec3 pos = camera.GetProjectionType() == ProjectionType::Perspective ?
						glm::vec3(tc.Translation) : glm::vec3(tc.Translation.x, tc.Translation.y, 1.3f);

					glm::vec3 rotation = camera.GetProjectionType() == ProjectionType::Perspective ?
						glm::vec3(tc.Rotation) : glm::vec3(0.0f, 0.0f, tc.Rotation.z);

					glm::mat4 transform = glm::translate(glm::mat4(1.0f), pos)
						* glm::toMat4(glm::qua(rotation)) * scale;

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

		auto& lighting = reg.view<TransformComponent, LightComponent>();
		for (auto& entity : lighting)
		{
			auto& tc = lighting.get<TransformComponent>(entity);
			drawIcon(tc, textures.at("lighting"), (int)entity);
		}

		Renderer2D::End();
	}

	void Gizmos::OnRender(const EditorCamera &camera)
	{
		Renderer3D::Begin(camera);
		Renderer2D::Begin(camera);

		if (camera.GetProjectionType() == ProjectionType::Orthographic)
		{
			if (Entity selectedEntity = EditorLayer::Get().m_SceneHierarchy.GetSelectedEntity())
			{
				if (m_Type == GizmoType::BOUNDARY2D)
				{
					CalculateBoundary2DSizing(camera);

					float size = camera.GetOrthoSize() * 0.03f;
					auto &tc = selectedEntity.GetComponent<TransformComponent>();

					// bottom left corner
					glm::vec4 red = glm::vec4(0.8f, 0.1f, 0.1f, 1.0f);
					glm::vec4 green = glm::vec4(0.1f, 0.8f, 0.1f, 1.0f);

					glm::vec4 col = m_Boundary2DCorner == Boundary2DCorner::BOTTOM_LEFT ? green : red;

					std::vector<glm::vec3> cornerOffsets = {
							{ -tc.Scale.x / 2.0f, -tc.Scale.y / 2.0f, 1.0f }, // Bottom left
							{ -tc.Scale.x / 2.0f,  tc.Scale.y / 2.0f, 1.0f }, // Top left
							{  tc.Scale.x / 2.0f, -tc.Scale.y / 2.0f, 1.0f }, // Bottom right
							{  tc.Scale.x / 2.0f,  tc.Scale.y / 2.0f, 1.0f }  // Top right
					};

					for (int i = 0; i < 4; ++i)
					{
						glm::vec4 col = (m_Boundary2DCorner == static_cast<Boundary2DCorner>(i)) ? green : red;

						glm::quat rotationQuat = glm::quat(tc.Rotation);

						glm::mat4 tf = glm::translate(glm::mat4(1.0f), tc.Translation + glm::vec3(rotationQuat * glm::vec4(cornerOffsets[i], 0.0f))) *
							glm::toMat4(rotationQuat) * glm::scale(glm::mat4(1.0f), glm::vec3(size));

						Renderer2D::DrawQuad(tf, col, BOUNDARY2D_ID - (i + 1));
					}
				}
				Renderer2D::End();
			}

			RenderCommand::SetLineWidth(1.1f);
			Draw2DOverlay();
			Draw2DVerticalGrid(camera);
			RenderCommand::SetLineWidth(1.0f);
		}

		DrawOverlay(camera);
		DrawIcons(camera);
	}

	void Gizmos::OnEvent(Event &e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<MouseButtonPressedEvent>(OGN_BIND_EVENT_FN(Gizmos::OnMouseButtonPressed));
	}

	bool Gizmos::OnMouseButtonPressed(MouseButtonPressedEvent &e)
	{
		if (e.GetMouseButton() == Mouse::ButtonLeft)
		{
			switch (m_Hovered)
			{
				// bottom left
				case BOUNDARY2D_ID - 1:
					m_Boundary2DCorner = Boundary2DCorner::BOTTOM_LEFT;
					break;
				// top left
				case BOUNDARY2D_ID - 2:
					m_Boundary2DCorner = Boundary2DCorner::TOP_LEFT;
					break;
				// bottom right
				case BOUNDARY2D_ID - 3:
					m_Boundary2DCorner = Boundary2DCorner::BOTTOM_RIGHT;
					break;
				// top right
				case BOUNDARY2D_ID - 4:
					m_Boundary2DCorner = Boundary2DCorner::TOP_RIGHT;
					break;
				default:
					m_Boundary2DCorner = Boundary2DCorner::NONE;
					break;
			}
		}

		return false;
	}

	void Gizmos::SetType(GizmoType type)
	{
		if (m_Hovered >= -1)
			m_Type = type;
	}

	void Gizmos::CalculateBoundary2DSizing(const EditorCamera &camera)
	{
		static glm::vec2 initialPosition = { 0.0f, 0.0f };
		const glm::vec2 mouse { Input::GetMouseX(), Input::GetMouseY() };
		const glm::vec2 delta { mouse - initialPosition };
		initialPosition = mouse;

		float orthoScale = camera.GetOrthoSize() / camera.GetHeight();

		if (Entity selectedEntity = EditorLayer::Get().m_SceneHierarchy.GetSelectedEntity())
		{
			if (Input::IsMouseButtonPressed(Mouse::ButtonLeft) && EditorLayer::Get().m_SceneViewportHovered)
			{
				auto &tc = selectedEntity.GetComponent<TransformComponent>();
				glm::vec2 localDelta = delta;

				glm::vec4 transformedDelta = glm::quat(glm::radians(tc.Rotation)) * glm::vec4(delta, 0.0f, 1.0f);
				localDelta.x = transformedDelta.x;
				localDelta.y = transformedDelta.y;

				switch (m_Boundary2DCorner)
				{
					case Boundary2DCorner::TOP_RIGHT:
						tc.Scale.x += localDelta.x * orthoScale;
						tc.Scale.y -= localDelta.y * orthoScale;
						tc.Translation.x -= localDelta.x * orthoScale / 2.0f;
						tc.Translation.y += localDelta.y * orthoScale / 2.0f;
						break;
					case Boundary2DCorner::BOTTOM_RIGHT:
						tc.Scale.x += localDelta.x * orthoScale;
						tc.Scale.y += localDelta.y * orthoScale;
						tc.Translation.x -= localDelta.x * orthoScale / 2.0f;
						tc.Translation.y += localDelta.y * orthoScale / 2.0f;
						break;
					case Boundary2DCorner::TOP_LEFT:
						tc.Scale.x -= localDelta.x * orthoScale;
						tc.Scale.y -= localDelta.y * orthoScale;
						tc.Translation.x -= localDelta.x * orthoScale / 2.0f;
						tc.Translation.y += localDelta.y * orthoScale / 2.0f;
						break;
					case Boundary2DCorner::BOTTOM_LEFT:
						tc.Scale.x -= localDelta.x * orthoScale;
						tc.Scale.y += localDelta.y * orthoScale;
						tc.Translation.x -= localDelta.x * orthoScale / 2.0f;
						tc.Translation.y += localDelta.y * orthoScale / 2.0f;
						break;
				}
			}
		}
	}

}
