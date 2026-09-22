# tests

`render`'s own tests: plain C programs with an ordinary `main()`, zero for pass, found by the build
and registered nowhere. A reader is here to find which file already makes a claim before making it
again. A test marked *headless* runs on the headless device — no window, no compositor — and checks
by reading the offscreen colour image back.

- `card.c` — which graphics card is taken, on made-up facts with no card: the discrete over the
  integrated wherever listed, the next one down when the fastest cannot present, a software
  rasteriser alone, the larger memory, and a card without 1.3 or a drawing queue never.
- `loader.c` — that a machine with a driver and no SDK reaches Vulkan.
- `pools.c` — two meshes and two ranges, a texture id that stops naming
  anything when it is destroyed, and a full pool as a returned failure. Its
  header says why those two cases are the ones worth a test.
- `transient.c` — geometry that lives one frame: an id refused by the frame after, the same slot
  drawing different contents, a static and a transient range in one frame, and an overrun refused
  without corrupting the frame. Headless.
- `elements.c` — rectangles and letters from records: the colours, the clip rectangle, paint order
  both ways round, the blend, the capacity refused, the two matrices, the near clip boundary and the
  stroke widths, thin, aligned and wide. Five of its claims need no graphics card. Headless.
- `matrix.c` — that slangc really was given `-matrix-layout-row-major`, checked by making a shader
  report a known matrix back. Its header says which two claims are not in it and where they live
  instead.
- `passes.c` — a frame as a sequence of passes: a second pass loading what the first left and
  reading its own camera, a frame with no pass presenting the clear colour, the pass capacity
  refused, and a pass with no camera drawing elements. Headless.
- `targets.c` — a target of one's own shown on the window the right way up, each frame slot reading
  its own slot's picture, a resize that keeps the id, the capacity refused, and a target read back
  as RGBA8. Headless.
- `offscreen.c` — that back faces are culled, that the Y flip, the winding and the front-face
  constant agree about which way round that is, that a texture arrives the right way up, and that an
  object record's colour tints what is drawn. Headless.
