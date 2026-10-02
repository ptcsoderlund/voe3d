# render

The public headers, one entry each; the fuller account of this one, and the
questions it answers, is `device.h`'s header.

- `device.h` — the whole public surface of the device: a window it opens onto,
  geometry, textures, shading records and targets uploaded for ids, and frames
  of passes drawn onto the window or a target, lit by a sun and the point lights each pass carries, each with its range and falloff, and the
  point shadow maps' capacity and readiness.
