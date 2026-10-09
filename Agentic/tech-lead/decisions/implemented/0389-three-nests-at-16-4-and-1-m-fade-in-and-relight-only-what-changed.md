# 0389 — Three nests at 16, 4 and 1 m fade in, and relight only what changed
date: 2026-10-08
by: planner

## Decision
How 0387 is built, for 082 bug 05.

1. **Volumes.** A target holds `VOE_RENDER_BOUNCE_VOLUMES` 4 probe volumes, each 24 × 12 × 24 as today: 0 the
   level grid (0331), 1 to 3 nests at 16, 4 and 1 m. A begin names its volume. 3d begins a nest only when its
   spacing is below the level grid's, so a small scene runs the 1 m nest alone. A nest not begun for 300
   frames is freed as a volume is now (0316). Cost: about 43.6 MB per nest a bounced target uses.
2. **Placement.** A nest's lowest cell is the eye's cell less (12, 9, 12): nine cells below the eye, two
   above, because what is looked at is mostly below. It moves only when the eye's cell is more than 2 cells
   from the nest's home cell on an axis, and then only as far as needed. render gives back where a volume
   was last placed (`voe_render_bounce_placed`). Cells are whole cells about the world origin, as 0331's.
   Known limit: from more than about 7 m above the ground, the 1 m nest covers only air and the 4 m nest
   reads instead.
3. **Budget.** The four capture passes a frame stay device-wide (0326). 3d begins coarse to fine. Volume i
   of n begun may open capture passes until 3d's count this frame reaches
   `VOE_RENDER_BOUNCE_CAPTURE_PASSES − (n − 1 − i)`, so every finer volume keeps at least one pass and the
   finest gets what the others leave.
4. **Fade-in.** A probe that gets its first picture starts at readiness 0 and gains 1 every place, up to
   `VOE_RENDER_BOUNCE_FADE` 16 (about a quarter second). A recaptured probe keeps its readiness. A probe
   brought in by a scroll starts at 0. Readiness is the validity image's second channel (RG16F) and goes in
   bits 16–20 of a list word. Changed probes are listed first, fading-only ones after, with both counts
   pushed. A begin with only fading probes runs settle alone: no relight and no sun-map pass.
5. **Relight what changed.** With the bouncing lights unchanged, relight and sum run over the listed changed
   probes only. When the lights change, they run over the whole grid. Lamps and blockers are compared about
   the world origin (corner − cell × spacing), not the corner, so a scroll alone never relights the grid.
   Known gap: levels 2 and 3 of an unchanged probe beside a new picture catch up at the next lights change.
6. **Read.** `voe_render_bounce_read` takes a band in cells and whether readiness counts. It returns rgb,
   normalised over the weights that count, and a weight a: the edge fade × the sum of the trilinear weights
   (× readiness when it counts). a is 0 when that sum is 0 or not finite. The edge fade is the product over
   axes of saturate((min(u, size − u) − 0.5) / band). The lit pass starts at 0 and takes volume 0 at band 1 as
   rgb × a. It then lerps towards each nest, coarse to fine, by a at band 2. The relight's own read is band 1
   without readiness, rgb × a. The frame block holds every volume of the drawn target; grid ~0u is none.
7. **Stale.** A caster's stale sphere has w its own world bounding radius (half its world box's diagonal).
   render queues probes within w + min(12 cells, 16 w): within a probe's reach, and no further than where
   the caster shrinks under one face texel. A caster marks when its position, rotation or scale changed
   between the lags, or when it was never remembered in a world that has a previous table (a new caster).
   It also marks when the shape system changed its colour or kind in this step's run. The editor remembers
   its world's transforms once a frame, before the step. Known gap: a removed caster marks nothing.
8. **Shape changes.** An opt-in runtime-only table `voe_3d_shape_changed`, registered by the game's world,
   holds whether the last shape run changed an entity's colour or kind. Only the shape system writes it.

Per frame (0388): `bounce capture N` stays at most 4 passes. `bounce sun shadow` becomes one pass per
relighting volume per casting sun, while something changes. `bounce relight` becomes one per volume begun,
sized by the probes that changed or fade. A still camera in a still world adds nothing to today's frame.

## Reasoning
- At 2 m spacing a probe stands 1 m up, so a 1 m box's face is beside or under it and the ground's +y
  irradiance never sees it. At 1 m a probe stands 0.5 m up, beside the face. 16 and 4 m bridge a level grid
  of 64 m and more in steps of 4.
- The curved edge is a capture front: the editor marks nothing (it never remembers), and where it does mark,
  the sphere is 3 cells while a probe sees 12. Marking out to the reach, and capped by the caster's size,
  removes the ring without recapturing a whole coarse grid for a moving pebble.
- Relighting all 6912 probes of four volumes on every change is the cost 0388 forbids while flying.
- Rejected: a single finer level grid (its reach shrinks with it); nests centred on the look point (turning
  would recapture, against 0387); a flag on the mesh row for recolours (the mesh row has one writer).

## Replaces
Nothing. Carries out 0387 and amends 0332 point 4: the stale radius is now the caster's, not 3 cells.
