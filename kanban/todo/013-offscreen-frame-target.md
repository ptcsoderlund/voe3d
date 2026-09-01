# 013 — the frame goes into an offscreen image

status: todo
claimed-by: -
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
