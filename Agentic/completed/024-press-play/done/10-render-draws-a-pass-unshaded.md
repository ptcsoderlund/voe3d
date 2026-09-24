# 10 — render draws a pass with no sun unshaded
folder: render
decisions: 0168, 0238

## Change
A pass can say it has no sun; every surface in it then draws its own base colour, as an unlit material
does (0238). The light's unused padding float becomes that flag, so the block's size and every offset
stay as they are and callers that zero the field keep today's picture.

- `render/include/render/device.h` — `voe_render_light`: `float reserved` becomes
  `uint32_t unshaded`; non-zero means no sun this pass, and direction, intensity and colour are then
  not read. The struct's header paragraphs gain: what `unshaded` means and who sets it (the owner of
  the scene's light, `3d`, when a scene has none); that zero keeps shading, so a zeroed light is
  still black as the paragraph already says.
- `render/shaders/draw.slang` — the shader's `voe_render_light`: `reserved` becomes `uint unshaded`.
  In `voe_render_draw_fragment`, a non-zero `sun.unshaded` takes the same exit as `shading.unlit`,
  placed beside it (after every sample). The file's lighting section gains a point: a pass with no
  sun takes the unlit exit for every surface, and why that is not ambient or a default light (0238).
- `render/src/descriptors.c` — beside the existing `offsetof(voe_render_light, colour)` assert, one
  for `unshaded` at 28.
- `render/tests/unshaded.c` — new, headless, built the way `render/tests/offscreen.c` builds its
  device, cube, camera and material (read that file for the pattern; copy no more than it needs).
  A lit material (not `unlit`), base colour a known non-grey colour, fully rough, no metalness, one
  cube in front of the camera, read back through the offscreen read `offscreen.c` uses:
  1. a light with intensity 0 and `unshaded` 1 reads the base colour × object colour at the centre,
     within a small tolerance;
  2. the same light with `unshaded` 0 reads black there;
  3. `unshaded` 1 with a real sun pointing away from the visible face still reads the base colour.
  Skips without a card the way `offscreen.c` does. Its header says what it claims and why.
- `render/tests/tests.md` — an entry for `unshaded.c`.
- `render/shaders/shaders.md` — the `draw.slang` entry's "two unlit exits" becomes three, or says a
  sunless pass is the third.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder render` prints `FINDINGS: 0`.
2. `ctest --test-dir build/debug -R '^render/unshaded'` runs one test and it passes (or skips on a
   machine with no Vulkan and says so).
