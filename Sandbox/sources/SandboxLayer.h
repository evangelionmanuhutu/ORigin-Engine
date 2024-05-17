#pragma once
#include <Origin.h>

namespace origin
{
  class SandboxLayer : public Layer
  {
  public:
    SandboxLayer() : Layer("Sandbox"){}
    ~SandboxLayer();

    void OnAttach() override;
    void OnUpdate(Timestep ts) override;
    void OnGuiRender() override;
    void OnEvent(Event& event) override;

    bool OnWindowResize(WindowResizeEvent& e);
    bool OnKeyPressedEvent(KeyPressedEvent& e);

    EditorCamera camera;
  };
}
