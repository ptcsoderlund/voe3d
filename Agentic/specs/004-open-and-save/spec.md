# 004 Open and save

Status: building
Approved: 2026-09-15
Accepted: -

The editor opens a game's project, saves the scene being worked on, and opens the last project
again the next time it starts — the way Blender does, down to the untitled cube and light it
starts on. Until now the editor showed a scene built into its code and nothing made in it
survived closing it; this is what makes the editor a place work is kept.

## Acceptance criteria

1. **First start.** With no project ever opened, the editor starts on an untitled scene holding one
   cube and one directional light that lights it. The top bar says the scene is untitled.
2. **The top bar.** A bar across the top of the window holds New, Open and Save and the open
   project's name. Ctrl+N, Ctrl+O and Ctrl+S do the same as the buttons.
3. **Unsaved changes show.** Dragging a number in the inspector marks the scene as unsaved in the
   top bar. Moving a scene view's camera does not.
4. **The first save makes the project.** Save on an untitled scene opens the editor's own file
   browser. Folders can be entered and left, and a name typed makes a new folder. Confirming
   writes the project there — the folder then holds `project.voe3d` and the scene — the unsaved
   mark clears and the top bar shows the project's name. A folder that is not empty is refused
   with a notice, and nothing is written.
5. **Save keeps the work.** In an opened project, change a number and Save; close the editor and
   open the project again: the number is as it was saved.
6. **The last project comes back.** Close the editor and start it again: it opens on the last
   project that was open, as last saved.
7. **Open.** Open shows the file browser; a folder holding a project is marked as one. Choosing it
   opens that project. Choosing a folder that is not a project is refused with a notice, and the
   editor stays on what it had.
8. **New.** New replaces the scene with a fresh untitled cube and light.
9. **Unsaved work is not lost by accident.** With unsaved changes, the first close of the window,
   the first New and the first Open are each refused, with a notice saying there are unsaved
   changes; doing it again goes ahead and discards them.
10. **A broken project is refused.** Opening a project whose scene file is broken is refused with a
    notice naming the file, the line and what is wrong. The editor stays on what it had, and the
    broken file is left untouched.
11. **A lost last project does not lock you out.** If the last project was moved, deleted or its
    scene broken, the editor starts on an untitled cube and light with a notice saying what was
    wrong, and writes nothing.
12. **From the command line.** `voe_editor <folder>` starts on that project. If it cannot be opened,
    the editor does not start and prints a plain message naming the file, the line and what is
    wrong.
13. `cmake -P check.cmake` exits zero on Linux.

## Out of scope

- A list of recent projects — later.
- Save As, ever. Moving or copying a project is done outside the editor. Copying a scene will be a
  clone in a future asset browser.
- More than one scene per project.
- The system's own file dialog.
- Making and removing things in a scene; editing stays what the inspector does today.
- Renaming a project inside the editor.

## Constraints

- Linux first: acceptance is on Linux.
- The file browser is the editor's own, the same on every platform.

## Defaults

- A project is a folder holding a `project.voe3d` file, as already decided for the engine
  (ADR-0146). Its name is the folder's name. Its one scene is a file beside `project.voe3d`.
- Which project was open last is remembered per person on the machine (on Linux under
  `~/.config/voe3d/`), never inside a project.
- A save that is interrupted leaves the previously saved file whole.
- The file browser starts in the folder it was last in, or the home folder. It lists folders only,
  not hidden ones, and can go up to the parent.
- A notice stays in the top bar until the next thing is done.
- A scene view's camera is not saved with the scene.
- `voe_editor --capture` keeps working, and draws the scene the editor would have started on.
- Typing a folder name takes letters (å, ä, ö included), digits, `-` and `_`. Characters that need
  AltGr, and a held key repeating, wait for a later typing feature (agreed with the sponsor
  2026-09-15).

## Open questions

- None.
