project(DemoApplication)

add_executable(DemoApplication
        main.cpp
)

target_sources(DemoApplication PUBLIC
        FILE_SET CXX_MODULES FILES
        DemoApplication.ixx
)

set_property(TARGET DemoApplication PROPERTY CXX_STANDARD 23)

if (WIN32)
    target_compile_definitions(DemoApplication PRIVATE
            _CRT_SECURE_NO_WARNINGS
            WIN32_LEAN_AND_MEAN
            NOMINMAX)
endif()

target_compile_definitions(DemoApplication PRIVATE
        TRACY_ENABLE
        TRACY_ON_DEMAND
        TRACY_NO_SYSTEM_TRACING
)

target_include_directories(DemoApplication PRIVATE
        .
        ..
        ../Caustix
        ${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(DemoApplication PRIVATE
#        CaustixFoundation
#        CaustixExternal
        CaustixApp
        ${Vulkan_LIBRARIES}
)

add_custom_command(TARGET DemoApplication POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different  $<TARGET_FILE:CaustixExternal> $<TARGET_FILE_DIR:DemoApplication>
        COMMENT "Copying required external dependencies"
)
