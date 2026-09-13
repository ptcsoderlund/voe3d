# 064 — `render`: a frame opens passes, and the window is one target

claimed-by: -
blocked-by: -
decision: *A frame is a sequence of passes, and a target of one's own is a texture* (ADR-0148) points 1–4, 8 (`passes` only), 9 and 10. Card 065 adds targets of one's own and the image element; this card adds neither.

## Goal

The frame no longer owns a camera. `voe_render_frame_begin` opens a frame; every draw happens
inside a **pass** opened onto a target with an optional camera. The only target that exists
after this card is the window's. **Every picture the engine draws today is unchanged.**

## Scope

**1. `render/include/render/device.h`.**

```c
typedef struct { uint32_t index; uint32_t generation; } voe_render_target; // shape of the other ids
#define VOE_RENDER_TARGET_WINDOW ((voe_render_target){ 0 })

typedef struct {
	voe_render_view view;
	voe_render_light light;
} voe_render_pass_camera;

[[nodiscard]] bool voe_render_frame_begin(voe_render_device *device,
					  voe_platform_size size, bool *drawing);
[[nodiscard]] bool voe_render_pass_begin(voe_render_device *device,
					 voe_render_target target,
					 const voe_render_pass_camera *camera);
void voe_render_pass_end(voe_render_device *device);
[[nodiscard]] bool voe_render_pass_is_open(const voe_render_device *device);
```

- Match the id shape the header already uses for geometry and textures rather than the sketch
  above if they differ. **`VOE_RENDER_TARGET_WINDOW` is the zeroed id.**
- `camera` **may be NULL** for a pass that draws only elements. `voe_render_frame_draw`,
  `_draw_blended` and `_clear_depth` in a pass with no camera **assert**. `_draw_elements` does
  not need one.
- `_draw`, `_draw_blended`, `_clear_depth`, `_draw_elements` assert a pass is open (replacing
  the frame-open assert). `_submit_element` stays frame-wide.
- Passes do not nest: `_pass_begin` with one open asserts, `_frame_end` with one open asserts.
- `_pass_begin` returns **false** when the frame already opened `passes` passes, with a report
  naming the numbers (the call in `base/report.h`) — the same terms as every other capacity.
- **`voe_render_capacities` gains `uint32_t passes`**, per frame slot, at least one; nought
  asserts at device creation.
- Rewrite the header's paragraphs that say the frame has one camera. Say: why the camera
  moved to the pass, why it may be NULL, the clear rule (point 2 below), that passes do not
  nest, and that element submission is frame-wide so one range may be drawn in any pass.

**2. The clear rule.** The window's target is **cleared the first time a pass opens onto it in
a frame and loaded on every later pass that frame** — colour and depth both. A frame that opens
no pass onto the window still presents a cleared picture.

**3. `render/src/`.** The camera-and-sun block becomes **one per pass per frame slot**
(`struct voe_render_frame_block` and the uniform buffer in `device_internal.h`, the binding in
`descriptors.c`), bound as the pass opens. `frame.c`'s rendering block opens in `_pass_begin`
and closes in `_pass_end` instead of in `_frame_begin`/`_frame_end`; `_frame_end` still submits
and blits the window's target. Keep the existing size and offset `static_assert`s true of each
block. Update the file headers in `frame.c`, `descriptors.c` and `src/src.md` that describe the
single block.

**4. Callers — each gets a pass, nothing else changes.**
- `app/include/app/app.h`, `app/src/app.c`: `voe_app_draw_open` loses `view` and `light`.
  Update the dozen-line loop in the header's comment. `app.md`.
- `3d/src/draw_system.c`, `3d/include/3d/draw_system.h`: assert `voe_render_pass_is_open`;
  every comment that says the loop hands the camera to `voe_render_frame_begin` now says it
  hands it to the pass. `voe_3d_frame` stays as it is.
- `dev/src/main.c`: one pass onto the window, with the camera `voe_3d_draw_system_frame` gave,
  around everything it draws today. `dev/src/interface.c` and `surface.c` need no change beyond
  being inside that pass.
- `editor/src/main.c`: one pass onto the window **with a NULL camera** around the interface.
  Delete the zeroed view and light and the comment defending them.
- Tests: `render/tests/transient.c`, `elements.c`, `offscreen.c`, `3d/tests/panel.c`,
  `import.c` — a pass around what each already draws. **No expected pixel changes.**

**5. New tests, `render/tests/passes.c`, headless.**
- Two passes onto the window in one frame: the second draws over the first, and a mesh in the
  second is hidden by a nearer mesh drawn in the first (depth loaded, not cleared).
- A frame opening no pass presents the clear colour.
- A device made with `passes = 1`: the second `_pass_begin` in a frame returns false, and the
  next frame's first succeeds.
- A pass with a NULL camera draws an element range correctly.

**6. `render/render.md`, `3d/3d.md`, `editor/editor.md`** — the entries that describe the frame's
camera, in the voice each file already uses.

## What must not change

- **No picture.** Every existing test's expected pixels stand untouched; if one needs
  editing, stop and report — that is a bug in this card, not in the test.
- No target other than the window's; no `targets` capacity; no new element kind. Card 065.
- The blit, the present modes, the frame timing, the transient reset and its place after the
  fence, the element record's layout.
- `voe_3d_frame`, `voe_3d_draw_system_frame`'s signature.
- No `render` knowledge of visibility.

## Verify

- Linux: `cmake -P check.cmake` green; `ctest` passes, including the new `passes` test.
- `./build/debug/dev/voe_dev` looks exactly as it did before the card; `voe_editor` looks
  exactly as before. Say in Notes that you compared both.
- `grep -rn 'voe_render_view){ *0 *}' editor dev` returns nothing.
- From the planning root, `bash tools/hot.sh` reports no new `OVER`.

## Done looks like

`dev` and the editor draw what they drew this morning; the editor no longer invents a camera;
and a frame can hold several passes onto the window, which a test proves.

## Notes
