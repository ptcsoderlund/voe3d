# voe_module() — the one function every folder's CMakeLists.txt calls.
#
# A folder is the unit CMake links, and its CMakeLists.txt is four lines. This
# file is what makes four lines enough: the compiler guards, the allowed
# dependency map, the static library target voe_<folder> and its alias
# voe::<folder>, the public include directory, the one flag set, a glob of src/
# so that adding a source file never needs a CMake edit, the same for tests/ so
# that adding a test never needs one either, and the same for dev/ (ADR-0036).
#
# It also knows one folder by name: platform. platform is the only folder that
# links something the operating system supplies and the only one that generates a
# source file, and the four-line rule leaves nowhere else to say so. That
# knowledge is a function of its own, voe_platform_backend(), kept apart from
# voe_module() so it is obvious how much of this file is general and how much is
# one folder's bill.
#
# Two things here are contracts with check.cmake, not free to reword:
#
#   - The guard messages below are searched for by substring (ADR-0005,
#     "Clang 18 or newer", ADR-0026, ADR-0022). Changing the text breaks the
#     verification silently, because a guard that fires with different wording
#     still fails configuration and the check still sees a non-zero exit.
#   - VOE_CHECK_FAKE_CLANG_VERSION exists only so check.cmake can exercise the
#     version guard on a machine whose clang is already new enough.
#
# The dependency map is the folder table in CLAUDE.md, as data. A DEPENDS entry
# outside a folder's row fails configuration; that is the point of having it.
# Adding an edge is an architecture change and belongs in a card, not here.

include_guard(GLOBAL)

# Testing is enabled in the scope that includes this file first, and that scope
# is the top level in both builds we support: the repository root when the whole
# engine is configured, and the folder itself when one folder is configured
# standalone. ctest therefore finds the tests either way, and no folder's
# CMakeLists.txt has to know that tests exist.
enable_testing()

# voe::testing is the check macros and nothing else — one header, nothing to
# build, nothing to link. It is deliberately not a folder: it has no
# CMakeLists.txt, so check.cmake's folder discovery never sees it and the
# dependency map never needs a row for it. Tests link it. A folder's src/ may
# not, and check.cmake enforces that rather than trusting it.
add_library(voe_testing INTERFACE)
add_library(voe::testing ALIAS voe_testing)
target_include_directories(voe_testing
    INTERFACE ${CMAKE_CURRENT_LIST_DIR}/../testing/include)

# The allowed dependency edges. A folder absent from this list has an empty row,
# so every DEPENDS on it is rejected — which is what makes an unknown folder
# name fail rather than quietly link.
function(voe_allowed_deps folder out_var)
    set(deps "")
    if(folder STREQUAL "base")
        set(deps "")
    elseif(folder STREQUAL "math")
        set(deps "")
    elseif(folder STREQUAL "ecs")
        set(deps base)
    elseif(folder STREQUAL "platform")
        set(deps base)
    elseif(folder STREQUAL "scene")
        set(deps ecs math base)
    elseif(folder STREQUAL "assets")
        set(deps platform math base)
    elseif(folder STREQUAL "render")
        set(deps platform math base)
    elseif(folder STREQUAL "3d")
        set(deps render scene ecs assets math base)
    elseif(folder STREQUAL "app")
        set(deps base math ecs scene platform assets render 3d)
    endif()
    set(${out_var} "${deps}" PARENT_SCOPE)
endfunction()

# The one flag set and the one language level, in one place, applied identically
# to a folder's library and to its test executables. A test compiled with looser
# flags than the code it tests is a test that lies.
function(voe_target_settings target)
    set_target_properties(${target} PROPERTIES
        C_STANDARD 23
        C_STANDARD_REQUIRED ON
        C_EXTENSIONS OFF)
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
endfunction()

