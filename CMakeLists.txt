cmake_minimum_required(VERSION 3.20)
project(CTR-Native C)

set(CMAKE_C_STANDARD 99)

if(NOT CMAKE_C_COMPILER_TARGET MATCHES "i.86" AND NOT CMAKE_SYSTEM_PROCESSOR MATCHES "i.86")
    set(CMAKE_C_FLAGS "-m32 ${CMAKE_C_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "-m32 ${CMAKE_EXE_LINKER_FLAGS}")
endif()

set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc ${CMAKE_EXE_LINKER_FLAGS}")

set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE NEVER)

set(GAME_SOURCES
    ctr_native.c
    platform/native_audio.c
    platform/native_checkpoint_file.c
    platform/native_cd.c
    platform/native_gpu.c
    platform/native_gte_core.c
    platform/native_glad.c
    platform/native_input.c
    platform/native_inline_c.c
    platform/native_libapi.c
    platform/native_libetc.c
    platform/native_libgte.c
    platform/native_libgpu.c
    platform/native_libpad.c
    platform/native_libspu.c
    platform/native_log.c
    platform/native_memcard.c
    platform/native_renderer.c
    platform/native_state.c
)

set(GAME_INCLUDES
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/externals/PsyCross/include
)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse -w -Wno-error -Wno-implicit-function-declaration -Wno-int-conversion -Wno-incompatible-pointer-types -g")

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
add_executable(ctr_native ${GAME_SOURCES})
target_include_directories(ctr_native PRIVATE ${GAME_INCLUDES})
target_compile_definitions(ctr_native PRIVATE
    CTR_NATIVE
    BUILD=926
    CTR_INTERNAL
)
target_compile_options(ctr_native PRIVATE -O2)
target_link_libraries(ctr_native SDL3::SDL3)
if(WIN32)
    target_link_options(ctr_native PRIVATE -static -static-libgcc -Wl,--image-base,0x400000 -Wl,--disable-dynamicbase -Wl,--disable-reloc-section)
else()
    target_link_options(ctr_native PRIVATE -static-libgcc -Wl,--image-base,0x400000 -no-pie -Wl,--allow-multiple-definition)
endif()
