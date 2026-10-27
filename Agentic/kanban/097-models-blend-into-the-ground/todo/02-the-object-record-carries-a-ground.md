# 02 — The object record carries a ground
folder: render
after: 01
decisions: 0168, 0481

## Change
0481 point 2: the record's room for a blending draw's ground. Fields and comments only; no
shader reads them yet (card 03).

- `render/include/render/device.h` — `voe_render_object` (about lines 744–828):
  - `reserved1` becomes `uint32_t ground`: a landscape's heights texture index plus one, nought
    none. `reserved2` becomes `float ground_reach`: metres the blend reaches up.
  - After them, `voe_math_float4 ground_rows[3]`: the top three rows of the affine map from
    camera-relative space to the landscape's own metres (its grid centred on the origin).
  - The comment above the struct gains a paragraph: a GROUND RECORD has `ground` not nought and
    `heights` nought; it names the landscape's `paint` and `layers` and its side in
    `terrain.w`, read only when `ground` and `paint[0]` are not nought; the mesh fragment stage
    blends toward the painted layers near the ground (0481 point 1), the shadow, point-shadow
    and capture passes do not; the fields grew the record from 272 to 320 bytes. The `dither`
    paragraph no longer speaks of three reserved words but of the words after it. The `terrain`
    field's comment says a ground record reads its `w`.
- `render/shaders/bindings.slangh` — the Slang object struct (about lines 255–310) mirrors it:
  `ground`, `ground_reach`, `ground_rows[3]`, its comment pointing at device.h's.
- `render/src/records_layout.c` — the size assert 320; offset asserts for `ground` (264),
  `ground_reach` (268) and `ground_rows` (272); its comment's sizes follow.
- `render/render.md` and `render/shaders/shaders.md` — where they say what the object record
  carries, a ground too.

Anything in `render/src` that zeroes or copies a record by size keeps working by `sizeof`; a
place that writes the reserved words by name now writes the new names.

## Done when
`render` builds (the layout asserts are compile-time) and the existing render tests pass.
