# 0164 The editor opens and saves projects itself, and the project file's code lives in `authoring`

Status: accepted
Date: 2026-09-15

ADR-0146 made a project a folder marked by `project.voe3d`, and planned it around the absence of
typing: `voe_editor new <folder>` on the command line, a project name written into the file,
New and Open from inside the editor parked on D-256, and the project file's code in `editor/`.
Spec 004, approved by the sponsor, replaces that plan's surface: New, Open and Save are in a top
bar with the editor's own file browser, the first Save makes the project, the name is the
folder's, and the last project reopens. ADR-0146 cannot be edited (`CLAUDE.md`, *Never touch*), so
this record states which of its points no longer hold.

## Decision

**Kept from ADR-0146:** points 1, 2 and 6 — the fixed file name, the authored text format, and
`dev_editor` as a project rather than a program.

**Replaced:**

- Point 3's name. **`project.voe3d` holds one section, `[project]`, with one key, `scene`**: the
  scene file's path relative to the project folder, `/`-separated, never absolute and never
  climbing out with `..`. **A project's name is its folder's name**, read from the path, never
  stored — a name in the file would disagree with the folder the first time someone renames it
  outside the editor.
- Point 4. There is no `voe_editor new`. `voe_editor <folder>` opens a project and, when it
  cannot, prints the reason and exits non-zero without opening a window. With no argument the
  editor opens the last project, or an untitled scene.
- Point 5. New and Open are in the editor, through its own file browser; D-256 is closed except
  for a recent-projects list, which the spec puts later.
- Point 7. **The project file's reader and writer live in `authoring`**, beside the scene's.
  `editor` is a program and has no tests (ADR-0121); `authoring` is where the authored-text code
  is tested, and the cook is the second caller ADR-0146 already names — ADR-0151 point 4's
  condition, met by the editor having no tests and the cook being decided.

**The scene file is `main.scene`**, beside `project.voe3d`, written by the first save. Its name is
the editor's choice at that save; a reader follows the `scene` key and assumes no name.

**The last project is per person, never inside a project**: one line, the project folder's
absolute path, in `voe3d/last_project` under the person's settings folder (`platform/folder.h`).
It is written when a project is opened or first saved, and never by `--capture` or by a failure.

## Rejected

- Keeping `name` in `project.voe3d` — two sources for one fact, and the spec names the folder as
  the source.
- The project file in `editor/` — untested code for a format other people's folders depend on.
- `scene.voe3d` as the scene's file name — two files ending `.voe3d` in one folder, one of which is
  the marker, invites opening the wrong one.
- A last-project list in the authored text format — the editor does not name `assets`, and a
  settings file with one line needs no sections.

## Consequences

- Editor-folder text that cites ADR-0146 for `voe_editor new` or the name key is wrong from the
  task that changes it, and is rewritten there.
- A project folder moved outside the editor keeps opening, under its new name.
- The cook will read `scene` from `authoring` and needs nothing new to find a project's scene.
