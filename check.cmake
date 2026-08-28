# check.cmake — the verification. There is no CI, so this script is it.
#
# Run from the repository root:
#
#     cmake -P check.cmake
#
# It prints one line per step, stops at the first failure, and exits non-zero.
# `skip` is a pass: a step whose tool is absent says so and the run continues.
# Only step 4a is expected to skip in normal use.
#
# Steps 1 to 4 shell out to cmake, so a Ninja and a clang must be on PATH.
# Step 5 reads sources only and needs no toolchain.
#
# Two conventions the steps below rely on:
#
#   - A folder is discovered, never listed: every top-level directory holding a
#     CMakeLists.txt is a folder. New folders are picked up with no edit here.
#   - The guard steps assert on message text from cmake/voe.cmake. They match by
#     substring, so they prove which guard fired, not merely that one did.

cmake_minimum_required(VERSION 3.28)

set(root "${CMAKE_CURRENT_LIST_DIR}")
set(checkdir "${root}/build/check")
set(generator -G Ninja)
set(common ${generator} -DCMAKE_C_COMPILER=clang)

function(step_ok name)
    message("ok    ${name}")
endfunction()

function(step_skip name)
    message("skip  ${name}")
endfunction()

# Prints FAIL, then the detail indented, then stops the script non-zero.
function(step_fail name detail)
    message("FAIL  ${name}")
    if(NOT detail STREQUAL "")
        string(REPLACE "\n" "\n      " detail "${detail}")
        message("      ${detail}")
    endif()
    message(FATAL_ERROR "check failed at: ${name}")
endfunction()

# Runs a command, returning its exit code and its stdout and stderr merged.
# A code that is not a number means the program could not be launched at all;
# callers distinguish "absent" from "ran and failed" on that.
function(run_capture out_code out_text)
    execute_process(
        COMMAND ${ARGN}
        RESULT_VARIABLE code
        OUTPUT_VARIABLE out
        ERROR_VARIABLE err)
    set(${out_code} "${code}" PARENT_SCOPE)
    set(${out_text} "${out}${err}" PARENT_SCOPE)
endfunction()

file(REMOVE_RECURSE "${checkdir}")
file(MAKE_DIRECTORY "${checkdir}")

# ---------------------------------------------------------------- 1 tools

set(step "tools")

run_capture(code text clang --version)
if(NOT code MATCHES "^[0-9]+$")
    step_fail("${step}" "clang could not be run: ${code}")
endif()
if(NOT code EQUAL 0)
    step_fail("${step}" "clang --version exited ${code}")
endif()
if(NOT text MATCHES "clang version ([0-9]+)")
    step_fail("${step}" "could not parse a version out of: ${text}")
endif()
set(clang_major "${CMAKE_MATCH_1}")
if(clang_major LESS 18)
    step_fail("${step}" "clang major ${clang_major} is below 18")
endif()

run_capture(code text "${CMAKE_COMMAND}" --version)
if(NOT code MATCHES "^[0-9]+$" OR NOT code EQUAL 0)
    step_fail("${step}" "cmake --version did not run: ${code}")
endif()
if(NOT text MATCHES "cmake version ([0-9]+\\.[0-9]+(\\.[0-9]+)?)")
    step_fail("${step}" "could not parse a version out of: ${text}")
endif()
set(cmake_version "${CMAKE_MATCH_1}")
if(cmake_version VERSION_LESS 3.28)
    step_fail("${step}" "cmake ${cmake_version} is below 3.28")
endif()

run_capture(code text slangc -v)
if(NOT code MATCHES "^[0-9]+$")
    step_fail("${step}" "slangc could not be run: ${code}")
endif()

step_ok("${step} (clang ${clang_major}, cmake ${cmake_version}, slangc)")

# ------------------------------------------------------------ folder list

file(GLOB entries LIST_DIRECTORIES true RELATIVE "${root}" "${root}/*")
set(folders "")
foreach(entry IN LISTS entries)
    if(IS_DIRECTORY "${root}/${entry}" AND EXISTS "${root}/${entry}/CMakeLists.txt")
        list(APPEND folders "${entry}")
    endif()
endforeach()
list(SORT folders)
if(folders STREQUAL "")
    step_fail("folders" "no top-level directory holds a CMakeLists.txt")
endif()

# ----------------------------------------------------------- 2 standalone

foreach(folder IN LISTS folders)
    set(step "standalone ${folder}")
    run_capture(code text "${CMAKE_COMMAND}"
        -S "${root}/${folder}" -B "${checkdir}/${folder}" ${common})
    if(NOT code MATCHES "^[0-9]+$" OR NOT code EQUAL 0)
        step_fail("${step}" "${text}")
    endif()
    step_ok("${step}")
endforeach()

# ----------------------------------------------------------------- 3 root

