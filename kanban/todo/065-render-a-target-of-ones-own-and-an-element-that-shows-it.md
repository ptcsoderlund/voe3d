# 065 — `render`: a target of one's own, and an element that shows it

claimed-by: -
blocked-by: 064
decision: *A frame is a sequence of passes, and a target of one's own is a texture* (ADR-0148) points 4–8. Card 064 made the pass; this card gives a pass somewhere other than the window to draw.

## Goal

A program creates a target at a size, draws into it through a pass, and shows its picture on an
element or a mesh through an ordinary colour texture id that never changes — across frames in
flight and across resizes.

## Scope

**1. `render/include/render/device.h`.**

```c
[[nodiscard]] bool voe_render_target_create(voe_render_device *device,
					    uint32_t width, uint32_t height,
					    voe_render_target *out_target,
					    voe_render_texture *out_texture,
					    voe_base_error *error);
void voe_render_target_resize(voe_render_device *device, voe_render_target target,
			      uint32_t width, uint32_t height);
```

- **`voe_render_capacities` gains `uint32_t targets`**, which may be nought: a device made with
  none refuses the first create with a report, as `elements` does.
- A width or height of zero asserts. A create over capacity or refused by the card returns false.
- `_resize` records the size; it is **applied at the top of the next `_frame_begin`**, where the
  window's own rebuild already happens. Calling it with the size it already has does nothing.
  Resizing inside a frame is allowed; it takes effect next frame.
- Header paragraphs: why a target is kept rather than asked for per frame; that the texture id
  is stable and which slot's image it reads is this folder's bookkeeping; that the picture is
  undefined until a pass draws into it and after a resize; that a resize waits for the GPU as the
  window's does; the clear rule for a target (below); the self-sampling assert (below).

**2. `render/src/target.c` and the texture slot table.**
- A target is a colour-and-depth pair **per frame slot**, same formats as the window's target.
- Its texture id occupies **one** slot in the texture table. A frame in slot *n* must read slot
  *n*'s colour image through it. How — a descriptor set per frame slot already exists or does
  not; find out and do the smaller thing — goes in `src/src.md`.
- The colour image is sampled **nearest**, like every texture in this engine.

**3. The clear rule extends.** A target is cleared the first time a frame opens a pass onto it
and loaded on later passes that frame. A target no pass opened keeps what it held.

**4. Self-sampling asserts.** In a debug build, drawing a mesh whose shading record, or an
element range any of whose records, names the texture of the target the open pass draws into
is an assert. Say in the header that it is a debug check.

**5. `VOE_RENDER_ELEMENT_IMAGE`** in `voe_render_element_kind`: samples `sheet_texture` through
the `sheet` rectangle over the element's bounds and multiplies by `colour`, premultiplied at
output like the other two kinds (ADR-0069). **The record's size and layout do not change.** The
element shader gets the third branch; the kind's comment says what it is for — a view, an icon,
a thumbnail — and that it is not a text kind.

**6. Tests, `render/tests/targets.c`, headless.**
- Draw a known opaque element into a 64×64 target; in a later pass onto the window, an IMAGE
  element covering the window samples it; read back and compare pixels.
- The same texture id on a shading record's base colour draws the target's picture on a mesh.
- **Frames in flight:** frame *k* draws red into the target, frame *k+1* green; each frame's
  window readback shows its own colour, for at least as many frames as there are slots.
- Resize to 32×16: the texture id is unchanged, and the next frame's picture is 32×16.
- `targets = 0`: create returns false. `targets = 1`: the second create returns false.
- An IMAGE element with `sheet` covering half the texture shows that half.

**7. `render/render.md`, `src/src.md`, `tests/tests.md`, `shaders/shaders.md`** entries.

## What must not change

- Card 064's pass API and its tests.
- The window's target, its formats, the blit.
- The element record's size, field order and the SOLID/GLYPH behaviour — `render/tests/elements.c`
  passes unedited.
- `voe_render_texture_create`'s behaviour and its startup-only rule.
- No linear filtering, no mipmaps, no post-processing pass.
- No folder but `render`.

## Verify

- Linux: `cmake -P check.cmake` green; `ctest` passes including `targets`.
- `dev` and `voe_editor` look as they did after card 064.
- Paste the frames-in-flight test's per-frame colours into Notes.
- From the planning root, `bash tools/hot.sh` reports no new `OVER`.

## Done looks like

A test draws a picture into a target of its own and shows it on the window twice — once on an
element and once on a mesh — with the right frame's picture in every frame slot.

## Notes
