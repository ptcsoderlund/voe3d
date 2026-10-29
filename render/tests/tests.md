# tests

`render`'s own tests: plain C programs with an ordinary `main()`, zero for pass, found by the build
and registered nowhere. A reader is here to find which file already makes a claim before making it
again. A test marked *headless* runs on the headless device — no window, no compositor — and checks
by reading the offscreen colour image back.

- `card.c` — which graphics card is taken, and which vendor-tagged Best Practices message ids it
  drops, on made-up facts with no card.
- `loader.c` — that a machine with a driver and no SDK reaches Vulkan.
- `pools.c` — two meshes and two ranges, a full pool that grows, ids refused once destroyed, a
  mesh's own box, and a sharing mesh walking its base's vertices. Headless.
- `textures.c` — 1024 texture slots: at least 1000 one-pixel textures made before one is refused,
  and a quad wearing the last of them, in the highest slot, drawn in its colour. Headless.
- `transient.c` — geometry that lives one frame: an id refused by the frame after, the same slot
  drawing different contents, a static and a transient range in one frame, and an overrun refused
  without corrupting the frame. Headless.
- `heights.c` — a 2049 × 2049 heights texture made and a side past 4096 refused; inside a frame a
  4 × 4 write taken, one past the slot's remaining `heights_texels` refused with the frame still
  drawing, and every slot's budget whole again the next frames. Headless.
- `pixels_write.c` — inside a frame a 4 × 4 red write into an RGBA8 FIELD texture taken, an 8 × 8
  one past the room refused with the frame still drawing, and an unlit quad wearing it red. Headless.
- `shading_write.c` — a quad's record written blue inside a frame before its pass reading blue that
  frame, and a write in each of three frames in a row reading the last. Headless.
- `terrain_vertex.c` — a terrain record over heights all 2 m drawing a quad grid as a 2 m plane
  would, and the same record with `heights` nought not covering the rows above the middle.
  Headless.
- `terrain_layers.c` — a painted landscape seen from above: its left quarter the red layer, its
  right the green, `paint_a` nought the object record's white, and a layer at weight 0 nowhere.
  Headless.
- `ground_blend.c` — a white square over a red-painted ground: 0.1 m above it red, 2 m above
  white, `ground` nought white, 0.5 m between. Headless.
- `worker_guard.c` — a thread creating and destroying 100 textures and 100 geometries while the
  main thread runs 100 element-only frames, every call true and the thread joined. Headless.
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
- `pass_times.c` — the frame breakdown: passes and spans listed by name in order, each above
  nought and within the frame's GPU time. Headless.
- `best_practices.c` — the Best Practices gate: in a debug build the checks on, and a frame of a
  shadow pass and a camera pass with a depth copy and an element closing with no new message; a
  release build says so and passes. Headless.
- `best_practices_wanted.c` — whether a device turns Best Practices on: headless always, a window
  only for `VOE_RENDER_BEST_PRACTICES` exactly `1`; needs no graphics card.
- `new_messages.c` — what the Best Practices classifier keeps for the close to name, on made-up
  messages; needs no graphics card.
- `prepare.c` — a device opened unprepared: prepare answering preparing then prepared within seven
  calls and one more than the step count says, a pass with no camera drawing an element before any,
  a camera pass preparing it all, and prepare on a thread while element frames draw. Headless.
- `pipeline_cache.c` — the pipeline cache as bytes: one device's handed out and taken by a second,
  which prepares; one payload byte flipped, 10 bytes, or none refused, and that device still
  prepares. Headless.
- `offscreen.c` — that back faces are culled, that the Y flip, the winding and the front-face
  constant agree about which way round that is, that a texture arrives the right way up, and that an
  object record's colour tints what is drawn. Headless.
- `mips.c` — a SMOOTH texture's chain: a 4×4 checker up close reads hard texels, a 256×256 one 16
  pixels across and a low-angle plane's far half read a grey blend, and `given_levels_are_drawn`: that
  checker given solid red lower levels reads red far off. Headless.
- `texture_blocks.c` — compressed textures: a BC7 chain of one colour drawn as its RGBA8 twin
  within 2, a BC5 DATA chain made, and the SMOOTH memory reading rising with an RGBA8 and a BC7
  texture, `compressed` with the BC7 one, and back again once both are destroyed. Headless.
