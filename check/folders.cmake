# folders.cmake — check.cmake's folder list, and step 1b, folders in the root build.
#
# Discovers every top-level directory holding a CMakeLists.txt, then fails when
# that list and the root CMakeLists.txt's add_subdirectory lines disagree.
#
# Included by check.cmake after tools.cmake, in order, and runs in that file's
# scope. Reads root and the report.cmake functions. Leaves folders, the sorted
# list every later part loops over.

# ------------------------------------------------------------ folder list

file(GLOB entries LIST_DIRECTORIES true RELATIVE "${root}" "${root}/*")
set(folders "")
foreach(entry IN LISTS entries)
    if(IS_DIRECTORY "${root}/${entry}" AND EXISTS "${root}/${entry}/CMakeLists.txt")
        list(APPEND folders "${entry}")
    endif()
endforeach()
list(SORT folders)
if(folders STREQUAL "")
    step_fail("folders" "no top-level directory holds a CMakeLists.txt")
endif()

# -------------------------------------------- 1b folders in the root build
#
# THE GLOB ABOVE AND THE ROOT CMakeLists.txt ARE TWO LISTS AND THIS IS WHERE THEY
# ARE MADE TO AGREE. Everything below discovers folders; the root build lists them
# by hand. Until this step existed nothing compared the two, so a folder left out
# of the root file passed its standalone configure — that step reads the folder's
# own CMakeLists.txt — and was then simply absent from the configure, the build
# and ctest. Every step printed ok and the only number that would have said
# otherwise was a test count nobody reads. That is the worst shape a failure can
# have: not a wrong answer, but a right answer to a question nobody asked.
#
# THE ROOT'S LIST IS DELIBERATE AND IS NOT THE THING TO FIX. It says what the
# engine *is*, and a glob there would let a scratch folder join the build without
# anyone saying so — the same bug pointed the other way. Two lists is the design;
# this step is what makes keeping them in step something the script does rather
# than something a person remembers.
#
# IT RUNS HERE BECAUSE IT NEEDS THE GLOB AND NOTHING ELSE. It reads one file and
# compares two lists, so it costs milliseconds and comes before the thirteen
# configures of step 2 — a folder that is not in the build should be named in a
# second, not after a full build has succeeded around it.
#
# THE PARSE IS A REGEX AND IT REFUSES ANYTHING IT CANNOT READ PLAINLY. A root file
# simple enough for one line per folder is the thing being relied on, so the four
# guards below fail loudly on the constructs that would make a regex lie —
# bracket comments, computed names, control flow — rather than quietly reporting
# a folder as listed when it is not. If this ever fires on a root file somebody
# meant, the fix is to make the root file simple again, not to make this cleverer.
set(step "folders in the root build")

file(READ "${root}/CMakeLists.txt" root_text)

# A bracket comment can hide an add_subdirectory on a line that does not itself
# start with a #, which is precisely a folder that looks listed and is not.
if(root_text MATCHES "#\\[")
    step_fail("${step}"
        "the root CMakeLists.txt holds a bracket comment, and this step cannot tell what one hides.\nRemove it, or read the note above this step in check.cmake.")
endif()
# A folder listed inside an if() or a foreach() is not listed unconditionally, and
# this step would report it as listed whatever the condition said.
if(root_text MATCHES "(^|\n)[ \t]*(if|foreach|while|macro|function)[ \t]*\\(")
    step_fail("${step}"
        "the root CMakeLists.txt holds control flow, and a folder inside it is not unconditionally in the build.\nThe root file is meant to be one add_subdirectory per line; see the note above this step in check.cmake.")
endif()

string(REPLACE "\n" ";" root_lines "${root_text}")
set(listed "")
foreach(line IN LISTS root_lines)
    string(STRIP "${line}" line)
    # A commented-out line is not in the build, which is the case this step
    # exists to catch, so it is skipped rather than matched.
    if(line MATCHES "^#")
        continue()
    endif()
    if(NOT line MATCHES "add_subdirectory")
        continue()
    endif()
    if(NOT line MATCHES "add_subdirectory[ \t]*\\(([^)]*)\\)")
        step_fail("${step}"
            "this line names add_subdirectory and this step cannot read it:\n${line}")
    endif()
    string(STRIP "${CMAKE_MATCH_1}" name)
    # A plain directory name and nothing else: no variable, no quotes, no path
    # and no second argument. Any of those and the name below would be wrong.
    if(NOT name MATCHES "^[A-Za-z0-9_.+-]+$")
        step_fail("${step}"
            "add_subdirectory(${name}) is not a plain folder name, so this step cannot say which folder it adds.\nSee the note above this step in check.cmake.")
    endif()
    list(APPEND listed "${name}")
endforeach()
list(SORT listed)

foreach(folder IN LISTS folders)
    if(NOT folder IN_LIST listed)
        step_fail("${step}"
            "${folder}/ holds a CMakeLists.txt but the root CMakeLists.txt never adds it.\nIts tests are not being built and not being run, and every step below this one would have said ok without them.\nAdd `add_subdirectory(${folder})` to the root CMakeLists.txt.")
    endif()
endforeach()

foreach(name IN LISTS listed)
    if(NOT name IN_LIST folders)
        step_fail("${step}"
            "the root CMakeLists.txt adds ${name}, and there is no ${name}/ holding a CMakeLists.txt.\nEither the folder was removed and its line was not, or the name is misspelt.")
    endif()
endforeach()

list(LENGTH folders folder_total)
list(JOIN folders " " folder_names)
step_ok("${step} (${folder_total}: ${folder_names})")
