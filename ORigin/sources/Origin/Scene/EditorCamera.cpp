// Copyright (c) Evangelion Manuhutu | ORigin Engine

#include "pch.h"
#include "EditorCamera.h"
#include "Origin/Core/Application.h"
#include "Origin/Core/Input.h"
#include "Origin/Core/KeyCodes.h"
#include "Origin/Core/MouseCodes.h"
#include "Origin/Scene/Components.h"
#include "Origin/Scene/Entity.h"
#include <GLFW/glfw3.h>
#include <algorithm>

namespace origin {

	void EditorCamera::InitPerspective(float fovy, float aspectRatio, float nearClip, float farClip)
	{
		m_ProjectionType = ProjectionType::Perspective;
		m_FOV = fovy;
		m_AspectRatio = aspectRatio;
		m_NearClip = nearClip;
		m_FarClip = farClip;

		UpdateView();
		UpdateProjection();
	}

	void EditorCamera::InitOrthographic(float size, float nearClip, float farClip)
	{
		m_ProjectionType = ProjectionType::Orthographic;
		m_OrthoScale = size;
		m_OrthoNearClip = nearClip;
		m_OrthoFarClip = farClip;

		UpdateView();
		UpdateProjection();
	}

	void EditorCamera::UpdateAudioListener(Timestep ts)
	{
		m_AudioListener.Set(GetPosition(), glm::vec3(0.0f), GetForwardDirection(), GetUpDirection());
	}

	void EditorCamera::UpdateProjection()
	{
		m_AspectRatio = m_ViewportWidth / m_ViewportHeight;

		switch (m_ProjectionType)
		{
		case ProjectionType::Perspective:	
			m_Projection = glm::perspective(glm::radians(m_FOV), m_AspectRatio, m_NearClip, m_FarClip);
			break;

		case ProjectionType::Orthographic:
			m_OrthoSize.x = m_OrthoScale * m_AspectRatio / 2.0f;
			m_OrthoSize.y = m_OrthoScale / 2.0f;
			m_Projection = glm::ortho(-m_OrthoSize.x, m_OrthoSize.x, -m_OrthoSize.y, m_OrthoSize.y, m_OrthoNearClip, m_OrthoFarClip);
			break;
		}
	}

	float EditorCamera::GetZoomSpeed() const
	{
		float speed = 0.0f;
		float distance = 0.0f;
		switch (m_ProjectionType)
		{
		case ProjectionType::Perspective:
			distance = m_Distance * 0.2f;
			distance = std::max(distance, 0.0f);
			speed = distance * distance;
			speed = std::min(speed, 100.0f);
			break;
		case ProjectionType::Orthographic:
			speed = m_OrthoScale * 0.5f;
			break;
		}
		
		return speed;
	}

	void EditorCamera::SetViewportSize(float width, float height)
	{
		m_ViewportWidth = width;
		m_ViewportHeight = height;

		UpdateProjection();
	}

    void EditorCamera::SetPosition(const glm::vec3 &position)
    {
		m_Position = position;
		UpdateProjection();
		UpdateView();
    }

    void EditorCamera::UpdateView()
	{
		switch (m_ProjectionType)
		{
		case ProjectionType::Perspective:
			m_View = glm::translate(glm::mat4(1.0f), m_Position) * glm::toMat4(glm::quat(glm::vec3(-m_Pitch, -m_Yaw, 0.0f)));
			m_View = glm::inverse(m_View);
			break;
		case ProjectionType::Orthographic:
			m_View = glm::translate(glm::mat4(1.0f), m_Position);
			m_View = glm::inverse(m_View);
			break;
		}
	}

	std::pair<float, float> EditorCamera::PanSpeed() const
	{
		float xFactor = 0.0f;
		float yFactor = 0.0f;

		float x = std::min(m_ViewportWidth * 0.01f, 1.8f);
		float y = std::min(m_ViewportHeight * 0.01f, 1.8f);

		if (m_CameraStyle == Pivot)
		{
			xFactor = 0.0366f * (x * x) - 0.1778f * x + 0.3021f;
			yFactor = 0.0366f * (y * y) - 0.1778f * y + 0.3021f;
		}
		else if (m_CameraStyle == FreeMove)
		{
			xFactor = GetZoomSpeed();
			yFactor = GetZoomSpeed();
		}

		return { xFactor, yFactor };
	}

	float EditorCamera::RotationSpeed() const
	{
		return 0.8f;
	}

