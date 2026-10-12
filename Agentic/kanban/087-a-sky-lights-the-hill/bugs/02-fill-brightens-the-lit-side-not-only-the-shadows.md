# 02 — Fill brightens the lit side, not only the shadows

## Seen
Raising the first directional light's fill strength makes everything brighter, the faces in full sun as
well as the shadows. "when i increase fill strength, everything becomes brighter. I dont want that, only the
casting shadows should be brighter. fill is supposed to be opposite areas of light, meaning shadows."

Haze is fine as it is: "I understand haze now, it looks beautiful."

## Expected
- Fill lights only what the light does not reach: cast shadows, and the side facing away from the light. A
  surface in full light reads the same whatever the fill strength.
- Raising fill makes the shadows brighter, never brighter than the same surface in full light.
- This holds with the sky drawn (the sky as light 0's fill, card 14) and without it.