# Run once per CMake run, before any target exists. Guarded by a global property
# rather than a variable so the flag survives across directory scopes.
function(voe_guards)
    get_property(already_run GLOBAL PROPERTY VOE_GUARDS_DONE)
    if(already_run)
        return()
    endif()
    set_property(GLOBAL PROPERTY VOE_GUARDS_DONE TRUE)

    if(NOT CMAKE_C_COMPILER_ID STREQUAL "Clang")
        message(FATAL_ERROR "VOE3D requires Clang (ADR-0005). Found: ${CMAKE_C_COMPILER_ID}")
    endif()

    # The fake-version hook. Test scaffolding for check.cmake; never set by hand.
    set(version "${CMAKE_C_COMPILER_VERSION}")
    if(DEFINED VOE_CHECK_FAKE_CLANG_VERSION)
        set(version "${VOE_CHECK_FAKE_CLANG_VERSION}")
    endif()
    if(version VERSION_LESS 18)
        message(FATAL_ERROR "VOE3D requires Clang 18 or newer (ADR-0005). Found: ${version}")
    endif()

    if(NOT CMAKE_C_COMPILER_FRONTEND_VARIANT STREQUAL "GNU")
        message(FATAL_ERROR "VOE3D requires the GNU-driver clang, not clang-cl (ADR-0026). Found frontend: ${CMAKE_C_COMPILER_FRONTEND_VARIANT}")
    endif()
endfunction()

# platform's bill, in one place. Two things no other folder needs:
#
#   - xdg-shell is a protocol description, not a library. wayland-scanner turns
#     the vendored XML into C at configure-and-build time, into the build tree.
#     Nothing generated is committed, and the generated files are compiled with
#     warnings off and reached through a SYSTEM include directory, because they
#     are wayland-scanner's code and not ours to keep clean.
#   - The Wayland client library and the Win32 libraries are linked, not built.
#     They are the programmer's to install — the same standing the Windows SDK
#     already has — which is why this fails configuration with a message rather
#     than trying to fetch anything.
#
# out_sources comes back holding the generated .c and .h. The .h is in the list
# on purpose: naming a generated header as a source is what makes it exist before
# anything that includes it is compiled.
function(voe_platform_backend folder_dir out_sources out_include)
    set(${out_sources} "" PARENT_SCOPE)
    set(${out_include} "" PARENT_SCOPE)

    if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
        return()
    endif()

    find_program(VOE_WAYLAND_SCANNER wayland-scanner)
    if(NOT VOE_WAYLAND_SCANNER)
        message(FATAL_ERROR "VOE3D requires wayland-scanner on Linux. Install the wayland development package.")
    endif()

    set(generated ${CMAKE_BINARY_DIR}/generated/platform)
    set(xml ${folder_dir}/protocol/xdg-shell.xml)
    file(MAKE_DIRECTORY ${generated})

    add_custom_command(
        OUTPUT ${generated}/xdg-shell-client-protocol.h
        COMMAND ${VOE_WAYLAND_SCANNER} client-header ${xml}
                ${generated}/xdg-shell-client-protocol.h
        DEPENDS ${xml}
        COMMENT "wayland-scanner: xdg-shell client header"
        VERBATIM)
    add_custom_command(
        OUTPUT ${generated}/xdg-shell-protocol.c
        COMMAND ${VOE_WAYLAND_SCANNER} private-code ${xml}
                ${generated}/xdg-shell-protocol.c
        DEPENDS ${xml}
        COMMENT "wayland-scanner: xdg-shell protocol code"
        VERBATIM)

    set_source_files_properties(${generated}/xdg-shell-protocol.c
        PROPERTIES COMPILE_OPTIONS "-w")

    set(${out_sources}
        ${generated}/xdg-shell-protocol.c
        ${generated}/xdg-shell-client-protocol.h
        PARENT_SCOPE)
    set(${out_include} ${generated} PARENT_SCOPE)
endfunction()

function(voe_platform_link target include_dir)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(WAYLAND REQUIRED IMPORTED_TARGET wayland-client)
        target_include_directories(${target} SYSTEM PRIVATE ${include_dir})
        target_link_libraries(${target} PRIVATE PkgConfig::WAYLAND)
    elseif(WIN32)
        target_link_libraries(${target} PRIVATE user32)
    else()
        message(FATAL_ERROR "VOE3D supports Windows and Linux desktop only. Found: ${CMAKE_SYSTEM_NAME}")
    endif()
