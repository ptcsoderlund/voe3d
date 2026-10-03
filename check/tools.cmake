# tools.cmake — step 1 of check.cmake: the tools, and the slangc floor.
#
# Proves clang, cmake, slangc and on Linux wayland-scanner run, and prints their
# versions on one line. slangc below its floor warns and does not fail (ADR-0117).
#
# Included by check.cmake after report.cmake, in order, and runs in that file's
# scope. Reads the report.cmake functions, and VOE_SLANGC and
# VOE_CHECK_FAKE_SLANGC_VERSION when set on the command line. Leaves
# slangc_warning, empty when there is no warning, which the closing block of
# check.cmake prints again as the run's last line.

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
