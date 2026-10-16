# 07 — A view chooses a model's level
folder: 3d
after: 06
decisions: 0168, 0425

## Change
0425 point 3 as one function, with no state, so the draws (card 08) and the Inspector (card 13)
ask the same question and get the same answer.

- `3d/include/3d/model_levels.h`: declare
  `uint32_t voe_3d_model_level(const voe_3d_model_entry *model, const voe_scene_transform *placed,
  voe_math_double3 eye, const voe_render_view *view)`; include what it needs
  (`<scene/transform_component.h>`, `<render/device.h>` or the header holding `voe_render_view`).
  Header gains a paragraph on the choice: `s` = world radius × |P[1][1]| ÷ the centre's clip w, the
  world radius the model radius times the largest scale component; the largest k below
  `level_count` whose `levels[k].bound` is above `s`; level 0 when the eye is inside the world
  sphere, when the w is not above nought, or for an entry with one level.
- `3d/src/model_levels.c`: the function. The centre placed with `voe_scene_transform_matrix(*placed,
  eye)` (eye-relative, as the draws place a row), then `view->view` and `view->projection`; the
  bounds were made non-increasing at load (card 06), so the walk is one loop from the top.
- `3d/tests/model_levels.c`: a case with a hand-filled `voe_3d_model_entry` (radius 1, centre 0,
  `level_count` 3, bounds infinite, 0.2, 0.05), a view looking down −z with a 60° projection from
  `3d/include/3d/projection.h`, the model placed at distance d on the axis:
  1. d = 0.5 (eye inside): 0;
  2. d = 2: 0;
  3. d where s is about 0.1: 1;
  4. d where s is about 0.01: 2;
  5. scale 10 at the distance of check 4: lower than 2.
  Compute the distances in the test from the projection's P[1][1], not as magic numbers. Its header
  names the case. `3d/tests/tests.md`: the entry names the choice.

## Done when
The test `3d/model_levels` passes after the folder's build, with the new case in it.
