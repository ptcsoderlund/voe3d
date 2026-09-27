# 24 — The light's header says the fill lifts only the shade
folder: scene
decisions: 0168, 0275, 0276

## Change
Words only; no code or test changes.

- `scene/include/scene/light_component.h`: the paragraph "THE FILL IS A FLAT, UNSHADOWED LIFT"
  and the fields comment that calls the fill "the flat lift": the fill lights only what the sun
  does not reach — shadowed sides and sides facing away — fading out as the sun reaches the
  surface (0275, 0276); a surface in full sun looks the same whatever the fill; 0 is still no
  fill.
- `scene/scene.md`, the light component entry: "a flat unshadowed fill" becomes a fill of the
  shade.

## Done when
`grep -niE "flat" scene/include/scene/light_component.h scene/scene.md | grep -i fill` prints
nothing, and `cmake -P check.cmake` exits 0.
