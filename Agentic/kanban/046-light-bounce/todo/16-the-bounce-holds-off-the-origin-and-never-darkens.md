# 16 — The bounce holds off the origin and never darkens
folder: render
after: none
decisions: 0168, 0307, 0308

## Change
Bug 01: in the editor no tint shows and the ground gets dark gray spots,
while `render/bounce` passes with the grid at lowest cell −16 under a light
view fitted tightly to its scene. This card makes render's own cases hold
where the editor runs, and fixes the stage that fails, here, once. Read
`render/tests/bounce.c` (the scene, light view and pixel lookup to reuse),
`render/src/bounce_schedule.h` / `.c`, `render/src/pass.c` (where it fills
the camera block's bounce record), `render/shaders/lighting.slangh` (the
bounce read), `render/shaders/bounce.slang` (gather), `render/tests/tests.md`
and `render/shaders/shaders.md`.

- `render/tests/bounce_scene.c`, new, header comment, headless, bounce.c's
  scene and shape (share nothing by include; copy the few statics it needs):
  - OFF THE ORIGIN: the wall and ground stand about (−71, 0, −45), the eye
    10 m from them, the grid's lowest cell (−52, −8, −39) (negative, not a
    multiple of 16 or 32) with its corner about the eye, and the bounce
    pass's light view an orthographic box 112 m across (the size 3d's fit
    gives). Ground 1 m from the wall has red over green greater than ground
    10 m away by at least 0.05.
  - NEVER DARKER: sun intensity 9, fill 0.09; a 7 × 7 lattice of ground
    points across the wall's sunlit side and its shadow, every channel with
    the update at least the same pixel with no update, less 1/255.
- Make both pass. The owner of each stage, where the fix goes:
  - which probe a cell is: one wrap of a world cell into 0..31, correct for
    negative cells, declared in `bounce_schedule.h` and used by both the
    schedule's index and `pass.c`'s cell mod 32, so writer and reader cannot
    disagree; a value sent to the shader is already 0..31, never a wrapped
    uint32 turned float;
  - `bounce.slang` gather: a VPL nearer the probe than 1 mm adds nothing (its
    direction is undefined), so no NaN reaches the grid, where the blend
    would keep it for good;
  - `lighting.slangh`: a non-finite E is 0, so the indirect term is never
    below the fill floor.
  Each is a phrase in its file's header comment.
- `render/src/bounce_schedule.h`, `render/tests/bounce_schedule.c`: a case
  that a first call at lowest cell (−37, −5, −23) lists every index in
  0..32767 once.
- `render/tests/tests.md`, `render/src/src.md`, `render/shaders/shaders.md`:
  entries changed or added.

If a case still fails with all three in place and the fault is outside the
files named here, block with the measured pixels.

## Done when
The tests `render/bounce_scene` and `render/bounce_schedule` pass, and
`render/bounce`, `render/bounce_grid` and `render/bounce_map` still pass,
after the folder's build.
