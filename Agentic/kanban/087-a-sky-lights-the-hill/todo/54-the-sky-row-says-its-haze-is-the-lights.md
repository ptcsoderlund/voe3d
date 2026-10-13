# 54 — The sky row says its haze is lit by the lights
folder: scene
after: none
decisions: 0168, 0415

## Change
Bug 07: the haze's colour is the directional lights' (0415 point 1), so the sky row's words about
`strength` and haze are wrong. Comments and index only; no code changes.

- `scene/include/scene/sky_component.h`, header: the first line's decisions add 0415; `strength`
  scales the backdrop only, not the haze and never a light's disc or glow; `haze` and `haze_start`
  set how far the fade reaches, and the colour it fades to is the directional lights', not the
  sky's (0415). Keep each field's bounds as they are.
- `scene/include/scene/scene.md` (`sky_component.h`): the haze is lit by the lights, not the sky.
  One sentence of at most 300 characters.

## Done when
The test `scene/sky` passes after the folder's build.
