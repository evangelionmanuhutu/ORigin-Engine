project "PhysXVehicle2"
kind "StaticLib"
language "C++"
cppdialect "C++17"
staticruntime "off"
location (vendorProjectFiles)
targetdir (vendorOutputdir)
objdir (vendorIntOutputdir)

	includedirs {
		"%{physxDir}/physx/include",
		"%{physxDir}/physx/source/common/include",
		"%{physxDir}/physx/source/common/src",
		"%{physxDir}/physx/source/physxvehicle2/src",
		"%{physxDir}/physx/source/physxvehicle2/src/physxmetadata/include",
		"%{physxDir}/physx/source/physxmetadata/extensions/include",
		"%{physxDir}/physx/source/physxmetadata/core/include",
		"%{physxDir}/physx/source/physxextensions/src/serialization/Xml",
		"%{physxDir}/physx/source/pvdsdk/src",
		"%{physxDir}/physx/source/pvd/include",
		"%{physxDir}/physx/source/foundation/include",
	}
	files { 
		"%{physxDir}/physx/source/physxvehicle2/**.cpp",
	} 
	
	defines {
		"PX_PHYSX_STATIC_LIB",
		"PX_SUPPORT_PVD=1"
	}

	links {
		"PhysXFoundation",
		"PhysXPvdSDK"
	}

	filter "system:linux"
		pic "On"
		systemversion "latest"
		
	filter "system:windows"
		systemversion "latest"
		defines {
			"WIN32",
			"WIN64",
			"_CRT_SECURE_NO_DEPRECATE",
			"_CRT_NONSTDC_NO_DEPRECATE",
			"_WINSOCK_DEPRECATED_NO_WARNINGS",
			"PX_PHYSX_GPU_SHARED_LIB_NAME=PhysXGpu_64.dll",
		}


	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"
		optimize "off"
		defines {
			"PX_DEBUG=1",
			"PX_CHECKED=1",
			"PX_NVTX=0",
			"_DEBUG"
		}

	filter "configurations:Release"
		runtime "Release"
		optimize "on"
		symbols "on"
		defines {
			"_HAS_ITERATOR_DEBUGGING=0",
			"_ITERATOR_DEBUG_LEVEL=0",
			"PX_CHECKED=1",
			"PX_NVTX=0",
			"NDEBUG"
		}

	filter "configurations:Dist"
		runtime "Release"
		optimize "on"
		symbols "off"
		defines {
			"_HAS_ITERATOR_DEBUGGING=0",
			"_ITERATOR_DEBUG_LEVEL=0",
			"PX_CHECKED=1",
			"NDEBUG"
		}