- `compress.c` — the GPU encoder: a gradient's and a noise picture's chains as BC7 and a normal
  map's as BC5 drawn beside their RGBA8 sources within the CPU encoder's bounds, a 13×9 picture's
  edge blocks those of its edge-repeated 16×12, and two jobs at once, one dropped. Headless.
- `cutout_far.c` — a cut-out of one-texel alpha stripes, cutoff 0.5: filling the picture about half
  its pixels drawn, and seen 16 pixels across, at level 4, at least a third still drawn. Headless.
- `dither.c` — an object record's `dither` on an unlit quad filling the picture: 0 every pixel, 0.5
  half, 0.5 then −0.5 in another colour each pixel exactly once, 0.25 a quarter. Headless.
- `instances.c` — three copies of a white quad in one instanced draw: each place white, a spot
  between the clear colour, one draw and three quads' triangles, a write past `instances` refused
  with the frame still ending. Headless.
- `unshaded.c` — that `unshaded` draws a lit cube in its base colour and black without it, that a
  fill lifts a face the sun misses, and that a face the sun meets head-on or at N·L 0.5 reads the
  same with fill and without. Headless.
- `exposure.c` — a pass's exposure record on a quad: a zeroed one the old byte, exposure ±1 at
  Clip, Soft as PBR Neutral, a lit quad's sun clipped or rolled off, a draw after the depth clear
  as authored, a blended one doubled, and no sky. Headless.
- `sky.c` — the sky drawn behind a pass: its pixels the CPU twin's radiance within 2/255, a quad
  over it, a zeroed sky the clear colour, a dark sun the plain gradient, `<pass>: sky` in the
  breakdown, and a red light and a blue one in `more` each drawing its own disc. Headless.
- `haze.c` — far surfaces fading into haze of the sky's hue at the lights' brightness, wherever the
  sun and whatever the sky: lights summed, no light or haze black, no sky the lights' colour, after
  the depth clear, over a veiled sky, from a haze start. Headless.
- `haze_sky.c` — the sky seen through the haze: dense haze the whole sky and the sun's disc, thin
  haze the horizon most, clear above the layer, and without a drawn sky the clear colour veiled.
  Headless.
- `clouds.c` — clouds drawn over the world, looking straight up: kind none the same bytes and no
  span, Volumetric and Flat at coverage 1 changing the centre with `<pass>: clouds` listed,
  coverage 0 as none, a quad under them kept, and an eye inside the layer. Headless.
- `fog.c` — a fog volume 30 m ahead: count 0 the same bytes and no span, the volume changing the
  centre with `<pass>: fog` listed, density 0 and a view missing the box as none, a quad before it
  kept, an eye inside it, and clouds and fog in one pass both drawn. Headless.
- `cloud_shadows.c` — clouds shadowing a lit floor: strength 1 darker, 0 the same bytes as none,
  0.5 between, a floor facing away lit by its fill alone unchanged, clouds below it nothing, and
  water darker under them. Headless.
- `cloud_reflections.c` — clouds in what mirrors the sky, seen at a slant: a smooth metal floor
  changed by Flat clouds of coverage 1, the same within 1/255 at coverage 0, and a water plane
  changed too. Headless.
- `sky_light.c` — the sky lighting nothing: a cube's face the same with the sky or without, a
  non-metal quad unchanged, a metal one the sky along R times the light on it, none indoors or
  unlit, no brighter than the sky head-on, the same at sky strength 0 and 1. Headless.
- `surface_maps.c` — that a flat normal map reads as none, blue 0 or not, a tilted one darker, and emission adds
  red to a black surface whether the sun faces it or not, a metal-roughness map replacing the
  factors, and a mirrored half's bump tilting mirrored along u and along v. Headless.
- `vertex_colours.c` — a vertex's colour tinting its surface: an unlit white record on red vertices
  red, on zeroed ones white, and a lit quad of half-grey vertices about half as bright in linear
  light as white ones. Headless.
- `two_sided.c` — a lit quad facing away from the camera: one-sided the clear colour, after
  `voe_render_frame_two_sided(true)` drawn and lit from the camera's side, brighter than with the
  sun behind, and a following pass one-sided again. Headless.
- `shadow.c` — the sun's shadow passes: a device without `shadow_size` drawing as before, four
  cascades and a window pass in one frame of five draws, a shadow pass past `passes` refused, and a
  cube shadowing the floor under it — alike with no cascades, base colour when unshaded. Headless.
