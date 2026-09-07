# 013 — the frame goes into an offscreen image

status: complete
claimed-by: claude-opus-5 (kanban-coder)
blocked-by: 012

Decided by ADR-0051. The renderer currently draws straight into the window's
image, which makes post-processing, resolution scaling and an editor viewport
impossible later without taking it apart. This is the one item on the renderer
roadmap that is a rewrite rather than an addition, which is why it is early.

## Goal

The scene draws into an offscreen colour image. Copying that into the acquired
swapchain image is a separate final step. The triangle looks exactly the same.

## The rules

- **One target per frame slot**, following the pattern card 012 established. Not
  one target.
- **Blit** (`vkCmdBlitImage`) for the copy, not a full-screen quad. A quad becomes
  necessary the moment tone mapping or any post-process arrives; nothing
  post-processes yet, so take the simpler one and say in the header that it is a
  starting point.
- **Target format matches the swapchain for now.** Eventually it should not — a
  higher-precision target is what makes tone mapping possible at all. Also a
  starting point, and say so.
- **Resize resizes targets.** The swapchain stops being the resolution authority.
- **Colour only. No depth target in this card** — nothing here has depth, and
  implement-on-demand applies. Depth arrives with the cube card and follows the
  same per-slot pattern. ADR-0051 names both; this is the deliberate reading, and
  it is stated so nobody thinks it was forgotten.

## Turn on back-face culling, and pin the flip

`VK_CULL_MODE_BACK_BIT`. This clears the `DEVIATION:` marker the triangle card left
in `device.c`: the rule says the front-face constant is proven by a test, and with
culling off there was nothing to assert on.

**This card is where that test becomes possible**, because an offscreen image can
be read back. Render a back-facing triangle, read a pixel, assert nothing was
drawn.

**Why it can live in `ctest`:** offscreen rendering needs no window and no
surface, so the test runs headless in `check.cmake` like any other. That is worth
knowing — it is the reason this is a real automated test and not a thing a person
has to look at.

What the test actually pins is the *combination* of the viewport Y flip, the
winding and the front-face constant. Any one of them changing later silently
inverts the result — `CLAUDE.md` warns that flipping twice looks identical to not
flipping at all until something is culled. That is the regression this test exists
to catch, and it will be a future card that triggers it, not this one.

## Verify

- `voe_dev` shows the same triangle, apex up, green on the left. Nothing visibly
  changed and that is the correct outcome.
- The back-face test passes, **and fails when the front-face constant is flipped
  on purpose.** Prove it fires.
- Resize and minimise: validation silent, targets resized.
- `check.cmake` exits zero; `render` builds standalone.
- Windows is the principal's.

## Not this card

Post-processing of any kind, a full-screen-quad composite, a higher-precision
target, depth. Each named above with the card it belongs to.

---

## What was done

`render/src/target.c` is new: the offscreen colour image, one per frame slot,
its memory and its view, built and torn down together on every resize. The
target is a field on `struct voe_render_frame`, which is the pattern card 012
put in place — no new array, no new index. `device->resolution` is the size they
are all built at, and it is what `renderArea`, the viewport and the scissor now
read. The swapchain keeps only its own extent, which is where a frame lands.

`frame.c` split into `voe_render_frame_viewport()` — the one Y flip, now a
function — and `voe_render_frame_draw()`, which clears and draws into a slot's
target and leaves it in `TRANSFER_SRC_OPTIMAL`. What follows in a frame is a
`vkCmdBlitImage` into the acquired swapchain image, `VK_FILTER_LINEAR`, with the
target's extent as the source and the swapchain's as the destination — the same
number today, and the blit is what lets them differ later. The swapchain gained
`VK_IMAGE_USAGE_TRANSFER_DST_BIT` and refuses with a message on a surface that
will not offer it. The acquire semaphore is now waited on at the transfer stage
rather than at colour output, because the scene never touches a swapchain image
any more.

`voe_render_device_new_headless()` is new and internal, in `device_internal.h`.
It is the same `open_device()` the windowed path runs, with the six
surface-dependent decisions taken the other way: no instance extension, no
surface, graphics-only queue family, format taken outright, no device extension,
no swapchain. `voe_render_loader_instance()` and `_device()` gained a flag each
so that extension functions that were never enabled are not demanded of the
driver. That is what lets the new test run under `ctest` with no display.

**Back-face culling is on, and the front-face constant was wrong.** The card
said to prove the flip with a test; the test proved it wrong on the first run.
`VK_FRONT_FACE_CLOCKWISE` with the negative-height viewport culled the triangle
outright — the reasoning written beside it ("the flip reverses the winding, so
reverse the constant") was the double negative `CLAUDE.md` warns about: the flip
is what *removes* the reversal. It is now `VK_FRONT_FACE_COUNTER_CLOCKWISE`,
which is the same word `triangle.slang` and the glTF convention use, with no
translation anywhere. Nothing culled before, so nothing could show it. The
comment in `device.c` and the header of `triangle.slang` were both wrong and are
rewritten.

`render/tests/offscreen.c` is new. It opens a headless device, draws the engine
triangle into slot 0's target through `voe_render_frame_viewport()` and into
slot 1's through the mirror of that viewport, copies both back and checks five
sample points and two whole-image counts. The mirrored viewport is how a back
face reaches the rasteriser without a second shader and without touching the
pipeline under test. The samples are deliberately asymmetric in Y — above the
apex, the middle, below the base, and one inside the wide bottom-left corner
where the green vertex is — so a picture that is upside down fails even though
it still covers the middle. Seven Vulkan functions it needs and `render` does
not are resolved in the test itself rather than added to the table.

## Verified

Linux, Wayland/KWin, clang 22, Debug. Windows unrun, as always.

- `cmake -P check.cmake` — all 15 steps ok, including `standalone render`,
  `tests (7 passed)` and `analyser (24 files)` with no findings.
- The new test fails when the front-face constant is flipped on purpose: 5
  checks report, exit 5. It also fails when the sign of the viewport height is
  removed on purpose: 4 checks report, exit 4. Both were run and both were put
  back.
- `voe_dev`: the same triangle, apex up, red apex, green bottom left, blue
  bottom right, on the same background. Screenshotted and compared rather than
  described.
- Resize and minimise, with `VK_LAYER_KHRONOS_validation` present and
  `VK_LAYER_VALIDATE_SYNC=1`: silent. Driven through 960x540, 640x360,
  1280x720, 0x0, 97x61, 1x1 and 800x600, three times round, by a throwaway
  program outside this repository that hands `voe_render_device_frame()` a
  different size each pass. Synchronization validation found one real hazard on
  the way — the barrier ending `voe_render_frame_draw()` named the blit stage
  where the test copies at the copy stage — and it is fixed with
  `VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT`.

## Notes

- No `DEVIATION:` or `BLOCKED:` markers were left. The one that existed, in
  `device.c`, is gone: it said the front-face constant was set but not proven,
  and it is now proven.
- `CLAUDE.md`'s line "Flipping Y reverses apparent winding, so the front-face
  constant is set to match" reads, in hindsight, as the same reasoning that put
  the wrong constant in the code. It names no constant so it is not wrong, but
  it is the sentence someone will follow next time. Worth a tech-lead's eye;
  not changed from here.
- `kanban/todo/math_init.md` was already staged-added and deleted from the
  working tree before this card started. Left alone.
