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
# THE STEPS LIVE IN check/, one file per step group, and this file only sets up,
# includes them and closes. Each part runs in this file's scope, in the order it
# is included, so a variable one part sets (the folder list, slangc_warning) is
# read by the parts after it and by the closing block. Besides the Debug build the
# steps run, the whole tree is also built in Release (step 3b, ADR-0340), so a
# fault that exists only at -O3 -DNDEBUG is caught before anyone ships.
#
# Two conventions the steps rely on:
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

include("${root}/check/report.cmake")

file(REMOVE_RECURSE "${checkdir}")
file(MAKE_DIRECTORY "${checkdir}")

include("${root}/check/tools.cmake")
include("${root}/check/folders.cmake")
include("${root}/check/build.cmake")
include("${root}/check/guards.cmake")
include("${root}/check/includes.cmake")
include("${root}/check/tests.cmake")
include("${root}/check/analyser.cmake")

# --------------------------------------------------------------- closing
#
# THE WARNING IS REPEATED HERE ON PURPOSE AND IT IS NOT A DUPLICATE. Step 1 is the
# first of a run's lines and the steps after it take the better part of a minute,
# so a warning printed only where it is found is a warning read once and
# then scrolled away. The last line of a run is the one a person actually reads,
# and this block exists so that on the machine below the floor that line is the
# thing they need to know rather than an ok. It is the only thing after the final
# step, and nothing may be printed after it.
if(NOT slangc_warning STREQUAL "")
    step_warn("tools" "${slangc_warning}")
endif()
