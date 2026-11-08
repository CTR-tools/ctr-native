# Test suites.
#
# Each suite is its own executable so a failure names the suite that failed and
# so CLion can run one without starting the game.
set(CTR_TEST_OUTPUT_DIR "${CMAKE_BINARY_DIR}/tests")
set(CTR_TEST_SUITES "")

add_test(NAME ctr_native_expanded_version COMMAND $<TARGET_FILE:ctr_native_expanded> --version)
set_tests_properties(ctr_native_expanded_version PROPERTIES PASS_REGULAR_EXPRESSION "^CTR Native Expanded ")

# Declares one suite. NAME is both the target and the CTest name. SOURCES are
# the files it compiles.
function(ctr_add_test_suite NAME)
    cmake_parse_arguments(ARG "" "" "SOURCES;LIBRARIES" ${ARGN})
    add_executable(${NAME} ${ARG_SOURCES})

    set_target_properties(${NAME} PROPERTIES
        C_STANDARD 17
        C_STANDARD_REQUIRED ON
        C_EXTENSIONS OFF
        RUNTIME_OUTPUT_DIRECTORY "${CTR_TEST_OUTPUT_DIR}"
    )

    # shared/ for the harness, the repository root so a suite can name
    # platform/ and game/ sources by path, and include/ for project headers.
    target_include_directories(${NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/include
    )

    ctr_native_apply_common_options(${NAME})

    if(CTR_NATIVE_EXPANDED_MSVC_FRONTEND)
        # main.c sets this for the game tree because it compiles the same platform
        # sources; a suite does too.
        target_compile_definitions(${NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
        # A suite asserts on compile-time constants on purpose: a timeout window,
        # a default, a version bump. /W4 reports every one of those as C4127.
        target_compile_options(${NAME} PRIVATE /wd4127)
    endif()

    if(ARG_LIBRARIES)
        target_link_libraries(${NAME} ${ARG_LIBRARIES})
    endif()
    if(MINGW)
        target_link_options(${NAME} PRIVATE -static)
    endif()

    add_test(NAME ${NAME} COMMAND ${NAME})
    # The suites resolve their own scratch directories rather than reading and
    # writing the working directory, but pinning it keeps CLion and ctest
    # starting them from the same place regardless.
    set_tests_properties(${NAME} PROPERTIES WORKING_DIRECTORY "${CTR_TEST_OUTPUT_DIR}")

    set(CTR_TEST_SUITES ${CTR_TEST_SUITES} ${NAME} PARENT_SCOPE)
endfunction()

# Header-only, so it needs none of the platform unit.
ctr_add_test_suite(test_track_text
    SOURCES test_track_text.c
)

ctr_add_test_suite(test_level_info
    SOURCES test_level_info.c
)

ctr_add_test_suite(test_end_race_camera
    SOURCES test_end_race_camera.c
)

ctr_add_test_suite(test_track_registry
    SOURCES test_track_registry.c
)

ctr_add_test_suite(test_track_library
    SOURCES test_track_library.c
    LIBRARIES SDL3::SDL3
)

ctr_add_test_suite(test_track_import
    SOURCES test_track_import.c
    LIBRARIES SDL3::SDL3
)

ctr_add_test_suite(test_track_import_flow
    SOURCES test_track_import_flow.c
)

ctr_add_test_suite(test_track_patch
    SOURCES test_track_patch.c
    LIBRARIES SDL3::SDL3
)

ctr_add_test_suite(test_track_records
    SOURCES test_track_records.c
    LIBRARIES SDL3::SDL3
)

ctr_add_test_suite(test_track_file
    SOURCES test_track_file.c
    LIBRARIES SDL3::SDL3
)

ctr_add_test_suite(test_track_validate
    SOURCES test_track_validate.c
)

ctr_add_test_suite(test_track_map
    SOURCES test_track_map.c
)

ctr_add_test_suite(test_cycle_tex
    SOURCES test_cycle_tex.c
)

ctr_add_test_suite(test_water_visibility
    SOURCES test_water_visibility.c
)

ctr_add_test_suite(test_config_menu
    SOURCES test_config_menu.c
)

ctr_add_test_suite(test_render_scale
    SOURCES test_render_scale.c
)

ctr_add_test_suite(test_race_field
    SOURCES test_race_field.c
)

# The Chaos catalogue and race gate, both header-only, for the same reason.
ctr_add_test_suite(test_chaos_effect
    SOURCES test_chaos_effect.c
)

ctr_add_test_suite(test_chaos_gate
    SOURCES test_chaos_gate.c
)

ctr_add_test_suite(test_chaos_runtime
    SOURCES test_chaos_runtime.c
)

ctr_add_test_suite(test_chaos_effects
    SOURCES test_chaos_effects.c
)

ctr_add_test_suite(test_warpball
    SOURCES test_warpball.c
)

ctr_add_test_suite(test_vehicle_spawn
    SOURCES test_vehicle_spawn.c
)

ctr_add_test_suite(test_chaos_pickup
    SOURCES test_chaos_pickup.c
)

ctr_add_test_suite(test_chaos_ipc
    SOURCES test_chaos_ipc.c
    LIBRARIES SDL3::SDL3
)

ctr_add_test_suite(test_howl_sample
    SOURCES test_howl_sample.c
)

ctr_add_test_suite(test_native_cd
    SOURCES test_native_cd.c
    LIBRARIES SDL3::SDL3
)

ctr_add_test_suite(test_chaos_new_file
    SOURCES test_chaos_new_file.c
)

ctr_add_test_suite(test_chaos_setup
    SOURCES test_chaos_setup.c
)

ctr_add_test_suite(test_menu_strings
    SOURCES test_menu_strings.c
)

ctr_add_test_suite(test_boss_cutscenes
    SOURCES test_boss_cutscenes.c
)

ctr_add_test_suite(test_decal_font_scale
    SOURCES test_decal_font_scale.c
)

ctr_add_test_suite(test_chaos_save
    SOURCES test_chaos_save.c
)

# test_pgxp.c #includes platform/native_pgxp.c so it reaches the module's
# file-static shadow tables; the repository root on the include path resolves it,
# and it is deliberately absent from SOURCES so it is not compiled twice.
ctr_add_test_suite(test_pgxp
    SOURCES test_pgxp.c
)

ctr_add_test_suite(test_native_controls
    SOURCES controls/test_native_controls.c
    LIBRARIES SDL3::SDL3
)

ctr_add_test_suite(test_controls_menu
    SOURCES controls/test_controls_menu.c
)

# test_memtrack.c #includes platform/native_memtrack.c so it can read the
# peaks, which are file-static and gated behind CTR_DIAGNOSTICS. It therefore
# needs the same definition the game build has, or the module compiles to
# nothing and the suite would pass without checking anything.
ctr_add_test_suite(test_memtrack
    SOURCES test_memtrack.c
)
target_compile_definitions(test_memtrack PRIVATE CTR_DIAGNOSTICS)

# Header-only, so it needs none of the platform unit.
ctr_add_test_suite(test_draw_distance
    SOURCES test_draw_distance.c
)

ctr_add_test_suite(test_level_lod
    SOURCES test_level_lod.c
)

# Compiles game/MAIN/MainDB.c into its own translation unit so the arena budget is
# checked against the real size tables, and platform/native_memory.c for the real
# mempack backing size.
ctr_add_test_suite(test_level_memory_budget
    SOURCES test_level_memory_budget.c
)

ctr_add_test_suite(test_clip_records
    SOURCES test_clip_records.c
)

ctr_add_test_suite(test_draw_tires
    SOURCES test_draw_tires.c
)
if(UNIX)
    target_link_libraries(test_draw_tires PRIVATE m)
endif()

# Builds the suites without the game, for when only they are in need.
add_custom_target(tests DEPENDS ${CTR_TEST_SUITES})
