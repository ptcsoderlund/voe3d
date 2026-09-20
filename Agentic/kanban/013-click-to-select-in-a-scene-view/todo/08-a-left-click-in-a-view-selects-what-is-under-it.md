# 08 — A left click in a view selects what is under it
folder: editor
decisions: 0168, 0193, 0202
read: feature.md

## Change
The editor's half of picking: one file that turns a left press over a scene view into a selection, and the
four lines in the loop that call it.

`editor/src/pick.h` — new:

```c
// A left click in a scene view selects the frontmost entity under the pointer,
// and a click on nothing clears the selection — the same selection a row in
// `Scene` moves (scene.h). The ray, and what it meets, are `3d`'s (3d/pick.h,
// 0202); what is here is the press edge and which view was clicked.
typedef struct {
	// Last frame's primary button, so a press is found as an edge, exactly
	// as voe_editor_scene's own pointer_was_down is.
	bool pointer_was_down;
} voe_editor_pick;

// This frame's press, once a frame, beside voe_editor_views_drag. `pointer` is
// in the root surface's millimetres and `down` is its primary button; `blocked`
// says the press is not the views' to take. Does nothing at all while `blocked`
// and still remembers the button, so a press that started over something else
// never selects when it is released over a view.
void voe_editor_pick_read(voe_editor_pick *pick, voe_editor_scene *scene,
                          const voe_editor_views *views,
                          const voe_3d_shape_geometries *geometries,
                          voe_math_float2 pointer, bool down, bool blocked);
```

Its header says: why the middle button is never read here — the middle button is the views' camera and the left
is the interface's, which is main.c's standing division, and orbiting must never change what is selected; what
`blocked` is for a caller (the browser showing, Preferences showing, or the colour picker open — each of them
paints over the views or takes the press for itself, and `feature.md` asks for the first two by name); why a
press and not a release is what selects (it is where `ui`'s own widgets act and where the Scene panel's rows
do); and that a click over no view at all leaves the selection alone, while a click in a view that meets
nothing clears it — empty space in a view is an answer, the rest of the editor is not.

`editor/src/pick.c` — new: the edge (`down && !pick->pointer_was_down`), then `voe_editor_views_under`
(card 07), then `voe_3d_pick_ray(view->camera, (voe_platform_size){ (int)view->width, (int)view->height },
point)` and `voe_3d_pick(scene->world, geometries, ray, NULL)`, then `voe_editor_scene_select` (card 06) with
whatever came back. `pointer_was_down` is set from `down` on every call, blocked or not.

`editor/src/main.c`, four places and nothing else:

- beside `views` and `browser`, a `voe_editor_pick pick = { 0 };` and a `voe_3d_shape_geometries geometries;`
  filled once at startup with `voe_3d_shape_geometries_create(arena, &geometries)` — the kept arena, the one
  everything else in this program lives in, because the store outlives every frame;
- in the loop, straight after the `voe_editor_views_drag` call, one `voe_editor_pick_read` with
  `roots[0].pointer.at`, `left && pointer.over` and
  `browser.showing || preferences.showing || scene.picking.open` as `blocked`;
- `EDITOR_CAPACITIES`' `.objects` becomes `(VOE_EDITOR_PROJECT_MAX_DRAWN + 1) * VOE_EDITOR_VIEWS`, the one more
  being the outline card 09 draws into every view's pass, and the comment above it says so;
- the file header's paragraph about the pointer's division gains a sentence: a left press over a view picks
  what is under it through pick.h, on the same millimetres the drag is handed, and the middle button still
  moves the camera and selects nothing.

`editor/src/src.md` gains `pick.h` and `pick.c`; `editor/editor.md`'s paragraph about the views says a left
click in one selects the frontmost entity under the pointer and a click on empty space clears the selection,
and that neither happens while the browser or Preferences shows.

## Done when
`checks.sh editor` exits 0, and at a running `voe_editor` on a project with two or three shapes: a left click
on a shape in either view marks its row in `Scene` and fills the Inspector; a click on empty space empties
both; a middle-button drag that starts over a shape orbits and leaves the selection as it was; and with the
browser open (Open) a click in a view behind it changes nothing.
