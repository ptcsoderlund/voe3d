# 0416 — Haze is its own component: the sky's hue at the lights' brightness
date: 2026-10-13
by: tech-lead

## Decision
For 087 bug 08. The sponsor: "I want directional lights to tint the sky. Haze should take its color from the
sky. But not its intensity for lighting, that should be calculated from directional lights. So i can only get
bright haze when we have bright light. As we said before, for creative freedom we want to be able to have a
dark sky with a bright scene."

1. **Haze is a component of its own**: Rendering / Haze, a scene row beside Sky and Picture. It holds `haze`
   (metres past the start at which half a surface's colour is haze; 0 none) and `start` (metres, at least 0,
   default and unsaid 100), with 0414's meaning and fade `f = 1 − 2^(−max(d − start, 0)/haze)`. The sky row
   loses `haze` and `haze_start`. No Haze row means no haze. No Sky, no Haze and no Picture row still mean
   the old picture byte for byte. Haze works with or without a sky.
2. **Its brightness is the lights'.** `B = Σ max(c_i) × I_i / (4π)` over the pass's shown directional lights
   (0415 point 1's set and gain). The largest channel of a light's colour times its intensity, so the
   light's colour does not change how bright the haze is. No shown light means `B = 0`, and far surfaces fade
   towards black. No fill, shadow, gate, point light, bounce or view term (0415 point 2).
3. **With a Sky row, its hue is the sky's.** The haze colour is `hue × B`. `hue` is the sky's gradient at
   the horizon (0409, no disc, no glow) divided by its largest channel, so its brightest channel is 1. A
   black horizon gives white. The sky's `strength` does not reach the haze, and neither does how bright its
   colours are. A dark sky over a bright scene gets bright haze of the sky's hue. The lights' glows and
   colours do not tint the haze when there is a sky. They tint the sky.
4. **Without a Sky row, its hue is the lights'.** The haze colour is 0415's `Σ c_i × I_i / (4π)`.
5. The planner chooses the records that carry it, and how a scene saved with haze on its sky row reads. The
   sponsor's hill should keep the haze it was given.

## Reasoning
- **Haze on the sky row:** it held a setting that no longer depends on the sky, which is bugs 06–08. Its own
  row lets you author a night sky with bright haze, or haze with no drawn sky, and turn haze off by removing
  the row.
- **The horizon colour as authored, against hue only:** a dark sky would give dark haze under any light,
  which breaks the dark-sky, bright-scene look the sponsor asked for in 0411.
- **No haze without a sky:** this would tie the new component back to the sky. Taking the lights' colour
  instead is 0415, already built.
- **Brightness by luminance, against largest channel:** a pure red sun would light the haze a fifth as
  much. The largest channel measures colour the same way the sky's hue does.

## Replaces
Amends 0411 point 4, 0414 (its fields move from the sky row to the Haze row), and 0415 (point 1 holds only
with no sky, and its brightness is as point 2 here).
