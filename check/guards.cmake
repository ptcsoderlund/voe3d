# guards.cmake — steps 4a to 4c of check.cmake: the configure guards fire.
#
# 4a configures with gcc, 4b with a faked clang 18, 4c with a dependency edge
# cmake/voe.cmake does not allow, and each requires its own guard's message.
#
# Included by check.cmake after build.cmake, in order, and runs in that file's
# scope. Reads root, checkdir, generator, common and the report.cmake functions.
# Leaves nothing a later part reads.

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

