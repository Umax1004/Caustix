workspace "Caustix"
    architecture "x64"
    startproject "Sandbox"
    configurations
    {
        "Debug",
        "Release",
        "Dist"
    }

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

IncludeDir = {}
IncludeDir["glfw"] = "Caustix/vendor/glfw/include"
IncludeDir["imgui"] = "Caustix/vendor/imgui"

include "Caustix/vendor/glfw"
include "Caustix/vendor/imgui"

project "Caustix"
    location "Caustix"
    kind "SharedLib"
    language "C++"
    staticruntime "Off"
    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

    files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
    }
    
    pchheader "cxpch.h"
    pchsource "Caustix/src/cxpch.cpp"
    
    vul_path = os.getenv("VULKAN_SDK")
    
    includedirs
	{
        "%{prj.name}/src",
        "%{prj.name}/vendor/spdlog/include",
        vul_path .. "/Include",
        "%{IncludeDir.glfw}",
        "%{IncludeDir.imgui}"
    }

    links
    {
        "GLFW",
        "ImGui",
        vul_path .. "/Lib/vulkan-1.lib"
    }
    
    filter "system:windows"
        cppdialect "C++17"
        systemversion "latest"

        defines 
        {
            "CX_PLATFORM_WINDOWS",
            "CX_BUILD_DLL",
            "CX_ENABLE_ASSERTS",
            "VK_USE_PLATFORM_WIN32_KHR"
        }

        postbuildcommands
        {
            ("{COPY} %{cfg.buildtarget.relpath} ../bin/" .. outputdir .. "/Sandbox")
        }
    
    filter "configurations:Debug"
        defines "CX_DEBUG"
		symbols "on"

	filter "configurations:Release"
        defines "CX_RELEASE"
		optimize "on"

	filter "configurations:Dist"
        defines "CX_DIST"
		optimize "on"


project "Sandbox"
	location "Sandbox"
	kind "ConsoleApp"
    language "C++"
    staticruntime "Off"
	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
    }
    
    includedirs
	{
        "Caustix/vendor/spdlog/include",
        "Caustix/src",
        vul_path .. "/Include",
    }

    links 
    {
        "Caustix"
    }

    filter "system:windows"
        cppdialect "C++17"
        systemversion "latest"

        defines 
        {
            "CX_PLATFORM_WINDOWS"
        }
    
    filter "configurations:Debug"
        defines "CX_DEBUG"
		symbols "on"

	filter "configurations:Release"
        defines "CX_RELEASE"
		optimize "on"

	filter "configurations:Dist"
        defines "CX_DIST"
		optimize "on"
