# 0419 — The haze's top reaches the pass above the eye, and the sky's stage veils
date: 2026-10-13
by: planner

## Decision
For 087 bug 09, carrying out 0418.

1. **The record gains `haze_top`.** `voe_render_sky`'s word at offset 56 becomes `float haze_top`, the haze
   layer's top in metres above the eye (the Haze row's `height` − the eye's world y), any finite; one
   reserved word stays; 96 bytes. The GPU is eye-relative (0250): `view.eye` is zero, so the eye's height
   cannot come from the view. A pass asserts it finite. `haze` 0 is no veil, so the zeroed record is still
   the old picture.
2. **`3d` computes it.** `voe_3d_draw_system_sky(world, eye)` takes the eye's world position and fills
   `haze_top` = `height` − `eye.y` in double, 0 with no Haze row. `_frame` passes its `eye`; the editor's
   views pass theirs.
3. **The path**, with `T` = `haze_top` and `v` the unit view: `T > 0` and `v.y ≤ 0`, or `T ≤ 0` and
   `v.y < 0`, is endless (`f` = 1); `T > 0` and `v.y > 0` is `L = T / v.y`; `T ≤ 0` and `v.y ≥ 0` is
   `L = 0`. `f` is 0414's fade over `L`; `haze` 0 gives `f` = 0.
4. **Only the sky's full-screen stage veils.** With `drawn` it shows the expose and tone of
   `lerp(radiance, H, f)`, `H` 0416's haze colour, discs and glows inside the radiance. Without `drawn`
   and with `haze` above 0 the stage now runs too and shows `lerp(clear, expose_tone(H), f)` in linear,
   `clear` the colour a pass clears to (a Slang twin of `pass.c`'s), so where `f` is 0 the backdrop is the
   clear colour byte for byte and where it is 1 it meets fully hazed ground. Reflections read the radiance,
   never the veil (0418 point 5).

## Reasoning
- **Metres above the eye, against the world height plus the eye's y:** one float in the record's last spare
  words, in the eye-relative space every other GPU position uses.
- **The eye as a parameter, against each caller subtracting it:** the editor's views build their own frame;
  a parameter cannot be forgotten.
- **A Slang twin of the clear colour, against blending the stage over the clear:** the pipeline's blend
  state stays opaque and the stage stays one formula per case.

## Replaces
Amends 0417 point 1 (the record changes by one reserved word, and `_sky` takes the eye).
