cmake_minimum_required(VERSION 3.24)

if(WIN32)
	include(FetchContent)
	FetchContent_Declare(vcpkg
	                     GIT_REPOSITORY https://github.com/microsoft/vcpkg.git
	                     )
	FetchContent_MakeAvailable(vcpkg)
endif()

set(CMAKE_TOOLCHAIN_FILE ${vcpkg_SOURCE_DIR}/scripts/buildsystems/vcpkg.cmake CACHE STRING "CMake toolchain file")

set(CMAKE_CXX_STANDARD 23)

if(WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

project(Caustix)

find_package(Vulkan REQUIRED)
find_package(cglm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(Tracy CONFIG REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)

add_library(CaustixFoundation STATIC
    source/caustix/foundation/array.hpp
    source/caustix/foundation/assert.cpp
    source/caustix/foundation/assert.hpp
    source/caustix/foundation/bit.cpp
    source/caustix/foundation/bit.hpp
    source/caustix/foundation/blob_serialization.cpp
    source/caustix/foundation/blob_serialization.hpp
    source/caustix/foundation/blob.hpp
    source/caustix/foundation/camera.cpp
    source/caustix/foundation/camera.hpp
    source/caustix/foundation/color.cpp
    source/caustix/foundation/color.hpp
    source/caustix/foundation/data_structures.cpp
    source/caustix/foundation/data_structures.hpp
    source/caustix/foundation/file.cpp
    source/caustix/foundation/file.hpp
    source/caustix/foundation/gltf.cpp
    source/caustix/foundation/gltf.hpp
    source/caustix/foundation/hash_map.hpp
    source/caustix/foundation/log.cpp
    source/caustix/foundation/log.hpp
    source/caustix/foundation/memory_utils.hpp
    source/caustix/foundation/memory.cpp
    source/caustix/foundation/memory.hpp
    source/caustix/foundation/numerics.cpp
    source/caustix/foundation/numerics.hpp
    source/caustix/foundation/platform.hpp
    source/caustix/foundation/process.cpp
    source/caustix/foundation/process.hpp
    source/caustix/foundation/relative_data_structures.hpp
    source/caustix/foundation/resource_manager.cpp
    source/caustix/foundation/resource_manager.hpp
    source/caustix/foundation/serialization.cpp
    source/caustix/foundation/serialization.hpp
    source/caustix/foundation/service_manager.cpp
    source/caustix/foundation/service_manager.hpp
    source/caustix/foundation/service.cpp
    source/caustix/foundation/service.hpp
    source/caustix/foundation/string.cpp
    source/caustix/foundation/string.hpp
    source/caustix/foundation/time.cpp
    source/caustix/foundation/time.hpp
)

set_property(TARGET CaustixFoundation PROPERTY CXX_STANDARD 23)

target_compile_definitions(CaustixFoundation PRIVATE
    _CRT_SECURE_NO_WARNINGS

    TRACY_ENABLE
    TRACY_ON_DEMAND
    TRACY_NO_SYSTEM_TRACING
)

target_include_directories(CaustixFoundation PRIVATE
    source
    source/caustix
)

add_library(CaustixApp STATIC
    source/caustix/application/application.cpp
    source/caustix/application/application.hpp
    source/caustix/application/game_camera.cpp
    source/caustix/application/game_camera.hpp
    source/caustix/application/game_application.cpp
    source/caustix/application/game_application.hpp
    source/caustix/application/input.cpp
    source/caustix/application/input.hpp
    source/caustix/application/keys.cpp
    source/caustix/application/keys.hpp
    source/caustix/application/window.cpp
    source/caustix/application/window.hpp
)

set_property(TARGET CaustixApp PROPERTY CXX_STANDARD 23)

target_compile_definitions(CaustixApp PRIVATE
    _CRT_SECURE_NO_WARNINGS

    TRACY_ENABLE
    TRACY_ON_DEMAND
    TRACY_NO_SYSTEM_TRACING
)

target_include_directories(CaustixApp PRIVATE
	source
	source/caustix
)

add_library(CaustixExternal SHARED
	source/external/tlsf/tlsf.c
)

find_path(WYHASH_INCLUDE_DIRS "wyhash.h")

target_include_directories(CaustixExternal PRIVATE
	${Stb_INCLUDE_DIR}
	${WYHASH_INCLUDE_DIRS}
)

target_link_libraries(CaustixExternal PUBLIC
	cglm::cglm
    imgui::imgui
	nlohmann_json::nlohmann_json
	Tracy::TracyClient
	GPUOpen::VulkanMemoryAllocator
)

set_property(TARGET CaustixExternal PROPERTY CXX_STANDARD 23)

target_link_libraries(CaustixFoundation PUBLIC CaustixExternal)
target_link_libraries(CaustixApp PUBLIC CaustixExternal)

add_subdirectory(source/chapter1)