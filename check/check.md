# check

The steps of `check.cmake`, one file per step group, included by it in this order and run in its scope. No build of its own and not a code folder: it holds no `CMakeLists.txt`, so `check.cmake` does not count it as a folder.

- `report.cmake` — `step_ok`, `step_skip`, `step_warn`, `step_fail` and `run_capture`, which every step uses.
- `tools.cmake` — step 1: the tools and their versions, and the slangc floor that warns.
- `folders.cmake` — the folder list, and step 1b: the folders and the root `CMakeLists.txt` agree.
- `build.cmake` — step 2, each folder configured standalone, and step 3, the root configured and built; step 3b builds the tree again in Release.
- `guards.cmake` — steps 4a to 4c: the compiler, version and dependency-map guards fire.
- `includes.cmake` — step 5: what each folder's sources may include.
- `tests.cmake` — steps 6, 6b and 6c: the tests, the harness reporting a failure, and scene with descriptions off.
- `analyser.cmake` — steps 7 and 7b: clang's static analyser, and its proof that a finding is reported.
