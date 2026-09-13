# check.cmake — the verification. There is no CI, so this script is it.
#
# Run from the repository root:
#
#     cmake -P check.cmake
#
# It prints one line per step, stops at the first failure, and exits non-zero.
# `skip` is a pass: a step whose tool is absent says so and the run continues.
# Only step 4a is expected to skip in normal use. `warn` is a pass as well, and
# the only one there is today is repeated as the last thing the script prints —
# see the closing block at the foot of this file (ADR-0117).
#
# ONE KNOB EXISTS AND IS NEVER SET BY HAND. VOE_CHECK_FAKE_SLANGC_VERSION makes
# step 1 behave as though slangc had reported that version, so the shader
# compiler's floor can be watched to fire on a machine that is above it. It is the
# counterpart of VOE_CHECK_FAKE_CLANG_VERSION in cmake/voe.cmake and exists for the
# same reason: a warning nobody has seen fire is a warning nobody has tested. It is
# read in script mode, so it goes before -P and not after:
#
#     cmake -DVOE_CHECK_FAKE_SLANGC_VERSION=2024.17 -P check.cmake
#
# Most steps shell out to cmake or ctest, so a Ninja and a clang must be on
# PATH. Step 5 reads sources only and needs no toolchain; step 7 runs clang
# itself, replaying the compile database step 3 wrote.
#
# Two conventions the steps below rely on:
#
#   - A folder is discovered, never listed: every top-level directory holding a
#     CMakeLists.txt is a folder. New folders are picked up with no edit here.
#     The root CMakeLists.txt does list them, on purpose, so step 1b compares the
#     two and fails when they disagree — a folder in the tree and not in the root
#     build used to pass every step below while never being built or run.
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

# Prints WARN, then the detail indented exactly as step_fail indents it, and
# returns. It is for a condition that makes a passing run less trustworthy without
# making it wrong, and today there is exactly one: the shader compiler being older
# than any version this engine has been verified under.
function(step_warn name detail)
    message("WARN  ${name}")
    if(NOT detail STREQUAL "")
        string(REPLACE "\n" "\n      " detail "${detail}")
        message("      ${detail}")
    endif()
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

# THE slangc THIS STEP RUNS IS THE ONE THE BUILD RUNS, AND THAT IS THE POINT OF
# THE STEP. Every .slang file is compiled by slangc on the machine doing the build
# and the SPIR-V is #embedded, so the Windows binary and the Linux binary do not
# hold the same shader bytes — they hold the output of two different installations
# of this tool. A version proved here about some *other* slangc would be a right
# answer to a question nobody asked, so this step resolves the compiler the way
# cmake/voe.cmake resolves it and under the same variable name: an override set on
# this script's command line is honoured here exactly as the build honours one.
find_program(VOE_SLANGC slangc)
if(NOT VOE_SLANGC)
    step_fail("${step}" "slangc was not found. It is a source-transforming tool, so it is installed by the programmer; the Vulkan SDK is the easiest way to get one.")
endif()
# What PATH alone would have found, so the two can be compared below.
find_program(slangc_on_path slangc)

run_capture(code text "${VOE_SLANGC}" -v)
if(NOT code MATCHES "^[0-9]+$")
    step_fail("${step}" "slangc could not be run: ${code}")
endif()
if(NOT code EQUAL 0)
    step_fail("${step}" "slangc -v exited ${code}: ${text}")
endif()
# Some builds print this banner on stdout and some on stderr; run_capture merges
# the two, so the parse reads it either way. The `-1-g84792eb15` tail some builds
# carry is kept for printing and left out of the comparison.
if(NOT text MATCHES "([0-9]+\\.[0-9]+(\\.[0-9]+)?)(-[0-9]+-g[0-9a-fA-F]+)?")
    step_fail("${step}" "could not parse a version out of: ${text}")
