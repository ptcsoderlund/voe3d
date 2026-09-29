# 39 — A view with no light is lit by the preview light
folder: editor
after: 37, 38
decisions: 0168, 0287, 0289, 0290

## Change
Bug 04, the editor half. After card 37, `voe_3d_draw_system_light` answers a lightless world with
a zeroed (black) light. Every view and the camera preview take their light from one function,
`voe_editor_view_light` (`editor/src/view.c`, called once a frame in `main.c`), which hands it to
`voe_editor_view_passes_preview` and `voe_editor_view_passes_draw`; those already cast shadows
from the light they are handed. So the fix is that one function; `main.c` and
`view_passes.c` do not change.

- `editor/src/project.c` / `project.h`: add `voe_render_light voe_editor_project_preview_light(void)`:
  the untitled scene's light as `render` takes it. Direction
  `voe_scene_light_direction(voe_scene_light_facing({LIGHT_X, LIGHT_Y, LIGHT_Z}))`; colour,
  intensity and fill (fill colour times fill intensity) from the same `voe_scene_light` value
  `voe_editor_project_new_untitled` adds to `Light`. Make that value one file-scope constant (or
  one static function) both use, beside the `LIGHT_*` macros, so the two cannot drift (0289).
  Header points: it is the preview light of 0287/0289; it is never an entity, never saved; it is
  one set of values with the untitled light, so changing one changes the other.
- `editor/src/view.c` / `view.h`: `voe_editor_view_light` returns
  `voe_editor_project_preview_light()` when `voe_scene_light_count(world)` is 0, and
  `voe_3d_draw_system_light(world)` otherwise (0290 point 4); include `project.h` and
  `scene/light_system.h` as needed. Its comment: a world (level or open prefab) with no light is
  shown by the preview light, shadows included, and a light added replaces it at once; Play and
  the game draw such a world black (0287). The header paragraph "THE LIGHT IS THE OPPOSITE" gains
  the no-light case. Drop the 0238 wording. Say "directional light" in lines you touch (0288).
- `editor/src/view_passes.h`, first paragraph: "the world's light" becomes the light
  `voe_editor_view_light` gives, the world's or the preview.
- `editor/src/src.md`: the `view.h` / `view.c` and `project.h` / `project.c` entries mention the
  preview light only if they fit the cap; otherwise leave them.

## Done when
`grep -q voe_scene_light_count editor/src/view.c && grep -q voe_editor_project_preview_light editor/src/project.h`
exits 0, and `! grep -rq 0238 editor/src`.

For the human, bug 04's steps: open `examples/tank_game` in the editor, open `tank_body.prefab`
from the Assets panel. The views show it shaded and casting shadows, as the untitled scene's cube
is lit. Back in the level, delete its light: the views look lit the same way. Press Play: the level
is black, the GUI still shows. Stop, undo: the level's own light is back.
