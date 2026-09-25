# 07 — A project swaps its world for one made with a newly loaded library
folder: editor
decisions: 0168, 0008, 0241, 0242

## Change
Points 5 and 6 of 0242: loading a library and the swap, with nothing yet calling them (card 09).
Read the headers of `editor/src/project.h`, `editor/src/world_step.h` (card 06),
`platform/library.h`, `platform/file.h`, `platform/folder.h`, `game/project.h`,
`authoring/scene_read.h` and `authoring/scene_write.h`.

- `editor/src/code.h`, `editor/src/code.c` (new) — a loaded project library:
  - `voe_editor_code` {library (`voe_platform_library *`), register (the resolved
    `voe_game_project_register`, cast back from `voe_platform_symbol`), path of the loaded copy};
    zeroed is no code.
  - `bool voe_editor_code_open(const char *built, const char *folder, uint32_t load,
    voe_base_arena *, voe_editor_code *out, voe_editor_notice *why)` — creates
    `<folder>/Build/editor/loaded/`, copies `built` to `project-<load>.so` there (`.dll` on
    Windows), opens the copy, resolves the entry point; a failure closes what it opened and `why`
    names the step and, for the open, the report card 01 made.
  - `bool voe_editor_code_same(const char *built, const voe_editor_code *, voe_base_arena *)` —
    the built file's bytes equal the loaded copy's.
  - `void voe_editor_code_close(voe_editor_code *)`.
  The header: why a copy (Windows locks, glibc returns the old handle for an open path), the
  one-lookup seam of ADR-0008, that a code is closed only after every world it registered into.
- `editor/src/project.h`, `editor/src/project.c`:
  - `world_arena` (new field): the world lives in an arena of its own, made where the world is now;
    `code` (a `voe_editor_code`, zeroed while none).
  - A static world maker: `voe_game_world_new`, then `code.register` when there is one. Untitled
    and opened use it; `scene_set` keeps re-reading into the same world, as now.
  - `bool voe_editor_project_code_set(voe_editor_project *, voe_editor_code, voe_editor_notice *why)`
    — point 6: the text (kept sections included) into a scratch arena; a new world arena and scene
    arena; the new world made with the new code; the text read in; then the old world arena and
    scene arena destroyed, the old code closed, `world`, `kept`, `code` set. A refused read destroys
    the new arenas, closes the new code, and leaves the project as it was. `unsaved` untouched.
    Every entity handle is stale after; the header says the caller re-finds by authored id.
  - `voe_editor_project_destroy` closes the code after the arenas. The header's world paragraph,
    the new field comments and the swap's contract.
- `editor/src/world_step.c`, `editor/src/world_step.h` — `voe_game_project_replaces_apply` right
  after the structural queue; the header says so.
- `editor/src/src.md` — entries for code.h and code.c; project's if its sentence changes.

## Done when
1. In `p=$(mktemp -d)` with `cp -r game/example/. $p`, a capture before this card and one after
   (`build/debug/editor/voe_editor --capture <png> $p`) are equal under `cmp`.
2. `checks.sh --folder editor` prints `FINDINGS: 0`.
