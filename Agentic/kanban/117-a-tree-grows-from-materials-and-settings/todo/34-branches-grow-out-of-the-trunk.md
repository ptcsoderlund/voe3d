# 34 — Branches grow out of the trunk
folder: 3d
after: 33
decisions: 0168, 0518, 0519, 0514, 0515

## Change
`3d/src/tree_mesh.c` (`tube_of`, `put_tube`, `put_rings`, `bark`, the vertex counts):
- When the trunk (the first limb put) is put at a level, keep its rings as that level draws them (centre,
  radius, v, frame, sides, u turns) on the build, so a later limb can find the trunk stretch around a point.
- An order-1 limb whose `voe_3d_tree_joint_of` succeeds is a trunk joint: its tube is the base ring, the
  collar rings (two at levels 0–1, one at 2–3), the ring at E, then today's rings at its skeleton points
  after its start, all from `voe_3d_tree_joint_ring` up to E. The start ring on the trunk's axis and the
  halfway collar are dropped for it. Its frame and own v are carried from E as `put_rings` carries them.
- Twigs, and a branch whose joint fails, keep today's start ring and halfway collar unchanged.
- Counts follow: `tube_of` counts a trunk joint's rings per level; raise `MOST_RINGS` if it must. No
  xorshift draw is added, removed or reordered, so the pinned leaf cards hold.

`3d/include/3d/tree_grow.h`: the levels point says a branch's joint on the trunk is a flare from an ellipse
on the trunk to the branch at half its angle, with the trunk's normals and uvs at its base (0519), at
every level, and that twigs keep the halfway collar. Fix the header of `tree_mesh.c` and the `tree_mesh.c`
entry in `3d/src/src.md` where they say "a child's collar".

New tests in `3d/tests/tree_grow.c`, on a hand-built skeleton: a straight trunk of 8 segments along +Y,
radius 0.2, one branch of 4 segments at 60° from trunk segment 3, one twig of 2 segments from the branch's
segment 2, meshed with default values:
- at every level, every bark vertex beside the trunk (its projection within the trunk's length) lies no
  nearer the trunk's axis than R·cos(π / that level's trunk sides) − 1 mm;
- at level 0, at least (branch sides + 1) bark vertices besides the trunk's own lie within 1 mm of its
  surface with the trunk's radial normal within 1°;
- at level 0 the twig still starts as today: some bark vertex lies within its base radius + 1 mm of its
  start on the branch's axis.
`3d/tests/model_prepare_grown.c` must still pass unchanged: the joint is at every level, so errors still
rise level by level.

## Meaning
- Believed (coder 117-02): today a child's first ring sits at its start on the parent's axis, then one
  ring halfway to the parent's surface radius; `tube_of` adds it to every limb of order > 0.
- Scout: errors are measured on positions only; no test sets a ceiling. 0518's "within 0514's errors"
  holds because the swap bounds come from the measured errors, which this card does not bypass.

## Done when
`ctest --test-dir build/debug -R '^3d/(tree_grow|tree_joint|model_prepare_grown)$'` exits 0 with the new
tests in it.

Human, how-to-test step 16 of `feature.md`: open the spruce and the birch, look close where branches leave
the trunk (no crease in the light, no break in the bark, a swelling that bends out), see branches on
branches unchanged, then fly away and see no pop.
