# tests

`render`'s own tests: plain C programs with an ordinary `main()`, zero for pass,
found by the build and registered nowhere. A reader is here to find which file
already makes a claim before making it again, or to find where a claim that has
started failing is written down.

The ones marked *headless* run on the headless device — no window and no
compositor anywhere — and check what happened by reading the offscreen colour
image back rather than by trusting the bookkeeping that drew it. That is what
lets them run under `ctest` on a machine with nothing on the screen.

- `loader.c` — that a machine with a driver and no SDK reaches Vulkan.
- `pools.c` — two meshes and two ranges, a texture id that stops naming
  anything when it is destroyed, and a full pool as a returned failure. Its
  header says why those two cases are the ones worth a test.
- `transient.c` — geometry that lives one frame: an id refused by the
  frame after, the same slot drawing different contents, a static and a
  transient range drawn in one frame with the rebind between them, and an
  overrun that is refused without corrupting the frame or the next. Its header
  says why it reads the picture back rather than trusting the bookkeeping.
  Headless.
- `elements.c` — rectangles and letters from records: four colours in one
  draw command read back out of the picture, that the clip rectangle really
  clips, that submission order is paint order both ways round and across the two
  kinds, that the blend multiplied by alpha exactly once, that overrunning the
  element capacity is refused without spoiling the frame or the next, that a mesh
  drawn after two element draws is still drawn right, that a solid and a glyph in
  one frame are one draw, that two ranges of the one buffer land where their own
  two matrices say, that a range past what was submitted is refused with the
  frame left intact, and that a screen-filling surface's corners come out
  strictly inside the near clip boundary rather than on it. Its header says why
  the arrangement is deliberately asymmetrical, why the draw count is counted rather than assumed, which of the
  claims has no picture of its own, why the sheet is hand-made rather than a
  font, what a glyph that named no sheet draws, and why the range test moves only
  the matrix. Five of its claims need no graphics card: the two matrices'
  directions, the near clip boundary, the surface size, and the record's size.
  Headless.
- `matrix.c` — that slangc really was given `-matrix-layout-row-major`,
  checked by making a shader report a known matrix back. Its header says which
  two claims are not in it and where they live instead, and why the three bytes
  it expects are the sRGB encoding of the elements rather than the elements.
- `passes.c` — a frame as a sequence of passes: a second pass loading the
  colour and depth the first left and reading its own camera, a frame with no
  pass presenting the clear colour, the pass capacity refused per frame, and a
  pass with no camera drawing elements. Its header says which wrong
  implementation each picture catches. Headless.
- `targets.c` — a target of one's own shown on the window the right way up,
  once through an image element and once on a mesh, each frame slot reading its
  own slot's picture, a resize that keeps the id and changes the picture's size,
  a sheet rectangle picking half the picture, and the target capacity refused.
  Its header says which wrong implementation each picture catches. Headless.
- `offscreen.c` — that back faces are culled, that the Y flip, the winding
  and the front-face constant agree about which way round that is, and that a
  texture arrives the right way up. Holds its own cube, its own camera and its
  own sun, and drives the same public calls `3d` does. Its header says why the
  sun is turned round for the mirrored case. Headless, so it runs under `ctest`
  with no window anywhere.
