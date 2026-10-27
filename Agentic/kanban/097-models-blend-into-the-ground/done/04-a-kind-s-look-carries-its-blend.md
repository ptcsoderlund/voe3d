# 04 — A kind's look carries its blend
folder: assets
after: none
decisions: 0168, 0481, 0476

## Change
0481 point 4, the file's half: a foliage kind's look says whether its copies blend with the
ground and how high.

- `assets/include/assets/landscape.h` —
  - `voe_assets_landscape_look` gains, after `shadowless`, `bool blends` and
    `uint16_t blend_height` (centimetres, VOE_ASSETS_LANDSCAPE_BLEND_HEIGHT_MIN 5 to _MAX 1000,
    0 meaning _DEFAULT 50); the three defines beside the spreads'.
  - The format example at the top shows `blend0=1` and `blendheight0=120`; the KIND'S LOOK
    paragraph names both keys; `_read`'s comment adds MALFORMED for a `blend<k>` other than 0
    or 1 and a `blendheight<k>` not a whole number from _MIN to _MAX; `_write`'s says
    `blend<k>=1` only when blending and `blendheight<k>` only when neither 0 nor 50;
    `_look_default`'s says not blending, height 0.
- `assets/src/landscape_foliage.h` and `assets/src/landscape_foliage.c` — the look keys read
  and printed beside `shadow<k>`, refused and asserted as above; the headers' key lists follow.
- `assets/src/landscape.c` — only if the look's default or a range assert lives there.
- `assets/src/src.md` — the `landscape_foliage.*` entries name the new keys.
- `assets/tests/landscape_blend.c` — new (`assets/tests/landscape.c` is 833 lines; read it only
  for how a text is read and written): a text whose kind 0 has `blend0=1` and
  `blendheight0=120` and kind 1 neither reads as blending at 120 and not blending at 0;
  written, it has exactly those two keys and reads back equal field by field; `blend0=2`,
  `blendheight0=4` and `blendheight0=1001` each refused MALFORMED; the default look written
  has neither key. Its header states the claims.
- `assets/tests/tests.md` — an entry for it.

## Done when
`ctest --test-dir build/debug -R '^assets/landscape_blend$'` passes, and the existing assets
tests still pass.
