project "LowLevelAABB"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "off"
	location (vendorProjectFiles)
	targetdir (vendorOutputdir)
	objdir (vendorIntOutputdir)

	includedirs {
		"%{physxDir}/include",
		"%{physxDir}/physx/include",
		"%{physxDir}/physx/source/Common/src/windows",
		"%{physxDir}/physx/source/lowlevelaabb/windows/include",
		"%{physxDir}/physx/source/gpubroadphase/include",
		"%{physxDir}/physx/source/gpubroadphase/src",
		"%{physxDir}/physx/source/common/include",
		"%{physxDir}/physx/source/common/src",
		"%{physxDir}/physx/source/geomutils/include",
		"%{physxDir}/physx/source/geomutils/src",
		"%{physxDir}/physx/source/lowlevel/api/include",
		"%{physxDir}/physx/source/lowlevel/common/include/utils",
		"%{physxDir}/physx/source/lowlevel/common/include/pipeline",
		"%{physxDir}/physx/source/lowlevelaabb/include",
		"%{physxDir}/physx/source/lowlevelaabb/src",
	}
	files { 
		"%{physxDir}/physx/source/lowlevelaabb/**.cpp",
	} 
		
	defines {
		"PX_PHYSX_GPU_SHARED_LIB_NAME=PhysXGpu_64.dll",
		"PX_PHYSX_STATIC_LIB",
		"PX_SUPPORT_PVD=1"
	}

	filter "system:linux"
		pic "On"
			
	filter "system:windows"
		systemversion "latest"
		defines {
			"WIN32",
			"WIN64",
			"_CRT_SECURE_NO_DEPRECATE",
			"_CRT_NONSTDC_NO_DEPRECATE",
			"_WINSOCK_DEPRECATED_NO_WARNINGS",
			"PX_PHYSX_GPU_SHARED_LIB_NAME=PhysXGpu_64.dll",
			"PX_PHYSX_STATIC_LIB",
			"PX_SUPPORT_PVD=1"
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
		}

	filter "configurations:Dist"
		runtime "Release"
		optimize "on"
		symbols "off"
		defines {
			"_HAS_ITERATOR_DEBUGGING=0",
			"_ITERATOR_DEBUG_LEVEL=0",
			"PX_CHECKED=1",
		}
		