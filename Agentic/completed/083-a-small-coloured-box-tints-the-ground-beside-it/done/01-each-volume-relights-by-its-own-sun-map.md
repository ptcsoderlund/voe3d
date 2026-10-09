# 01 — Each volume relights by its own sun map
folder: render
after: none
decisions: 0168, 0389, 0391

## Change
082 left one fault: in 3d's frame (`3d/tests/bounce_scene.c`), after a 1 m move of the red box is
recaptured and relit incrementally, a whole relight (bounce strength 1 → 2 → 1, or a blocker added and
removed) leaves the 1 m nest's patch at `98 98 98` against `106 98 98`. Card 65 repeated 3d's frame in
render and it passed, but with one bounce sun view, fitted at volume 0's centre, for both volumes. 3d draws
each volume's sun map at a view fitted to that volume (`voe_3d_bounce_grid_sun`). That is the last
difference left between the two, so this card closes it.

- `render/tests/blocked_bounce.c`, case `a_whole_relight_keeps_the_nests_bounce`: each begun volume's
  bounce shadow pass uses the view 3d would hand it, volume 0's for the level grid at cell (−12,−6,−12),
  spacing 2, and the nest's for cell (−12,−4,−4), spacing 1, eye (0,5,8). render cannot link 3d, so add a
  test-local helper that repeats `voe_3d_bounce_grid_sun`'s arithmetic. Read it in
  `3d/include/3d/bounce_grid.h` (the sun-map paragraph), the body of `voe_3d_bounce_grid_sun` in
  `3d/src/bounce_grid.c`, and the light-box calls it makes in `3d/src/light_box.c`. The steps, A, B and C,
  and the assertions stay as they are. Header: the case's paragraph says each volume has its own view, and
  why.
- If the case now fails, the fault is render's. Find it from the headers of `render/src/bounce_shadow.c`
  (one map per slot, a layer per sun, redrawn per begin; each layer's view × projection kept in the slot),
  `render/src/bounce_relight.c`, `render/src/bounce_relight_build.c`, `render/src/bounce_relight.h` and
  `render/shaders/bounce_relight.slang`, then read the body of the one at fault. One suspect: a value
  written on the CPU when a relight is recorded but read by the GPU when it runs, so that two volumes
  relit in one frame both read the last volume's sun view. Fix it in its owner and correct that file's
  header if it said otherwise. Also add the entry to `render/src/src.md` or `render/shaders/shaders.md` if
  what the file does changed.
- If the case passes with no fix, keep it: it proves render keeps a nest's bounce under its own sun map, and
  card 03 then knows the fault is 3d's. Say so in the commit message, with A, B and C.
- `render/tests/tests.md`: the `blocked_bounce.c` entry says the volumes have their own sun views.

## Done when
After `cmake --build --preset debug --target voe_test_render_blocked_bounce`,
`ctest --test-dir build/debug -R '^render/blocked_bounce$'` passes with the case changed as above.