endif()
set(slangc_version "${CMAKE_MATCH_1}")
set(slangc_reported "${CMAKE_MATCH_1}${CMAKE_MATCH_3}")
# THE FLOOR IS THE NEWER OF THE TWO MACHINES' slangc, WHICH IS DELIBERATE AND IS
# NOT WHAT A FLOOR USUALLY IS. 2026.13.1 is the only version any shader in this
# engine has ever been checked on. The other machine's 2024.17 is the compiler
# that built the binary bug 002 is about and is not known good, so flooring at it
# would write a suspect configuration into the build as a supported one.
# Compared as a version and not as a string, because 2026.9 is *greater* than
# 2026.13 as text and *less* as a version — a comparison this floor will meet.
#
# BEING BELOW THE FLOOR WARNS AND DOES NOT FAIL (ADR-0117), AND THAT IS NOT A
# SOFTENING. The one machine below the floor is the only machine that tests
# Windows, so failing here would gate the testing rather than the risk: the
# Windows half of every card would go unverified to keep a number tidy. The
# warning is what is spent instead, and it is printed twice — see below and the
# closing block at the foot of this file. There is deliberately no flag to silence
# it; ADR-0117 considered one and chose an unmissable warning instead.
set(slangc_floor 2026.13.1)
# The fake-version hook. Test scaffolding, documented at the top of this file and
# never set by hand: the whole step then behaves as though slangc had said this,
# the tools line included, which is the point — a probe should print what the
# machine below the floor prints.
if(DEFINED VOE_CHECK_FAKE_SLANGC_VERSION)
    set(slangc_version "${VOE_CHECK_FAKE_SLANGC_VERSION}")
    set(slangc_reported "${VOE_CHECK_FAKE_SLANGC_VERSION}")
endif()
# Read by step 1 below and again by the closing block; empty means no warning.
set(slangc_warning "")
if(slangc_version VERSION_LESS slangc_floor)
    set(slangc_warning "slangc ${slangc_reported} is below ${slangc_floor}, which is the oldest version any shader in this engine has been verified under.\nEvery .slang file in this build was compiled by this slangc and its SPIR-V is embedded in the binary, so a rendering fault seen on this machine is suspect until the compiler is current.\nThe check has not failed and will not fail on this (ADR-0117).")
endif()

set(tools "clang ${clang_major}, cmake ${cmake_version}, slangc ${slangc_reported}")
# The path is printed only when it is not the one PATH alone would have found:
# that is the case where which file ran is the whole of the answer.
if(NOT "${VOE_SLANGC}" STREQUAL "${slangc_on_path}")
    string(APPEND tools " at ${VOE_SLANGC}")
endif()

# wayland-scanner is a source-transforming tool on the same footing as slangc and
# as the Windows SDK: the programmer installs it, the build does not fetch it.
# platform's only Linux backend is Wayland, so on Linux it is required and on
# Windows it is not looked for.
#
# ITS VERSION IS DOCUMENTATION AND HAS NO FLOOR, WHERE slangc'S IS A DEPENDENCY.
# It generates C that this build then compiles, so none of its own bytes reach the
# binary and two machines on two versions of it produce the same program. So an
# output with no version in it is printed as it stands rather than failing the
# step — the opposite of slangc, which fails when it will not say what it is.
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    run_capture(code text wayland-scanner --version)
    if(NOT code MATCHES "^[0-9]+$")
        step_fail("${step}" "wayland-scanner could not be run: ${code}")
    endif()
    if(NOT code EQUAL 0)
        step_fail("${step}" "wayland-scanner --version exited ${code}: ${text}")
    endif()
    if(text MATCHES "([0-9]+\\.[0-9]+(\\.[0-9]+)?)")
        string(APPEND tools ", wayland-scanner ${CMAKE_MATCH_1}")
    else()
        string(STRIP "${text}" said)
        string(APPEND tools ", wayland-scanner (said: ${said})")
    endif()
endif()

step_ok("${step} (${tools})")
if(NOT slangc_warning STREQUAL "")
    step_warn("${step}" "${slangc_warning}")
endif()

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

# -------------------------------------------- 1b folders in the root build
#
# THE GLOB ABOVE AND THE ROOT CMakeLists.txt ARE TWO LISTS AND THIS IS WHERE THEY
# ARE MADE TO AGREE. Everything below discovers folders; the root build lists them
# by hand. Until this step existed nothing compared the two, so a folder left out
# of the root file passed its standalone configure — that step reads the folder's
# own CMakeLists.txt — and was then simply absent from the configure, the build
# and ctest. Every step printed ok and the only number that would have said
# otherwise was a test count nobody reads. That is the worst shape a failure can
# have: not a wrong answer, but a right answer to a question nobody asked.
#
# THE ROOT'S LIST IS DELIBERATE AND IS NOT THE THING TO FIX. It says what the
# engine *is*, and a glob there would let a scratch folder join the build without
# anyone saying so — the same bug pointed the other way. Two lists is the design;
# this step is what makes keeping them in step something the script does rather
# than something a person remembers.
#
# IT RUNS HERE BECAUSE IT NEEDS THE GLOB AND NOTHING ELSE. It reads one file and
# compares two lists, so it costs milliseconds and comes before the thirteen
# configures of step 2 — a folder that is not in the build should be named in a
# second, not after a full build has succeeded around it.
#
# THE PARSE IS A REGEX AND IT REFUSES ANYTHING IT CANNOT READ PLAINLY. A root file
# simple enough for one line per folder is the thing being relied on, so the four
# guards below fail loudly on the constructs that would make a regex lie —
# bracket comments, computed names, control flow — rather than quietly reporting
# a folder as listed when it is not. If this ever fires on a root file somebody
# meant, the fix is to make the root file simple again, not to make this cleverer.
set(step "folders in the root build")