endfunction()

# voe_module(<folder> [DEPENDS <folder>...])
#
# Inside a function CMAKE_CURRENT_LIST_DIR is the calling listfile's directory,
# so it is the folder's own directory here — which is why a sibling is reached
# as ../<dep> and why a standalone configure of one folder works unchanged.
function(voe_module folder)
    cmake_parse_arguments(arg "" "" "DEPENDS" ${ARGN})
    if(arg_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "voe_module(${folder}): unexpected argument(s): ${arg_UNPARSED_ARGUMENTS}")
    endif()

    voe_guards()

    voe_allowed_deps("${folder}" allowed)
    foreach(dep IN LISTS arg_DEPENDS)
        if(NOT dep IN_LIST allowed)
            message(FATAL_ERROR "voe_module(${folder}): ${dep} is not an allowed dependency (ADR-0022)")
        endif()
    endforeach()

    foreach(dep IN LISTS arg_DEPENDS)
        if(NOT TARGET voe_${dep})
            add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../${dep} ${CMAKE_BINARY_DIR}/${dep})
        endif()
    endforeach()

    file(GLOB sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/src/*.c)

    # A source whose name ends in _wayland or _win32 is one platform's, and the
    # other platform never compiles it. The alternative is #ifdef'ing a whole
    # file out, which leaves an empty translation unit — not valid ISO C, and
    # -Wpedantic says so. This is a general rule, not platform's: any folder that
    # ever grows two backends gets it for free.
    foreach(source IN LISTS sources)
        set(drop OFF)
        if(source MATCHES "_wayland\\.c$" AND NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
            set(drop ON)
        elseif(source MATCHES "_win32\\.c$" AND NOT WIN32)
            set(drop ON)
        endif()
        if(drop)
            list(REMOVE_ITEM sources ${source})
        endif()
    endforeach()

    if(folder STREQUAL "platform")
        voe_platform_backend(${CMAKE_CURRENT_LIST_DIR} generated generated_dir)
        list(APPEND sources ${generated})
    endif()

    add_library(voe_${folder} STATIC ${sources})
    add_library(voe::${folder} ALIAS voe_${folder})
    target_include_directories(voe_${folder} PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
    foreach(dep IN LISTS arg_DEPENDS)
        target_link_libraries(voe_${folder} PUBLIC voe::${dep})
    endforeach()

    voe_target_settings(voe_${folder})

    if(folder STREQUAL "platform")
        voe_platform_link(voe_${folder} "${generated_dir}")
    endif()

    # Tests. One executable per file in tests/, named <folder>/<file> so that
    # `ctest -R math` runs one folder's tests. A folder with no tests/ globs
    # nothing and is silent, not an error.
    file(GLOB test_sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/tests/*.c)
    foreach(test_source IN LISTS test_sources)
        cmake_path(GET test_source STEM test_name)
        set(test_target voe_test_${folder}_${test_name})
        add_executable(${test_target} ${test_source})
        target_link_libraries(${test_target} PRIVATE voe_${folder} voe::testing)
        voe_target_settings(${test_target})
        add_test(NAME ${folder}/${test_name} COMMAND ${test_target})
    endforeach()

    # Dev programs (ADR-0036). One file in dev/, one main(), one executable
    # voe_<folder>_dev_<file>, built by an ordinary build and never registered
    # with ctest — a program that wants a desktop and a person is not a test, and
    # putting it in ctest would hang the check script waiting for someone to
    # close a window. It links the folder's library and not voe::testing: it is
    # something to look at, not something that reports.
    file(GLOB dev_sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/dev/*.c)
    foreach(dev_source IN LISTS dev_sources)
        cmake_path(GET dev_source STEM dev_name)
        set(dev_target voe_${folder}_dev_${dev_name})
        add_executable(${dev_target} ${dev_source})
        target_link_libraries(${dev_target} PRIVATE voe_${folder})
        voe_target_settings(${dev_target})
    endforeach()
endfunction()
