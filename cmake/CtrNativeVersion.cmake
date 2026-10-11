# Resolves the version a build reports.
#
# Creating a release is a tag. Nothing here is edited by hand, and the version
# is derived in priority order:
#
#   1. CTR_NATIVE_EXPANDED_VERSION from the command line, which CI passes from
#      the release tag.
#   2. .git_archival.txt, which `git archive` fills in. GitHub builds its
#      release source archives that way, so a tarball reports the tag it was
#      cut from even though it carries no .git.
#   3. git describe, for a clone that has tags.
#   4. 0.0.0-dev, for a tree with none of the above.
#
# Exports CTR_NATIVE_EXPANDED_VERSION, CTR_NATIVE_EXPANDED_BUILD_ID and
# CTR_NATIVE_EXPANDED_VERSION_SOURCE into the including scope. This is an
# include(), not a function(), so plain set() is what lands them in the caller.

# Parses `git describe --long` output such as v1.0.0-beta3-10-g877de50e5, which
# is the shape both the archive file and git itself produce. The base version
# keeps any hyphenated prerelease of its own, so the parse anchors on the
# trailing "-g<hash>". A tag that is not a release tag, such as the older
# beta-N names still in this repository, is rejected rather than misread.
#
# Sets ctr_parse_describe_base, _metadata and _hash in the caller.
function(ctr_parse_describe describe_text)
    set(base "")
    set(metadata "")
    set(hash "")
    if(describe_text MATCHES "^v([0-9][0-9A-Za-z.+-]*)-([0-9]+)-g([0-9a-f]+)$")
        # Copy out before the next match resets CMAKE_MATCH_N.
        set(captured_base "${CMAKE_MATCH_1}")
        set(captured_distance "${CMAKE_MATCH_2}")
        set(captured_hash "${CMAKE_MATCH_3}")
        set(base "${captured_base}")
        set(hash "${captured_hash}")
        if(NOT captured_distance EQUAL 0)
            set(metadata "${captured_distance}.g${captured_hash}")
        endif()
    endif()
    set(ctr_parse_describe_base "${base}" PARENT_SCOPE)
    set(ctr_parse_describe_metadata "${metadata}" PARENT_SCOPE)
    set(ctr_parse_describe_hash "${hash}" PARENT_SCOPE)
endfunction()

set(tag_pattern "v[0-9]*")
set(archive_file "${CMAKE_SOURCE_DIR}/.git_archival.txt")
set(version "${CTR_NATIVE_EXPANDED_VERSION}")
set(build_metadata "")
set(describe_hash "")

if(version)
    set(source "cache")
else()
    set(describe_text "")
    set(source "fallback")

    if(EXISTS "${archive_file}")
        file(READ "${archive_file}" archive_text)
        string(REGEX MATCH "node:[ \t]*([^\r\n]+)" archive_match "${archive_text}")
        # Copy the capture out before anything else matches: any later
        # if(... MATCHES ...) or string(REGEX ...) resets CMAKE_MATCH_N.
        set(archive_node "${CMAKE_MATCH_1}")
        # In a working tree this is still the literal $Format:$ placeholder.
        # Only `git archive` expands it, and not every git understands the
        # %(describe:...) ref-filter there, so treat anything left unexpanded
        # as absent and fall through.
        if(archive_node AND NOT archive_node MATCHES "Format")
            set(describe_text "${archive_node}")
            set(source "archive")
        endif()
    endif()

    if(NOT describe_text)
        execute_process(
            COMMAND git describe --tags --match "${tag_pattern}" --long --abbrev=12
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE describe_text
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
            RESULT_VARIABLE describe_result
        )
        if(describe_result EQUAL 0)
            set(source "git")
        else()
            set(describe_text "")
        endif()
    endif()

    ctr_parse_describe("${describe_text}")
    if(ctr_parse_describe_base)
        set(version "${ctr_parse_describe_base}")
        set(build_metadata "${ctr_parse_describe_metadata}")
        set(describe_hash "${ctr_parse_describe_hash}")
    else()
        set(version "0.0.0-dev")
        set(source "fallback")
    endif()
endif()

execute_process(
    COMMAND git rev-parse --short=12 HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE repo_hash
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

if(repo_hash)
    set(git_hash "${repo_hash}")
    set(has_git TRUE)
else()
    # No repository here, which is not the same as a tree with local edits. The
    # hash the archive carried is the best identity available.
    set(has_git FALSE)
    set(git_hash "${describe_hash}")
    if(NOT git_hash)
        set(git_hash "unknown")
    endif()
endif()

set(dirty_suffix "")
if(has_git)
    execute_process(
        COMMAND git diff --quiet --exit-code
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        RESULT_VARIABLE git_dirty
        ERROR_QUIET
    )
    if(NOT git_dirty EQUAL 0)
        set(dirty_suffix "-dirty")
        if(build_metadata)
            set(build_metadata "${build_metadata}.dirty")
        elseif(source STREQUAL "git")
            # Sitting on the tag but the tree has edits, so this is not the
            # build the tag promises.
            set(build_metadata "dirty")
        endif()
    endif()
endif()

if(build_metadata)
    set(version "${version}+${build_metadata}")
endif()

set(CTR_NATIVE_EXPANDED_VERSION "${version}")
set(CTR_NATIVE_EXPANDED_BUILD_ID "${git_hash}${dirty_suffix}")
set(CTR_NATIVE_EXPANDED_VERSION_SOURCE "${source}")