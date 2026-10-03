# analyser.cmake — steps 7 and 7b of check.cmake: clang's static analyser.
#
# 7 replays every entry of the root build's compile database as an analysis and
# requires zero findings; 7b proves a planted leak is reported and that the
# documented suppression silences it. Defines analyse_entry for both.
#
# Included by check.cmake after tests.cmake, the last part, and runs in that
# file's scope. Reads root, checkdir, common, folders, the report.cmake functions
# and ${checkdir}/root/compile_commands.json, which build.cmake left. Leaves
# nothing a later part reads; the closing block of check.cmake follows it.

# ------------------------------------------------------------- 7 analyser

# The step rule 8 (ADR-0168) has always promised and ADR-0042 decided: clang's
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

