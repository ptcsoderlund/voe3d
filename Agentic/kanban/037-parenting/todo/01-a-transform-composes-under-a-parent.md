# 01 — A transform composes under a parent and back
folder: scene
decisions: 0168, 0281, 0250

## Change
Pure arithmetic, no table yet (0281 point 3). Nothing outside `scene` calls it yet.

- `scene/include/scene/transform_component.h`, `scene/src/transform_component.c`:
  - `voe_scene_transform voe_scene_transform_compose(voe_scene_transform parent,
    voe_scene_transform child);` — the child's world transform when `child` is relative to
    `parent`: position = parent position + parent rotation · (parent scale ∘ child position),
    the rotation applied in double so a far position keeps its millimetres (ADR-0250);
    rotation = parent · child, normalised; scale = parent scale ∘ child scale.
  - `voe_scene_transform voe_scene_transform_relative(voe_scene_transform parent,
    voe_scene_transform placed);` — the inverse: the row that, composed under `parent`,
    gives `placed`. A zero parent scale component gives zero for that component, never a
    division by zero or a NaN.
  - `math` has no quaternion-rotates-a-vector or conjugate; write both as `static` helpers in
    `transform_component.c` (double for the position), not in `math`.
  - Header points, beside the two declarations: what composing means and its order; exact while
    scales are uniform, no shear for a rotated child of a non-uniformly scaled parent, and why
    (0281); zero scale in `relative`.
- `scene/tests/transform.c`: new cases — a parent at (10, 0, 0) turned 90° about Y with scale 2
  and a child at (1, 0, 0) composes to (10, 0, −2) (check the sign against the engine's
  right-handed +Y up and fix the expectation, not the code, if the arithmetic disagrees);
  `relative(parent, compose(parent, child))` gives `child` back within 1e-5 for position,
  rotation and scale; an identity parent composes to the child unchanged; a parent at 1e7 m
  keeps a 1 mm child offset exact in double; a zero parent scale gives a finite result.
- `scene/tests/tests.md`: the `transform.c` line mentions composing and its inverse.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_scene $(ninja -C build/debug
-t targets all | grep -oE "^voe_test_scene_[A-Za-z0-9_]+") && ctest --test-dir build/debug -R
"^scene/"` exits 0, `voe_test_scene_transform` among them.