file(READ "${root}/CMakeLists.txt" root_text)

# A bracket comment can hide an add_subdirectory on a line that does not itself
# start with a #, which is precisely a folder that looks listed and is not.
if(root_text MATCHES "#\\[")
    step_fail("${step}"
        "the root CMakeLists.txt holds a bracket comment, and this step cannot tell what one hides.\nRemove it, or read the note above this step in check.cmake.")
endif()
# A folder listed inside an if() or a foreach() is not listed unconditionally, and
# this step would report it as listed whatever the condition said.
if(root_text MATCHES "(^|\n)[ \t]*(if|foreach|while|macro|function)[ \t]*\\(")
    step_fail("${step}"
        "the root CMakeLists.txt holds control flow, and a folder inside it is not unconditionally in the build.\nThe root file is meant to be one add_subdirectory per line; see the note above this step in check.cmake.")
endif()

string(REPLACE "\n" ";" root_lines "${root_text}")
set(listed "")
foreach(line IN LISTS root_lines)
    string(STRIP "${line}" line)
    # A commented-out line is not in the build, which is the case this step
    # exists to catch, so it is skipped rather than matched.
    if(line MATCHES "^#")
        continue()
    endif()
    if(NOT line MATCHES "add_subdirectory")
        continue()
    endif()
    if(NOT line MATCHES "add_subdirectory[ \t]*\\(([^)]*)\\)")
        step_fail("${step}"
            "this line names add_subdirectory and this step cannot read it:\n${line}")
    endif()
    string(STRIP "${CMAKE_MATCH_1}" name)
    # A plain directory name and nothing else: no variable, no quotes, no path
    # and no second argument. Any of those and the name below would be wrong.
    if(NOT name MATCHES "^[A-Za-z0-9_.+-]+$")
        step_fail("${step}"
            "add_subdirectory(${name}) is not a plain folder name, so this step cannot say which folder it adds.\nSee the note above this step in check.cmake.")
    endif()
    list(APPEND listed "${name}")
endforeach()
list(SORT listed)

foreach(folder IN LISTS folders)
    if(NOT folder IN_LIST listed)
        step_fail("${step}"
            "${folder}/ holds a CMakeLists.txt but the root CMakeLists.txt never adds it.\nIts tests are not being built and not being run, and every step below this one would have said ok without them.\nAdd `add_subdirectory(${folder})` to the root CMakeLists.txt.")
    endif()
endforeach()

foreach(name IN LISTS listed)
    if(NOT name IN_LIST folders)
        step_fail("${step}"
            "the root CMakeLists.txt adds ${name}, and there is no ${name}/ holding a CMakeLists.txt.\nEither the folder was removed and its line was not, or the name is misspelt.")
    endif()
endforeach()

list(LENGTH folders folder_total)
list(JOIN folders " " folder_names)
step_ok("${step} (${folder_total}: ${folder_names})")

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
    set(test_total "${CMAKE_MATCH_1}")

    # WHICH FOLDERS THOSE TESTS CAME FROM, BECAUSE A TOTAL ON ITS OWN CANNOT BE
    # READ. When `ui` arrived, thirty-six became thirty-seven and nothing in the
    # output told the two runs apart; a reader who can see `ui 1` can see a folder
    # that contributed nothing, and step 1b catches the case a reader cannot see.
    #
    # THE NAMES ARE ALREADY <folder>/<test> AND NOTHING HERE INVENTS A CONVENTION.
    # cmake/voe.cmake registers every test that way so that `ctest -R math` runs
    # one folder's, and this reads the same names back out of the run it just did
    # rather than asking ctest a second time.
    #
    # A FOLDER WITH NO TESTS SHOWS A NOUGHT AND IS NOT A FAILURE. `dev` is a
    # program and has none; asserting that every folder has tests would need a
    # list of exemptions, which is the listed-not-discovered mistake in a second
    # place. A visible nought is what a person reads.
    foreach(folder IN LISTS folders)
        set(tally_${folder} 0)
    endforeach()
    set(unfolded 0)
    string(REGEX MATCHALL "Test +#[0-9]+: +[^ \t\r\n]+" ran "${text}")
    foreach(hit IN LISTS ran)
        string(REGEX REPLACE "^Test +#[0-9]+: +" "" name "${hit}")
        set(owner "")
        if(name MATCHES "^([^/]+)/")
            set(owner "${CMAKE_MATCH_1}")
        endif()
        if(owner STREQUAL "" OR NOT owner IN_LIST folders)
            math(EXPR unfolded "${unfolded} + 1")
        else()
            math(EXPR tally_${owner} "${tally_${owner}} + 1")
        endif()
    endforeach()

    set(breakdown "")
    set(counted 0)
    foreach(folder IN LISTS folders)
        list(APPEND breakdown "${folder} ${tally_${folder}}")
        math(EXPR counted "${counted} + ${tally_${folder}}")
    endforeach()
    list(JOIN breakdown ", " breakdown)

    # THE BREAKDOWN HAS TO ADD UP TO THE TOTAL ctest REPORTED, and this fails
    # rather than printing a breakdown that quietly covers fewer tests than ran —
    # which would be this card's own bug wearing a different hat. It can only
    # happen if the naming convention in cmake/voe.cmake changed or if ctest's
    # progress lines changed shape.
    if(NOT counted EQUAL test_total)
        step_fail("${step}"
            "ctest ran ${test_total} tests and this step could only account for ${counted} of them, ${unfolded} of which were not named <folder>/<test>.\nEither cmake/voe.cmake stopped naming tests that way or ctest's output changed, and the per-folder line would have understated what ran.")
    endif()

    step_ok("${step} (${test_total} passed — ${breakdown})")
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

