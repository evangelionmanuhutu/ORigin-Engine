project "msdf-atlas-gen"
	location (vendorProjectFiles)
	kind "StaticLib"
	language "C++"
	cppdialect "c++17"
	staticruntime "off"

	targetdir (vendorOutputdir)
	objdir (vendorIntOutputdir)

	files {
		"%{wks.location}/ORigin/vendor/msdf-atlas-gen/msdf-atlas-gen/**.cpp",
	}

	includedirs {
		"%{wks.location}/ORigin/vendor/msdf-atlas-gen/msdf-atlas-gen",
		"%{wks.location}/ORigin/vendor/msdf-atlas-gen/msdfgen",
		"%{wks.location}/ORigin/vendor/msdf-atlas-gen/msdfgen/include"
	}

	defines {
		"_CRT_SECURE_NO_WARNINGS"
	}

	links "msdfgen"

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		runtime "Release"
		optimize "on"