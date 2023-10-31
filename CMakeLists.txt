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
set(CMAKE_CXX_STANDARD_REQUIRED True)

project(Caustix CXX)

add_executable(Caustix main.cpp)