	void EditorCamera::OnUpdate(Timestep ts)
	{
		const glm::vec2 mouse { Input::GetMouseX(), Input::GetMouseY() };
		const glm::vec2 delta = (mouse - m_InitialMousePosition) * 0.003f;
		m_InitialMousePosition = mouse;

		const float wWidth = static_cast<float>(Application::Get().GetWindow().GetWidth());
		const float wHeight = static_cast<float>(Application::Get().GetWindow().GetHeight());

		if (Input::IsMouseButtonPressed(Mouse::ButtonRight)|| Input::IsMouseButtonPressed(Mouse::ButtonMiddle))
		{
			if (mouse.x > wWidth - 2.0f)
			{
				m_InitialMousePosition.x = 2.0f;
				Input::SetMousePosition(2.0f, mouse.y);
			}
			else if (mouse.x < 2.0f)
			{
				m_InitialMousePosition.x = wWidth - 2.0f;
				Input::SetMousePosition(wWidth - 2.0f, mouse.y);
			}

			if (mouse.y > wHeight - 2.0f)
			{
				m_InitialMousePosition.y = 2.0f;
				Input::SetMousePosition(mouse.x, 2.0f);
			}
			else if (mouse.y < 2.0f)
			{
				m_InitialMousePosition.y = wHeight - 2.0f;
				Input::SetMousePosition(mouse.x, wHeight - 2.0f);
			}
		}

		static glm::vec3 lastPosition = glm::vec3(0.0f);
		glm::vec3 velocity(0.0f);

		if (m_ProjectionType == ProjectionType::Perspective)
		{
			switch (m_CameraStyle)
			{
				case CameraStyle::Pivot:
					if (Input::IsMouseButtonPressed(Mouse::ButtonRight) && !Input::IsKeyPressed(Key::LeftControl))
						MouseRotate(delta);
					if (Input::IsMouseButtonPressed(Mouse::ButtonMiddle) || (Input::IsMouseButtonPressed(Mouse::ButtonRight) && Input::IsKeyPressed(Key::LeftControl)))
						MousePan(delta);

					m_Position = glm::lerp(m_Position, m_FocalPoint - GetForwardDirection() * m_Distance, 0.8f);
					lastPosition = m_FocalPoint - GetForwardDirection() * m_Distance;
					break;

				case CameraStyle::FreeMove:
					if (Input::IsMouseButtonPressed(Mouse::ButtonRight))
						MouseRotate(delta);
					if (Input::IsMouseButtonPressed(Mouse::ButtonMiddle))
						MousePan(delta);

					if (m_CanMoving)
					{
						if (Input::IsKeyPressed(Key::A))
						{
							velocity -= GetRightDirection();
							m_MoveSpeed += ts * 2.0f;
						}
						else if (Input::IsKeyPressed(Key::D))
						{
							velocity += GetRightDirection();
							m_MoveSpeed += ts * 2.0f;
						}
						if (Input::IsKeyPressed(Key::W))
						{
							velocity += GetForwardDirection();
							m_MoveSpeed += ts * 2.0f;
						}
						else if (Input::IsKeyPressed(Key::S))
						{
							velocity -= GetForwardDirection();
							m_MoveSpeed += ts * 2.0f;
						}
						else
						{
							m_MoveSpeed -= ts * 2.0f;
						}

						m_MoveSpeed = glm::clamp(m_MoveSpeed, 2.0f, 20.0f);
						m_Position = glm::lerp(m_Position, m_Position + velocity, ts * m_MoveSpeed);

						lastPosition = m_Position;
						m_Distance = 5.0f;
						m_FocalPoint = lastPosition + GetForwardDirection() * m_Distance;
					}
					
					break;
			}
		}

		else if (m_ProjectionType == ProjectionType::Orthographic)
		{
			if (Input::IsMouseButtonPressed(Mouse::ButtonMiddle) || (Input::IsMouseButtonPressed(Mouse::ButtonRight) && Input::IsKeyPressed(Key::LeftControl)))
				MousePan(delta);

			lastPosition = m_Position;
		}

		UpdateView();
		UpdateProjection();
	}

	void EditorCamera::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<MouseScrolledEvent>(OGN_BIND_EVENT_FN(EditorCamera::OnMouseScroll));
	}

	bool EditorCamera::OnMouseScroll(MouseScrolledEvent& e)
	{
		if (!m_CanScrolling)
			return false;

		float delta = e.GetYOffset();

		switch (m_ProjectionType)
		{
		case ProjectionType::Perspective:
			if (m_CameraStyle == Pivot)
			{
				MouseZoom(delta * 0.1f);
				UpdateView();
				UpdateProjection();
			}
			else if (m_CameraStyle == FreeMove)
			{
				m_MoveSpeed += delta;
			}
			break;
		case ProjectionType::Orthographic:
			MouseZoom(delta * 0.1f);
			UpdateView();
			UpdateProjection();
			break;
		}

		return true;
	}

	void EditorCamera::MousePan(const glm::vec2& delta)
	{
		if (!m_CanDragging)
			return;

		auto [xSpeed, ySpeed] = PanSpeed();

		switch (m_ProjectionType)
		{
		case ProjectionType::Perspective:
			if (m_CameraStyle == Pivot)
			{
				m_FocalPoint += -GetRightDirection() * delta.x * xSpeed * m_Distance;
				m_FocalPoint += GetUpDirection() * delta.y * ySpeed * m_Distance;
			}
			else
			{
				m_Position -= GetRightDirection() * delta.x * m_MoveSpeed;
				m_Position += GetUpDirection() * delta.y * m_MoveSpeed;
			}
			
			break;
		case ProjectionType::Orthographic:
			m_Position += glm::vec3(-delta.x * (m_OrthoScale / m_ViewportHeight) * 300.0f, delta.y * (m_OrthoScale / m_ViewportHeight) * 300.0f, 0.0f);
			break;
		}
	}

	void EditorCamera::MouseRotate(const glm::vec2& delta)
	{
		if (!m_CanDragging)
			return;

		float yawSign = GetUpDirection().y < 0 ? -1.0f : 1.0f;
		m_Yaw += yawSign * delta.x * RotationSpeed();
		m_Pitch += delta.y * RotationSpeed();
	}

	void EditorCamera::MouseZoom(const float delta)
	{
		switch (m_ProjectionType)
		{
		case ProjectionType::Perspective:
			m_Distance -= delta * GetZoomSpeed();
			if (m_Distance <= 1.0f)
			{
				m_FocalPoint += GetForwardDirection();
				m_Distance = 1.0f;
			}
			break;
		case ProjectionType::Orthographic:
			m_OrthoScale -= delta * GetZoomSpeed();
			m_OrthoScale = std::max(m_OrthoScale, 1.0f);
			m_OrthoScale = std::min(m_OrthoScale, m_MaxOrthoScale);
			break;
		}
	}

	const glm::mat4& EditorCamera::GetProjection() const
	{
		return m_Projection;
	}

	const glm::mat4 &EditorCamera::GetViewMatrix() const
	{
		return m_View;
	}

}