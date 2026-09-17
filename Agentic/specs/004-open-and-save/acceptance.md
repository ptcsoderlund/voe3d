# 004 Open and save — acceptance

## Start it

1. Build the editor:

       cmake --preset debug
       cmake --build --preset debug --target voe_editor

2. Start it with no arguments:

       ./build/debug/editor/voe_editor

3. If you have run the editor before and want to try the very first start again, remove what it
   remembers first:

       rm -rf ~/.config/voe3d

## Try this

1. **First start** — with `~/.config/voe3d` removed, start the editor. Expected: a scene holding one
   cube lit by one directional light, and the top bar saying the scene is untitled.
2. **The top bar** — look at the bar across the top. Expected: New, Open and Save, and the open
   project's name. Ctrl+N, Ctrl+O and Ctrl+S do the same as the three buttons.
3. **Unsaved changes show** — drag a number in the inspector. Expected: the top bar marks the scene
   as unsaved. Then drag a scene view's camera around. Expected: no mark from that.
4. **The first save makes the project** — press Save on the untitled scene. Expected: the editor's own
   file browser opens, listing folders only; you can enter a folder and go up to the parent. Type a
   name and make a new folder, then confirm. Expected: the unsaved mark clears, the top bar shows the
   project's name, and the folder holds `project.voe3d` and the scene file beside it. Try confirming
   on a folder that already has something in it. Expected: refused with a notice, nothing written.
5. **Save keeps the work** — in the saved project, change a number, press Save, close the editor, and
   open the project again. Expected: the number is as you saved it.
6. **The last project comes back** — close the editor and start it again with no arguments. Expected:
   it opens the project you last had open, as last saved.
7. **Open** — press Open. Expected: the file browser, with folders that hold a project marked as such.
   Choose one. Expected: it opens. Choose a folder that is not a project. Expected: refused with a
   notice, and the editor stays on what it had.
8. **New** — press New. Expected: a fresh untitled cube and light.
9. **Unsaved work is not lost by accident** — make a change, then try to close the window. Expected:
   the first close is refused with a notice saying there are unsaved changes; closing again goes
   ahead. The same for the first New and the first Open after a change.
10. **A broken project is refused** — with the editor open, edit a saved project's scene file by hand
    to break it (for instance delete a character mid-file), then Open that project. Expected: refused
    with a notice naming the file, the line and what is wrong; the editor stays on what it had and the
    broken file is left as you left it.
11. **A lost last project does not lock you out** — close the editor, rename or delete the folder of
    the project it last had open, and start it again. Expected: an untitled cube and light, with a
    notice saying what was wrong. Nothing is written.
12. **From the command line** — `./build/debug/editor/voe_editor <folder>` on a project folder.
    Expected: it starts on that project. On a folder that is not a project, or one whose scene is
    broken: the editor does not start, and prints a plain message naming the file, the line and what
    is wrong.
13. **The checks** — from the repository root:

        cmake -P check.cmake

    Expected: it exits zero.

## Results

### 2026-09-17 — Windows

Criterion 13 (`check.cmake`) on Windows: **failed**. Every step up to and including `includes`
passed; `tests` failed with 55 of 56 passing and `platform/folder` failing at
`platform/tests/folder.c:122`, on `listing.entries[0].hidden`. Toolchain: clang 22, cmake 4.3.3,
slangc 2026.14.1.

Criterion 13 as written is Linux only, and the check is green on Linux. The failure is in a folder
this feature changed (task 3), so it is recorded as a defect of 004 rather than as a pre-existing
platform gap.

### 2026-09-17 — Linux, hands on

Criteria 1–12: **all passed**. The sponsor tried every one and reported nothing wrong.

Verdict: rejected on criterion 13 alone, and only on Windows. The Windows `hidden` defect is fixed
inside this feature; the sponsor re-runs the Windows check afterwards.

### Second hand-over, after the fix

Task 15 makes `hidden` one meaning on both platforms: a leading `.` anywhere, and on Windows the
`FILE_ATTRIBUTE_HIDDEN` attribute as well (ADR-0166). Linux behaviour is unchanged by design, so the
green Linux check proves nothing about the fix.

Only criterion 13 needs retrying, and only on Windows:

    cmake -P check.cmake

Expected: it exits zero, with `platform/folder` among the 56 tests passing. The Windows-only part of
that test was written without being compiled; a compile error there is the likeliest way this fails.
