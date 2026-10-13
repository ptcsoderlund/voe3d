# 0411 — Lights light the world and tint the sky; the sky lights nothing
date: 2026-10-13
by: tech-lead

## Decision
For 087, answering its needs-decision (card 32 blocked after three replans). This engine wants creative
freedom and speed. Being physically correct is not a goal.

1. **The sky lights nothing.** It gives no ambient light, no SH, and no fill. Lighting is directional
   lights (each with colour, strength, fill colour and fill strength, plus bounce when turned on), point
   lights and bounce, exactly as before 087. A world with a sky is lit the same as one without. The
   sky-as-fill of 0406 point 3 and 0407 point 7, all of 0408's diffuse term, and 0410's lifted reach are
   withdrawn. Light 0 is no longer special.
2. **Lights drive the sky.** Every directional light draws its own disc and glow in the sky. They gain
   **disc size** and **glow size**. Disc size 0 means the light is not seen in the sky, and glow size 0
   means it has no glow. The disc is the light's colour at a fixed brightness, independent of the light's
   strength and of the sky's strength, so a red sun reads red and never tone-maps to white. The glow tints
   the sky around the disc in the light's colour. Two suns or two moons are two lights. Sun size and sun
   glow leave the sky row. New lights and old scenes default to a visible sun-sized disc with a modest
   glow; the planner picks the numbers.
3. **The sky row holds only its backdrop:** zenith, horizon and ground colours, horizon height, and a new
   **strength** that scales the backdrop (and therefore the haze, point 4) but not the discs. A dark sky
   under a strong light is a supported look. **Sunset goes**: the sky does not react to how high a light
   is. Game code animates the sky's colours for a day and night cycle.
4. **Haze is the sky's.** Far surfaces fade into the sky's gradient at the horizon times the sky's strength
   (0409, plus strength), so the far ground always melts into the horizon behind it, and under a dark sky
   it fades to dark. Lights' glows do not tint the haze. Lights are responsible for all lighting, and the
   sky only for the haze. Richer fog (volumetric and other kinds) is its own later work.
5. **Reflections of the sky stay.** A shiny surface reflects the drawn sky, discs and glows included,
   gated by blockers as before (0408 point 2). It counts as the sky seen in a mirror, not as lighting.
6. **The Picture row (exposure, tone) stays** as 0406 point 4 and 0407 have it.
7. No sky row and no picture row still mean the old picture byte for byte: the tank and coin games are
   unchanged (0405).

## Reasoning
The sponsor: "Dont let sky affect any lighting, but let the lighting affect the sky. … If dev want they
should be able to do dark sky with bright lighting." Since 0406 the sky had been acting as the light, and
bugs 02 and 04 and card 32's block are symptoms of that. Alternatives:
- **Keep the sky as fill and patch the reach (0410):** this is the road that blocked three times.
- **Sun size and glow on the sky:** this cannot draw two suns of different sizes, or a light that is not
  seen in the sky.
- **Disc brightness from the light's strength:** a bright coloured light would tone-map to a white disc.
- **Haze with its own strength:** under a dark sky the far ground would stop matching the horizon.
- **Glows tinting the haze:** the sponsor agreed to keep the haze purely the sky's and leave that to later
  fog work.
- **Drop reflections:** chrome reflecting black looks broken, and the reflection costs almost nothing.

## Replaces
Amends 0405 (point 2: the sky lights no surface; its sun comes from each directional light), 0406 (points 1
and 3: the sun's size and glow move to the light, sunset goes, the sky is not the fill), 0407 (points 6 and 7:
the sky record loses `fill` and the sun's shape; haze as point 4 above), 0409 (haze times strength). Replaces
0408 point 1 and all of 0410. 087 bugs 02 and 04 are void: their fill and reach problems only existed under
sky-as-fill.
