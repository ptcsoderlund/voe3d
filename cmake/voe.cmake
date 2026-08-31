# voe_module() — the one function every folder's CMakeLists.txt calls.
#
# A folder is the unit CMake links, and its CMakeLists.txt is four lines. This
# file is what makes four lines enough: the compiler guards, the allowed
# dependency map, the static library target voe_<folder> and its alias
# voe::<folder>, the public include directory, the one flag set, a glob of src/
# so that adding a source file never needs a CMake edit, and the same for tests/
# so that adding a test never needs one either.
#
# voe_executable() is the same thing for a folder that produces a program rather
# than a library. The two share everything up to the add_library/add_executable
# line, which is voe_folder_sources() below; what differs is that a library has
# an alias, a public include directory and tests, and a program has none of the
# three because nothing links a program.
#
# Both also drop a compile_commands.json beside the folder's CMakeLists.txt.
# CMake writes one database per build tree, inside that build tree, which is the
# one place an editor opening a single folder will not look. The copy is what
# lets clangd work in a folder without being told where the build is. It is a
# build step rather than a configure step because the database does not exist
# until the generate phase has finished, and a copy rather than a symlink
# because Windows does not hand those out without being asked nicely.
#
# It also knows two folders by name, because the four-line rule leaves nowhere
# else to say what they need. platform links something the operating system
# supplies; render runs slangc over its shaders. Each is a function of its own —
# voe_platform_backend() and voe_render_shaders() — kept apart from voe_module()
# so it is obvious how much of this file is general and how much is one folder's
# bill.
#
# Neither is a mechanism for anyone else. The shader rule in particular is
# deliberately not generalised: render is the only folder with a shader, and
# where that call belongs when a second one turns up is a question that needs the
# second folder to answer.
#
# Two things here are contracts with check.cmake, not free to reword:
#
#   - The guard messages below are searched for by substring (ADR-0005,
#     "Clang 19 or newer", ADR-0026, ADR-0022). Changing the text breaks the
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

# The database is exported on every configure, not only the ones a preset drives:
# check.cmake configures each folder standalone with no preset in sight, and a
# folder whose editor support depends on which entry point configured it is a
# folder that works on one machine and not the next. This is a normal variable,
# so it takes hold in whichever scope included this file first — the repository
# root in a full build, the folder itself in a standalone one — and every
# subdirectory inherits it from there.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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
    elseif(folder STREQUAL "dev")
        # dev may depend on anything, app included: it is the one program a
        # person runs to see the current state, so whatever exists is fair game.
        # It is a leaf and it stays one — dev appears in no other row, and
        # putting it in one would be the mistake this map exists to catch.
        set(deps base math ecs scene platform assets render 3d app)
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

# The database, copied out of the build tree to sit beside the CMakeLists.txt of
# the project that owns it. prefix is the project's name, so the target reads
# voe_math_compile_commands and the root's reads voe_compile_commands, and every
# target named after dir gains a dependency on it — which is what makes building
# one folder refresh that folder's database without building the whole tree.
#
# The copy runs on every build rather than being an output with the database as
# its input, and that is deliberate. Several build trees write to the same file:
# build/debug, whatever an IDE made, and build/check/root every time check.cmake
# runs. A rule keyed on timestamps cannot see that a different tree overwrote the
# copy, because it left the copy newer than this tree's database, so the file
# would sit there naming a build directory nobody is using and no ordinary build
# would put it right. Running unconditionally means the last build wins, which is
# the only answer that is right from the editor's point of view. It costs one
# file comparison per project, and copy_if_different leaves the timestamp alone
# when the content matches, so nothing downstream churns.
function(voe_export_compile_commands prefix dir)
    add_custom_target(${prefix}_compile_commands ALL
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_BINARY_DIR}/compile_commands.json"
                "${dir}/compile_commands.json"
        COMMENT "compile_commands.json -> ${dir}"
        VERBATIM)

    foreach(target IN LISTS ARGN)
        add_dependencies(${target} ${prefix}_compile_commands)
    endforeach()
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
    # 19 and not 18 because #embed is how a compiled shader gets into the binary
    # (ADR-0046), and clang grew #embed in 19 (ADR-0047).
    if(version VERSION_LESS 19)
        message(FATAL_ERROR "VOE3D requires Clang 19 or newer (ADR-0005, ADR-0047). Found: ${version}")
    endif()

    if(NOT CMAKE_C_COMPILER_FRONTEND_VARIANT STREQUAL "GNU")
        message(FATAL_ERROR "VOE3D requires the GNU-driver clang, not clang-cl (ADR-0026). Found frontend: ${CMAKE_C_COMPILER_FRONTEND_VARIANT}")
    endif()
