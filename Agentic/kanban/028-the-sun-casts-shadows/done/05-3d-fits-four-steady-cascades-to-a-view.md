# 05 — 3d fits four steady cascades to a view
folder: 3d
decisions: 0168, 0258, 0250

## Change
The cascade arithmetic on the CPU (0258 point 4), no GPU, nothing calls it yet but its test;
card 06 does.

- `3d/include/3d/projection.h`, `3d/src/projection.c` — `voe_math_float4x4
  voe_3d_projection_orthographic(float half_width, float half_height, float near_plane, float
  far_plane)`: a box to clip space with reversed depth (near 1, far 0), no Y negation. Header:
  the three files that agree about depth now agree for this one too; asserts on empty or
  inverted extents.
- `3d/include/3d/shadow_cascades.h` (new) — `VOE_3D_SHADOW_TEXELS` (2048, what a program opens
  its device's `shadow_size` with), `VOE_3D_SHADOW_REACH` (500 m), `VOE_3D_SHADOW_SPLIT_BLEND`
  (0.9), `VOE_3D_SHADOW_CASTER_REACH` (200 m); `typedef struct { voe_render_shadow shadow;
  voe_render_view light[VOE_RENDER_SHADOW_CASCADES]; } voe_3d_shadow_cascades;` and
  `voe_3d_shadow_cascades voe_3d_shadow_cascades_fit(voe_render_view view,
  voe_math_double3 eye, voe_math_float3 direction, uint32_t texels)`. Header points: what a
  cascade is; near and far are read back out of the view's projection; the split scheme; the
  bounding sphere and why its radius does not turn with the view; the snap in double about the
  world origin and why that is what keeps shadows still and the same 100 km out (0250); the
  box's reach toward the sun; the light's basis and its fallback when the sun is straight up or
  down; every matrix is eye-relative like `view`.
- `3d/src/shadow_cascades.c` (new) — the fit: splits from the near plane to the lesser of the
  far plane and the reach; per cascade the slice's corners from the inverted view × projection,
  the sphere (radius rounded up to a centimetre so float noise cannot change it), its centre taken to double world space, into the light's basis, snapped to whole
  texels (texel = diameter / texels), back to eye-relative; a light view looking along
  `direction` with the orthographic box of the sphere, pulled back by the caster reach;
  `shadow.cascades[i]` = projection × view, `splits`, `texels`, `count` 4. The `light[i]`
  views carry the same view and projection for the shadow passes.
- `3d/include/3d/draw_system.h` — the sentence "no orthographic projection exists anywhere in
  this engine" corrected: the sun's cascades are the one.
- `3d/3d.md`, `3d/include/3d/3d.md` — entry for `shadow_cascades.h` in each; `projection.h`'s
  mentions the box.
- `3d/src/src.md`, `3d/tests/tests.md` — entries for the new files.
- `3d/tests/projection.c` — add: the box's near plane at depth 1, far at 0, a corner at ±1.
- `3d/tests/shadow_cascades.c` (new, no graphics card) — a 60° view at the origin: splits rise
  and the last is min(far, reach); every corner of each slice lands inside its cascade's clip
  box, x and y within ±1, depth within 0..1. Stability: the eye moved by (0.37, 0.11, 0.21) m
  and turned 20°, a fixed world point's texel coordinate in each cascade moves by a whole number
  of texels (within 1e-3) and each cascade's texel size is unchanged. The same three checks with
  the eye 100 km along X. Sun straight down gives a finite basis.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder 3d` prints `FINDINGS: 0`.
2. `ctest --test-dir build/debug -R '^3d/(projection|shadow_cascades)'` passes.
