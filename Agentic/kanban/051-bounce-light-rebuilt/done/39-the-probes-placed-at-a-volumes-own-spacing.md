# 39 — Probes placed at a volume's own spacing, and lamps compared without the eye
folder: render
after: 37
decisions: 0168, 0326, 0332

## Change
0332 points 3 (its CPU half) and 5, for bug 03 of 051. `render/src/bounce_probes.h` and `.c` are
the one owner of which probes are captured and when a relight is needed; read the header first.
- `render/src/bounce_probes.h` and `.c`:
  - `voe_render_bounce_probes` keeps the spacing it was last placed at;
  - `voe_render_bounce_probes_place` takes `float spacing` after `corner`: the metres between this
    grid's probes, `cell` counted in them. A spacing other than the last one empties the whole grid
    as a jump of a whole grid does (every probe loses its picture, is queued and marked changed).
    Stale spheres are tested against probe centres at that spacing;
  - the take's nearest-the-eye order uses the placed spacing;
  - `voe_render_bounce_probes_relight_needed`: the sun, its bounces and strength, the lamp count
    and each lamp's other fields compare as now, but a lamp's position compares about the grid's
    lowest corner (position − corner) within a millimetre on each axis, and its `shadow` by
    whether it is slotted, not which slot. The relit call keeps the corner with the lights. So an
    eye that moves, which moves every lamp and the corner about it alike, and reorders slots,
    relights nothing (0331).
  - Header: the PLACE paragraph names the spacing and what a new one does; the RELIGHT paragraph
    loses "an eye that moves under a bouncing lamp relights every frame" and says why the compare
    is about the corner (0332 point 5); usage block to match.
- `render/src/bounce_volume.c`: its one call of the place passes `VOE_RENDER_BOUNCE_SPACING` for
  now; card 40 gives it the frame's.
- `render/tests/bounce_probes.c`: every place passes `VOE_RENDER_BOUNCE_SPACING`; new cases:
  placed again at the same cell with spacing 4 queues all 6912; at spacing 4 a stale sphere of 6 m
  queues only probes whose centres (4 m apart) lie in it; after a relit call, the eye moved 7.3 m
  (every lamp position and the corner shifted by the same float3) and a lamp's slot swapped with
  another's: no relight needed; a lamp moved 1 cm about the corner: needed; a lamp going from
  slotted to unslotted: needed. Header to match.
- `render/src/src.md` (`bounce_probes.c`), `render/tests/tests.md` (`bounce_probes.c`): entries.

## Done when
`ctest --test-dir build/debug -R "^render/bounce_"` passes.
