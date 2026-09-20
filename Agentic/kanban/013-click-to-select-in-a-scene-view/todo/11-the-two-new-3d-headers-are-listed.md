# 11 — The two new 3d headers are listed
folder: 3d
decisions: 0168, 0202, 0203

## Change
Cards 01 and 03 added `3d/include/3d/outline.h` and `3d/include/3d/shape_geometry.h` and gave both their full
account on `3d/3d.md`, but the headers' own table of contents, `3d/include/3d/3d.md`, never learned them. Every
folder holding code carries a `.md` that lists its files (decision 0168, rule on folder maps), so the tree does
not check until it does.

Change that one file and no other. Add two entries in the one-line style the file already uses, each in the
same place the fuller account has it on `3d/3d.md`:

- `shape_geometry.h`, directly after the `shape_system.h` entry — the built-in shapes' triangles, and the edges
  of each surface with the two normals that meet along them, as the CPU sees them.
- `outline.h`, directly after the `pick.h` entry — one entity's silhouette, seen from one camera, as the quads a
  selection outline is drawn from.

Nothing else on that file moves, and no header, source or test is touched: the two files exist, are built and
are tested already. Read no code.

## Done when
`checks.sh --structure` prints `FINDINGS: 0` and exits 0 — it is the run that reads every folder's `.md`, and
today it reports exactly the two missing entries this card adds.

`checks.sh --folder 3d` exits 0.
