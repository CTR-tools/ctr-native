cmake_minimum_required(VERSION 3.20)
project(CTR-Native C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CTR_NATIVE_VERSION "0.1.0-beta.5")

if(NOT APPLE AND NOT CMAKE_C_COMPILER_TARGET MATCHES "i.86" AND NOT CMAKE_SYSTEM_PROCESSOR MATCHES "i.86")
    set(CMAKE_C_FLAGS "-m32 ${CMAKE_C_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "-m32 ${CMAKE_EXE_LINKER_FLAGS}")
endif()

if(NOT APPLE)
    set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc ${CMAKE_EXE_LINKER_FLAGS}")
endif()

set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE NEVER)

execute_process(
    COMMAND git rev-parse --short=12 HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE CTR_NATIVE_GIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
if(NOT CTR_NATIVE_GIT_HASH)
    set(CTR_NATIVE_GIT_HASH "unknown")
endif()

execute_process(
    COMMAND git diff --quiet --exit-code
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    RESULT_VARIABLE CTR_NATIVE_GIT_DIRTY
    ERROR_QUIET
)
if(CTR_NATIVE_GIT_DIRTY EQUAL 0)
    set(CTR_NATIVE_BUILD_ID "${CTR_NATIVE_GIT_HASH}")
else()
    string(TIMESTAMP CTR_NATIVE_BUILD_TIMESTAMP "%Y%m%d%H%M%S" UTC)
    set(CTR_NATIVE_BUILD_ID "${CTR_NATIVE_GIT_HASH}-dirty-${CTR_NATIVE_BUILD_TIMESTAMP}")
endif()

set(GAME_INCLUDES
    ${CMAKE_SOURCE_DIR}/include
)

if(NOT APPLE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse")
endif()
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-error -Wno-incompatible-pointer-types -g")

# Force-include the layout-assert shim so CTR_STATIC_ASSERT_LAYOUT is defined
# before the first offsetof/sizeof assert in any header (see the header for why).
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -include ${CMAKE_SOURCE_DIR}/include/platform/native_static_assert.h")

# SDL3
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
set(SDL_GPU OFF CACHE BOOL "" FORCE)
set(SDL_RENDER OFF CACHE BOOL "" FORCE)
set(SDL_CAMERA OFF CACHE BOOL "" FORCE)
set(SDL_SENSOR OFF CACHE BOOL "" FORCE)
set(SDL_DIALOG OFF CACHE BOOL "" FORCE)
set(SDL_TRAY OFF CACHE BOOL "" FORCE)
set(SDL_POWER OFF CACHE BOOL "" FORCE)
set(SDL_VULKAN OFF CACHE BOOL "" FORCE)
set(SDL_OPENGLES OFF CACHE BOOL "" FORCE)
set(SDL_OFFSCREEN OFF CACHE BOOL "" FORCE)
set(SDL_SNDIO OFF CACHE BOOL "" FORCE)
set(SDL_SNDIO_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_X11_XSCRNSAVER OFF CACHE BOOL "" FORCE)
add_subdirectory(externals/SDL)

# CTR Native
add_executable(ctr_native main.c)
target_include_directories(ctr_native PRIVATE ${GAME_INCLUDES})
target_compile_definitions(ctr_native PRIVATE
    CTR_NATIVE
    BUILD=926
    CTR_INTERNAL
    CTR_NATIVE_VERSION=\"${CTR_NATIVE_VERSION}\"
    CTR_NATIVE_BUILD_ID=\"${CTR_NATIVE_BUILD_ID}\"
)
target_compile_options(ctr_native PRIVATE -O2)
target_link_libraries(ctr_native SDL3::SDL3)
if(WIN32)
    target_link_options(ctr_native PRIVATE -static -static-libgcc -Wl,--image-base,0x400000 -Wl,--disable-dynamicbase -Wl,--disable-reloc-section)
elseif(APPLE)
    # ld64 has no equivalent of --image-base/-no-pie/--allow-multiple-definition.
    # The whole game is one translation unit (main.c #includes everything), so
    # a multiple-definition failure here would mean a real name collision with
    # SDL3/libc/system frameworks -- fix that by renaming, not by reaching for a flag.
    #
    # NOTE: shrinking __PAGEZERO to make the low 4 GiB mappable (so retail 32-bit
    # pointer slots round-trip) is NOT viable: recent macOS kernels SIGKILL any
    # arm64 binary with a non-default __PAGEZERO at launch. All process memory
    # therefore lives above 4 GiB, so the pointer-truncation gap (LOAD_RunPtrMap
    # etc.) must be fixed in code, not by address placement. See docs/MEMORY_MODEL.md.
else()
    target_link_options(ctr_native PRIVATE -static-libgcc -Wl,--image-base,0x400000 -no-pie -Wl,--allow-multiple-definition)
endif()
