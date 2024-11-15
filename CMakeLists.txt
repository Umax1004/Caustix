cmake_minimum_required(VERSION 3.30)

if(WIN32)
	include(FetchContent)
	FetchContent_Declare(vcpkg
	                     GIT_REPOSITORY https://github.com/microsoft/vcpkg.git
	                     )
	FetchContent_MakeAvailable(vcpkg)
endif()

set(CMAKE_TOOLCHAIN_FILE ${vcpkg_SOURCE_DIR}/scripts/buildsystems/vcpkg.cmake CACHE STRING "CMake toolchain file")

set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "0e5b6991-d74f-4b3d-a41c-cf096e0b2508")

set(CMAKE_CXX_STANDARD 23)

set(CMAKE_CXX_MODULE_STD 1)

set(CMAKE_CXX_SCAN_FOR_MODULES ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

project(Caustix)

find_package(Vulkan REQUIRED)
find_package(cglm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(Tracy CONFIG REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)

add_library(CaustixFoundation)

target_sources(CaustixFoundation PUBLIC
		FILE_SET CXX_MODULES FILES
		Source/Caustix/Foundation/Memory/Allocators/Allocator.ixx
		Source/Caustix/Foundation/Memory/Allocators/LinearAllocator.ixx
		Source/Caustix/Foundation/Memory/Allocators/StackAllocator.ixx
		Source/Caustix/Foundation/Assert.ixx
		Source/Caustix/Foundation/Blob.ixx
		Source/Caustix/Foundation/Camera.ixx
		Source/Caustix/Foundation/Log.ixx
		Source/Caustix/Foundation/Platform.ixx
		Source/Caustix/Foundation/Color.ixx
		Source/Caustix/Foundation/DataStructures.ixx
		Source/Caustix/Foundation/glTF.ixx
		Source/Caustix/Foundation/File.ixx
		Source/Caustix/Foundation/Memory/MemoryDefines.ixx
		Source/Caustix/Foundation/Memory/Allocators/HeapAllocator.ixx
		Source/Caustix/Foundation/Services/ServiceManager.ixx
		Source/Caustix/Foundation/Services/Service.ixx
		Source/Caustix/Foundation/Services/MemoryService.ixx
		Source/Caustix/Foundation/Numerics.ixx
		Source/Caustix/Foundation/Process.ixx
		Source/Caustix/Foundation/ResourceManager.ixx
)

set_property(TARGET CaustixFoundation PROPERTY CXX_STANDARD 23)

target_compile_definitions(CaustixFoundation PRIVATE
    _CRT_SECURE_NO_WARNINGS

    TRACY_ENABLE
    TRACY_ON_DEMAND
    TRACY_NO_SYSTEM_TRACING
)

target_include_directories(CaustixFoundation PRIVATE
    Source
    Source/Caustix
)

add_library(CaustixApp)

target_sources(CaustixApp PUBLIC
		FILE_SET CXX_MODULES FILES
		Source/Caustix/Application/Application.ixx
		Source/Caustix/Application/GameApplication.ixx
		Source/Caustix/Application/GameCamera.ixx
		Source/Caustix/Application/Window.ixx
		Source/Caustix/Application/Input.ixx
		Source/Caustix/Application/Keys.ixx
		Source/Caustix/Application/Graphics/GPUDefines.ixx
		Source/Caustix/Application/Graphics/GPUResources.ixx
		Source/Caustix/Application/Graphics/GPUDevice.ixx
		Source/Caustix/Application/Graphics/CommandBuffer.ixx
		Source/Caustix/Application/Graphics/GPUProfiler.ixx
		Source/Caustix/Application/Graphics/Renderer.ixx
		Source/Caustix/Application/Graphics/ImGuiService.ixx
)

target_sources(CaustixApp PUBLIC 
		Source/Caustix/Application/Graphics/GPUDevice.cxx
		Source/Caustix/Application/Graphics/CommandBuffer.cxx)

set_property(TARGET CaustixApp PROPERTY CXX_STANDARD 23)

target_compile_definitions(CaustixApp PRIVATE
		_CRT_SECURE_NO_WARNINGS

		TRACY_ENABLE
		TRACY_ON_DEMAND
		TRACY_NO_SYSTEM_TRACING
)

target_include_directories(CaustixApp PRIVATE
		Source
		Source/Caustix
		${Vulkan_INCLUDE_DIRS}
)

add_library(CaustixExternal SHARED
	Source/External/tlsf/tlsf.c
)

find_path(WYHASH_INCLUDE_DIRS "wyhash.h")

target_include_directories(CaustixExternal PRIVATE
	${Stb_INCLUDE_DIR}
	${WYHASH_INCLUDE_DIRS}
	${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(CaustixExternal PUBLIC
	cglm::cglm
	imgui::imgui
#		nlohmann_json::nlohmann_json
	spdlog::spdlog
	Tracy::TracyClient
	GPUOpen::VulkanMemoryAllocator
		$<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
		$<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
	${Vulkan_LIBRARIES}
)

set_property(TARGET CaustixExternal PROPERTY CXX_STANDARD 23)

target_link_libraries(CaustixFoundation PUBLIC CaustixExternal)
#target_link_libraries(CaustixApp PUBLIC CaustixExternal)
target_link_libraries(CaustixApp PUBLIC CaustixFoundation)
add_subdirectory(Source/DemoApplication)