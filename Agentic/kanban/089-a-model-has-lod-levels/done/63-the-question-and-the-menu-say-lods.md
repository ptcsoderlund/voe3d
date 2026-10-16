# 63 — The Assets menu and its question say LODs, 1 to 6
folder: editor
after: 59, 61, 62
decisions: 0168, 0429, 0430, 0433

## Change
0433: on screen a model's levels are LODs; only words shown change, never code names or `.parts`
keys. The question offers 1 to `VOE_ASSETS_LEVELS` (6, `assets/include/assets/parts.h`).

- `editor/src/assets_menu.c`: the row's label "Make LODs" (line ~21). `assets_menu.h`: its
  comments name the row Make LODs; the enum name `VOE_EDITOR_ASSETS_MENU_MAKE_LEVELS` stays.
- `editor/src/levels_ask.h`: `title`'s room `sizeof "LODs of "`; the header says the panel is
  "LODs of <file>", its LODs row 1 to 6 (0433).
- `editor/src/levels_ask.c`: the title "LODs of %s" (line ~100); the row label "LODs" (line
  ~142); `COUNT_NAMES` "1" to "6".
- `editor/src/interface_assets.c`: the notice when the save fails says its LODs could not be
  saved (line ~232); the header's Make levels sentence says Make LODs.
- `editor/src/interface.h`, THE LEVELS QUESTION paragraph (line ~350): fifteen buttons, 1 to 6
  among them, so thirty-nine nodes; elements: the title "LODs of " less the zero, 136; "LODs",
  "Strength" and "Swap", sixteen; each button's border and fill, thirty; "1" to "6" and the
  rest, fifty-four; 238 in all. The sums 2563 + 39 = 2602 and 51508 + 238 = 51746, and
  `VOE_EDITOR_INTERFACE_NODES` and `_ELEMENTS` (line ~389) set to them.
- `editor/src/src.md`: the entries naming Make levels (`loading`'s near line 12,
  `assets_menu.h`'s near 202, `interface_assets.c`'s near 248) say Make LODs; `levels_ask.h`'s
  says LODs 1 to 6; each entry still one sentence.

## Done when
`voe_editor` builds in the folder's check with no warning, and
`grep -n '"Make LODs"' editor/src/assets_menu.c` and `grep -n '"LODs of %s"' editor/src/levels_ask.c`
each print a line. The human: right-click a `.glb` in Assets: the row says Make LODs; press it:
"LODs of <file>", a LODs row 1 to 6, and 6 lit on a model just imported or whose `.parts` has
no `[Levels]` count (one that already says `count=4` shows 4, as 0433 keeps written counts).
