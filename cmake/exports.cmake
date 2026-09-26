# exports.cmake — writes the module definition the editor links on Windows.
#
# Run with `cmake -P`, by the build rule voe_executable() adds to voe_editor on
# WIN32 (ADR-0245 point 1). A DLL built on the MSVC ABI binds its undefined
# names only to names the program exports, so a project library can call the
# engine only if the editor exports all of it. This lists every defined external
# `voe_` symbol of the editor's DEPENDS archives in a `.def`: `EXPORTS`, then one
# name per line, each once, sorted. The link then writes voe_editor.lib, the
# import library the project links.
#
# Functions (nm type T) are listed bare. Data (D, B, R, C) is listed with DATA:
# a DATA export has no thunk, so a project reaches it only through
# __declspec(dllimport), which VOE_BASE_IMPORTED gives it (ADR-0245 point 2).
#
# An archive member that lists `voe_game_project_register`,
# `voe_game_project_systems_run` or `voe_game_project_systems_after_move` as
# undefined (game's run.c) is skipped: those
# entry points are the project's to define, and exporting the editor's caller
# of them would say the editor defines what it only calls.
#
# Inputs:
#   NM        the nm to run; llvm-nm and GNU nm both print `-A -P -g` as
#             `archive[member]: name type value size`.
#   ARCHIVES  a `;` list of static archives.
#   OUT       the `.def` to write; written only when its bytes change, so an
#             unchanged engine relinks nothing.
#
# Constraints: a failed NM is a FATAL_ERROR naming the archive. Symbol types
# other than T, D, B, R and C (weak, absolute) are not exported.

foreach(input NM ARCHIVES OUT)
    if(NOT DEFINED ${input} OR "${${input}}" STREQUAL "")
        message(FATAL_ERROR "exports.cmake: -D${input}= is required")
    endif()
endforeach()

set(names "")
foreach(archive IN LISTS ARCHIVES)
    execute_process(COMMAND "${NM}" -A -P -g "${archive}"
        RESULT_VARIABLE result OUTPUT_VARIABLE listing ERROR_VARIABLE errors)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "exports.cmake: ${NM} failed on ${archive} (${result}): ${errors}")
    endif()
    string(REPLACE ";" "" listing "${listing}")
    string(REPLACE "\n" ";" lines "${listing}")

    # First pass: the members that leave a project entry point undefined.
    foreach(line IN LISTS lines)
        if(line MATCHES "^(.*\\[[^]]*\\]): (voe_game_project_register|voe_game_project_systems_run|voe_game_project_systems_after_move) U")
            string(MD5 key "${CMAKE_MATCH_1}")
            set(skip_${key} 1)
        endif()
    endforeach()

    # Second pass: every defined voe_ symbol of every other member.
    foreach(line IN LISTS lines)
        if(NOT line MATCHES "^(.*\\[[^]]*\\]): (voe_[A-Za-z0-9_]*) ([TDBRC])( |$)")
            continue()
        endif()
        string(MD5 key "${CMAKE_MATCH_1}")
        if(DEFINED skip_${key} OR DEFINED seen_${CMAKE_MATCH_2})
            continue()
        endif()
        set(seen_${CMAKE_MATCH_2} 1)
        if(CMAKE_MATCH_3 STREQUAL "T")
            list(APPEND names "${CMAKE_MATCH_2}")
        else()
            list(APPEND names "${CMAKE_MATCH_2} DATA")
        endif()
    endforeach()
endforeach()

list(SORT names)
list(JOIN names "\n" body)
set(text "EXPORTS\n${body}\n")

set(old "")
if(EXISTS "${OUT}")
    file(READ "${OUT}" old)
endif()
if(NOT old STREQUAL text)
    file(WRITE "${OUT}" "${text}")
endif()
