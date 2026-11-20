cmake_minimum_required(VERSION 3.20)
project(vkgl VERSION 0.1.36 LANGUAGES C CXX)

include(CTest)
include(GNUInstallDirs)
include(FindPkgConfig)

option(VKGL_FETCH_SPIRV_CROSS "Fetch the pinned SPIRV-Cross source when no system installation is found" OFF)
option(VKGL_REPRODUCIBLE "Map build paths out of compiler inputs for reproducible builds" ON)
option(VKGL_BUILD_HARDWARE_TESTS "Build the opt-in LG110 Vulkan smoke test; never run it as a host unit test" OFF)
set(VKGL_SPIRV_CROSS_ROOT "" CACHE PATH "Optional SPIRV-Cross source/build/install prefix")
set(VKGL_ICD_LIBRARY_PATH "" CACHE STRING
    "Override the absolute ICD library path; empty derives it from the current install prefix")
if(NOT VKGL_ICD_LIBRARY_PATH)
    set(VKGL_ICD_LIBRARY_PATH "${CMAKE_INSTALL_FULL_LIBDIR}/vkgl/libvkgl.so")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Threads REQUIRED)
find_path(VULKAN_INCLUDE_DIR NAMES vulkan/vulkan.h REQUIRED)
pkg_check_modules(WAYLAND REQUIRED IMPORTED_TARGET wayland-client)
pkg_check_modules(EGL REQUIRED IMPORTED_TARGET egl)
pkg_check_modules(GLESV2 REQUIRED IMPORTED_TARGET glesv2)
pkg_check_modules(GBM REQUIRED IMPORTED_TARGET gbm)

# Prefer an existing system or caller-provided installation. Both common
# include layouts (spirv_cross and spirv-cross) and lib/lib64 are supported.
find_path(SPIRV_CROSS_INCLUDE_DIR NAMES spirv_glsl.hpp
    HINTS "${VKGL_SPIRV_CROSS_ROOT}" "$ENV{SPIRV_CROSS_ROOT}"
    PATH_SUFFIXES include/spirv_cross include/spirv-cross include)
find_library(SPIRV_CROSS_GLSL_LIBRARY NAMES spirv-cross-glsl
    HINTS "${VKGL_SPIRV_CROSS_ROOT}" "$ENV{SPIRV_CROSS_ROOT}"
    PATH_SUFFIXES lib lib64 build)
find_library(SPIRV_CROSS_UTIL_LIBRARY NAMES spirv-cross-util
    HINTS "${VKGL_SPIRV_CROSS_ROOT}" "$ENV{SPIRV_CROSS_ROOT}"
    PATH_SUFFIXES lib lib64 build)
find_library(SPIRV_CROSS_CORE_LIBRARY NAMES spirv-cross-core
    HINTS "${VKGL_SPIRV_CROSS_ROOT}" "$ENV{SPIRV_CROSS_ROOT}"
    PATH_SUFFIXES lib lib64 build)

