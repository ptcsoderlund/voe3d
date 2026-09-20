# 05 — The draw system outlines one entity
folder: 3d
decisions: 0168, 0177, 0203

## Change
A pass may outline one entity, the way it may already hide one.

`3d/include/3d/draw_system.h`:

- Include `<3d/outline.h>`, and give `voe_3d_frame` one more field: `voe_3d_outlined outlined;`, with a comment
  saying a zeroed `entity` outlines nothing, that it is one entity and not a list for the same reason `hidden`
  is one, and that a caller that never sets it loses nothing.
- A paragraph of the header, beside the one about the two layers: A PASS MAY OUTLINE ONE ENTITY, AND IT IS
  DRAWN AFTER EVERYTHING ELSE (0203). Why it is after the overlay's own groups and therefore after the depth
  clear — that is the whole of what makes the outline show through something standing in front of the entity,
  and the entity itself is drawn where it really is, in its own layer, untouched. That the quads come from
  `voe_3d_outline_quads` and go into this frame's transient pool, so a program that outlines anything sizes
  the three transient numbers of its `voe_render_capacities` from `VOE_3D_OUTLINE_VERTICES` and
  `VOE_3D_OUTLINE_INDICES` and counts one more object per pass. That the record is the caller's unlit one
  (`voe_3d_shapes.outline`) and the colour the caller's, because whose theme it is drawn in is not this
  folder's business. And that a refused transient range draws no outline, says so on stderr and leaves the
  frame alone, which is the rule every other draw in here already follows.

`3d/src/draw_system.c`, at the end of `voe_3d_draw_system_run` only, after the overlay's blended group and
before the arena is rewound: when `frame.outlined.entity` is live and `voe_3d_outline_quads` answers true for
it — handed `frame.view`, the run's own arena and the frame's `outlined` — upload the mesh with
`voe_render_geometry_create_transient` and issue one `voe_render_frame_draw` with a `voe_render_object` whose
`world` and `normal` are both the identity (the quads are already in world space), whose `shading` is
`frame.outlined.material.shading.index` — the same field the walk already takes off a material row — and whose
`colour` is `frame.outlined.colour` with alpha 1.

`3d/tests/outline.c` gains a second half, which skips without a graphics card, as `3d/tests/draw_system.c`'s
does. A headless device sized with the transient numbers above, a target, a world with a light, a grey cube at
the origin and a second cube at (0, 0, 2) between it and the camera, and a camera entity looking down −Z from
(0, 0, 5):
- one frame drawn with `frame.outlined` zeroed, read back with `voe_render_target_read` (ADR-0177): no pixel of
  the picture is (255, 0, 255);
- one frame drawn with `frame.outlined.entity` naming the **back** cube, a colour nothing else in the picture
  can be — linear (1, 0, 1), which an sRGB target hands back as the bytes (255, 0, 255) exactly, because nought
  and one are the two values the curve leaves alone — and `pixels` 3: more than a hundred pixels are those
  bytes, and at least one of them lies inside the rectangle the front cube covers, which is what "it shows
  through what is in front of it" means;
- the same frame with the front cube's entity outlined instead puts its pixels round that cube's own outline
  and not round the back one's — take the leftmost outlined pixel in each of the two frames and they differ.

`3d/tests/tests.md`'s line for `outline.c` says the first half needs no graphics card and the second skips
without one.

## Done when
`checks.sh 3d` exits 0 with both halves of `3d/outline` running on this machine's card and passing.
