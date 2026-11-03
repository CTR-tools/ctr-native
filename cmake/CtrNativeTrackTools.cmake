# The native Windows build prepares standalone tools next to the source-tree
# assets used by development builds. Only the game target needs them.
if(WIN32)
    find_program(CTR_TRACK_TOOLS_POWERSHELL NAMES pwsh powershell REQUIRED)
    add_custom_target(ctr_track_tools
        COMMAND "${CTR_TRACK_TOOLS_POWERSHELL}"
            -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass
            -File "${CMAKE_SOURCE_DIR}/scripts/setup-track-tools.ps1"
            -Assets "${CMAKE_SOURCE_DIR}/assets"
            -Cache "${CMAKE_BINARY_DIR}/_deps/track-tools"
        COMMENT "Verifying Windows track import tools (download if missing)"
        VERBATIM
    )
    add_dependencies(ctr_native_expanded ctr_track_tools)
endif()
