# 0472 — Grass is blades made on the GPU over the painted patches near the eye
date: 2026-10-23
by: planner

## Decision
For 094 (grass grows on the grassy parts, 0376):

1. **The setting is the landscape's.** `.landscape` gains `[Grass]` with `layer=` (0–7), `density=` (blades
   per m², 1–64, default 24) and `height=` (millimetres, 50–2000, default 300). No section, no grass. In
   assets it is `voe_assets_landscape_grass { uint8_t layer; uint8_t density; uint16_t height; }`, member
   `grass` of `voe_assets_landscape`, `layer` held as the layer plus one so a zeroed struct is none.
   Resample carries it, flat makes none. The game's cooked table gains it as its last field.
2. **Blades are made on the GPU** from the vertex and instance index, with no geometry and nothing on the
   CPU per blade. A patch is a 4 m square of the landscape's own space; patches out to a reach of 50 m are
   chosen by 3d each camera pass, culled by their box (heights plus blade height) against the view's
   planes, and skipped where the CPU paint has no grass weight at any grid point they touch. A patch is a
   `voe_render_instance` whose `rows[0]` is (corner x, corner z, side, 0); its other fields are unread.
3. **Levels, not a draw per patch.** A patch's level L (0–5) is the floor of log2 of 1 ÷ its nearest
   point's kept share, and draws full ÷ 2^L blades; patches of one level are one instanced draw, so at most
   six grass draws a landscape. Blade i's place is a hash of (patch corner, i), so a blade is the same
   blade at every level and a level change pops nothing.
4. **Fade by thinning.** The kept share is 1 out to 10 m and (10 ÷ d)² past it; blade i is kept while
   i < full × share at its own root. Over the last 15 m of the reach (35–50 m) blades shrink to nothing,
   so there is no line where grass stops. The paint weight thins too: a second hash below the weight
   keeps a blade, and its height is lerp(0.5, 1, weight) of the setting's.
5. **Look.** A blade's colour is the grass layer material's colour, sampled hex-tiled at its root as the
   ground is (0402), so the far slopes keep their green from the ground's own colour. It is lit through
   `lighting.slangh` with the ground's normal at its root leaned toward the blade's facing, receives the
   sun's cascade shadows, and is hazed and toned as every mesh is. Two-sided and opaque. It casts no
   shadow and is in no capture, sun map or point shadow: camera passes only.
6. **Render.** A seventh mesh pipeline `grass` from `render/shaders/grass.slang`, no vertex input, and
   `voe_render_frame_draw_grass(device, object, grass, first, count)`. `object` is the landscape's
   painted record (heights, paint, layers, world, normal, `terrain.w` its side); `voe_render_grass`
   (layer, blades drawn, blades full, height, near, reach, fade) is packed by render into the record's
   `waves` and `sky`, which only water reads.
7. **3d** draws grass in camera passes after the foliage, inside a span named `grass`, so the frame
   breakdown lists `view window: grass` (0358, 0388). `VOE_3D_GRASS_PATCHES` instances and
   `VOE_3D_GRASS_DRAWS` objects a camera pass are added to every program's capacities.
8. **Editor.** An Inspector Grass section between Layers and Foliage: a "Grows on" choice of None or a
   layer by its material's file name, and Density and Height sliders. A change goes to the store at
   once through `voe_3d_models_landscape_grass_set`, marks the landscape edited and the project unsaved,
   and is no undo step, as a layer set is (0402). Save writes it with the rest of the file.

## Reasoning
Per-blade CPU data for a painted field is millions of records written and uploaded on every repaint;
made on the GPU from the paint render already holds, repainting shows at once with no rebuild (feature
step 2). Patches and levels bound the work by the reach, not the field's size, which is what keeps a big
field smooth on the laptop (0386, 0388). Thinning by a share of blades rather than alpha keeps grass
opaque and out of the blend (0467). A landscape-wide setting is the smallest surface that makes "grass
grows where grass is painted" true; per-layer kinds of grass wait for a game that wants them.

## Replaces
none
