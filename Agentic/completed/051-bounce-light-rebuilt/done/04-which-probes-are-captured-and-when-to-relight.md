# 04 — Which probes are captured, and when the grid relights, on the CPU
folder: render
after: 03
decisions: 0168, 0326

## Change
0326 points 2, 4 and 6 (its last sentence), as a pure-CPU module with no Vulkan, beside
`render/src/bounce_schedule.h` (read its header for the shape; that module is 046's and stays
until card 14 removes it). Nothing calls the new one yet.
- `render/include/render/device.h`: beside the old `VOE_RENDER_BOUNCE_*`,
  `VOE_RENDER_BOUNCE_PROBES_XZ` 24, `VOE_RENDER_BOUNCE_PROBES_Y` 12, `VOE_RENDER_BOUNCE_CAPTURE`
  16 (probes a capture pass), `VOE_RENDER_BOUNCE_CAPTURE_PASSES` 4 (a frame),
  `VOE_RENDER_BOUNCE_LAMPS` 16, each with a one-line comment naming 0326.
  `VOE_RENDER_BOUNCE_SPACING` stays 2 m and serves both.
- `render/src/bounce_probes.h` and `.c`, new:
  - a probe's toroidal index from a world cell: each axis wrapped into 0..size − 1 (negative cells
    too), x + 24·y + 288·z; one inline wrap per axis, the only place a cell becomes a probe
    coordinate;
  - `voe_render_bounce_probes`, one grid's state, zeroed is "nothing yet": the last lowest cell,
    a bit per probe for "holds a picture", a bit per probe for "queued for capture", a bit per
    probe for "captured or emptied since the last relight", and the bouncing lights last relit
    (sun record, its bounces and strength, up to `VOE_RENDER_BOUNCE_LAMPS` lamp records);
  - a place call: the new lowest cell and `corner` about the eye, stale spheres (xyz about the
    eye, w radius), the sun record, its bounces and strength, and the frame's point lights. Probes
    a move brings in (all on the first call or a jump of a whole grid) lose their picture, are
    queued and marked changed; probes whose centre lies in a stale sphere are queued. Picks the
    bouncing lamps: the first 16 of the point lights with bounces ≥ 1, into the caller's array;
  - a take call: up to `room` queued probes, nearest the eye first (centre at the cell's middle),
    unqueued, marked holding a picture and changed; returns the count, 0 when none is queued;
  - a relight-needed call: true when any probe is marked changed or the lights differ from the
    last relit; and a relit call that clears the marks and keeps the lights.
- `render/tests/bounce_probes.c`, new, no card: the first place queues all 6912; a take of 16
  gives the 16 nearest the eye; a move of one cell along +x queues 288, keeps the rest; cell −1
  wraps to 23; a stale sphere of 6 m queues only probes within it; nothing changed and the same
  lights: no relight needed; a lamp's strength changed: needed; the 17th bouncing lamp is left out.
- `render/src/src.md`, `render/tests/tests.md`: entries.

## Done when
`ctest --test-dir build/debug -R "^render/bounce_probes$"` passes.
