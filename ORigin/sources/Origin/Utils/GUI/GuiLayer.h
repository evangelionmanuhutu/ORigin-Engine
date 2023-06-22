// Copyright (c) 2022 Evangelion Manuhutu | ORigin Engine

#pragma once
#include "Origin\Utils\Layer.h"

#include "Origin\IO\Events\AppEvent.h"
#include "Origin\IO\Events\KeyEvent.h"
#include "Origin\IO\Events\MouseEvent.h"

namespace origin
{
	class GuiLayer : public Layer
	{
	public:
		GuiLayer() : Layer("Gui Layer") {}
		~GuiLayer();

		void OnAttach() override;
		void OnDetach() override;
		void OnEvent(Event& e) override;
		void BlockEvents(bool block) { m_BlockEvents = block; }

		void Begin();
		void End();
		uint32_t GetActiveWidgetID();

	private:
		bool m_BlockEvents = false;
		float m_Time = 0.0f;
	};

}