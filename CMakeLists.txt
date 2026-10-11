cmake_minimum_required(VERSION 3.20)
project(CTR-Native-Expanded C)
include(CheckCCompilerFlag)

set(CTR_NATIVE_EXPANDED_VERSION "0.1.0-beta.7.1")

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR
        "CTR Native Expanded currently requires a 32-bit target. "
        "Select Win32 for MSVC, an i686 MinGW compiler on Windows, or pass -m32 "
        "in the initial compiler and linker flags on Linux."
    )
endif()

set(CTR_NATIVE_EXPANDED_MSVC_FRONTEND OFF)
if(CMAKE_C_COMPILER_ID STREQUAL "MSVC" OR CMAKE_C_SIMULATE_ID STREQUAL "MSVC")
    set(CTR_NATIVE_EXPANDED_MSVC_FRONTEND ON)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE NEVER)

execute_process(
    COMMAND git rev-parse --short=12 HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE CTR_NATIVE_EXPANDED_GIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
if(NOT CTR_NATIVE_EXPANDED_GIT_HASH)
    set(CTR_NATIVE_EXPANDED_GIT_HASH "unknown")
endif()

execute_process(
    COMMAND git diff --quiet --exit-code
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    RESULT_VARIABLE CTR_NATIVE_EXPANDED_GIT_DIRTY
    ERROR_QUIET
)
if(CTR_NATIVE_EXPANDED_GIT_DIRTY EQUAL 0)
    set(CTR_NATIVE_EXPANDED_BUILD_ID "${CTR_NATIVE_EXPANDED_GIT_HASH}")
else()
    set(CTR_NATIVE_EXPANDED_BUILD_ID "${CTR_NATIVE_EXPANDED_GIT_HASH}-dirty")
endif()

function(ctr_native_add_label_warning_if_supported target warning flag_check)
    set(saved_required_flags "${CMAKE_REQUIRED_FLAGS}")
    if(CMAKE_C_COMPILER_ID MATCHES "Clang")
        set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -Werror=unknown-warning-option")
    endif()

    check_c_compiler_flag("${warning}" "${flag_check}")
    set(CMAKE_REQUIRED_FLAGS "${saved_required_flags}")

    if(${flag_check})
        target_compile_options(${target} PRIVATE "${warning}")
    endif()
endfunction()

# The warning set every first-party target is held to, so a test and the code it
# covers report the same diagnostics.
function(ctr_native_apply_common_options target)
    if(CTR_NATIVE_EXPANDED_MSVC_FRONTEND)
        target_compile_options(${target} PRIVATE
            /W4
            /bigobj
            /arch:SSE2
            /wd4152
            /wd4244
            /wd4245
            /wd4310
            /wd5287
        )
    else()
        target_compile_options(${target} PRIVATE
            -msse
            -Wenum-conversion
            -Wenum-compare
            -Wswitch-enum
            -Wformat
            -Wparentheses
            $<$<CONFIG:Release>:-O2>
            $<$<CONFIG:Release>:-g>
        )

        if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
            target_compile_options(${target} PRIVATE -Wall -Wextra -Wstrict-aliasing=2)
            ctr_native_add_label_warning_if_supported(${target} -Wfree-labels CTR_HAS_WFREE_LABELS)
        elseif(CMAKE_C_COMPILER_ID MATCHES "Clang")
            ctr_native_add_label_warning_if_supported(${target} -Wc23-extensions CTR_HAS_WC23_EXTENSIONS)
        endif()
    endif()
endfunction()

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
set(SDL_X11_XTEST OFF CACHE BOOL "" FORCE)
add_subdirectory(externals/SDL)

# Copy default config.ini to build directory if config.ini doesn't exist
if(NOT EXISTS "${CMAKE_BINARY_DIR}/config.ini")
    configure_file("${CMAKE_SOURCE_DIR}/default_config.ini"
                   "${CMAKE_BINARY_DIR}/config.ini" COPYONLY)
    message(STATUS "Copied default_config.ini → ${CMAKE_BINARY_DIR}/config.ini")
endif()

# Same for controls.ini, which is written by the in-game Controls screen. Copied
# only as a reference: the game creates it on the first binding change, and an
# existing build/controls.ini must never be overwritten.
if(NOT EXISTS "${CMAKE_BINARY_DIR}/controls.ini")
    configure_file("${CMAKE_SOURCE_DIR}/default_controls.ini"
                   "${CMAKE_BINARY_DIR}/controls.ini" COPYONLY)
    message(STATUS "Copied default_controls.ini → ${CMAKE_BINARY_DIR}/controls.ini")
endif()

# CTR Native Expanded
add_executable(ctr_native_expanded main.c)
set_target_properties(ctr_native_expanded PROPERTIES
    C_STANDARD 17
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
target_include_directories(ctr_native_expanded PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(ctr_native_expanded PRIVATE
    CTR_INTERNAL
    CTR_NATIVE_EXPANDED_VERSION=\"${CTR_NATIVE_EXPANDED_VERSION}\"
    CTR_NATIVE_EXPANDED_BUILD_ID=\"${CTR_NATIVE_EXPANDED_BUILD_ID}\"
)
#TODO(aalhendi): fix warnings
ctr_native_apply_common_options(ctr_native_expanded)
target_link_libraries(ctr_native_expanded SDL3::SDL3)
if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_link_options(ctr_native_expanded PRIVATE -static-libgcc)
endif()
if(MINGW)
    target_link_options(ctr_native_expanded PRIVATE -static)
endif()

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