set(VKGL_SPIRV_CROSS_MODE "system")
if(NOT SPIRV_CROSS_INCLUDE_DIR OR NOT SPIRV_CROSS_GLSL_LIBRARY OR
   NOT SPIRV_CROSS_UTIL_LIBRARY OR NOT SPIRV_CROSS_CORE_LIBRARY)
    if(NOT VKGL_FETCH_SPIRV_CROSS)
        message(FATAL_ERROR
            "SPIRV-Cross C++ GLSL/util/core development files were not found. "
            "Install them, set VKGL_SPIRV_CROSS_ROOT, or configure with "
            "-DVKGL_FETCH_SPIRV_CROSS=ON for the hash-pinned public source fallback.")
    endif()

    include(FetchContent)
    set(SPIRV_CROSS_CLI OFF CACHE BOOL "" FORCE)
    set(SPIRV_CROSS_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
    set(SPIRV_CROSS_ENABLE_C_API OFF CACHE BOOL "" FORCE)
    set(SPIRV_CROSS_ENABLE_REFLECT OFF CACHE BOOL "" FORCE)
    set(SPIRV_CROSS_ENABLE_HLSL OFF CACHE BOOL "" FORCE)
    set(SPIRV_CROSS_ENABLE_MSL OFF CACHE BOOL "" FORCE)
    set(SPIRV_CROSS_ENABLE_CPP OFF CACHE BOOL "" FORCE)
    set(SPIRV_CROSS_SHARED OFF CACHE BOOL "" FORCE)
    set(SPIRV_CROSS_STATIC ON CACHE BOOL "" FORCE)
    set(SPIRV_CROSS_FORCE_PIC ON CACHE BOOL "" FORCE)
    set(SPIRV_CROSS_SKIP_INSTALL ON CACHE BOOL "" FORCE)
    FetchContent_Declare(spirv_cross
        URL "https://github.com/KhronosGroup/SPIRV-Cross/archive/refs/tags/vulkan-sdk-1.4.363.0.tar.gz"
        URL_HASH "SHA256=f708982e88b763ef5b0394ed468fc2c3628f68cdab89f2e7a946cd628c04e721"
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_MakeAvailable(spirv_cross)
    set(VKGL_SPIRV_CROSS_MODE "pinned-fetch-vulkan-sdk-1.4.363.0")
    set(VKGL_SPIRV_CROSS_LIBRARIES spirv-cross-glsl spirv-cross-util spirv-cross-core)
else()
    set(VKGL_SPIRV_CROSS_LIBRARIES
        "${SPIRV_CROSS_GLSL_LIBRARY}"
        "${SPIRV_CROSS_UTIL_LIBRARY}"
        "${SPIRV_CROSS_CORE_LIBRARY}")
endif()
message(STATUS "vkgl SPIRV-Cross provider: ${VKGL_SPIRV_CROSS_MODE}")

string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" VKGL_SYSTEM_PROCESSOR)
set(VKGL_LOONGARCH64 OFF)
if(VKGL_SYSTEM_PROCESSOR STREQUAL "loongarch64")
    set(VKGL_LOONGARCH64 ON)
endif()

function(vkgl_apply_compile_options target)
    target_compile_options(${target} PRIVATE
        "$<$<COMPILE_LANG_AND_ID:C,GNU,Clang>:-O3;-flto>"
        "$<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang>:-O3;-flto>")
    if(VKGL_LOONGARCH64)
        if(NOT CMAKE_C_COMPILER_ID STREQUAL "GNU" OR NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            message(FATAL_ERROR "The validated loongarch64 flag set requires GCC")
        endif()
        # Keep the production ISA baseline and LA664 tuning on loongarch64 only.
        target_compile_options(${target} PRIVATE -march=loongarch64 -mtune=la664)
    endif()
    if(VKGL_REPRODUCIBLE)
        target_compile_options(${target} PRIVATE
            "$<$<COMPILE_LANG_AND_ID:C,GNU,Clang>:-ffile-prefix-map=${CMAKE_CURRENT_SOURCE_DIR}=.>"
            "$<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang>:-ffile-prefix-map=${CMAKE_CURRENT_SOURCE_DIR}=.>"
            "$<$<COMPILE_LANG_AND_ID:C,GNU,Clang>:-ffile-prefix-map=${CMAKE_CURRENT_BINARY_DIR}=./build>"
            "$<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang>:-ffile-prefix-map=${CMAKE_CURRENT_BINARY_DIR}=./build>")
    endif()
endfunction()

add_library(vkgl_protocol OBJECT protocol/linux-dmabuf-protocol.c)
target_include_directories(vkgl_protocol PRIVATE protocol)
target_link_libraries(vkgl_protocol PRIVATE PkgConfig::WAYLAND)
vkgl_apply_compile_options(vkgl_protocol)

add_library(vkgl SHARED src/vkgl.cpp $<TARGET_OBJECTS:vkgl_protocol>)
target_include_directories(vkgl PRIVATE src protocol "${VULKAN_INCLUDE_DIR}")
if(SPIRV_CROSS_INCLUDE_DIR)
    target_include_directories(vkgl PRIVATE "${SPIRV_CROSS_INCLUDE_DIR}")
endif()
target_link_libraries(vkgl PRIVATE
    PkgConfig::WAYLAND PkgConfig::EGL PkgConfig::GLESV2 PkgConfig::GBM
    ${VKGL_SPIRV_CROSS_LIBRARIES} Threads::Threads)
target_link_options(vkgl PRIVATE "$<$<CXX_COMPILER_ID:GNU,Clang>:-flto>")
set_target_properties(vkgl PROPERTIES OUTPUT_NAME vkgl)
vkgl_apply_compile_options(vkgl)

configure_file(icd/vkgl_icd.json.in "${CMAKE_CURRENT_BINARY_DIR}/vkgl_icd.json" @ONLY)
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/vkgl_icd_build.json" CONTENT
"{\n  \"file_format_version\": \"1.0.0\",\n  \"ICD\": {\n    \"library_path\": \"$<TARGET_FILE:vkgl>\",\n    \"api_version\": \"1.0.0\"\n  }\n}\n")

install(TARGETS vkgl LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/vkgl")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/vkgl_icd.json"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/vulkan/icd.d")

if(BUILD_TESTING)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_executable(vkgl-test-fence tests/host/test_submit_fence.cpp)
    target_include_directories(vkgl-test-fence PRIVATE src)
    add_test(NAME host-fence COMMAND vkgl-test-fence)

    add_executable(vkgl-test-protocol tests/host/test_protocol.c
        $<TARGET_OBJECTS:vkgl_protocol>)
    target_include_directories(vkgl-test-protocol PRIVATE protocol)
    target_link_libraries(vkgl-test-protocol PRIVATE PkgConfig::WAYLAND)
    add_test(NAME host-protocol COMMAND vkgl-test-protocol)

    add_test(NAME host-repository-policy
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/host/test_repository.py"
        --source-dir "${CMAKE_CURRENT_SOURCE_DIR}" --build-dir "${CMAKE_CURRENT_BINARY_DIR}")
    add_test(NAME host-secret-scan
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/scripts/check_secrets.py"
        "${CMAKE_CURRENT_SOURCE_DIR}")
    set_tests_properties(host-fence host-protocol host-repository-policy host-secret-scan
        PROPERTIES LABELS "host-unit")
endif()

if(VKGL_BUILD_HARDWARE_TESTS)
    find_library(VULKAN_LOADER_LIBRARY NAMES vulkan REQUIRED)
    add_executable(vkgl-lg110-smoke tests/hardware/lg110_vulkan_smoke.cpp)
    target_include_directories(vkgl-lg110-smoke PRIVATE "${VULKAN_INCLUDE_DIR}")
    target_link_libraries(vkgl-lg110-smoke PRIVATE "${VULKAN_LOADER_LIBRARY}")
endif()
