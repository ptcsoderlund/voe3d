# render

The public headers, one entry each; the fuller account of this one, and the
questions it answers, is `device.h`'s header.

- `device.h` — the whole public surface of the device: opened and prepared, it
  takes geometry, textures (RGBA8 or BC blocks), shading records and targets for
  ids, counts what material textures take, and draws frames of lit, shadowed,
  tone-mapped passes onto its window or a target.
