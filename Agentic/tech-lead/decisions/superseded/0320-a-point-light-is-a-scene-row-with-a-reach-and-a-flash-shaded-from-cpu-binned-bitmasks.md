# 0320 — A point light is a scene row with a reach and a flash, shaded from CPU-binned bitmasks
date: 2026-10-02
by: planner

## Decision
For 048:
1. **The component.** `voe_scene_point_light` in `scene`, a component of its own beside the sun (0288):
   `colour` (COLOUR, linear), `intensity` (FLOAT32, a multiplier as the sun's is), `range`
   (FLOAT32, metres, the reach past which it lights nothing), `flash` (FLOAT32 seconds, 0 is steady) and
   `flash_when_made` (BOOL). Default: white, 1, 5 m, steady. It needs a transform; it is at the entity's
   world position, so a parent carries it (0281), and its rotation and scale change nothing. Menu
   "Rendering / Point light". The whole-row intent is its replace; it refuses a non-finite number, a
   channel outside 0–1, a negative intensity or flash, and a range of nought or less.
2. **The flash.** A light with `flash` above nought is dark until flashed, then fades linearly from full
   to dark over `flash` seconds. It flashes when its runtime-only glow row is first made if
   `flash_when_made`, on each flash intent naming it, and on each accepted replace (so an edit shows).
   The point light system, run by the frame's or step's seconds as the emitters are, drains both
   intents, makes the glow rows and counts them down; `voe_scene_point_light_strength` is what a reader
   draws with.
3. **No shadow, no bounce** (0301): no Cast shadows option; the sun's cascades, its fill fade and the
   bounce ignore point lights. The unshaded pass ignores them.
4. **Shading.** Forward, in `lighting.slangh`, the sun's glTF BRDF with the light's direction and
   `colour × strength × saturate(1 − (d/range)²)²`: no inverse square, so a strength means what the
   sun's does and the light ends at its range.
5. **Binning on the CPU, in render.** A pass carries at most `VOE_RENDER_POINT_LIGHTS` (256) lights,
   about the eye like every draw. Opening the pass bins each light's sphere into 16 × 9 screen tiles and
   32 depth slices (exponential from 0.1 to 1000 m, the last open-ended), one bit per light per tile and
   per slice; a fragment loops over the bits of its tile's mask AND its slice's mask. The lights and the
   masks are two storage buffers per frame slot, one region per pass. Water and particles are not lit.
6. **3d fills a pass's lights** from the table at the frame's lag, colour × strength, dark ones left out,
   past 256 the rest left out in table order; the editor's views, its preview and the game call it, so
   they agree. Dev does not.
7. **Seen in the editor.** Every point light in a view is marked by three wire circles of radius 0.25 m
   about its position in the world's axes, picked by a cube of half extent 0.25 m; the selected one in the
   outline's colour, the rest in the gizmo's rest colour (0274's rule).
8. **Duplicable and in prefabs.** 0273's refusals name the sun only; a point light duplicates and sits in
   a prefab like any row.
9. **Room.** The game world registers 256; the editor's transient pool marks that many per view.

## Reasoning
100 lamps on a GTX 1060 (0318) rule out looping every light per fragment; binning with a tile mask and a
slice mask (Drobot's zbin) is a few kB a pass, pure CPU arithmetic testable without a card, and needs no
compute pass or depth prepass. A fixed 256 is ten times the brief at 13 kB a pass. The flash in the
engine means game code lights a shot with one call and a prefab lights an explosion with no code, the
emitter's burst-when-it-plays pattern (0298).
- Deferred shading: a G-buffer and a second path for blended draws, for a forward engine.
- Compute-built clusters: a compute pass and a depth bound per frame for what the CPU bins in microseconds.
- Inverse-square falloff: a strength unlike the sun's and a tail that never ends without a cut.

## Replaces
Nothing. Carries out 0288 and 0301 for 048.
