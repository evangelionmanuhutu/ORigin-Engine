-- Copyright (c) 2022-present Evangelion Manuhutu | ORigin Engine

project "YAMLCPP"
	location (ThirdPartyProjectFiles)
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "off"

	targetdir (ThirdPartyOutputdir)
	objdir (ThirdPartyIntOutputdir)

	files {
        "%{THIRD_PARTY_DIR}/yaml-cpp/src/**.cpp",
        "%{THIRD_PARTY_DIR}/yaml-cpp/src/**.h",
        "%{THIRD_PARTY_DIR}/yaml-cpp/include/yaml-cpp/**.h",
    }

	defines {
        "YAMLCPP_USE_STATIC_LIBS"
    }

    includedirs {
        "%{THIRD_PARTY_DIR}/yaml-cpp/include/"
    }

	filter "system:linux"
		pic "On"

	filter "system:windows"
		systemversion "latest"

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		runtime "Release"
		optimize "on"