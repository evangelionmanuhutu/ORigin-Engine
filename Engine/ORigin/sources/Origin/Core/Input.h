// Copyright (c) 2022 Evangelion Manuhutu | ORigin Engine

#ifndef INPUT_H
#define INPUT_H

#include "KeyCodes.h"
#include "MouseCodes.h"

#include <glm/glm.hpp>

namespace origin
{
	class OGN_API Input
	{
	public:
		Input();
		void Init(void *window);
		bool IsKeyReleased(const KeyCode keycode);
		bool IsKeyPressed(const KeyCode keycode);
		bool IsMouseButtonPressed(const MouseCode button);
		float GetMouseX();
		float GetMouseY();
		void SetMousePosition(float x, float y);
		void SetMouseLastPosition(float x, float y);

		void Update();
		void ToggleMouseLock();
		void MouseHide();
		void MouseUnHide();
		void SetMouseHide(bool hide);
		bool IsHidden() const;
		bool IsDragging() const;
		void SetMouseToCenter();
		void ResetMouseDelta();
		glm::vec2 GetMousePosition() const;
		glm::vec2 GetMouseDelta() const;
		void *GetWindow();
		static Input &Get();


	private:
		static Input* s_Instance;
		bool m_IsMouseLocked = false;
		bool m_IsMouseDragging = false;
		bool m_IsMouseHidden = false;
		glm::vec2 m_MouseDelta = { 0.0f, 0.0f };
		glm::vec2 m_RawMouseDelta = { 0.0f, 0.0f };
		glm::vec2 m_LastMousePosition = { 0.0f, 0.0f };
		friend class EditorCamera;
	};
}

#endif