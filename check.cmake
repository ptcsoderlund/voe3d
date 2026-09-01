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
# Most steps shell out to cmake or ctest, so a Ninja and a clang must be on
# PATH. Step 5 reads sources only and needs no toolchain; step 7 runs clang
# itself, replaying the compile database step 3 wrote.
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
if(clang_major LESS 19)
    step_fail("${step}" "clang major ${clang_major} is below 19")
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

# wayland-scanner is a source-transforming tool on the same footing as slangc and
# as the Windows SDK: the programmer installs it, the build does not fetch it.
# platform's only Linux backend is Wayland, so on Linux it is required and on
# Windows it is not looked for.
set(tools "clang ${clang_major}, cmake ${cmake_version}, slangc")
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    run_capture(code text wayland-scanner --version)
    if(NOT code MATCHES "^[0-9]+$")
        step_fail("${step}" "wayland-scanner could not be run: ${code}")
    endif()
    string(APPEND tools ", wayland-scanner")
endif()

step_ok("${step} (${tools})")

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

# The faked version is one below the floor, and that is the whole value of this
# step. Anything lower fires the guard whatever the floor is, so it would keep
# passing after a floor moved and prove nothing; 18 against a floor of 19 fails
# the moment someone lowers the guard back. When the floor moves again, this
# number moves with it — as does the string below, which is what proves it was
# the version guard that fired and not some other configuration error.
set(step "guard version")
run_capture(code text "${CMAKE_COMMAND}"
    -S "${root}" -B "${checkdir}/guard_version" ${common} -DVOE_CHECK_FAKE_CLANG_VERSION=18)
if(code MATCHES "^[0-9]+$" AND code EQUAL 0)
    step_fail("${step}" "a faked clang 18 configured; the guard did not fire")
endif()
if(NOT text MATCHES "Clang 19 or newer")
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
    # platform is the folder that talks to the operating system, so it is the one
    # folder these two rules do not apply to. Everything else about it is still
    # checked, including that its public headers name no other folder it does not
    # depend on. Until card 004 this loop banned an OS header everywhere, message
    # and behaviour disagreeing, because no folder had yet tried to include one.
    set(os_headers_allowed OFF)
    if(folder STREQUAL "platform")
        set(os_headers_allowed ON)
    endif()

    # The folder's own declared dependencies, read back out of its four lines.
    file(READ "${root}/${folder}/CMakeLists.txt" listfile)
    set(declared "")
    if(listfile MATCHES "voe_(module|executable)\\([ \t]*${folder}[ \t]+DEPENDS([^)]*)\\)")
        string(STRIP "${CMAKE_MATCH_2}" declared)
        string(REGEX REPLACE "[ \t\r\n]+" ";" declared "${declared}")
    endif()

    file(GLOB_RECURSE sources "${root}/${folder}/*.c" "${root}/${folder}/*.h")
    foreach(source IN LISTS sources)
        file(RELATIVE_PATH shown "${root}" "${source}")
        file(STRINGS "${source}" lines REGEX "^[ \t]*#[ \t]*include")
        foreach(line IN LISTS lines)
            if(NOT os_headers_allowed)
                foreach(bad IN LISTS forbidden)
                    if(line MATCHES "<${bad}>")
                        list(APPEND violations "${shown}: <${bad}> is not allowed outside platform")
                    endif()
                endforeach()
                if(line MATCHES "<(X11/|xcb/|wayland)")
                    list(APPEND violations "${shown}: ${line} is a window-system header")
                endif()
            endif()
            if(line MATCHES "[<\"]testing/" AND NOT shown MATCHES "^${folder}/tests/")
                list(APPEND violations "${shown}: only tests/ may include testing/")
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

# ctest against the build step 3 already made, so this runs what the engine
# actually builds rather than a configuration invented here. A tree with no
# tests in it is not a failure; a tree whose tests fail is.
set(step "tests")

run_capture(code text "${CMAKE_CTEST_COMMAND}"
    --test-dir "${checkdir}/root" --output-on-failure)
if(NOT code MATCHES "^[0-9]+$")
    step_fail("${step}" "ctest could not be run: ${code}")
endif()
if(text MATCHES "No tests were found")
    step_ok("${step} (the tree has none yet)")
elseif(NOT code EQUAL 0)
    step_fail("${step}" "${text}")
