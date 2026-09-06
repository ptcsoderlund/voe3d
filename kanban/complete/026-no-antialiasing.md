# 026 — no antialiasing anywhere

status: review
claimed-by: claude-opus-5 (kanban-coder)
blocked-by: -

**Written by the coder, which is not how cards arrive.** The principal gave this
instruction directly during the review of card 025 — *"Everywhere. We dont want
it, period. It is irrelevant if it makes it look like an engine from 1995"* —
after being shown what it costs. It is written down here because the work is
done and the board should say so, not because a coder may decide this.

**AN ADR IS OWED AND IT IS THE TECH LEAD'S.** This is an engine-wide convention,
and the root's rule is that a decision is not made until its ADR exists. The
coder may not write above `voe3d/`, so this card is the only record inside the
engine. Until that ADR exists, the reason every one of the file headers below
gives is "the engine does not want antialiasing" with nothing to cite.

## What the rule is

**No antialiasing anywhere.** Not on geometry, not on text, not on textures, not
on the final blit. The 1995 look is accepted explicitly and is not a cost to be
negotiated down later by a card that reintroduces one filter "just here".

## Where antialiasing was, and what happened to each

Found by reading rather than assumed — every filtering site in the engine.

| Where | Was | Now |
|---|---|---|
| Geometry edges | `VK_SAMPLE_COUNT_1_BIT` everywhere already | unchanged; there never was MSAA |
| Text edges | `smoothstep` across one screen pixel, width from `fwidth` | `step(0.5, field)` — a hard cut |
| Texture magnification | `VK_FILTER_LINEAR` | `VK_FILTER_NEAREST` |
| Texture minification | `VK_FILTER_LINEAR` over a generated mip chain | `VK_FILTER_NEAREST`, one level |
| Mipmap generation | a blit chain per texture | **removed** — see below |
| Target to swapchain | `VK_FILTER_LINEAR` blit | `VK_FILTER_NEAREST` |
| Glyph coverage AA | `VOE_TEXT_RASTER_SAMPLES`, 8 sub-scanlines | **left alone** — see below |

### The mipmap code is gone rather than switched off

`mip_levels_for`, `can_generate_mipmaps` and `generate_mipmaps` are deleted, with
the transitions and the `levels` parameters that existed only to serve them. A
texture image is now created with `mipLevels = 1` and without
`VK_IMAGE_USAGE_TRANSFER_SRC_BIT`, which it only ever had so the blit chain could
read it back out of itself.

**Deleted and not left dormant, because `-Werror` decides it.** With levels
pinned at one those three functions become unreachable, and an unreachable static
is a build failure here. There is no version of this change that keeps them.

### `VOE_TEXT_RASTER_SAMPLES` is untouched, and that is deliberate

It is the coverage antialiasing in `voe_text_raster_fill`, which **nothing calls**
— the sheet is a distance field now. It antialiases nothing because it runs
never. The tech lead's answer to question 1 of card 025 was to keep that function
pending D-106, so it stays; if D-106 ever revives it for card 023's panel, **that
card has to make it produce hard coverage**, because reviving it as written would
put antialiasing back in through a side door.

## The consequence nobody should rediscover as a bug

- **Textures show their texels close up and shimmer at a distance.** The mip chain
  was the only thing preventing the second, and it is gone on purpose.
- **Text a long way off breaks into disconnected specks, and they move as the
  camera moves.** The dev sign is where to see it.
- **A render resolution below the window's will show square pixels**, not a blur.

Every one of those has the reason written at the site, because each looks exactly
like a fault in the sampler to somebody who does not know.

## What card 025 turns out to have been for

**The distance field is what makes not antialiasing look like a letter.** A hard
cut through the field puts the edge exactly on the outline at any size — a letter
walked up to has straight sides, a clean apex and square corners, with the
stair-steps one screen pixel each. A hard cut through the coverage atlas that
preceded it would have put the edge on the nearest texel, and the same letter
would have come out in blocks the size of the sheet's texels.

So 025 is not undone by this and its three channels still earn their place: the
corner reconstruction is what keeps a hard cut square rather than notched.

## Two things this leaves behind for a decision

1. **`voe_render_sampling`'s values are now misnamed.** With filtering gone,
   `SMOOTH` and `SHARP` differ only in addressing — REPEAT against
   CLAMP_TO_EDGE — and neither smooths or sharpens anything. The headers say so
   plainly rather than pretending, but **renaming them is an API change and a
   decision**, so it was not taken here. The honest names are about wrapping.
2. **`3d`, `assets` and `dev` still pass `SMOOTH` at every call site**, which is
   still correct for what it now means, and is worth re-reading once the enum is
   renamed.

## Verified

Linux, `cmake -P check.cmake` **exits zero** — all 21 steps, 31 tests, the
analyser over 86 files with no finding. Windows is the principal's.

Screenshots before and after were taken of the whole dev scene and of one letter
magnified until it fills much of the screen. The cube texture is visibly
pixelated close up, the text is a hard cut, and the magnified letter's edges are
straight lines with one-pixel steps rather than texel blocks — which is the
claim the shader's header now makes.

**No `DEVIATION:` and no `BLOCKED:` markers.**
