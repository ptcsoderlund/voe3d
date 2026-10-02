# 05 — A lit surface reads its point lights' shadows
folder: render
after: 04
decisions: 0168, 0325

## Change
0325 point 4, the lookup.
- `render/shaders/lighting.slangh`: in the binned point lights, for a light with `shadow` non-zero:
  face from `point_shadow.slangh` (include it), the surface pushed one texel along its normal (a
  face texel spans 2·d / size metres at distance d; the size from the frame block or the image's
  dimensions), clip position on layer 6(slot − 1) + face, one compare through binding 9, the light's
  term times lerp(1, visibility, strength). Header phrase: point lights cast now (0325).
- `render/shaders/draw.slang`, `bindings.slangh`: only what the lookup needs reaching it (the
  size, if put in the frame block, with its C twin in `descriptors.c` and its asserts).
- `render/src/pass.c`: when copying a camera pass's points on a device whose maps are not ready,
  every `shadow` copied as 0. Header phrase.
- `render/include/render/device.h`: the point light and pass camera comments say a slotted light
  shadows its term, and reads none on a device not ready.
- `render/shaders/shaders.md`: `lighting.slangh`'s entry.
- `render/tests/point_shadows.c`, under a dark sun on a ground quad, a lamp 1.5 m up at x = −1, a
  0.5 m cube on the ground at x = 0, one point-shadow pass then the camera pass, read back:
  - the ground at x = +1 (behind the cube) darker than the lamp alone lights it with no cube
    drawn in the shadow pass; the ground at x = −2 the same either way;
  - the lamp moved to x = +1: the dark side swings to x = −1;
  - slot 0, or strength 0, reads as no cube in the pass;
  - a device with `point_shadow_size` 0 and the slot set reads as no shadow.
  Entry updated in `render/tests/tests.md`.

## Done when
`ctest --test-dir build/debug -R "^render/point_shadows$"` passes.