# ---------------------------------------------------- 6c descriptions off

# cmake/voe.cmake turns the field descriptions on for every target this root
# builds, because nothing built here ships (ADR-0145). So every step above builds
# them in, and this is the only thing in the repository that compiles the
# `#else return NULL` branch of scene/src/transform_system.c and
# identity_system.c, and the only thing that runs the `!BUILD_DESCRIBES` half of
# scene/tests/transform.c and identity.c. Without it that branch rots unseen, and
# the first to find out is a game developer whose own tree leaves them off.
#
# scene alone, configured standalone, and only its library and its own tests
# built: those are the files the switch changes, and building the whole tree a
# second time would prove nothing more about them.
set(step "descriptions off")

run_capture(code text "${CMAKE_COMMAND}"
    -S "${root}/scene" -B "${checkdir}/descriptions-off" ${common}
    -DCMAKE_BUILD_TYPE=Debug -DVOE_BASE_DESCRIPTIONS=OFF)
if(NOT code MATCHES "^[0-9]+$" OR NOT code EQUAL 0)
    step_fail("${step}" "scene did not configure with descriptions off:\n${text}")
endif()

# The test targets are named the way cmake/voe.cmake names them, from the files
# that are there, so a scene test added later is built here with no edit.
file(GLOB off_tests "${root}/scene/tests/*.c")
set(off_targets voe_scene)
foreach(test_source IN LISTS off_tests)
    cmake_path(GET test_source STEM test_name)
    list(APPEND off_targets voe_test_scene_${test_name})
endforeach()

run_capture(code text "${CMAKE_COMMAND}"
    --build "${checkdir}/descriptions-off" --target ${off_targets})
if(NOT code MATCHES "^[0-9]+$" OR NOT code EQUAL 0)
    step_fail("${step}" "scene did not build with descriptions off:\n${text}")
endif()

run_capture(code text "${CMAKE_CTEST_COMMAND}"
    --test-dir "${checkdir}/descriptions-off" --output-on-failure
    -R "^scene/")
if(NOT code MATCHES "^[0-9]+$")
    step_fail("${step}" "ctest could not be run: ${code}")
endif()
if(text MATCHES "No tests were found")
    step_fail("${step}" "ctest found no scene tests to run with descriptions off:\n${text}")
endif()
if(NOT code EQUAL 0)
    step_fail("${step}" "${text}")
endif()
string(REGEX MATCH "out of ([0-9]+)" matched "${text}")
step_ok("${step} (scene, ${CMAKE_MATCH_1} passed)")

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

# --------------------------------------------------------------- closing
#
# THE WARNING IS REPEATED HERE ON PURPOSE AND IT IS NOT A DUPLICATE. Step 1 is the
# first of twenty-four lines and the steps after it take the better part of a
# minute, so a warning printed only where it is found is a warning read once and
# then scrolled away. The last line of a run is the one a person actually reads,
# and this block exists so that on the machine below the floor that line is the
# thing they need to know rather than an ok. It is the only thing after the final
# step, and nothing may be printed after it.
if(NOT slangc_warning STREQUAL "")
    step_warn("tools" "${slangc_warning}")
endif()
