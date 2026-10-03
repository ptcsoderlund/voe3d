# build.cmake — steps 2, 3 and 3b of check.cmake: each folder standalone, then
# the root in Debug, then the root in Release.
#
# Step 2 configures every folder on its own; step 3 configures and builds the
# whole tree in Debug; step 3b configures and builds it again with the `release`
# preset's settings (Ninja, clang, Release), every target, tests included, and
# runs no tests.
#
# Why Release is built: some faults exist only at -O3 -DNDEBUG (052 bug 04, a
# debug assert's expression rejected in Release), and Debug cannot see them.
# Decision 0340 catches them before the human tests a feature, not at Ship.
# Why the whole tree once per feature: this file runs in the whole suite, which
# runs once after a feature's last card; the per-folder check stays Debug, so a
# card is never built twice.
# Why ${checkdir}/release and not build/release: the check owns build/check and
# wipes it on every run, so it never touches the human's own preset build.
#
# Included by check.cmake after folders.cmake, in order, and runs in that file's
# scope. Reads root, checkdir, common, folders and the report.cmake functions.
# Leaves the root build at ${checkdir}/root and its compile_commands.json, which
# tests.cmake runs ctest in and analyser.cmake replays.

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

# ------------------------------------------------------ 3b root release build

set(step "3b root release build")
run_capture(code text "${CMAKE_COMMAND}"
    -S "${root}" -B "${checkdir}/release" ${common} -DCMAKE_BUILD_TYPE=Release)
if(NOT code MATCHES "^[0-9]+$" OR NOT code EQUAL 0)
    step_fail("${step}" "${text}")
endif()
run_capture(code text "${CMAKE_COMMAND}" --build "${checkdir}/release")
if(NOT code MATCHES "^[0-9]+$" OR NOT code EQUAL 0)
    step_fail("${step}" "${text}")
endif()
step_ok("${step}")
