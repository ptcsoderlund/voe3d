# 04 — Render schedules probe refreshes
folder: render
after: 03
decisions: 0168, 0307, 0308

## Change
0308 points 3 and 5, pure CPU, no Vulkan. Read the bounce part of
`render/include/render/device.h` (card 03), `render/src/src.md` and
`render/tests/tests.md`; the other files are new.

- `device.h`: beside `VOE_RENDER_BOUNCE_TEXELS`, `VOE_RENDER_BOUNCE_PROBES`
  32 (a side), `VOE_RENDER_BOUNCE_SPACING` 2.0f (metres),
  `VOE_RENDER_BOUNCE_BUDGET` 4096 and `VOE_RENDER_BOUNCE_BLEND` 0.5f, each
  with a one-line comment; nothing uses them outside render yet.
- `render/src/bounce_schedule.h`, new, internal, header comment:
  - `typedef struct voe_render_bounce_schedule` — the last lowest cell
    (int32 [3]) and whether there is one, the cycle's position, the last
    light record (direction, intensity, colour).
  - `uint32_t voe_render_bounce_schedule_next(voe_render_bounce_schedule *s,
    const int32_t cell[3], float3 corner, const voe_render_light *sun,
    const float4 *stale, uint32_t stale_count, uint32_t *probes,
    float *blends, uint32_t room)` — `cell` the grid's lowest world cell,
    `corner` its lowest corner about the eye, `stale` spheres about the eye
    (xyz centre, w radius). Writes probe indices, toroidal (x mod 32 + 32 ×
    (y mod 32) + 1024 × (z mod 32) of the world cell), and each one's blend;
    returns the count. Order:
    1. cells the move from the last cell brings in, blend 1, all of them
       whatever the budget (the first call, or a move of 32+ cells on any
       axis, is the whole grid);
    2. cells whose centre lies inside a stale sphere;
    3. the cycle: a stride coprime with 32³ so neighbours spread; a changed
       light record restarts the cycle so the next eight calls cover the grid.
    Steps 2 and 3 blend `VOE_RENDER_BOUNCE_BLEND` and together stop at
    `VOE_RENDER_BOUNCE_BUDGET`; no index twice in a call; never past `room`.
- `render/src/bounce_schedule.c`, new, header comment.
- `render/tests/bounce_schedule.c`, new, cases:
  - the first call lists all 32768, blend 1, each once;
  - with nothing changed a call lists 4096, none twice, and 8 calls visit
    every cell;
  - a move of one cell in +x lists the 1024 entered cells first, blend 1;
  - a 3 m sphere at a cell centre lists that cell before any cycle cell;
  - after a changed light colour, 8 calls visit every cell;
  - `room` smaller than the entered cells truncates, never overflows.
- `render/include/render/render.md`, `render/src/src.md`,
  `render/tests/tests.md`: entries changed or added.

## Done when
The test `render/bounce_schedule` passes, and `render/bounce_map` still
passes, after the folder's build.
