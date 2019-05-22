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

project "Caustix"
    location "Caustix"
    kind "SharedLib"
    language "C++"
    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

    files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
    }
    
    pchheader "cxpch.h"
	pchsource "Caustix/src/cxpch.cpp"
    
    includedirs
	{
        "%{prj.name}/src",
		"%{prj.name}/vendor/spdlog/include"
    }
    
    filter "system:windows"
        cppdialect "C++17"
        staticruntime "On"
        systemversion "latest"

        defines 
        {
            "CX_PLATFORM_WINDOWS",
            "CX_BUILD_DLL"
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
        "Caustix/src"
    }

    links 
    {
        "Caustix"
    }

    filter "system:windows"
        cppdialect "C++17"
        staticruntime "On"
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