set(step "root configure and build")
run_capture(code text "${CMAKE_COMMAND}"
    -S "${root}" -B "${checkdir}/root" ${common} -DCMAKE_BUILD_TYPE=Debug)
if(NOT code MATCHES "^[0-9]+$" OR NOT code EQUAL 0)
    step_fail("${step}" "${text}")
endif()
run_capture(code text "${CMAKE_COMMAND}" --build "${checkdir}/root")
if(NOT code MATCHES "^[0-9]+$" OR NOT code EQUAL 0)
    step_fail("${step}" "${text}")
endif()
step_ok("${step}")

# ------------------------------------------------------- 4a guard compiler

set(step "guard compiler")
find_program(gcc_path gcc)
if(NOT gcc_path)
    step_skip("${step} (no gcc)")
else()
    run_capture(code text "${CMAKE_COMMAND}"
        -S "${root}" -B "${checkdir}/guard_compiler" ${generator} -DCMAKE_C_COMPILER=gcc)
    if(code MATCHES "^[0-9]+$" AND code EQUAL 0)
        step_fail("${step}" "configuring with gcc succeeded; the guard did not fire")
    endif()
    if(NOT text MATCHES "ADR-0005")
        step_fail("${step}" "failed, but not on the compiler guard:\n${text}")
    endif()
    step_ok("${step}")
endif()

# -------------------------------------------------------- 4b guard version

set(step "guard version")
run_capture(code text "${CMAKE_COMMAND}"
    -S "${root}" -B "${checkdir}/guard_version" ${common} -DVOE_CHECK_FAKE_CLANG_VERSION=17)
if(code MATCHES "^[0-9]+$" AND code EQUAL 0)
    step_fail("${step}" "a faked clang 17 configured; the guard did not fire")
endif()
if(NOT text MATCHES "Clang 18 or newer")
    step_fail("${step}" "failed, but not on the version guard:\n${text}")
endif()
step_ok("${step}")

# ------------------------------------------------------------ 4c guard map

set(step "guard map")
file(MAKE_DIRECTORY "${checkdir}/badedge/src")
file(WRITE "${checkdir}/badedge/CMakeLists.txt"
"cmake_minimum_required(VERSION 3.28)
project(voe_badedge C)
include(${root}/cmake/voe.cmake)
voe_module(badedge DEPENDS math)
")
run_capture(code text "${CMAKE_COMMAND}"
    -S "${checkdir}/badedge" -B "${checkdir}/badedge-build" ${common})
if(code MATCHES "^[0-9]+$" AND code EQUAL 0)
    step_fail("${step}" "a disallowed edge configured; the guard did not fire")
endif()
if(NOT text MATCHES "ADR-0022")
    step_fail("${step}" "failed, but not on the dependency map:\n${text}")
endif()
step_ok("${step}")

# ------------------------------------------------------------- 5 includes

set(step "includes")
set(forbidden "windows.h" "unistd.h" "dlfcn.h" "pthread.h")
set(violations "")

foreach(folder IN LISTS folders)
    # The folder's own declared dependencies, read back out of its four lines.
    file(READ "${root}/${folder}/CMakeLists.txt" listfile)
    set(declared "")
    if(listfile MATCHES "voe_module\\([ \t]*${folder}[ \t]+DEPENDS([^)]*)\\)")
        string(STRIP "${CMAKE_MATCH_1}" declared)
        string(REGEX REPLACE "[ \t\r\n]+" ";" declared "${declared}")
    endif()

    file(GLOB_RECURSE sources "${root}/${folder}/*.c" "${root}/${folder}/*.h")
    foreach(source IN LISTS sources)
        file(RELATIVE_PATH shown "${root}" "${source}")
        file(STRINGS "${source}" lines REGEX "^[ \t]*#[ \t]*include")
        foreach(line IN LISTS lines)
            foreach(bad IN LISTS forbidden)
                if(line MATCHES "<${bad}>")
                    list(APPEND violations "${shown}: <${bad}> is not allowed outside platform")
                endif()
            endforeach()
            if(line MATCHES "<(X11/|xcb/|wayland)")
                list(APPEND violations "${shown}: ${line} is a window-system header")
            endif()
            if(line MATCHES "#[ \t]*include[ \t]*[<\"]([A-Za-z0-9_]+)/")
                set(named "${CMAKE_MATCH_1}")
                if(named IN_LIST folders
                        AND NOT named STREQUAL folder
                        AND NOT named IN_LIST declared)
                    list(APPEND violations
                        "${shown}: includes ${named}/, which ${folder} does not DEPENDS on")
                endif()
            endif()
        endforeach()
    endforeach()
endforeach()

if(NOT violations STREQUAL "")
    string(REPLACE ";" "\n" violations "${violations}")
    step_fail("${step}" "${violations}")
endif()
step_ok("${step}")

# ---------------------------------------------------------------- 6 tests

step_ok("tests (none yet)")
