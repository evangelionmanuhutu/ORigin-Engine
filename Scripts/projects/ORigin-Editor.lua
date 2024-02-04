-- Copyright (c) Evangelion Manuhutu | ORigin Engine

-- ORigin Editor Project
project "Editor"
location "%{wks.location}/ORigin-Editor"
language "C++"
cppdialect "C++17"
staticruntime "off"

links "ORigin"

targetdir ("%{wks.location}/Binaries/%{cfg.buildcfg}/ORigin")
objdir ("%{wks.location}/Binaries/Intermediates/%{cfg.buildcfg}/ORigin")

files {
    "%{prj.location}/sources/**.h",
    "%{prj.location}/sources/**.cpp",
    "%{prj.location}/**.h",
    "%{prj.location}/**.rc",
    "%{prj.location}/**.aps",
    "%{prj.location}/Resources/Shaders/**.glsl",
    "%{prj.location}/Resources/UITextures/**.png",
    "%{prj.location}/Resources/UITextures/**.jpg",
    "%{prj.location}/**.ico",
    "%{prj.location}/**.png",
}

includedirs {
    "%{wks.location}/ORigin/sources",
    "%{IncludeDir.SPDLOG}",
    "%{IncludeDir.ASSIMP}",
    "%{IncludeDir.GLFW}",
    "%{IncludeDir.GLAD}",
    "%{IncludeDir.BOX2D}",
    "%{IncludeDir.ENTT}",
    "%{IncludeDir.Miniaudio}",
    "%{IncludeDir.IMGUI}",
    "%{IncludeDir.IMGUIZMO}",
    "%{IncludeDir.GLM}",
    "%{IncludeDir.YAML_CPP}",
    "%{IncludeDir.PhysX}",
    "%{IncludeDir.JoltPhysics}",
    "%{IncludeDir.msdfgen}",
    "%{IncludeDir.msdf_atlas_gen}",
}

defines {
    "NV_USE_STATIC_WINCRT",
    "GLFW_INCLUDE_NONE",
    "_CRT_SECURE_NO_WARNINGS",
    "_SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING",
    "_SILENCE_ALL_MS_EXT_DEPRECATION_WARNINGS"
}

linkoptions { "/ignore:4099,4006" }

filter "system:windows"
    systemversion "latest"
    

filter "configurations:Debug"
    defines {
        "PHYSX_CXX_FLAGS_DEBUG",
        "OGN_DEBUG",
        "_DEBUG"
    }
    kind "ConsoleApp"
    runtime "Debug"
    symbols "On"
    links  {
        "%{Library.ShaderC_Debug}",
        "%{Library.SPIRV_Cross_Debug}",
        "%{Library.SPIRV_Cross_GLSL_Debug}",
        "%{Library.SPIRV_Tools_Debug}",
    }

filter "configurations:Release"
   defines {
        "PX_PHYSX_STATIC_LIB",
        "OGN_RELEASE",
        "NDEBUG"
    }
    kind "WindowedApp"
    runtime "Release"
    optimize "On"

filter "configurations:Dist"
    defines {
        "PX_PHYSX_STATIC_LIB",
        "OGN_RELEASE",
        "NDEBUG"
    }
    kind "WindowedApp"
    runtime "Release"
    optimize "On"
    postbuildcommands {
      "{COPY} %{wks.location}ORigin/vendor/Assimp/lib/x64/assimp-vc143-mt.dll %{wks.location}Binaries/%{cfg.buildcfg}/ORigin",
      "{COPY} %{prj.location}imgui.ini %{wks.location}Binaries/%{cfg.buildcfg}/ORigin",
      "{COPYDIR} %{prj.location}lib %{wks.location}Binaries/%{cfg.buildcfg}/ORigin/lib",
      "{COPYDIR} %{prj.location}resources %{wks.location}Binaries/%{cfg.buildcfg}/ORigin/resources",
    }
