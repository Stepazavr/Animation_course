cmake_minimum_required(VERSION 3.10)

set(EXE_NAME animations)

message(PROJECT = ${EXE_NAME} )
message(${CMAKE_BINARY_DIR})
project(${EXE_NAME})



set(SRC_ROOT ${CMAKE_SOURCE_DIR})

set(CMAKE_CXX_STANDARD 20)
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "-m64 -Wall -Wno-pragma-pack -Wno-deprecated-declarations -Wno-deprecated-copy -g")
else()
    set(CMAKE_CXX_FLAGS "/w /Zi /EHsc")
endif()


if(BUILD_TYPE STREQUAL "dbg")
    set(CMAKE_BUILD_TYPE "Debug")
else()
    if(BUILD_TYPE STREQUAL "dev")
        set(CMAKE_BUILD_TYPE "RelWithDebInfo")
    elseif(BUILD_TYPE STREQUAL "rel")
        set(CMAKE_BUILD_TYPE "Release")
    endif()
endif()


if(WIN32)
    set(ADDITIONAL_LIBS winmm.lib Version.lib Setupapi.lib)
    set(ADDITIONAL_LIBS ${ADDITIONAL_LIBS} SDL2 imm32)

    add_library(SDL2 STATIC IMPORTED)
    set_target_properties(SDL2 PROPERTIES
        IMPORTED_LOCATION_RELEASE "${CMAKE_BINARY_DIR}/SDL2.lib"
        IMPORTED_LOCATION_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/SDL2.lib"
        IMPORTED_LOCATION_DEBUG "${CMAKE_BINARY_DIR}/SDL2.lib"
        LINKER_LANGUAGE CXX)

    set(ADDITIONAL_LIBS ${ADDITIONAL_LIBS} assimp)

    add_library(assimp STATIC IMPORTED)
    set_target_properties(assimp PROPERTIES
        IMPORTED_LOCATION_DEBUG "${CMAKE_BINARY_DIR}/assimp.lib"
        IMPORTED_LOCATION_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/assimp.lib"
        IMPORTED_LOCATION_RELEASE "${CMAKE_BINARY_DIR}/assimp.lib"
        LINKER_LANGUAGE CXX)
else()
    set(ADDITIONAL_LIBS -ldl)
    find_package(SDL2 REQUIRED)
    include_directories(${SDL2_INCLUDE_DIRS})
    find_package(assimp REQUIRED)
endif()



macro(add_folder folder)
    file(GLOB_RECURSE TMP_SOURCES RELATIVE ${SRC_ROOT} ${folder}/*.cpp)
    set(EXE_SOURCES ${EXE_SOURCES} ${TMP_SOURCES})
endmacro()

set(EXE_SOURCES )
set(TMP_SOURCES )

add_folder(application)
add_folder(engine)
add_folder(3rd_party/imgui)

set(EXE_SOURCES ${EXE_SOURCES} ${SRC_ROOT}/3rd_party/glad/glad.c)

include_directories(${SRC_ROOT})
include_directories(${SRC_ROOT}/engine)
include_directories(${SRC_ROOT}/3rd_party)
include_directories(${SRC_ROOT}/3rd_party/ozz/include)

# Add ozz as a subdirectory to build it
set(ozz_build_gltf OFF CACHE BOOL "Build gltf tools" FORCE)
set(ozz_build_tests OFF CACHE BOOL "Build tests" FORCE)
set(ozz_build_samples OFF CACHE BOOL "Build samples" FORCE)
set(ozz_build_howtos OFF CACHE BOOL "Build howtos" FORCE) 
add_subdirectory(${SRC_ROOT}/3rd_party/ozz ozz_build)

add_executable(${EXE_NAME} ${EXE_SOURCES})

target_link_libraries(${EXE_NAME} ${ADDITIONAL_LIBS} ozz_animation ozz_animation_offline ozz_geometry ozz_base)


