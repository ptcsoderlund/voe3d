# 05 — The scene.h and undo.h entries in src.md fit the cap
folder: editor/src
after: none
decisions: 0168

## Change
`editor/src/src.md`: the entries for `scene.h` (342 characters) and
`undo.h` (323) are over the 300-character entry cap. Shorten each to one
sentence that says what the file is for:
- `scene.h`: the project's world and selection, and the Scene panel and
  Inspector state built on them (rows, folds, drag, picker, gizmo mode,
  the reveal of a selection made elsewhere).
- `undo.h`: the line of whole-scene texts that Ctrl+Z and Ctrl+Y step
  through (ADR-0204), with the level's line set aside while a prefab is open.

Open the header comments of `editor/src/scene.h` and `editor/src/undo.h`.
Every point dropped from an entry (Add entity, Delete, Duplicate, the
colour picker and dropdown targets, the drag's threshold, target and
cancel; an edit marked, a settled edit recorded, a settled reveal amending
the current state) must be in that file's header; add any that is missing.
No code changes; no other entries change.

## Done when
`checks.sh --folder editor/src` reports no entry-cap finding for `scene.h`
or `undo.h` in `editor/src/src.md`.
