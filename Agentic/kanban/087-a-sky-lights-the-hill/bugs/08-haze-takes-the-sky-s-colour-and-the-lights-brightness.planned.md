# 08 — Haze takes the sky's colour and the lights' brightness

## Seen
Bug 07 was filed as "haze is lit by the directional lights, not the sky", and 0415 and cards 54–55 made the
haze's colour the lights'. The sponsor then said what they meant: "I want directional lights to tint the sky.
Haze should take its color from the sky. But not its intensity for lighting, that should be calculated from
directional lights. So i can only get bright haze when we have bright light. As we said before, for creative
freedom we want to be able to have a dark sky with a bright scene. Maybe, just maybe its reasonable to move out
haze to a component of its own, yeah?"

## Expected
- The directional lights tint the sky (as 0411 has it).
- The haze's colour (its hue) is the sky's.
- The haze's brightness comes from the directional lights, not from the sky's strength. Bright haze needs
  bright light; with no directional light, the haze adds no light.
- A dark sky over a brightly lit scene stays possible: turning the sky's strength down does not darken the
  haze, and turning it up does not brighten it.

- Haze is a component of its own, Rendering / Haze, and works with or without a sky. Without a sky it takes
  the lights' colour as now.

Decided in 0416, which also says what stays of 0414 (haze start) and 0415 (haze coloured by the lights).

## How to reproduce
1. Open lumberhill_project in the editor, with the sky on and haze above 0.
2. Turn the sky's strength down to near 0 with a bright directional light: the haze should stay bright and keep
   the sky's hue.
3. Turn the directional light's strength down: the haze should darken with it.
