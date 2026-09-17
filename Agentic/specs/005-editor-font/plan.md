# 005 Editor font — plan

The engine carries a second typeface. `text` gains Pixel Operator Regular beside Oxanium, both
embedded, and a font says which face it reads when it is created; the editor asks for Pixel
Operator and everything else keeps asking for Oxanium. That is criteria 1, 6, 7 and 8.

**Criteria 2, 3, 4 and 5 are not planned here, and no task below touches them.** They are the
Preferences font choice, switching live, remembering it and a theme row keeping its own font —
every one of them sits on 006's Preferences panel, its themes and a font that cascades through a
`ui` subtree, and none of those exist in the tree today (there is no theme, no Preferences panel,
and `voe_ui_font_set` is one font per context set once at startup). The spec itself says 005 is
built after 006. The sponsor decides whether 006 goes first or this half ships alone; the question
is in the report.

## Decisions

- Both faces live in `text/fonts/` and are chosen by name from an enum, not by a path or by bytes
  handed in — a theme may then name a font the program carries (006), and runtime font loading
  stays deferred. Project-wide: `Agentic/decisions/0167-the-engine-carries-pixel-operator-beside-oxanium-and-a-font-is-asked-for-by-name.md`.
- How finely a face is sampled into its sheet is a fact of the face, not of the folder. Pixel
  Operator's design pixel is 100 of its 1600 units to the em — two texels at today's 32 texels to
  the em, against a field spread of four — so the sheet's resolution is the one number this feature
  may have to move, and only for the new face. Same record.
- The licence is CC0 1.0 Universal, stated in the font's own copyright string; shipping it is
  permitted with no notice obligation, and the licence file travels beside it anyway, as OFL.txt
  does for Oxanium. Same record.

## Folders

- `text/` — changed — `voe_text_typeface` added and `voe_text_font_new` takes one; `fonts/` gains
  `PixelOperator.ttf` and its licence. Four call sites move with the signature.
- `editor/` — changed — internal only: it asks for Pixel Operator instead of Oxanium.

## Verification

- `cmake -P check.cmake` — exits zero on Linux, tests and analyser included (criterion 8).
- `build/debug/editor/voe_editor --capture /tmp/005-after.png --size 1280x800` — exits zero and the
  picture's panels are in Pixel Operator, differing byte for byte from the same capture taken before
  the change (criteria 1 and 6).
- `build/debug/dev/voe_dev` — its sign and its statistics are still Oxanium (criterion 7); the
  binary asks for `VOE_TEXT_TYPEFACE_OXANIUM` and a person's look confirms it.
- Criteria 2, 3, 4 and 5 are not verified by this plan. They wait on 006.
