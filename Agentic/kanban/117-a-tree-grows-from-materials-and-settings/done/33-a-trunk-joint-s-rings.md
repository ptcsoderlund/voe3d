# 33 — A trunk joint's rings
folder: 3d
after: none
decisions: 0168, 0518, 0519, 0514, 0515

## Change
New private files `3d/src/tree_joint.h` and `3d/src/tree_joint.c`: the geometry of 0519, pure arithmetic,
no build, no arena. `tree_mesh.c` does not call it yet (card 34).

- A struct for one trunk stretch as a level draws it: its lower and upper ring (centre, radius, v in metres
  along the trunk, the lower ring's unit frame across the trunk at angle 0 and 90°), its side count and its
  u turns (whole metres of the trunk's base circumference, at least 1, as `tree_mesh.c` uses).
- A struct for one joint: J, E, unit `t` and around-direction, A and B (0519 points 1–2).
- `bool voe_3d_tree_joint_of(voe_3d_tree_segment trunk, voe_3d_tree_segment branch, voe_3d_tree_joint *out)`:
  the joint from the skeleton alone, using `radius_at`'s rule for the trunk's radius (copy the rule; it is
  static in `tree_mesh.c`). False when the branch's first segment never leaves the trunk.
- `void voe_3d_tree_joint_ring(joint, stretch, offsets, count, radius, own_turns, own_v, share, positions,
  normals, uvs)`: one ring of `count` vertices (sides + 1, the last the seam) at `share` 0–1 of the flare:
  0 the base ring pushed onto the stretch's facets, 1 the branch's plain ring at E; between, on the chords
  (point 3); normals and uvs blended by point 4's weight. `offsets` are the branch's unit ring offsets o_k
  at E; the branch's own u is `own_turns` × k / sides and its own v `own_v`.

`3d/src/src.md`: an entry for `tree_joint.c`/`.h`. The header says what a trunk joint is, why chords, why J
is set back, how the trunk's uv is read without wrapping, and that far levels differ only in the snap.

New test `3d/tests/tree_joint.c` (registered by the folder's glob). A straight trunk stretch along +Y,
R 0.2, a branch of r 0.08 at 60°, 8 sides; again at 30° and 110°, and with a 3-sided trunk:
- share 0: every vertex lies on the stretch's faceted surface within 0.1 mm, its normal within 0.5° of the
  trunk's radial normal there, its uv the trunk's there within 1e-4;
- shares 0, 1/3, 1/2, 2/3, 1: no vertex nearer the axis than the faceted surface minus 0.1 mm;
- share 1: the plain ring of radius r about E, normals o_k, uvs the branch's own;
- the base ring spans further along the trunk than around it;
- E − J makes θ/2 with the trunk within 0.5°;
- the u of the base ring never jumps by more than half a turn between neighbours, J placed across the
  trunk's u seam.

## Meaning
- Believed (coder 117-02): bark wraps a limb over whole metres of its own base circumference, so u can be
  shifted by whole numbers without a visible change. The coder confirms the trunk's u turns in `put_rings`
  or `put_tube` and blocks if the mapping differs.

## Done when
`ctest --test-dir build/debug -R '^3d/tree_joint$'` exits 0 with every test above in it.
