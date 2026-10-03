# tests.cmake — steps 6, 6b and 6c of check.cmake: the tests.
#
# 6 runs ctest on the root build and counts the tests per folder; 6b proves the
# harness reports a failure; 6c builds and tests scene with descriptions off.
#
# Included by check.cmake after includes.cmake, in order, and runs in that file's
# scope. Reads root, checkdir, common, folders, the report.cmake functions and the
# root build build.cmake left at ${checkdir}/root. Leaves nothing a later part
# reads.

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

