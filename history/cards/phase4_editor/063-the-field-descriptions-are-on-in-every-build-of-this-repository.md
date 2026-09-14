# 063 — The field descriptions are on in every build of this repository

claimed-by: claude-opus-5 (kanban-coder)
blocked-by: -
status: review
decision: *This repository is a development tree, and field descriptions are on throughout
  it* (ADR-0145) — nothing shipped is built from this root, so the cost the switch avoids is
  not charged here. Fixes bug report 004. The half above it is *the cook emits C source*
  (ADR-0144).

## Goal

`voe_editor` inspects identically whichever preset built it. The `editor` preset is gone,
`VOE_BASE_DESCRIPTIONS` is set by `cmake/voe.cmake` for every target in this repository, and
`check.cmake` proves the switched-off path still compiles and still returns `NULL`.

## Where you are starting from

An interim fix is already in the working tree, made on the principal's instruction so he
could review card 059: the **`debug` preset carries `"CMAKE_C_FLAGS":
"-DVOE_BASE_DESCRIPTIONS=1"`**, and `editor/editor.md`'s build paragraph was corrected to
match. This card **supersedes that edit** — the flag comes back out of the presets and goes
into `voe_target_settings`. Bug report 004's *What was changed, on instruction* section
describes exactly what was done and is worth reading first.

## Scope

**`cmake/voe.cmake` — `voe_target_settings()`.** Add the definition beside the one flag set,
so it reaches a folder's library and its test executables identically:

    target_compile_definitions(${target} PRIVATE
        VOE_BASE_DESCRIPTIONS=$<BOOL:${VOE_BASE_DESCRIPTIONS}>)

with a cache variable defaulting to on:

    option(VOE_BASE_DESCRIPTIONS "Compile the field descriptions in" ON)

The comment above the function says what the one flag set is for; extend it to say why the
descriptions are part of it — **every program built from this root is a tool, a guarantee, a
library or a test, and none of them ships**. Say that the cache variable exists for the
`check.cmake` step below and is not a knob for daily use: there is no supported way to
configure this root without descriptions and still run the editor.

**`CMakePresets.json`.** Delete the `editor` configure preset and the `editor` build preset,
and take `CMAKE_C_FLAGS` back out of `debug`. `debug` and `release` remain, and neither
carries `CMAKE_C_FLAGS` any more — they differ by `CMAKE_BUILD_TYPE` and nothing else, which
is what their names claim.

**`check.cmake` — a new step `6c descriptions off`, after `6b`.** Configure a scratch tree
with `-DVOE_BASE_DESCRIPTIONS=OFF`, build **`voe_scene` and its tests only** — not the whole
tree — and run them. It must exit zero. Follow the idiom the existing scratch-folder steps
use (the two configures near the analyser step) for the directory, the failure text and the
`step_fail` call. The comment above it says what it is for in the terms ADR-0145 point 5
uses: with the switch on everywhere else, this is the only thing in the repository that
compiles the `#else return NULL` branch in `scene/src/transform_system.c` and
`identity_system.c`, and the only thing that runs the `!BUILD_DESCRIBES` half of the two
scene tests. Without it that branch rots and a game developer finds out.

**`editor/editor.md`.** The *Building and running it* section names `--preset editor` and
the paragraph under it explains the `editor` preset. Replace both: the build is
`cmake --preset debug`, the target is unchanged, and the paragraph becomes one or two lines
saying the descriptions are compiled into everything built here and why that is free —
naming ADR-0145 rather than restating it. Keep the file under its 120-line ceiling.

**Downstream call sites: none.** No public header changes and no folder's `DEPENDS` line
moves. Nothing outside the four files above needs an edit to keep the tree building.

## What must not change

- **`base/include/base/describe.h` and the switch's own semantics.** It stays off unless a
  build asks; what changes is that this build asks. ADR-0128 point 3 is untouched, and the
  header's paragraph about the price is still true and still the reason a game's tree does
  not define it.
- **`scene/src/transform_system.c` and `identity_system.c`.** The `#if` / `#else` around
  `transform_description()` and `identity_description()` stays exactly as it is. Bug 004
  established that no code is at fault in any folder; this card changes what the build says,
  not what the code does.
- **`scene/tests/transform.c` and `scene/tests/identity.c`.** Their `BUILD_DESCRIBES`
  constant and both branches under it stay. Step 6c is what makes the false branch reachable
  again.
