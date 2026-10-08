# 65 — A whole relight keeps the nest's bounce
folder: render
after: none
decisions: 0168, 0389, 0391

## Change
Card 63 traced 3d's failing BLOCKED check (`3d/tests/bounce_scene.c:566`, patch `98 98 98 against 106 98
98`) to render, with no blocker involved: 3d begins volume 0 at cell (−12,−6,−12), corner (−24,−17,−32),
spacing 2, and volume 3, the 1 m nest, at cell (−12,−4,−4), corner (−12,−9,−12), spacing 1, eye (0,5,8).
After the box moved 1 m along +x was recaptured and relit incrementally the patch reads 106 98 98; a
lights change (bounce strength 1 → 2 → 1, each settled) relights the whole grid and leaves it 98 98 98,
as no bounce. Card 61's case passed because its nest's cell equalled its corner; here they differ, so
suspect the whole path mixing cell and corner (the toroidal index, or the about-the-origin compare).

- `render/tests/blocked_bounce.c`: new case `a_whole_relight_keeps_the_nests_bounce`, in the sun-2
  scene at bounces 1, no blocker. Every frame begins volume 0 and volume 3 with 3d's cells, corners and
  spacings above, the camera pass from eye (0,5,8), the patch read where it projects from there. Steps,
  each settled as the file settles: settle, the patch read (A); move the red wall 1 m along +x and mark
  it stale as `a_recapture_relights_only_what_it_changed` in `render/tests/bounce_probes_scene.c` does
  (read its header), settle, read (B), and check that frame relit the nest incrementally; set the sun's
  `bounce_strength` to twice and back, settle each, read (C). Assert B redder than the patch at bounces 0
  by more than 2/255, and C within 1/255 of B. Header: a paragraph for the case.
- Find the fault by the headers of `render/src/bounce_relight.c` (the `whole` path of
  `voe_render_bounce_relight`), `render/src/bounce_relight.h`, `render/src/bounce_probes.h`,
  `render/src/bounce_volume.c` and `render/shaders/bounce_relight.slang`, then the body of the one at
  fault. Fix it there, in its owner only; correct that file's header if it said otherwise.
- `render/tests/tests.md`: the `blocked_bounce.c` entry names the new case.
- `render/src/src.md` or `render/shaders/shaders.md`: the fixed file's entry, if what it says changed.

If the case passes with no fix, keep it in and block, giving A, B and C and what differs from 3d's frame
(0391: a further block splits bug 05 out of 082).

## Done when
`ctest --test-dir build/debug -R '^render/(blocked_bounce|bounce_)'` passes with
`a_whole_relight_keeps_the_nests_bounce`; then, after `cmake --build --preset debug --target
voe_test_3d_bounce_scene`, `ctest --test-dir build/debug -R '^3d/bounce_scene$'` passes.

## Blocked
The case passes with no fix: A 152 138 138, B 148 138 138 (the move frame relit both volumes by 192
workgroups, incrementally), C 148 138 138, bounces 0 redness 0 — the whole relight keeps the red, in render,
with the scene drawn about the eye (0,5,8) and 3d's cells, corners and spacings. What differs from 3d's frame:
3d also begins volumes 1 and 2, draws each volume's own bounce shadow view (here one view at volume 0's
centre), and its failing check (`3d/tests/bounce_scene.c:566`, still `98 98 98 against 106 98 98`) reads after
a blocker added and removed, not a strength change; per 0391 bug 05 now splits out of 082.
