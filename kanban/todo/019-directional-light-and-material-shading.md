# 019 — one directional light, with material shading

status: todo
claimed-by: -
blocked-by: 018

The card that makes it stop looking like a programming exercise.

## Goal

A model lit by one sun, with its material actually affecting how it looks.

## Scope

- **One directional light.** Direction, colour, intensity. No point lights, no
  spotlights — *many lights* is on the *later* list.
- **No shadows.** The principal's decision, and shadows are their own large card.
- **Material shading using what card 018 already parsed**: base colour, metalness,
  roughness, occlusion. Metalness–roughness, matching glTF, because that is what
  the importer produces and inventing a second model would mean converting.
- Normals from the model. **Normal matrices are not model matrices** — non-uniform
  scale is where that becomes visible, and it looks like broken lighting rather
  than broken maths.

## Where this card is likely to go wrong

- **Colour space.** Base colour textures are sRGB; normal, occlusion, roughness and
  metalness maps are **not**. Getting this wrong makes everything slightly too dark
  or too flat and it is almost impossible to spot without a reference. State the
  choice per texture kind explicitly.
- **Nothing here is tone mapped yet**, so bright values clip. That is expected, it
  is on the *later* list, and card 013's offscreen target is what will make it
  possible. Do not sneak it in.

## Verify

- Rotate the light and the shading follows. Rotate the model and the shading stays
  attached to the surface, not to the screen.
- A rough material and a smooth one look different. A metal and a non-metal look
  different.
- A non-uniformly scaled model is still lit correctly — the normal matrix test.
- `check.cmake` zero. Windows is the principal's.
