# 008 — `compile_commands.json`, one per project, beside its `CMakeLists.txt`

status: review  
claimed-by: claude-opus-5 (kanban-coder)  
blocked-by: -

Make sure every cmakelists project exports compile_commands.json, one per project.  
Move it out to the same folder as the cmakelists and make sure to gitignore it.

we use clang on every platform, which makes this viable.  
No extra scripts for this, cmake should do all the work needed.

## What was done

`cmake/voe.cmake`

- `set(CMAKE_EXPORT_COMPILE_COMMANDS ON)` beside `enable_testing()`, as a normal
  variable, so it takes hold in whichever scope includes the file first — the
  root in a full build, the folder itself in a standalone one — and every
  subdirectory inherits it. Not in `CMakePresets.json`: `check.cmake` configures
  each folder with no preset in sight, and the card says every project.
- `voe_export_compile_commands(prefix dir [target...])`, a target that copies
  the build tree's database to `<dir>/compile_commands.json` with
  `cmake -E copy_if_different`. Called at the end of `voe_module()` and of
  `voe_executable()`, so no folder's four lines change.

`CMakeLists.txt` (root) — one call for the root project itself. The only call
made from outside `voe_module()`/`voe_executable()`, because the root builds no
target of its own for one to hang off.

`.gitignore` — the bare `compile_commands.json` line was already there and
already matches at any depth; it now carries a comment saying why depth matters.

Nothing else changed. No new script, no new tool, no folder `CMakeLists.txt`
edit, no `<folder>.md` edit — no folder's public C surface moved.

## Two decisions worth knowing about

**The copy is the whole build tree's database, not that folder's slice.** CMake
emits one database per build tree and offers no per-directory export; slicing it
would mean a JSON-editing script, which the card forbids. clangd looks an entry
up by file path, so a full database in `math/` answers for `math/src/float3.c`
exactly as a sliced one would. In a *standalone* configure the tree only holds
that folder, so the file is naturally its slice — verified: `cmake -S math`
alone yields 4 entries, all `math/src/*.c`.

**The copy runs on every build instead of being an output keyed on the
database.** The first version was output-keyed, and it was wrong. Several build
trees write the same file — `build/debug`, an IDE's, and `build/check/root` on
every `check.cmake` run — and a timestamp rule cannot see a foreign overwrite,
because that overwrite leaves the copy *newer* than this tree's database. The
observed failure: run `check.cmake`, then `cmake --build --preset debug` says
"no work to do" and every folder's database still names `build/check/root`.
Running unconditionally makes the last build win, which is the only answer that
is right from the editor's seat. Cost is one file comparison per project, and
`copy_if_different` leaves the timestamp alone when content matches.

The consequence to expect: **the tree you built last owns the databases.** A
`check.cmake` run repoints them at `build/check/root`; the next ordinary build
takes them back. Configuring `render` standalone also refreshes `base/` and
`platform/`, because it pulls them in. This is inherent to one fixed filename
per folder, which is what the card asks for.

## Verified — Linux, clang 22.1.8, cmake 4.3.0, ninja 1.13.2

- `cmake -P check.cmake` — **could not be run as-is.** Step 1 fails with
  `slangc could not be run: no such file or directory`; `slangc` is not
  installed on this machine, and that is a pre-existing environment gap, not
  this change. Verified with a scratch copy of `check.cmake` in which the four
  lines of the `slangc` probe were deleted and nothing else. All 13 steps ok,
  exit 0, including standalone configure of all five folders, root configure and
  build, all three guards, and 2 tests passed. **The real `check.cmake` still
  needs to be run by someone with `slangc` on PATH before this reaches
  `complete/`.**
- Fresh `cmake --preset debug` + build → all six databases present and naming
  `build/debug`: root, `base`, `dev`, `math`, `platform`, `render`. All parse as
  JSON, 20 entries each.
- Rebuild with nothing changed → no relink, no churn.
- `ninja voe_math` alone → refreshes `math/compile_commands.json` and touches no
  other folder.
- Delete `math/compile_commands.json`, build `voe_math` → restored.
- `check.cmake` run, then `cmake --build --preset debug` → all six taken back to
  `build/debug`. (This is the case the first design got wrong.)
- Standalone `cmake -S math -B …` + build → `math/compile_commands.json` holds
  math's 4 entries only.
- gitignore proved rather than assumed: a throwaway `git init` with this
  `.gitignore` and the file at root, `base/` and `math/` — `git check-ignore -v`
  matches line 22 for all three, `git status --porcelain` lists none of them.
  (Proved this way because git does not work in this checkout; see below.)

No `DEVIATION:` and no `BLOCKED:` markers were needed.

## Reported, not fixed

- **`CLAUDE.md` contradicts `check.cmake`.** `CLAUDE.md` rule 8 and the tool
  table both describe a *step 7*, `clang --analyze`, "an analyser warning fails
  the script exactly as a warning does". `check.cmake` has no step 7; it ends at
  6b. Either the step is missing or the prose is. Not this card.
- **git does not work in this checkout.** `.git` is a gitlink reading
  `gitdir: ../.git/modules/voe3d`, and that parent is not present, so every git
  command reports "not a git repository". This card was moved to `review/` with
  plain `mv`, and nothing here can be committed until that is sorted.
- **`dev/cmake-build-debug/` is not ignored.** `.gitignore` has
  `/cmake-build-*/`, anchored to the root, so the root's `cmake-build-debug/` is
  ignored and `dev/`'s is not. Noticed on the way past; left alone.

## Suggestion, not done

`check.cmake` could be taught to leave the source-tree databases alone, so a
verification run does not repoint a person's editor mid-session. It would want a
switch this card did not ask for, so it is a card of its own if you want it.
