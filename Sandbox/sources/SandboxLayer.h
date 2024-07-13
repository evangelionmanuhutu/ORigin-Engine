#pragma once
#include <Origin.h>

namespace origin
{
  class SandboxLayer : public Layer
  {
  public:
    SandboxLayer() : Layer("Sandbox") { }
    ~SandboxLayer();

    void OnAttach() override;
    void OnUpdate(Timestep ts) override;
    void OnGuiRender() override;
    void OnEvent(Event& event) override;

    bool OnWindowResize(FramebufferResizeEvent& e);
    bool OnKeyPressedEvent(KeyPressedEvent& e);

    EditorCamera m_Camera;

    void DrawGrid();
  };
}
