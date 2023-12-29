project(test)

add_executable(test
        main.cpp
)

set_property(TARGET test PROPERTY CXX_STANDARD 23)

if (WIN32)
    target_compile_definitions(test PRIVATE
            _CRT_SECURE_NO_WARNINGS
            WIN32_LEAN_AND_MEAN
            NOMINMAX)
endif()

target_compile_definitions(test PRIVATE
        TRACY_ENABLE
        TRACY_ON_DEMAND
        TRACY_NO_SYSTEM_TRACING
)

target_include_directories(test PRIVATE
        .
        ..
        ../caustix
        ${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(test PRIVATE
        CaustixFoundation
        CaustixExternal
        CaustixApp
        ${Vulkan_LIBRARIES}
)

add_custom_command(TARGET test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different  $<TARGET_FILE:CaustixExternal> $<TARGET_FILE_DIR:test>
        COMMENT "Copying required external dependencies"
)
