# tests

`render`'s own tests: plain C programs with an ordinary `main()`, zero for pass, found by the build
and registered nowhere. A reader is here to find which file already makes a claim before making it
again. A test marked *headless* runs on the headless device — no window, no compositor — and checks
by reading the offscreen colour image back.

- `card.c` — which graphics card is taken, on made-up facts with no card: the discrete over the
  integrated wherever listed, the next one down when the fastest cannot present, a software
  rasteriser alone, the larger memory, and a card without 1.3 or a drawing queue never.
- `loader.c` — that a machine with a driver and no SDK reaches Vulkan.
- `pools.c` — two meshes and two ranges, a full pool as a returned failure, and ids that stop naming
  anything when destroyed: a texture, a shading record whose slot is reused, and a mesh whose range
  the next mesh takes, and a mesh's own box handed back and refused once destroyed. Headless.
- `textures.c` — 1024 texture slots: at least 1000 one-pixel textures made before one is refused,
  and a quad wearing the last of them, in the highest slot, drawn in its colour. Headless.
- `transient.c` — geometry that lives one frame: an id refused by the frame after, the same slot
  drawing different contents, a static and a transient range in one frame, and an overrun refused
  without corrupting the frame. Headless.
- `elements.c` — rectangles from records: the colours, the clip rectangle, paint order both ways
  round, the blend, the capacity refused, a mesh after them, an empty frame and two ranges with two
  matrices. Headless.
- `glyphs.c` — letters from records: one draw with a rectangle, the sheet read the right way round,
  the clip, no sheet, paint order across kinds, and the ink of thin, aligned and wide strokes with
  an edge partly covered. Headless.
- `element_transform.c` — the element arithmetic with no graphics card: the transform's origin, the
  near clip boundary, the surface matrix and size, `ui` scale and the eighty-byte record.
- `element_scene.h` — the device, readback, record builders and colour counts `elements.c` and
  `glyphs.c` share. Its header says why it is a header.
- `matrix.c` — that slangc really was given `-matrix-layout-row-major`, checked by making a shader
  report a known matrix back. Its header says which two claims are not in it and where they live
  instead.
- `passes.c` — a frame as a sequence of passes: a second pass loading what the first left and
  reading its own camera, a frame with no pass presenting the clear colour, the pass capacity
  refused, and a pass with no camera drawing elements. Headless.
- `targets.c` — a target of one's own shown on the window the right way up, each frame slot reading
  its own slot's picture, a resize that keeps the id, the capacity refused, and a target read back
  as RGBA8. Headless.
- `depth_copy.c` — the copy of a pass's depth refused outside a camera pass, a cube behind one
  drawn before the copy still hidden after it, and colour drawn before it kept, on the window and
  on a target. Headless.
- `water.c` — water over a depth copy: clear at the shore and darkened in the deep, waves that move
  with seconds and wrap at 60, the sun's glint at the mirror angle, a shadow on it, and `water` 0
  drawing as an ordinary blended surface. Headless.
- `prepare.c` — a device opened unprepared: prepare answering preparing then prepared within six
  calls, a pass with no camera drawing an element before any, and a camera pass preparing it all.
  Headless.
- `offscreen.c` — that back faces are culled, that the Y flip, the winding and the front-face
  constant agree about which way round that is, that a texture arrives the right way up, and that an
  object record's colour tints what is drawn. Headless.
- `unshaded.c` — that `unshaded` draws a lit cube in its base colour and black without it, that a
  fill lifts a face the sun misses, and that a face the sun meets head-on or at N·L 0.5 reads the
  same with fill and without. Headless.
- `surface_maps.c` — that a flat normal map reads as none, a tilted one darker, and emission adds
  red to a black surface whether the sun faces it or not. Headless.
- `shadow.c` — the sun's shadow passes: a device without `shadow_size` drawing as before, four
  cascades and a window pass in one frame of five draws, a shadow pass past `passes` refused, and a
  cube shadowing the floor under it — alike with no cascades, base colour when unshaded. Headless.
- `bounce_probes.c` — which captured probes are queued, with no card: all on the first place, a
  one-cell move queuing 288, a stale sphere queuing only within at 2 and 4 m, a new spacing queuing
  all, a relight only on change (light blockers too) and not for an eye that moves, and the 17th
  bouncing lamp left out.
- `bounce_volume.c` — a probe volume wanted on the first begin and built the next frame, the
  window's and a target's apart, a sum image 48 wide, freed after 300 frames with no begin, and
  both relit in one frame each into its own record, at 2 and 4 m each its own spacing. Headless.
- `bounce_capture.c` — the capture pass: open with a near cube one draw and a far one none, the
  fifth in a frame not open, refused with `passes` spent, a red cube in the albedo atlas, and at
  spacing 4 a cube 30 m off one draw. Headless.
- `bounce_shadow.c` — the relight's sun map: opened after a frame's captures with a cube one draw
  and a texel past the clear, not opened when settled, when only a lamp bounces, or with `passes`
  spent, and opened for the window and a target in one frame, each record saying drawn. Headless.
- `bounce_settle.c` — captured probes settling: beside a cube, validity 1 and a moments mean to its
  face; inside it, validity 0; a settled frame dispatching nothing. Headless.
- `bounce_read.c` — lit surfaces reading the probe volume: a pass names a begun, built volume, and
  a sunlit grey ground with fill 0.1 under it, nothing captured, reads as with no begin. Headless.
- `bounce_probes_scene.c` — probes relit a level at a time, checked in the picture, a red wall's
  lit side at spacing 2 and 4. Headless.
- `light_bins.c` — which tiles and slices point lights mark, with no card: a light ahead the middle,
  one behind nothing, one around the eye every tile, one to the side none, light 40 word 1 bit 8,
  and slices rising from NEAR to FAR.
- `light_blockers.c` — which boxes hold a point, with no card: a box's centre and face and not
  past it, a turned box's diagonal, two boxes both bits, blocker 31 bit 31, none mask 0, and
  inside the sphere but outside the box 0.
- `point_shadow_faces.c` — which cube faces a caster's sphere reaches, with no card: along +X bit 0,
  the +X/+Y diagonal bits 0 and 2, about the light all six, beyond range none, a cube's sphere, and
  one moved and scaled by a matrix.
- `point_shadows.c` — the point shadow maps: ready with `point_shadow_size` and not without, the
  point-shadow pass drawing a caster once or not at all, and a lamp's cube darkening the ground
  behind it. Headless.
- `point_lights.c` — a pass's point lights on a ground quad under a dark sun: lit only where they
  reach, none when unshaded, falloff ordering brightness, and a shadow slot ignored with no point
  shadows, and bounces and bounce strength lighting nothing directly. Headless.
- `blocked_light.c` — a light blocker over half a sunlit ground: that half black, the other and a
  far or uncounted box the old picture, a lamp outside lighting only outside and one inside only
  inside, and an unshaded pass unchanged. Headless.
