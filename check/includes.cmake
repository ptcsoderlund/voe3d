# includes.cmake — step 5 of check.cmake: what each folder's sources may include.
#
# Reads sources only, no toolchain: no OS or window-system header outside
# platform, testing/ only from tests/, and no folder named that is not DEPENDS.
#
# Included by check.cmake after guards.cmake, in order, and runs in that file's
# scope. Reads root, folders and the report.cmake functions. Leaves nothing a
# later part reads.

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

