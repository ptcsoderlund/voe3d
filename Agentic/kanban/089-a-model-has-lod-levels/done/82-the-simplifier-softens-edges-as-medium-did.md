# 82 — The simplifier softens edges as Medium did
folder: 3d
after: 78, 81
decisions: 0168, 0435, 0437

## Change
0437 point 2. Follows card 81's `voe_assets_levels`, which has no `strength`. Levels
files stay version 5 (0437 point 4).

- `3d/src/model_levels.c`:
  - `struct strength`, `STRENGTHS` and the comment above them go. In their place, two
    named constants with one comment citing 0437: the angle within which normals share a
    side, 60°, and the divisor of the bounding radius that bounds a collapse's cost, 4.
  - `simplify_level` loses its strength parameter and uses the angle constant; its
    caller (near line 164) computes the error bound with the divisor constant.
  - `voe_3d_model_levels_make` loses the strength assert (near line 273).
- `3d/include/3d/model_levels.h`:
  - THE LEVELS paragraph: normals within 60° share a side and a collapse costs at most a
    quarter of the radius, fixed (0437); the three strengths go.
  - `_make`'s comment drops "as hard as `levels.strength` allows". `_read`'s version
    history stays as it is.
  - The header stays at 60 lines or fewer.
- `3d/src/src.md`, `3d/include/3d/3d.md`: an entry changes only where it names strength.
  Card 78 has just rewritten `src.md`'s `model_levels.c` entry; keep its length.

The tests need no change: they make levels with `VOE_ASSETS_LEVELS_DEFAULT`, which was
Medium, so passing unchanged proves the defaults make the LODs Medium made.

## Done when
The tests `3d/model_levels` and `3d/models_levels` pass after the folder's build,
`grep -ci strength 3d/src/model_levels.c` prints 0, and
`bash ~/.claude/skills/checks/scripts/checks.sh --header-lines 3d/include/3d/model_levels.h`
prints 60 or less.
