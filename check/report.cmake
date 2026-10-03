# report.cmake — how every step of check.cmake reports and runs a command.
#
# Holds no step: the functions step_ok, step_skip, step_warn and step_fail, which
# print one line per step and stop the script on a failure, and run_capture, which
# runs a command and hands back its exit code and merged output.
#
# Included first by check.cmake, before every other part, and runs in that file's
# scope. Reads nothing. Leaves the five functions for every part after it and for
# the closing block of check.cmake.

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
