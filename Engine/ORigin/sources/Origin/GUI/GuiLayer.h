// Copyright (c) 2022-present Evangelion Manuhutu | ORigin Engine

#ifndef GUI_LAYER_H
#define GUI_LAYER_H

#include "Origin/Core/Layer.h"
#include "Origin/Core/AppEvent.h"
#include "Origin/Core/KeyEvent.h"
#include "Origin/Core/MouseEvent.h"

#include <vulkan/vulkan.h>

namespace origin
{
    class OGN_API Window;
    class OGN_API GuiLayer : public Layer
    {
    public:
        GuiLayer(const std::shared_ptr<Window> &window);

        void Init();
        void OnAttach() override;
        void OnDetach() override;
        void OnEvent(Event& e) override;
        void BlockEvents(bool block) { m_BlockEvents = block; }
        void Begin();
        void End();

        uint32_t GetActiveWidgetID();

    private:
        std::shared_ptr<Window> m_WindowContext;
        bool m_BlockEvents = false;
        float m_Time = 0.0f;
    };
}

#endif