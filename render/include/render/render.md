# render

The public headers, one entry each; the fuller account of this one, and the
questions it answers, is `device.h`'s header.

- `device.h` — the whole public surface of the device: geometry, textures,
  shading records and targets for ids, BC7 or BC5 encoding on the GPU, and
  frames of lit, shadowed, tone-mapped passes onto its window or a target,
  clouds drawn over their world.
