# build.cmake — steps 2 and 3 of check.cmake: each folder standalone, then the root.
#
# Step 2 configures every folder on its own; step 3 configures and builds the
# whole tree in Debug.
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
