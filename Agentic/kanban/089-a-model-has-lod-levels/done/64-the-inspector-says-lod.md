# 64 — The Inspector says LOD
folder: editor
after: 63
decisions: 0168, 0425, 0433

## Change
0433: the Inspector's Level section reads LOD, LOD 0 the full model. Only the words on screen
change; type and function names stay.

- `editor/src/inspector_level.c`: the heading "LOD" (line ~78); each row
  "View <n>: LOD <k> of <count>, made" or "…, own", k the level drawn counted from 0 and count
  the model's levels; a model without levels "View <n>: LOD 0 of 1". The longest,
  "View 4: LOD 5 of 6, made", fits the 32-byte buffer.
- `editor/src/inspector_level.h`: the header says the "LOD" heading and the row form, with 0433;
  the constraints say "LOD"'s three, and `VOE_EDITOR_INSPECTOR_LEVEL_ELEMENTS` counts 3 for the
  heading where it counts 5.
- `editor/src/inspector.h`: the comments naming the Level section (lines ~327, ~362) call it the
  LOD section.
- `editor/src/interface.h`: THE INSPECTOR'S LEVEL SECTION paragraph (line ~377): "LOD", three, so
  129 elements and 51365 + 129 = 51494; the ENABLED ROW paragraph's sum 51494 + 12 = 51506; THE
  LEVELS QUESTION's 51506 + 238 = 51744; `VOE_EDITOR_INTERFACE_ELEMENTS` 51744. Nodes unchanged.
- `editor/src/src.md`: the `inspector_level.h` and `.c` entries say the LOD section, each still
  one sentence.

## Done when
`voe_editor` builds in the folder's check with no warning, and
`grep -n '"LOD"' editor/src/inspector_level.c` prints a line. The human runs bug 06's How to test
(`Agentic/kanban/089-a-model-has-lod-levels/bugs/06-it-should-say-lod-and-go-to-six.planned.md`):
1. Right-click a `.glb` in Assets: Make LODs; the question "LODs of <file>", LODs 1 to 6.
2. Choose 6 and Medium on the stable, Make; place it, select it, fly slowly away: the Inspector
   says LOD 0, 1, … up to 5, and the Frame panel's Triangles drops at each.
3. Make LODs with 6 on a small rock: it stops at fewer, and the Inspector's "of <count>" says how
   many.
4. Choose 2 on a model: the question shows 2 when opened again; the Inspector goes no higher than
   LOD 1.
And the feature's How to test, steps 1 to 6, still hold.
