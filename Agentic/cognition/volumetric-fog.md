# Volumetric fog

## Thickness
- The sponsor finds the fog too thin: thickness could only be set to 1.0, and they want it thicker. [intent · tech-lead · 2026-10-30]
- The cap is raised to 4, the mapping kept; see 0499. [confirmed · tech-lead · 2026-10-30]
- The refusal above the top is the only place the bound is enforced; the Inspector's slider takes its ends from the described range, so it follows the bound with no editor change. [observed · planner-112 · 2026-10-31]
- The slider's top and the refusal's top are one named number, so they cannot drift apart. [assumed · planner-112 · 2026-10-31]
- A thickness that is NaN or infinite is refused by the range comparisons themselves; there is no separate finiteness check. [observed · coder-112-01 · 2026-10-31]
- At 1.6/m the fog march's 32 steps put the front surface, soft edges and godrays at one or two samples per pixel with a fixed per-pixel jitter, so thick fog may show a still grain (not banding or flicker); not rendered, not verified. [inferred · planner-112 · 2026-10-31]

## Look beside haze
- "When i set haze to be thicker, its alot thicker then fog. I want fog and haze to look kinda the same. Except that fog has more visual features and is more expensive as we already decided." [intent · tech-lead · 2026-10-30]
- Fog's look is set aside until 106 is built. [intent · tech-lead · 2026-10-30]

## Lighting (0411)
- The fog in the tank project looks brown (the scene has no clouds). It is lit only by the orange sun and the red-brown fill: no sky light (0411) and no many-scattered share as the clouds have. [observed · tech-lead · 2026-10-30]
- Not decided: should fog be lit by the sky and bounce like the clouds, reversing 0411 for fog? The sponsor wants fog and haze to look alike first, fog only richer and dearer; it comes back after 106. [open · tech-lead · 2026-10-30]
