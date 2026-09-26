# 06 — The draw system casts the sun's shadows
folder: 3d
decisions: 0168, 0258, 0238, 0254

## Change
The call a loop makes between its frame's begin and a view's pass (0258 points 5–6). A caller
that does not make it draws as before, with no shadow.

- `3d/include/3d/draw_system.h`:
  - `voe_3d_frame` gains `voe_render_shadow shadow`; `_frame` leaves it zeroed. Field comment:
    zero is none; `_shadows` fills it; handed to the pass beside `view` and `light`.
  - `[[nodiscard]] bool voe_3d_draw_system_shadows(voe_ecs_world *world, voe_render_device
    *device, voe_3d_frame *frame)`: with the light shaded and the frame not blind, fits the
    cascades to `frame->view`, `frame->eye` and the light's direction at `VOE_3D_SHADOW_TEXELS`
    (card 05), sets `frame->shadow`, and opens one shadow pass per cascade drawing every caster
    at the frame's lag; otherwise leaves `shadow` zeroed and opens nothing. False when a pass or
    a draw is refused, with render's line; `shadow` is then zeroed so the view draws unshadowed.
    Asserts on a pass open. Paragraph: casters are world-layer, lit, opaque or cutout meshes
    with a transform, `hidden` left out; panels and marks cast nothing; the device must have
    `shadow_size` and room for 4 passes and 4 × drawn objects more per view.
  - The usage block at the top: `_shadows` after begin, the pass camera `{ frame.view,
    frame.light, frame.shadow }`; the phase-order paragraph names the shadow passes.
- `3d/src/draw_shadows.c` (new) — the function above, walking the mesh table as `_run` does and
  building each caster's record with `draw_group.c`'s record builder (the object's matrix about
  the eye, at the lag). Header: why this is its own call and not inside `_run` (a pass does not
  nest), and why casters are chosen as they are (0258).
- `3d/src/draw_group.h` / `draw_group.c` — only if the record builder is not yet reachable
  from another file: declare it there, no behaviour change.
- `3d/src/src.md` — entry for `draw_shadows.c`.
- `3d/3d.md`, `3d/include/3d/3d.md` — `draw_system.h`'s entry in each mentions the sun's shadow passes.
- `3d/tests/shadows.c` (new) — headless device with `shadow_size` `VOE_3D_SHADOW_TEXELS`: a
  world with a camera, a light straight down, a flattened cube as floor and a cube over it;
  `_frame`, `_shadows` true with `shadow.count` 4, the pass with the shadow record, `_run`,
  read the window: floor under the cube darker than floor beside it. The same world with no
  light: `_shadows` true, `shadow.count` 0, draw count unchanged by the call, floor pixels
  alike. The cube moved 100 km along X with the camera and floor: the same two floor pixels
  read as they did. Skips the drawn half without a graphics card.
- `3d/tests/tests.md` — entry for `shadows.c`.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder 3d` prints `FINDINGS: 0`.
2. `ctest --test-dir build/debug -R '^3d/'` passes, `shadows` among them.