else()
    string(REGEX MATCH "out of ([0-9]+)" matched "${text}")
    step_ok("${step} (${CMAKE_MATCH_1} passed)")
endif()

# --------------------------------------------- 6b harness reports a failure

# Step 6 passing proves nothing on its own: a harness that always reports green
# passes it too, and would make every test written from here on worthless. So a
# scratch folder with one passing and one deliberately failing test is built and
# run, and the failure is required to come back — by exit code, by count, and by
# the text of all three checks, which also proves a run does not stop at the
# first failure.
set(step "harness reports a failure")

file(MAKE_DIRECTORY "${checkdir}/harness/include/harness")
file(MAKE_DIRECTORY "${checkdir}/harness/src")
file(MAKE_DIRECTORY "${checkdir}/harness/tests")
file(WRITE "${checkdir}/harness/CMakeLists.txt"
"cmake_minimum_required(VERSION 3.28)
project(voe_harness C)
include(${root}/cmake/voe.cmake)
voe_module(harness)
")
file(WRITE "${checkdir}/harness/include/harness/value.h"
"#pragma once
int voe_harness_value(void);
")
file(WRITE "${checkdir}/harness/src/value.c"
"#include <harness/value.h>
int voe_harness_value(void) { return 7; }
")
file(WRITE "${checkdir}/harness/tests/pass.c"
"#include <harness/value.h>
#include <testing/test.h>

int main(void)
{
        VOE_TEST_CHECK(voe_harness_value() == 7);
        VOE_TEST_CHECK_INT(voe_harness_value(), 7);
        VOE_TEST_CHECK_FLOAT(1.0, 1.0, 1e-9);
        return voe_test_result();
}
")
file(WRITE "${checkdir}/harness/tests/fail.c"
"#include <testing/test.h>

int main(void)
{
        VOE_TEST_CHECK(1 == 2);
        VOE_TEST_CHECK_INT(2 + 2, 5);
        VOE_TEST_CHECK_FLOAT(0.25, 0.5, 1e-9);
        return voe_test_result();
}
")

run_capture(code text "${CMAKE_COMMAND}"
    -S "${checkdir}/harness" -B "${checkdir}/harness-build" ${common}
    -DCMAKE_BUILD_TYPE=Debug)
if(NOT code MATCHES "^[0-9]+$" OR NOT code EQUAL 0)
    step_fail("${step}" "the scratch folder did not configure:\n${text}")
endif()

run_capture(code text "${CMAKE_COMMAND}" --build "${checkdir}/harness-build")
if(NOT code MATCHES "^[0-9]+$" OR NOT code EQUAL 0)
    step_fail("${step}" "the scratch tests did not build:\n${text}")
endif()

run_capture(code text "${CMAKE_CTEST_COMMAND}"
    --test-dir "${checkdir}/harness-build" --output-on-failure)
if(code MATCHES "^[0-9]+$" AND code EQUAL 0)
    step_fail("${step}" "a failing test reported success:\n${text}")
endif()

# One passing and one failing, and every check in the failing one reported.
foreach(expected
        "1 tests failed out of 2"
        "1 == 2"
        "2 \\+ 2 == 5"
        "expected: 5"
        "off by 0.25")
    if(NOT text MATCHES "${expected}")
        step_fail("${step}"
            "ctest reported failure, but its output lacks '${expected}':\n${text}")
    endif()
endforeach()
step_ok("${step}")

# ------------------------------------------------------------- 7 analyser

# The step CLAUDE.md rule 8 has always promised and ADR-0042 decided: clang's
# static analyser over every folder's own sources, zero findings, no baseline
# file and no tolerated count. It is step 7 because rule 8 already calls it
# that, and 7b below is its can't-fail proof exactly as 6b is step 6's.
#
# THE FLAGS ARE NOT INVENTED HERE. Reconstructing them would be a second copy of
# what cmake/voe.cmake already knows — the include directories, the C standard,
# the warning set, render's --embed-dir — and the copy would go stale the first
# time one of them moved. So the compile database step 3 wrote is read back and
# every entry replayed as an analysis of the same file with the same flags. A
# folder added later is covered with no edit here, for the same reason step 2
# needs none. It is read after step 3 has *built* and not merely configured,
# because a generated header must exist before anything that includes it can be
# analysed.
#
# Only files under <folder>/src/ and <folder>/tests/ are looked at, which is what
# leaves out wayland-scanner's generated protocol code: that lives in the build
# tree and is compiled -w on purpose, because it is not ours to keep clean.
#
# The checker set is the default one, with no -Xanalyzer enables (ADR-0042).
# Nothing in this tree needed more than the default to be caught.
#
# A finding the analyser has wrong is suppressed AT THE SITE, never here and
# never globally, and never by bending correct code into a shape that quiets it:
#
#         // <one line saying why the analyser is wrong here>
#         #ifndef __clang_analyzer__
#                 ...
#         #endif
#
# __clang_analyzer__ is defined only while analysing, so the real code is still
# the code that compiles. The spelling is greppable, which is the whole
# requirement — `grep -rn __clang_analyzer__` lists every suppression in the
# engine — and the reason sits on the line above, where the next reader is
# already looking.

# Replays one compile database entry as an analysis. out_text comes back empty
# when the file is clean, and holding the analyser's report when it is not.
#
# THREE THINGS ABOUT THIS INVOCATION, ALL THREE LEARNED BY BEING BITTEN:
#
#   - clang --analyze EXITS ZERO WITH FINDINGS IN HAND, and -Werror does not
#     change that, because an analyser finding is not a compile warning. A step
#     that read the exit code would pass forever while reporting nothing, which
#     is precisely the failure card 002 and step 6b exist to prevent. So the
#     output is what is judged, and the code is read only to catch clang failing
#     to launch at all.
#   - -analyzer-output=text is what stops a .plist being dropped beside every
#     source analysed. It is an output format, not a checker, so it is not the
#     kind of -Xanalyzer flag ADR-0042 ruled out.
#   - -o and -c produce an object, -MD/-MT/-MF a dependency file; none belongs in
#     an analysis. They are dropped, and dropping them is why
#     -Wno-unused-command-line-argument is needed: the entry carries -Werror, and
#     a flag that only had meaning for codegen or linking becomes an unused
#     argument once there is no object to make — an error, on a clean file.
function(analyse_entry command out_code out_text)
    separate_arguments(argv NATIVE_COMMAND "${command}")

    set(kept "")
    set(drop_next OFF)
    foreach(arg IN LISTS argv)
        if(drop_next)
            set(drop_next OFF)
        elseif(arg STREQUAL "-o" OR arg STREQUAL "-MT" OR arg STREQUAL "-MF")
            set(drop_next ON)
        elseif(arg STREQUAL "-c" OR arg STREQUAL "-MD" OR arg STREQUAL "-MMD")
        else()
            list(APPEND kept "${arg}")
        endif()
    endforeach()

    run_capture(code text ${kept}
        --analyze -Xanalyzer -analyzer-output=text
        -Wno-unused-command-line-argument)
    set(${out_code} "${code}" PARENT_SCOPE)
    set(${out_text} "${text}" PARENT_SCOPE)
endfunction()

set(step "analyser")

set(database "${checkdir}/root/compile_commands.json")
if(NOT EXISTS "${database}")
    step_fail("${step}" "step 3 left no compile database at ${database}")
endif()
file(READ "${database}" db)

string(JSON entry_count LENGTH "${db}")
if(entry_count EQUAL 0)
    step_fail("${step}" "the compile database is empty")
endif()

# Findings accumulate into a string and not a list, because the analyser quotes
# the offending line back and C is full of semicolons.
set(findings "")
set(analysed 0)
math(EXPR last "${entry_count} - 1")

foreach(i RANGE 0 ${last})
    string(JSON entry_file GET "${db}" ${i} file)
    string(JSON entry_command GET "${db}" ${i} command)

    # Matched by prefix rather than by regex: a repository path is free to
    # contain characters a regex would read as syntax.
    set(ours OFF)
    foreach(folder IN LISTS folders)
        foreach(sub src tests)
            string(FIND "${entry_file}" "${root}/${folder}/${sub}/" at)
            if(at EQUAL 0)
                set(ours ON)
            endif()
        endforeach()
    endforeach()
    if(NOT ours)
        continue()
    endif()

    file(RELATIVE_PATH shown "${root}" "${entry_file}")
    analyse_entry("${entry_command}" code text)
    if(NOT code MATCHES "^[0-9]+$")
        step_fail("${step}" "clang could not be run on ${shown}: ${code}")
    endif()
    if(NOT text STREQUAL "")
        string(APPEND findings "${text}")
    endif()
    math(EXPR analysed "${analysed} + 1")
endforeach()

# No folder source in the database means the filter above stopped matching — a
# renamed layout, or a database from somewhere else. Silently analysing nothing
# is the one outcome this step must never report as a pass.
if(analysed EQUAL 0)
    step_fail("${step}"
        "the database holds no <folder>/src/ or <folder>/tests/ file of ours")
endif()

if(NOT findings STREQUAL "")
    step_fail("${step}" "${findings}")
endif()
step_ok("${step} (${analysed} files)")

# ------------------------------------------- 7b analyser reports a finding

# Step 7 passing proves nothing on its own — an analyser that reported nothing
# would pass it too, and so would a filter that quietly matched no files. So a
# leak the default checker set is certain to see is put in front of it and the
# finding is required to come back, named: unix.Malloc and not merely some
# diagnostic, so that a step which started failing for an unrelated reason
# cannot pass as this one.
#
# Then the same leak is wrapped in the suppression documented above and the file
# is required to come back clean. That is the second half of what this step
# proves: the spelling this script tells people to use actually silences a
# finding, so nobody discovers otherwise while trying to land a card. It stands
# in for "removing the leak passes" and proves one thing more.
#
# Only configured, never built: the database is written at generate time, and
# the leak includes nothing that has to be generated first.
set(step "analyser reports a finding")

file(MAKE_DIRECTORY "${checkdir}/analyser/include/analyser")
file(MAKE_DIRECTORY "${checkdir}/analyser/src")
file(WRITE "${checkdir}/analyser/CMakeLists.txt"
"cmake_minimum_required(VERSION 3.28)
project(voe_analyser C)
include(${root}/cmake/voe.cmake)
voe_module(analyser)
")

# Written twice, to the same path, so that the two runs differ in the source and
# in nothing else — not in flags, not in the database they are replayed from.
set(leak_body
"#include <stdlib.h>

int voe_analyser_leak(void)
{
        int *value = malloc(sizeof *value);

        *value = 7;
        return *value;
}
")
set(leak_suppressed
"#include <stdlib.h>

int voe_analyser_leak(void)
{
        // Not a real leak: this file exists so check.cmake step 7b can prove
        // both that a finding is reported and that this suppression silences it.
        #ifndef __clang_analyzer__
        int *value = malloc(sizeof *value);

        *value = 7;
        return *value;
        #else
        return 7;
        #endif
}
")

file(WRITE "${checkdir}/analyser/src/leak.c" "${leak_body}")

run_capture(code text "${CMAKE_COMMAND}"
    -S "${checkdir}/analyser" -B "${checkdir}/analyser-build" ${common}
    -DCMAKE_BUILD_TYPE=Debug)
if(NOT code MATCHES "^[0-9]+$" OR NOT code EQUAL 0)
    step_fail("${step}" "the scratch folder did not configure:\n${text}")
endif()

set(leak_database "${checkdir}/analyser-build/compile_commands.json")
if(NOT EXISTS "${leak_database}")
    step_fail("${step}" "the scratch folder left no compile database")
endif()
file(READ "${leak_database}" leak_db)

# The scratch folder is one source file and no tests, so entry 0 is the leak.
# Said out loud rather than assumed, so that a second file appearing here fails
# with the reason instead of analysing the wrong entry and looking clean.
string(JSON leak_entries LENGTH "${leak_db}")
if(NOT leak_entries EQUAL 1)
    step_fail("${step}"
        "the scratch database holds ${leak_entries} entries, expected 1")
endif()
string(JSON leak_command GET "${leak_db}" 0 command)

analyse_entry("${leak_command}" code text)
if(NOT code MATCHES "^[0-9]+$")
    step_fail("${step}" "clang could not be run on the leak: ${code}")
endif()
if(NOT text MATCHES "unix\\.Malloc")
    step_fail("${step}"
        "a deliberate leak was not reported as unix.Malloc:\n${text}")
endif()

file(WRITE "${checkdir}/analyser/src/leak.c" "${leak_suppressed}")
analyse_entry("${leak_command}" code text)
if(NOT code MATCHES "^[0-9]+$")
    step_fail("${step}" "clang could not be run on the suppressed leak: ${code}")
endif()
if(NOT text STREQUAL "")
    step_fail("${step}"
        "the documented suppression did not silence the finding:\n${text}")
endif()
step_ok("${step}")
