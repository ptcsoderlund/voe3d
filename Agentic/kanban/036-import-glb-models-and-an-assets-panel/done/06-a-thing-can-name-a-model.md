# 06 — A thing can name a model
folder: 3d
decisions: 0168, 0277, 0191, 0221

## Change
A new component and its system (0277 point 1), made the way the shape is: read the headers of
`3d/include/3d/shape_component.h`, `3d/include/3d/shape_system.h` and `3d/src/shape_component.c`
and follow them. Nothing draws it yet.

- New `3d/include/3d/model_component.h`:
  - `#define VOE_3D_MODEL_PATH 128`; `voe_3d_model` described with one field,
    `F(char, path, CHAR, VOE_3D_MODEL_PATH)`; `voe_3d_model_key`.
  - `void voe_3d_model_register(voe_ecs_world *world, uint32_t capacity);` — table, default
    row the empty path, needs a transform, menu path `Rendering / Model`, the intent below as
    its replace with room for `capacity`.
  - `voe_3d_model_add`, `voe_3d_model_get`, `voe_3d_model_count`, `voe_3d_model_rows`,
    `voe_3d_model_entities`, shaped as the shape's.
  - `voe_3d_model_intent { voe_ecs_entity entity; voe_3d_model model; }`,
    `[[nodiscard]] bool voe_3d_model_submit(voe_ecs_world *, voe_3d_model_intent);` and
    `void voe_3d_model_system_run(voe_ecs_world *);` which drains: a dead entity or one with
    no model is dropped silently; a path with no NUL in its 128 bytes has its last byte made
    NUL and is reported as corrected, edge-triggered as the shape's corrections are.
  - Header points: the path is project-relative with `/`; empty draws nothing; what loads and
    draws it is `3d/models.h` and whoever reads files (0277 points 2–4); a path, not an id,
    because the Inspector, scene text, undo and the cook already carry CHAR fields.
- New `3d/src/model_component.c`.
- New `3d/tests/model_component.c`: register transforms then models; add a row, submit a new
  path, run, read it back; a dead entity's intent is dropped; the description has one CHAR
  field `path` of 128. List it in `3d/tests/tests.md`.
- `3d/3d.md` and `3d/src/src.md`: an entry each.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_3d $(ninja -C build/debug -t
targets all | grep -oE "^voe_test_3d_[A-Za-z0-9_]+") && ctest --test-dir build/debug -R "^3d/"`
exits 0, `voe_test_3d_model_component` among them.