endfunction()

# platform's bill, in one place. Two things no other folder needs:
#
#   - A Wayland protocol is a description, not a library. wayland-scanner turns
#     every XML in platform/protocol/ into C at configure-and-build time, into
#     the build tree. Nothing generated is committed, and the generated files are
#     compiled with warnings off and reached through a SYSTEM include directory,
#     because they are wayland-scanner's code and not ours to keep clean.
#     protocol/ is globbed for the same reason src/ is: vendoring a protocol is
#     dropping in a file, and it needs no edit here.
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
    file(MAKE_DIRECTORY ${generated})

    file(GLOB protocols CONFIGURE_DEPENDS ${folder_dir}/protocol/*.xml)
    if(NOT protocols)
        message(FATAL_ERROR "voe_platform_backend: no protocol XML in ${folder_dir}/protocol")
    endif()

    set(sources "")
    foreach(xml IN LISTS protocols)
        cmake_path(GET xml STEM name)
        set(header ${generated}/${name}-client-protocol.h)
        set(code ${generated}/${name}-protocol.c)

        add_custom_command(
            OUTPUT ${header}
            COMMAND ${VOE_WAYLAND_SCANNER} client-header ${xml} ${header}
            DEPENDS ${xml}
            COMMENT "wayland-scanner: ${name} client header"
            VERBATIM)
        add_custom_command(
            OUTPUT ${code}
            COMMAND ${VOE_WAYLAND_SCANNER} private-code ${xml} ${code}
            DEPENDS ${xml}
            COMMENT "wayland-scanner: ${name} protocol code"
            VERBATIM)

        set_source_files_properties(${code} PROPERTIES COMPILE_OPTIONS "-w")
        list(APPEND sources ${code} ${header})
    endforeach()

    set(${out_sources} "${sources}" PARENT_SCOPE)
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

# render's bill, in one place, and the second folder this file knows by name.
#
# A shader is a description too, the same as a Wayland protocol is: slangc turns
# every .slang in render/shaders/ into a SPIR-V module in the build tree, and
# nothing generated is committed. shaders/ is globbed for the same reason src/
# is, so adding a shader needs no edit here.
#
# THE SPIR-V IS NOT COMPILED, IT IS EMBEDDED. Nothing links it and nothing reads
# it from disk at run time: a C file #embeds the .spv and the bytes end up in the
# binary (ADR-0046). That is why the generated directory comes back to be handed
# to the compiler rather than added to the sources, and it is the whole reason
# the Clang floor is 19.
#
# It is handed over as --embed-dir and NOT as an include directory. #embed has a
# search path of its own and does not look at -I, so an -I here finds nothing —
# and because the diagnostic for that lands on a line already inside a
# declaration, the mistake reads as a broken array rather than as a missing flag.
#
# #embed is invisible to CMake's dependency scanning, so the tie between the .spv
# and the object file that embeds it is made by hand, with OBJECT_DEPENDS on the
# folder's sources. That property does two jobs at once and both are needed: it
# orders the build, so the .spv exists before anything tries to embed it, and it
# rebuilds the object when the shader changes. It is set on every source in the
# folder rather than on the one file that embeds today, because naming that file
# here would put a source file's name back into CMake, which is the thing the
# glob exists to avoid.
#
# Three flags, and none of them is a preference:
#
#   -matrix-layout-row-major     this engine stores matrices row-major so that an
#                                upload is a straight copy. slangc's default is
#                                the other one, and getting it wrong transposes
#                                every transform without failing to compile.
#   -fvk-use-entrypoint-name     keep the entry point names the shader gave them.
#                                Without it a single-entry-point module is called
#                                `main`, and the C side names entry points.
#   -target spirv                the only target this engine has.
function(voe_render_shaders folder_dir out_compiled out_include)
    find_program(VOE_SLANGC slangc)
    if(NOT VOE_SLANGC)
        message(FATAL_ERROR "VOE3D requires slangc. It is a source-transforming tool, so it is installed by the programmer; the Vulkan SDK is the easiest way to get one.")
    endif()

    set(generated ${CMAKE_BINARY_DIR}/generated/render)
    file(MAKE_DIRECTORY ${generated})

    file(GLOB shaders CONFIGURE_DEPENDS ${folder_dir}/shaders/*.slang)
    if(NOT shaders)
        message(FATAL_ERROR "voe_render_shaders: no shaders in ${folder_dir}/shaders")
    endif()

    set(compiled "")
    foreach(shader IN LISTS shaders)
        cmake_path(GET shader STEM name)
        set(spv ${generated}/${name}.spv)

        add_custom_command(
            OUTPUT ${spv}
            COMMAND ${VOE_SLANGC} ${shader}
                    -target spirv
                    -matrix-layout-row-major
                    -fvk-use-entrypoint-name
                    -o ${spv}
            DEPENDS ${shader}
            COMMENT "slangc: ${name}"
            VERBATIM)

        list(APPEND compiled ${spv})
    endforeach()

    set(${out_compiled} "${compiled}" PARENT_SCOPE)
    set(${out_include} ${generated} PARENT_SCOPE)
endfunction()

# Everything voe_module() and voe_executable() do before they part company: the
# guards, the dependency-map check, pulling each dependency in, and the src/ glob
# with one-platform-only sources dropped from it.
#
# folder_dir is passed rather than read from CMAKE_CURRENT_LIST_DIR, because that
# variable follows the listfile being processed and this function is two calls
# deep. The callers are the ones sitting in the folder's own listfile, so they
# are the ones that know.
function(voe_folder_sources caller folder_dir folder deps out_sources)
    voe_guards()

    voe_allowed_deps("${folder}" allowed)
    foreach(dep IN LISTS deps)
        if(NOT dep IN_LIST allowed)
            message(FATAL_ERROR "${caller}(${folder}): ${dep} is not an allowed dependency (ADR-0022)")
        endif()
    endforeach()

    foreach(dep IN LISTS deps)
        if(NOT TARGET voe_${dep})
            add_subdirectory(${folder_dir}/../${dep} ${CMAKE_BINARY_DIR}/${dep})
        endif()
    endforeach()

    file(GLOB sources CONFIGURE_DEPENDS ${folder_dir}/src/*.c)

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

    set(${out_sources} "${sources}" PARENT_SCOPE)
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

    voe_folder_sources(voe_module ${CMAKE_CURRENT_LIST_DIR} "${folder}" "${arg_DEPENDS}" sources)

    if(folder STREQUAL "platform")
        voe_platform_backend(${CMAKE_CURRENT_LIST_DIR} generated generated_dir)
        list(APPEND sources ${generated})
    endif()

    # Not appended to sources: a .spv is embedded, not compiled. See
    # voe_render_shaders() for why the tie is OBJECT_DEPENDS.
    if(folder STREQUAL "render")
        voe_render_shaders(${CMAKE_CURRENT_LIST_DIR} compiled shader_dir)
        set_source_files_properties(${sources} PROPERTIES
            OBJECT_DEPENDS "${compiled}")
    endif()

    add_library(voe_${folder} STATIC ${sources})
    add_library(voe::${folder} ALIAS voe_${folder})
    target_include_directories(voe_${folder} PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)

    # PRIVATE: the SPIR-V is render's own and nothing outside it embeds anything.
    if(folder STREQUAL "render")
        target_compile_options(voe_${folder} PRIVATE --embed-dir=${shader_dir})
    endif()
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

    voe_export_compile_commands(voe_${folder} ${CMAKE_CURRENT_LIST_DIR} voe_${folder})
endfunction()

# voe_executable(<folder> [DEPENDS <folder>...])
#
# A folder whose src/ produces a program instead of a library. Same guards, same
# dependency map, same glob, same flags — a dev program compiled with looser
# warnings than the engine is a dev program that stops building the day someone
# looks at it.
#
# Three things a library has that this does not: an alias, because nothing links
# a program; a public include directory, because nothing includes one; and tests,
# because a folder that is a program has nothing to unit test that would not be
# better off in a folder that is a library.
#
# The target is never registered with ctest. voe_dev opens a window and waits for
# a person, and a check script waiting for someone to close a window is a check
# script that hangs.

function(voe_executable folder)
    cmake_parse_arguments(arg "" "" "DEPENDS" ${ARGN})
    if(arg_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "voe_executable(${folder}): unexpected argument(s): ${arg_UNPARSED_ARGUMENTS}")
    endif()

    voe_folder_sources(voe_executable ${CMAKE_CURRENT_LIST_DIR} "${folder}" "${arg_DEPENDS}" sources)

    add_executable(voe_${folder} ${sources})
    foreach(dep IN LISTS arg_DEPENDS)
        target_link_libraries(voe_${folder} PRIVATE voe::${dep})
    endforeach()

    voe_target_settings(voe_${folder})

    voe_export_compile_commands(voe_${folder} ${CMAKE_CURRENT_LIST_DIR} voe_${folder})
endfunction()
