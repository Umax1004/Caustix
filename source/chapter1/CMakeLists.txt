project(Chapter1)

add_executable(Chapter1
    graphics/command_buffer.cpp
    graphics/command_buffer.hpp
    graphics/gpu_device.cpp
    graphics/gpu_device.hpp
    graphics/gpu_enum.hpp
    graphics/gpu_profiler.cpp
    graphics/gpu_profiler.hpp
    graphics/gpu_resources.cpp
    graphics/gpu_resources.hpp
    graphics/renderer.cpp
    graphics/renderer.hpp

    graphics/caustix_imgui.cpp
    graphics/caustix_imgui.hpp

    main.cpp
)

set_property(TARGET Chapter1 PROPERTY CXX_STANDARD 23)

if (WIN32)
    target_compile_definitions(Chapter1 PRIVATE
        _CRT_SECURE_NO_WARNINGS
        WIN32_LEAN_AND_MEAN
        NOMINMAX)
endif()

target_compile_definitions(Chapter1 PRIVATE
    TRACY_ENABLE
    TRACY_ON_DEMAND
    TRACY_NO_SYSTEM_TRACING
)

target_include_directories(Chapter1 PRIVATE
    .
    ..
    ../caustix
    ${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(Chapter1 PRIVATE
    CaustixFoundation
    CaustixExternal
    CaustixApp
    ${Vulkan_LIBRARIES}
)

add_custom_command(TARGET Chapter1 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different  $<TARGET_FILE:CaustixExternal> $<TARGET_FILE_DIR:Chapter1>
    COMMENT "Copying required external dependencies"
)
