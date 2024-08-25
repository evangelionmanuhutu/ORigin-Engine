-- Copyright (c) 2022-present Evangelion Manuhutu | ORigin Engine

project "ORigin"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    staticruntime "off"
    location "%{wks.location}/ORigin"

    if _ACTION ~= "cmake" then
        pchheader "pch.h"
        pchsource "%{prj.location}/sources/pch.cpp"
    end

    targetdir ("%{wks.location}/Binaries/%{cfg.buildcfg}/ORigin")
    objdir ("%{wks.location}/Binaries/Intermediates/%{cfg.buildcfg}/ORigin")

    files {
        "%{prj.location}/sources/pch.cpp",
        "%{prj.location}/sources/pch.h",
        "%{prj.location}/sources/Origin.h",
        "%{prj.location}/sources/Origin/**.cpp",
        "%{prj.location}/sources/Origin/**.h",
        "%{prj.location}/sources/Platform/OpenGL/**.cpp",
        "%{prj.location}/sources/Platform/OpenGL/**.h",
        "%{prj.location}/sources/Platform/Vulkan/**.cpp",
        "%{prj.location}/sources/Platform/Vulkan/**.h",
    }

    includedirs {
        "%{prj.location}/sources",
        "%{IncludeDir.BOX2D}",
        "%{IncludeDir.SPDLOG}",
        "%{IncludeDir.STBI}",
        "%{IncludeDir.GLFW}",
        "%{IncludeDir.IMGUI}",
        "%{IncludeDir.IMGUIZMO}",
        "%{IncludeDir.GLAD}",
        "%{IncludeDir.GLM}",
        "%{IncludeDir.MINIAUDIO}",
        "%{IncludeDir.ENTT}",
        "%{IncludeDir.FILEWATCHER}",
        "%{IncludeDir.YAMLCPP}",
        "%{IncludeDir.ASSIMP}",
        "%{IncludeDir.JOLT}",
        "%{IncludeDir.MSDFGEN}",
        "%{IncludeDir.MSDFATLASGEN}",
        "%{IncludeDir.MONO}"
    }

    links {
        "ASSIMP",
        "BOX2D",
        "GLFW",
        "GLAD",
        "IMGUI",
        "MSDFATLASGEN",
        "MSDFGEN",
        "FreeType",
        "YAMLCPP",
        "JOLT",
    }
    
    defines { "GLFW_INCLUDE_NONE", "_CRT_SECURE_NO_WARNINGS" }

    -- ////////////////////////////////
    -- Windows
    filter "system:windows"
        systemversion "latest"
        links {
            "opengl32.lib",
            "%{Library.Vulkan1Lib}",
            "%{Library.MONO}",
            "%{Library.WinSock}",
            "%{Library.WinMM}",
            "%{Library.WinVersion}",
            "%{Library.BCrypt}"
        }
        files {
            "%{prj.location}/sources/Platform/DX11/**.cpp",
            "%{prj.location}/sources/Platform/DX11/**.h",
            "%{prj.location}/sources/Platform/Win32/**.cpp",
            "%{prj.location}/sources/Platform/Win32/**.h",
        }
        includedirs { "%{IncludeDir.VulkanSDK}" }

        defines {
            "OGN_PLATFORM_WINDOWS",
            "_SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING",
            "_SILENCE_ALL_MS_EXT_DEPRECATION_WARNINGS",
        }

        filter "configurations:Debug"
            runtime "Debug"
            symbols "on"
            defines { "OGN_DEBUG", "_DEBUG" }
            links {
                "%{Library.ShaderC_Debug}",
                "%{Library.SPIRV_Cross_Debug}",
                "%{Library.SPIRV_Cross_GLSL_Debug}",
                "%{Library.SPIRV_Tools_Debug}",
            }

        filter "configurations:Release"
            runtime "Release"
            optimize "on"
            defines { "OGN_RELEASE", "NDEBUG" }
            links {
                "%{Library.ShaderC_Release}",
                "%{Library.SPIRV_Cross_Release}",
                "%{Library.SPIRV_Cross_GLSL_Release}",
            }

        filter "configurations:Dist"
            runtime "Release"
            symbols "off"
            optimize "on"
            defines { "OGN_RELEASE", "NDEBUG" }
            links {
                "%{Library.ShaderC_Release}",
                "%{Library.SPIRV_Cross_Release}",
                "%{Library.SPIRV_Cross_GLSL_Release}",
            }
    -- !Windows
    -- ////////////////////////////////


    -- ////////////////////////////////
    -- Linux
    filter "system:linux"
        defines { "OGN_PLATFORM_LINUX", "VK_VERSION_1_0", "GLFW_INCLUDE_VULKAN" }
        pic "On"
        files { "%{prj.location}/sources/Platform/Linux/**.cpp" }
        libdirs { "/usr/lib" }
        includedirs { 
            "/usr/include/",
        }
        links {
            "vulkan", "shaderc_shared", "spirv-cross-core", "spirv-cross-glsl",
            "monosgen-2.0", "pthread", "dl", "m", "rt", "glib-2.0"
        }

        filter "configurations:Debug"
            runtime "Debug"
            symbols "on"
            defines { "OGN_DEBUG", "_DEBUG" }

        filter "configurations:Release"
            runtime "Release"
            optimize "on"
            defines { "OGN_RELEASE", "NDEBUG" }

        filter "configurations:Dist"
            runtime "Release"
            optimize "on"
            defines { "OGN_DISTRIBUTION", "NDEBUG" }
  
