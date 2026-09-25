# 15 — dev places things at double positions and draws about the eye
folder: dev
decisions: 0168, 0250, 0254, 0177

## Change
`dev` mends what cards 03, 08 and 09 broke, and nothing more (ADR-0113). dev steps nothing.

- `dev/src/facing.c` — `voe_scene_transform_matrix` takes an origin (card 03): the basis is the
  eye's rotation, so pass the eye's own position; the placed `at` is the eye's double position
  plus the float offsets (`math/double3.h`: `from_float3`, `add`).
- `dev/src/cubes.c`, `model.c`, `text.c`, `motion.c`, `quad.c`, `sprites.c` — every position
  written or read as a `voe_math_double3`; float offsets widened with `from_float3`.
- `dev/src/main.c` — `voe_3d_draw_system_frame(world, size, 0.0f)`.
- `dev/src/monitor.c` — the frame it builds by hand sets `eye` to the monitor pose's position
  and `lag` 0.
- `dev/src/src.md`, the touched files' headers — only where one says a position is float.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder dev` prints `FINDINGS: 0`.
2. `cmake --build --preset debug --target voe_dev` exits 0.
