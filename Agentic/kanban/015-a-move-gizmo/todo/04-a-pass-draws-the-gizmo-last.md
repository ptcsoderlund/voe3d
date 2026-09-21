# 04 — A pass draws the gizmo last
folder: 3d
decisions: 0168, 0177, 0205

## Change
`3d/include/3d/draw_system.h` and `3d/src/draw_system.c`: a pass may draw one move gizmo, and it is the last
thing in the frame. Read draw_system.h's paragraphs on the overlay's depth clear and on the outline, and
follow the outline's own draw in draw_system.c — this is the same shape of thing one step further on.

`voe_3d_frame` gains a field beside `outlined`:

```c
typedef struct {
	voe_ecs_entity entity;        // where the gizmo stands; zeroed for none
	voe_3d_material material;     // voe_3d_shapes' unlit outline record
	voe_math_float3 colour;        // the handles at rest
	voe_math_float3 marked_colour; // the hovered or held handle
	voe_3d_gizmo_handle marked;
	float pixels;                 // the arrow's shaft on the picture
	voe_platform_size size;       // that picture, in pixels
} voe_3d_gizmoed;
```

In the run, after the outline and after everything else: when `gizmo.entity` is live and has a transform,
`voe_render_frame_clear_depth` a second time, then `voe_3d_gizmo_at` with the frame's own view, the field's
`pixels` and `size`, then `voe_3d_gizmo_quads` into the frame's arena, then a transient range and one draw
per non-empty mesh — the plain one in `colour`, the marked one in `marked_colour`. The world matrix is the
identity and the normal matrix with it: the vertices are already in world metres, exactly as the outline's
are. A refused transient range draws no gizmo and leaves the rest of the frame alone, as the outline's does.

That second clear is what puts the gizmo in front of the outline, which is otherwise in the same depth buffer
and cuts across an arrow that stands in front of it.

The header gains: that a gizmo is drawn after the outline behind a clear of its own and why; that it is one
entity and not a list, for the same reason `outlined` is; that a zeroed entity, a dead one and one without a
transform each draw nothing, which is what no selection looks like; that the two colours and the marked
handle are the caller's, because whose theme a gizmo wears is the editor's business; and what a program adds
to its `voe_render_capacities` for one — `VOE_3D_GIZMO_VERTICES` and `_INDICES`, two transient ranges and two
objects per pass. `voe_3d_draw_system_frame` hands the field back zeroed, as it does `outlined`.

`3d/src/src.md`'s `draw_system.c` entry gains the gizmo.

## Done when
`checks.sh --folder 3d` exits 0 and `ctest --test-dir build/debug -R "^3d/draw_system$"` passes with one
claim added, skipped without a graphics card: a frame drawn into a small target with a shaped cube at the
centre, its own colour dark, and a gizmo on that cube in a bright colour reads the bright colour at a pixel
the arrow along +X covers and that the cube alone would have covered — the gizmo shows through the thing it
stands in. The same frame with `gizmo.entity` zeroed reads the cube's colour at that pixel.