- **`editor/src/main.c`'s `say_whether_descriptions_are_in`.** Keep it and its text. It is
  now unreachable in its second form from any supported configure, and it stays because a
  hand-rolled `cmake -B` is exactly how bug 004 was proved and how the next one will be.
- **The root `CMakeLists.txt`.** No control flow, no conditional `add_subdirectory`. Step 1b
  refuses it on purpose, and ADR-0145 rejected that remedy — the editor staying out of a
  finished game is the game's own build tree, not a preset here.
- `dev` still builds and runs. It is the proof the engine is usable without the editor.

## Verify

Linux is enough (ADR-0130); note what could not be checked there.

1. `cmake --preset debug` then `cmake --build --preset debug --target voe_editor`. Run
   `./build/debug/editor/voe_editor`. Startup says `descriptions  compiled in`. Select
   `Cube`: the transform expands to its nine number boxes and the identity to its two
   labels. **This is the reported bug and this is the line that closes it.**
2. The same with `--preset release`, binary at `./build/release/editor/voe_editor`. Same
   nine boxes. Both presets, same program.
3. `cmake --preset editor` fails with CMake saying there is no such preset.
4. `cmake -P check.cmake` exits zero, and its output names step 6c.
5. Confirm 6c is doing its job: temporarily break the `#else` branch in
   `scene/src/transform_system.c` — return something that does not compile — and check that
   `cmake -P check.cmake` fails at 6c rather than passing. **Revert it.** Say in the notes
   that you ran this and what it said.

## Done looks like

The principal opens the editor from the `debug` build, selects an entity, and sees its
fields. He does not have to know a preset name. `check.cmake` is green and has one more step
than it did, and that step is the only place the switched-off path is built.

## Notes

Verified on **Linux** (Fedora 44, clang 22, RTX 4070). Windows not checked; nothing
here is platform-specific.

Changed: `cmake/voe.cmake` (`option()` at file scope just above
`voe_target_settings`, the definition inside it, both comments), `CMakePresets.json`,
`check.cmake` (new step after 6b), `editor/editor.md` (build block and paragraph; 99 lines).

1. `build/debug` was deleted first. Configured earlier in this session under the old
   preset, it still cached `CMAKE_C_FLAGS`, which would have made this check meaningless.
   After a fresh `cmake --preset debug`: `CMAKE_C_FLAGS` empty in the cache, and
   `compile_commands.json` for `scene/src/transform_system.c` has
   `-DVOE_BASE_DESCRIPTIONS=1 -g`. `voe_editor` builds; running it prints
   `descriptions  compiled in`.
   **Not checked: selecting `Cube` and seeing the nine boxes and two labels.** This session
   cannot click in a Wayland window. That line closes bug 004 and needs a person.
2. Fresh `--preset release`: `-DVOE_BASE_DESCRIPTIONS=1 -O3`, and
   `./build/release/editor/voe_editor` prints `descriptions  compiled in`. Same caveat
   about the boxes.
3. `cmake --preset editor` →
   `CMake Error: No such preset in …/voe3d: "editor"`.
4. `cmake -P check.cmake` → every step `ok`, including
   `ok    descriptions off (scene, 4 passed)` between `harness reports a failure` and
   `analyser`. Output lines carry no step numbers, so the new step follows that idiom.
   `6c descriptions off` is its banner comment in `check.cmake`.
5. Replaced `return NULL;` in the `#else` of `transform_description()` with
   `return not_a_declared_name;` and ran `cmake -P check.cmake`. Every earlier step
   passed, then `FAIL  descriptions off`, "scene did not build with descriptions off",
   `ninja: build stopped`, `check failed at: descriptions off`. **Reverted** (grep finds
   0 occurrences), re-ran: green.

- 6c configures `scene` standalone into `build/check/descriptions-off`, builds only
  `voe_scene` plus one `voe_test_scene_<file>` target per `scene/tests/*.c`, globbed so a
  new scene test is covered, and runs `ctest -R ^scene/`. Finding no tests is a failure.
- `editor/src/main.c:105` still says `cmake --preset editor`, kept as the card says. It
  now names a preset that does not exist; a later card could reword it.
- `bash tools/hot.sh`: all under ceilings. No `DEVIATION:` / `BLOCKED:` markers.
