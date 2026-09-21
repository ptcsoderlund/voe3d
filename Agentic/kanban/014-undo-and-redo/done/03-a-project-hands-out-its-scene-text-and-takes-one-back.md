# 03 — A project hands out its scene text and takes one back
folder: editor
decisions: 0168, 0190, 0204

## Change
Undo is a line of scene texts (0204), and this card gives `project.h` the two calls that line is made of.
Read `editor/src/project.h`'s header first; the two points below join it.

`editor/src/project.h` and `project.c`:

- the struct gains `voe_base_arena *scene_arena` — what a scene read owns, and nothing else: the kept
  sections, and the read's own working memory. Made with a block size of its own in both constructors,
  destroyed by `voe_editor_project_destroy` beside the project's arena and on `new_opened`'s failure paths.
- `voe_editor_project_new_opened` passes `scene_arena`, not the project's arena, to
  `voe_authoring_scene_read`. Everything else it pushes — the absolute folder, the files' bytes, the world —
  stays where it is.
- new: `bool voe_editor_project_scene_text(const voe_editor_project *project, voe_base_arena *arena,
  voe_authoring_text *out)` — the world and `project->kept` written as scene text into the caller's arena
  through `voe_authoring_scene_write`; false and reported when the writer refuses. `voe_editor_project_save`
  goes through it instead of calling the writer itself, on its own scratch as now.
- new: `bool voe_editor_project_scene_set(voe_editor_project *project, const char *text, size_t size,
  voe_editor_notice *why)` — the world made into what that text says: every entity that has a
  `voe_scene_identity` destroyed through `voe_ecs_structure_destroy` (walk `voe_scene_identity_entities` and
  `voe_scene_identity_count`), then `voe_ecs_structure_apply`, then `voe_base_arena_clear` on `scene_arena`,
  then `voe_authoring_scene_read` into the same world with that arena, and `project->kept` set from it. True
  with the world holding exactly what the text says. False with `why` filled from the report.

The header's new points: why a scene read owns an arena of its own — a re-read's kept sections cannot be
pushed on top of the last read's for ever, and the world's tables were allocated before the first of them, so
clearing this one is always right; that `scene_set` is the one call in this program that empties a world, and
that it leaves every registration, every table and the project's folder and unsaved flag exactly as they were
— it is not New; that no entity handle survives it, so a caller holding one re-finds what it wants by
authored id; that it is called between frames, before the structural queue is applied and the systems run;
and that a refusal can only be a text this program did not write itself, a world's own text always fitting
back into the world it came from, so the world is left half-loaded and the caller says so rather than
carrying on as though nothing happened.

`editor/src/src.md`: `project.h`'s and `project.c`'s entries say the scene handed out as text and read back
in, and the arena a read owns.

## Done when
`checks.sh --folder editor` exits 0, and a project with a section this program registers no type for still
survives a trip through it: make `<scratch>/p/project.voe3d` (`[project]`, `scene = "main.scene"`) and a
`main.scene` with one `[1]` entity, its `[1.voe_scene_transform]`, and a `[1.game_health]` section holding
`points = 3`; `voe_editor <scratch>/p --capture <scratch>/p/f.png` writes a picture with no complaint about
the file.
