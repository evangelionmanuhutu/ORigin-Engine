// Copyright (c) Evangelion Manuhutu | ORigin Engine
#include <Origin.h>
#include "SandboxLayer.h"

namespace origin
{
    class SandboxApp : public Application
    {
    public:
        SandboxApp(ApplicationSpecification &spec)
            : Application(spec)
        {
            PushLayer(new SandboxLayer());
        }
    };

    Application* CreateApplication(ApplicationCommandLineArgs args)
    {
        ApplicationSpecification spec;
        spec.Name = "Sandbox";
        spec.Width = 1280;
        spec.Height = 640;
        spec.CommandLineArgs = args;

        OGN_CORE_INFO(spec.Name);
        return new SandboxApp(spec);
    };
}