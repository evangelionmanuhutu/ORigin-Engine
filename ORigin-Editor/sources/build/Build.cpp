// Copyright (c) 2022 Evangelion Manuhutu | ORigin Engine

#include <Origin.h>
#include "..\Editor.h"

namespace Origin
{
  class EditorBuild : public Application
  {
  public:
    EditorBuild(const ApplicationSpecification& spec)
      : Application(spec)
    {
      Application::Get().GetWindow().SetIcon("assets/textures/icon_origin.png");
	    PushLayer(new Editor());
    }
    ~EditorBuild() { }
  };

  Application* CreateApplication(ApplicationCommandLineArgs args)
  {
    ApplicationSpecification spec;
    spec.Name = "ORigin-Editor";
    spec.CommandLineArgs = args;

    OGN_CORE_INFO(spec.Name);
    return new EditorBuild(spec);
  };
}