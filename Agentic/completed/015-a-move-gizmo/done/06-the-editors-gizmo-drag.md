# 06 — The editor's gizmo drag
folder: editor
decisions: 0168, 0205, 0207

## Change
New `editor/src/gizmo.h` and `gizmo.c`: what the primary button does to the selected entity's move gizmo.
Read `editor/src/pick.h` — this is the same shape of file, reads the same button, takes the same `blocked`
cases, and is asked first so a press it takes is one `pick.h` never sees. Nothing is called yet; card 07
wires it in.

```c
typedef struct {
	bool pointer_was_down;
	voe_3d_gizmo_handle held;     // NONE when no drag is running
	voe_3d_gizmo_handle hovered;  // what the pointer is over this frame
	uint32_t captured;            // the view the drag started in; read only while held
	uint32_t hovered_view;
	voe_math_float3 grab;         // where on the handle the press landed
	voe_math_float3 start;        // the entity's position then
	uint32_t moved;               // moves submitted this frame; zeroed every read
} voe_editor_gizmo;

void voe_editor_gizmo_read(voe_editor_gizmo *gizmo, voe_editor_scene *scene,
			   const voe_editor_views *views, float pixels,
			   voe_math_float2 pointer, bool down, bool blocked);
bool voe_editor_gizmo_taking(const voe_editor_gizmo *gizmo);
voe_3d_gizmo_handle voe_editor_gizmo_marked(const voe_editor_gizmo *gizmo, uint32_t view);
```

One read a frame, at `pick.h`'s place in the loop. `pixels` is the shaft's length on the picture, the same
number the pass is handed, so what is hit is what is drawn. The ray is `voe_3d_pick_ray` from the view's own
camera and picture size, exactly as `pick.c` builds it; the gizmo is `voe_3d_gizmo_at` from that view's
`voe_editor_view_pass_camera(view, (voe_render_light){ 0 }).view` — the light is nothing to a gizmo — at the
selected entity's `voe_scene_transform_get` position.

The rules: no selection, no transform, or `blocked`, and there is no hover and no press to take, though the
button is still remembered. Not dragging, the pointer's view and handle are found and kept as `hovered`. A
press edge on a hovered handle captures that view, keeps `held`, `grab` and `start`; a refused grab captures
nothing. While held and still down, the ray is met against the handle taken through `start` — not through
where the entity has got to — and the entity's transform is submitted with `start` plus the travel as its
position, through `voe_scene_transform_submit` off the row as it is, counting one in `moved` when the
position actually changed. The release ends it. A drag goes on in its captured view after the pointer has
left it, as the views' own drag does.

`_taking` is true from the press that lands on a handle until the release, which is what stops a click on a
handle from selecting. `_marked` is `held` for the captured view, `hovered` for the hovered view when nothing
is held, and NONE elsewhere — one view shows the marking at a time.

The header makes these points: why the middle button is never read in here, in `pick.h`'s words; why the drag
is measured from the press position rather than the moving one; why the world's axes and not the entity's
(`feature.md`, a later toggle); why a whole transform is submitted and not a delta, drained a frame later
rather than written into the table (`scene/transform_system.h`); that a zeroed struct holds nothing, because
`VOE_3D_GIZMO_NONE` is nought and `captured` is read only while `held`; and that `moved` is what tells the
caller an edit reached the project.

`editor/src/view.h` and `view.c` gain `voe_math_float3 voe_editor_view_gizmo_colour(const voe_ui_theme
*palette, bool marked)`: the outline's colour (0203) as it stands for a marked handle, and multiplied by a
fixed fraction under one for one at rest (0207), with the reasoning in its comment.

`editor/src/src.md` gains the two files; `view.h`'s and `view.c`'s entries gain the gizmo's colour.

## Done when
`checks.sh --folder editor` exits 0 and the editor still starts, draws and selects exactly as before — this
card adds a file nothing calls yet.
