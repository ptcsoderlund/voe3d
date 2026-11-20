# 0511 — Docs is written from the readers' own tables, and `voe_editor --check` reads strictly
date: 2026-11-20
by: planner

## Decision
For 116, carrying out 0505 and 0506:

1. **Every format publishes a key table beside its reader.** One row type, `voe_assets_sectioned_key`
   in `assets/sectioned.h`: section, key (or a pattern such as `layer<N>`), a kind phrase, the default
   as spelled in the file ("" when required), least and greatest (both 0 for no range), unit, whether
   the strict read refuses a value outside the range, and one line of meaning. A table is reached through
   a function per format (`voe_assets_material_keys`, `_parts_keys`, `_landscape_keys`,
   `voe_authoring_project_keys`), never extern data (0284). Its ranges are the owner's named constants,
   and the editor's panels clamp to the same constants, so the two cannot drift. Scene components need
   no table: their descriptions (0498) already say it.
2. **The reference text is written by `authoring/reference.h`**: one Markdown text for the scene from the
   world's registered, non-runtime-only types (fields, kinds, ranges and units, names, what a missing key
   reads as, former names, needs, menu), and one per key table. Ranges are in the file's units.
3. **The editor owns exactly three files in `<project>/Docs/`:** `voe3d.md` (the index: which file kind is
   documented where, the check command written out whole with the editor's absolute path, its output and
   exits, where the engine headers are), `voe3d-scene.md` and `voe3d-files.md`. It writes them on every
   open, every Refresh and every code reload, each only where its bytes differ, and touches nothing else
   in `Docs/`. It never writes nor names an agent instruction file.
4. **The check is `voe_editor --check [<folder>]`**, folder defaulting to the current one, run with no
   window and no device. It walks what the watch walks (0509) and reads each file with the strict reader
   the editor uses for that kind; scene and prefab against the engine's types plus the project's code
   when `Build/editor` holds a built library. One stdout line per refused file, `<path>: <first error>`,
   then a summary. Exit 0 all well, 1 any refusal or not a project, 2 usage. A project whose `Code/` is
   not built is checked without its own components, and the check says so.
5. **Examples' `Docs/` is ignored by git**: it holds machine-local paths.

## Reasoning
One source per fact: the reader's table and the description are what the strict read enforces, so a
document generated from them cannot promise a key the reader refuses. Function accessors keep the tables
importable from the DLL on Windows. Writing only differing bytes keeps the files' stamps still when nothing
changed. The check reuses the editor's own readers rather than a second validator; first error per file is
what those readers report, and an agent fixes and reruns. The check lives in the editor binary because the
editor is the program the project already has, and its path is written into `Docs/voe3d.md`.

## Replaces
None.