- `shadow_lights.c` — the shadow array growing: one light ready at first and none without
  `shadow_size`, a second ready the frame after it is wanted, layer 7 drawn and slot 1 read, nine
  capped at four, and nought shrinking nothing. Headless.
- `bounce_probes.c` — probes queued and relit, with no card: a sphere's reach, no relight on a
  scroll under a still lamp, a new picture fading in over sixteen places, a retaken one keeping its
  readiness, a scrolled-in one not ready, a new shade count relighting with nothing queued.
- `bounce_volume.c` — a probe volume built the frame after its first begin, freed after 300
  idle frames, a window's and a target's and two of one target kept apart, each relit into its own
  record at its own spacing. Headless.
- `bounce_capture.c` — the capture pass: open with a near cube one draw and a far one none, the
  fifth in a frame not open, refused with `passes` spent, a red cube in the albedo atlas, and at
  spacing 4 a cube 30 m off one draw. Headless.
- `bounce_shadow.c` — the relight's sun map: a cube's draw, or a copy's instanced draw under a
  new shade count, past the clear; not opened when settled, when only a lamp bounces, or with
  `passes` spent; the window and a target in one frame. Headless.
- `bounce_settle.c` — captured probes settling: validity and moments beside and inside a cube,
  readiness fading in over sixteen frames, a fading frame one settle, a settled one nothing, and
  `bounce relight` in the breakdown only when relit. Headless.
- `bounce_read.c` — lit surfaces reading the probe volumes: nothing captured as no begin, an
  empty 1 m nest as the level grid alone, a settled one changing its middle, and the ground beside
  a red wall redder than 3 m out. Headless.
- `bounce_probes_scene.c` — probes relit a level at a time, checked in the picture by a red wall
  at spacing 2 and 4, a box, a room and a lamp; a stale pebble relighting less than the grid, a
  moved sun all of it. Headless.
- `bounce_suns.c` — every sun lighting the probes: a sun alone, an empty `more` the same bytes, a
  dark sun's copy in `more` within 1%, a moon adding, a moon at bounces 0 nothing, a moon shadowed
  behind a wall nothing. Headless.
- `light_bins.c` — which tiles and slices point lights mark, with no card: a light ahead the middle,
  one behind nothing, one around the eye every tile, one to the side none, light 40 word 1 bit 8,
  and slices rising from NEAR to FAR.
- `sky_model.c` — the backdrop, glow and irradiance, with no card: colours as authored, the
  horizon band, strength scaling the backdrop, each light's disc at 2c, a light not shown leaving
  the backdrop, glow in a black sky, and the harmonics within 5% of a brute-force sum.
- `clouds_noise.c` — the cloud field's baked textures, with no card: the weather and the detail
  each the same bytes made twice, each tiling across every axis's wrap, and each darkest under 64
  and brightest over 192.
- `light_blockers.c` — which boxes hold a point, with no card: a box's centre and face and not
  past it, a turned box's diagonal, two boxes both bits, blocker 31 bit 31, none mask 0, and
  inside the sphere but outside the box 0, the call reached through `render/device.h`.
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
- `directional_lights.c` — a moon beside the sun over a ground and box: no further light the old
  picture, a zeroed one too, a blue moon only adding, two shadows either side of the box, a Room
  giving each light its own half, and a Fill box filled by the moon inside it. Headless.
- `blocker_kinds.c` — kinds over a sunlit ground: a floating Wall's shadow the fill alone, ground
  under and clear of it as before; a Wall stopping a lamp; Indoors keeping the sun, not the fill;
  a Room holding the sun lit inside, black outside; zero kinds the old picture. Headless.
- `blocked_bounce.c` — the bounce kept by a blocker: a blocked patch by a red wall takes no red,
  a lamp inside bounces it red and one outside not, the 1 m nest relit when the blocker goes, and
  3d's nest keeping its red through a lights change, each volume under its own sun view. Headless.
- `blocker_kinds_bounce.c` — kinds gating the bounce by a red wall: a Wall slab between it and a
  patch leaving the patch as with no bounce, ground on the wall's side red; an Indoors box round the
  patch as with no blocker; a Room box with zero kinds as with no bounce. Headless.
