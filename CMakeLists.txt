cmake_minimum_required(VERSION 3.24)

# Copyright (C) 2026 DeadOnKeyboard
# SPDX-License-Identifier: GPL-3.0-or-later

project(SilentDialogueUniversal VERSION 1.0.0 LANGUAGES C CXX)

include(FetchContent)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(COMMONLIBSSE_ROOT "" CACHE PATH "Optional path to a CommonLibSSE-NG 8.0.0 checkout")
set(MINHOOK_ROOT "" CACHE PATH "Optional path to a MinHook 1.3.4 checkout")

set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(ENABLE_SKYRIM_SE ON CACHE BOOL "" FORCE)
set(ENABLE_SKYRIM_AE ON CACHE BOOL "" FORCE)
set(ENABLE_SKYRIM_VR OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(FETCHCONTENT_SOURCE_DIR_HDE64 "${MINHOOK_ROOT}" CACHE PATH "" FORCE)

if(COMMONLIBSSE_ROOT)
    if(NOT EXISTS "${COMMONLIBSSE_ROOT}/CMakeLists.txt")
        message(FATAL_ERROR "CommonLibSSE-NG was not found at '${COMMONLIBSSE_ROOT}'")
    endif()
    add_subdirectory("${COMMONLIBSSE_ROOT}" CommonLibSSE EXCLUDE_FROM_ALL)
else()
    FetchContent_Declare(
        commonlibsse_ng
        GIT_REPOSITORY https://github.com/alandtse/CommonLibVR.git
        GIT_TAG 1504349dddfc622d4d25704bba19e2ade669dc5a
        GIT_SHALLOW FALSE
        GIT_SUBMODULES_RECURSE TRUE
    )
    FetchContent_MakeAvailable(commonlibsse_ng)
    set(COMMONLIBSSE_ROOT "${commonlibsse_ng_SOURCE_DIR}")
endif()

if(MINHOOK_ROOT)
    if(NOT EXISTS "${MINHOOK_ROOT}/CMakeLists.txt")
        message(FATAL_ERROR "MinHook was not found at '${MINHOOK_ROOT}'")
    endif()
    add_subdirectory("${MINHOOK_ROOT}" minhook EXCLUDE_FROM_ALL)
else()
    FetchContent_Declare(
        minhook_source
        GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
        GIT_TAG c3fcafdc10146beb5919319d0683e44e3c30d537
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(minhook_source)
endif()

include("${COMMONLIBSSE_ROOT}/cmake/CommonLibSSE.cmake")

add_commonlibsse_plugin(
    SilentDialogueUniversal
    NAME "Silent Dialogue Universal"
    AUTHOR "DeadOnKeyboard"
    VERSION ${PROJECT_VERSION}
    USE_ADDRESS_LIBRARY
    MINIMUM_SKSE_VERSION 2.0.20
    SOURCES
        src/Config.cpp
        src/Config.h
        src/DialogueHook.cpp
        src/DialogueHook.h
        src/main.cpp
)

target_link_libraries(SilentDialogueUniversal PRIVATE minhook)
target_precompile_headers(SilentDialogueUniversal PRIVATE src/PCH.h)
target_compile_definitions(SilentDialogueUniversal PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)

if(MSVC)
    target_compile_options(SilentDialogueUniversal PRIVATE /W4 /permissive- /utf-8 /Zc:preprocessor /Zi)
    target_link_options(SilentDialogueUniversal PRIVATE /DEBUG:FULL)
endif()

set_target_properties(SilentDialogueUniversal PROPERTIES
    OUTPUT_NAME "SilentDialogueUniversal"
    PDB_NAME "SilentDialogueUniversal"
    PDB_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
