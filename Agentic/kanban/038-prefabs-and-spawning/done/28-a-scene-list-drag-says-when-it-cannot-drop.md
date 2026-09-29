# 28 — A Scene list drag says when it cannot drop
folder: editor
after: none
decisions: 0168, 0282, 0285, 0286

## Change
Bug 02: a Scene list drag over a place that will not take it looks the same as over one that
will. Make one ghost function every drag uses, and give the Scene list its refused answer.

- New `editor/src/drag_ghost.h` and `drag_ghost.c`:
  `void voe_editor_drag_ghost_draw(voe_ui_context *ui, const voe_ui_theme *dim, const char *name, bool refused, voe_math_float2 at);`
  The anchored raised panel of `name` a few mm right of and below `at`, taking no pointer
  (what `voe_editor_scene_list_ghost_draw` in `scene_list.c` draws today, moved here); when
  `refused`, drawn under the pushed `dim` theme with a second label "Can't drop here" below the
  name. Header points: every editor drag's ghost is this one (0285, 0286 point 1); `name` and
  `dim` must outlive the frame; called in the root surface.
- `editor/src/prefabs.h`, `prefab_make.c`: add
  `[[nodiscard]] bool voe_editor_prefab_make_refused(const voe_editor_project *project, voe_ecs_entity root, voe_editor_notice *why);`
  true with why for every refusal `voe_editor_prefab_make` has except the file already
  existing (untitled, no identity, a prefab open, `voe_editor_prefab_refused`);
  `voe_editor_prefab_make` calls it first instead of its own checks. Header: why the file test
  is left out (0286 point 5). Say whether `why` may be NULL.
- `editor/src/scene.h`: a `list_refused` field beside `list_target`, zeroed with the other
  `list_` fields; its comment says it is the release's own answer, drawn a frame late.
- `editor/src/scene_list.h`, `scene_list.c`: `voe_editor_scene_list_drop` gains
  `bool assets_take` after `over_assets`; while dragging it sets `list_refused` when there is no
  row target, no heading target and not (`over_assets` and `assets_take`). The release itself
  does not change. `voe_editor_scene_list_ghost_draw` keeps its alive-and-identity check and
  now calls `voe_editor_drag_ghost_draw` with the entity's name, `&scene->list_dim` and
  `list_refused`. Header: the ghost's dim and second line join the three marks; what refused
  means.
- `editor/src/interface.c`: at the `voe_editor_scene_list_drop` call, `assets_take` is
  `!voe_editor_prefab_make_refused(project, scene->list_held, <notice or NULL>)` for the
  session's project; header's drop paragraph says so.
- `editor/src/interface.h`: the Scene list drag marks paragraph counts the ghost's second label
  (and any column holding the two lines) as nodes and "Can't drop here"'s drawn characters as
  elements; `VOE_EDITOR_INTERFACE_NODES` and `_ELEMENTS` raised by the same.
- `editor/src/src.md`: entries for `drag_ghost.h`/`.c`; `scene_list.c`, `prefabs.h` and
  `prefab_make.c` entries amended as short phrases.

Read only these files and `ui/include/ui/widgets.h` / `ui/include/ui/layout.h` headers for
the calls the ghost already uses.

## Done when
- `grep -c "Can't drop here" editor/src/drag_ghost.c` prints 1 and
  `grep -c "voe_editor_prefab_make_refused" editor/src/prefab_make.c` prints at least 2.
- Human: open `examples/tank_game`, drag a thing from the Scene list over the Inspector and
  over a view: the ghost is dimmed and says "Can't drop here"; over another row or the heading
  it looks normal and the rim shows; over the Assets panel it looks normal.
