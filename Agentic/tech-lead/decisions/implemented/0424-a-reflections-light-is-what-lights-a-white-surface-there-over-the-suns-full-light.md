# 0424 — A reflection's light is what lights a white surface there, over the suns' full light
date: 2026-10-13
by: planner

## Decision
For 088 bug 01, 0423 point 4: how the light on a surface is measured and how it scales the sky's
reflection.

1. **The light at a surface, L (rgb),** is what lights a white surface there, times π, from the terms
   the lit exit already computes: each directional light's colour × intensity × its reach (N·L times its
   shadow) where its gate passes, each point light's colour × its falloff × N·L × its visibility where
   its gate passes, and π × the indirect light (the bounce's E/π with the fills' floor, as
   `voe_render_indirect` gives it for a base colour of one).
2. **Full light, F,** is the largest channel of Σ colour × intensity over the pass's directional
   lights shown in the sky (`voe_render_sky_shown`), and never below 1: a white light of intensity 1
   head-on is full light when the suns are dimmer or absent.
3. **The reflection is scaled by k = saturate(max channel of L / F)**, one number: the reflection
   keeps the sky's own colours and only its brightness follows the light.
4. **The sky a surface mirrors is at strength 1:** `sky(R)` and the nine harmonics for a rough metal
   are the backdrop's authored colours, glows and discs as before, whatever the row's strength.
5. Everything else of 0411 point 5 stays: metalness, Karis's fit, the roughness lerp, the blockers'
   gate, nought unless the sky is drawn. Without a Sky row k is never applied to anything.

## Reasoning
Reusing the terms the lit exit already has costs no extra shadow lookup or bounce read, and makes the
reflection dark exactly where the colour is dark (0423 point 2). Normalising by the suns makes full
sunlight show the sky at its own colours whatever the sun's intensity; the floor of 1 keeps a night
lit only by lamps from mirroring the whole sky at full brightness, and avoids dividing by nought.
Alternatives:
- **k = the shadow alone:** a face turned from the sun or lit only by fill would still mirror fully.
- **Scale by L itself, no normalising:** a strong sun would push the reflection far past the sky's own
  colours, against 0423 point 2.
- **Tint the reflection by the light's colour:** it would no longer read as the sky in a mirror.

## Replaces
Nothing; it fills in 0423 point 4.
