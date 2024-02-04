-- Copyright (c) Evangelion Manuhutu | ORigin Engine

VULKAN_SDK = os.getenv("VULKAN_SDK" or "VK_SDK_PATH")
ASSIMP_SDK = os.getenv("ASSIMP_SDK")

IncludeDir = {}
IncludeDir["GLFW"] = "%{wks.location}/ORigin/vendor/glfw/include"
IncludeDir["GLAD"] = "%{wks.location}/ORigin/vendor/glad/include"
IncludeDir["ASSIMP"] = "%{wks.location}/ORigin/vendor/assimp/include"
IncludeDir["IMGUI"] = "%{wks.location}/ORigin/vendor/imgui"
IncludeDir["IMGUIZMO"] = "%{wks.location}/ORigin/vendor/ImGuizmo"
IncludeDir["BOX2D"] = "%{wks.location}/ORigin/vendor/Box2D/include"
IncludeDir["STBI"] = "%{wks.location}/ORigin/vendor/stb_image"
IncludeDir["SPDLOG"] = "%{wks.location}/ORigin/vendor/spdlog/include"
IncludeDir["GLM"] = "%{wks.location}/ORigin/vendor/glm"
IncludeDir["ENTT"] = "%{wks.location}/ORigin/vendor/entt/"
IncludeDir["MONO"] = "%{wks.location}/ORigin/vendor/mono/include"
IncludeDir["Miniaudio"] = "%{wks.location}/ORigin/vendor/Miniaudio"
IncludeDir["FILEWATCHER"] = "%{wks.location}/ORigin/vendor/Filewatcher/include"
IncludeDir["YAML_CPP"] = "%{wks.location}/ORigin/vendor/yaml-cpp/include"
IncludeDir["VulkanSDK"] = "%{VULKAN_SDK}/Include"
IncludeDir["msdf_atlas_gen"] = "%{wks.location}/ORigin/vendor/msdf-atlas-gen/msdf-atlas-gen"
IncludeDir["msdfgen"] = "%{wks.location}/ORigin/vendor/msdf-atlas-gen/msdfgen"
IncludeDir["PhysX"] = "%{wks.location}/ORigin/vendor/PhysX/physx/include"
IncludeDir["JoltPhysics"] = "%{wks.location}/ORigin/vendor/JoltPhysics"

LibraryDir = {}
LibraryDir["VulkanSDK"] = "%{VULKAN_SDK}/Lib"
LibraryDir["FMOD"] = "%{wks.location}/ORigin/vendor/FMOD/lib"
LibraryDir["MONO"] = "%{wks.location}/ORigin/vendor/mono/lib/%{cfg.buildcfg}"
LibraryDir["AssimpSDK"] = "%{wks.location}/ORigin/vendor/Assimp/lib/x64"

Library = {}
Library["FMOD"] = "%{LibraryDir.FMOD}/fmod_vc.lib"
Library["MONO"] = "%{LibraryDir.MONO}/libmono-static-sgen.lib"
Library["Assimp"] = "%{LibraryDir.AssimpSDK}/assimp-vc143-mt.lib"

Library["Vulkan"] = "%{LibraryDir.VulkanSDK}/vulkan-1.lib"

Library["ShaderC_Debug"] = "%{LibraryDir.VulkanSDK}/shaderc_sharedd.lib"
Library["SPIRV_Cross_Debug"] = "%{LibraryDir.VulkanSDK}/spirv-cross-cored.lib"
Library["SPIRV_Cross_GLSL_Debug"] = "%{LibraryDir.VulkanSDK}/spirv-cross-glsld.lib"
Library["SPIRV_Tools_Debug"] = "%{LibraryDir.VulkanSDK}/SPIRV-Toolsd.lib"

Library["ShaderC_Release"] = "%{LibraryDir.VulkanSDK}/shaderc_shared.lib"
Library["SPIRV_Cross_Release"] = "%{LibraryDir.VulkanSDK}/spirv-cross-core.lib"
Library["SPIRV_Cross_GLSL_Release"] = "%{LibraryDir.VulkanSDK}/spirv-cross-glsl.lib"

-- Windows-Only
Library["WinSock"] = "Ws2_32.lib"
Library["WinMM"] = "Winmm.lib"
Library["WinVersion"] = "Version.lib"
Library["BCrypt"] = "Bcrypt.lib"

-- Dependencies Lua File
group "Dependecies"
include "glfw.lua"
include "glad.lua"
include "imgui.lua"
include "yaml-cpp.lua"
include "box2d.lua"
include "msdf-atlas-gen.lua"
include "JoltPhysics.lua"
group ""

group "PhysX"
include "FastXml.lua"
include "LowLevel.lua"
include "LowLevelAABB.lua"
include "LowLevelDynamics.lua"
include "PhysX.lua"
include "PhysXCharacterKinematic.lua"
include "PhysXCommon.lua"
include "PhysXCooking.lua"
include "PhysXExtensions.lua"
include "PhysXFoundation.lua"
include "PhysXPvdSDK.lua"
include "PhysXTask.lua"
include "PhysXVehicle.lua"
include "PhysXVehicle2.lua"
include "SceneQuery.lua"
include "SimulationController.lua"
group ""