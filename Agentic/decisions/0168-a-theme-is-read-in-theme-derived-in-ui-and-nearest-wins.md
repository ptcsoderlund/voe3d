# 0168 A theme is read in `theme`, derived in `ui`, and the nearest one wins

Status: accepted
Date: 2026-09-17

Spec 006 makes every panel, label and button take its look from an authored theme file, in the
editor and in a game alike, and asks for a check that proves it with no editor involved. The
parsing is `assets`' (ADR-0096) and `ui` may not name `assets` (ADR-0093), while `editor`'s row in
`cmake/voe.cmake` deliberately has no `assets` either — so no folder that exists can hold the step
between a theme file and a widget, and the one place a check could span the whole chain is the
editor, which the criterion rules out.

## Decision

**A new folder, `theme`, holds the step between authored text and `ui`.** Its row is
`ui text assets math base`. It turns the bytes of a theme file into `voe_ui_theme_inputs` — the
accent, the two scalars, the mode, the text size — plus the `voe_text_typeface` the file named and
the theme's own display name. It opens no file: the caller hands it bytes, as every reader in this
tree does.

**`ui` keeps the derivation and the palette, exactly as ADR-0096 point 4 said.** Inputs in, a
`voe_ui_theme` of roles out; the roles and the rules that make them are one piece of knowledge and
one folder's. `ui` still names no file, no font face and no operating system, and its tests still
need neither `assets` nor `platform`.

**The perceptual colour conversion lives inside `ui`, not in `math`** — answering D-131. `math` is
spelled the way Slang spells it and OKLab is not a Slang concept; nothing else has asked for one
(rule 10). It is `ui/src/oklab.h`, internal, and it moves to `math` on the day a second folder
needs it.

**A theme applies to a subtree and the nearest one wins**, which is ADR-0087 point 2 in an
immediate-mode tree: `voe_ui_theme_push`/`voe_ui_theme_pop` around the calls it covers, every node
recording the theme in force when it was made, and the widget pass reading the node's own. A whole
theme replaces the one above it — no per-role fall-through, because there are no partial themes to
fall through from once a theme is a derived palette rather than a list of authored roles.

**`editor` gains `theme` and nothing else; `dev` does not.** Nothing in `dev` asks for a theme
(rule 10), so CLAUDE.md's `dev` row reads *every folder but `editor`, `authoring` and `theme`*.

## Rejected

- **The reader in `assets`** — no new folder and no new edge, and it is the cheapest option. It
  cannot name `voe_ui_theme_inputs` or `voe_text_typeface`, so the inputs would be duplicated as an
  `assets` struct and a font name would come back as text for every caller to map itself; and
  criterion 10's check would split into two halves either side of the seam where a mistake hides.
- **The reader in `editor`** — `assets` is absent from its row on purpose, criterion 10 says no
  editor, and a game would have to write the step itself.
- **`assets` on `ui`'s row** — rejected once already by ADR-0096 and still right: it pulls
  `platform` into a folder that is deliberately testable without one.
- **The derivation in `theme` too, leaving `ui` only the roles** — tidy on paper, and it puts a
  folder boundary through the middle of one idea: a role added is a derivation rule added, and both
  would move together across two folders for ever.
- **`voe_ui_theme` carrying a `voe_text_typeface` and `ui` holding a font per face** — it makes
  `ui` name faces, which ADR-0090 forbids. The program resolves the typeface to one of the fonts it
  created, once, when it loads the theme.

## Consequences

- One more folder to configure, check and keep a `<folder>.md` for, for what is today a few hundred
  lines. The edge it buys is the one the tree could not otherwise draw.
- A game wanting themes links `theme` beside `ui`; the glue is reading the file through `platform`
  and handing over the bytes.
- `voe_ui_theme` is the caller's memory and must outlive the frame, like the font.
- ADR-0096 point 4 is unchanged in substance — the caller still loads the theme and hands `ui` the
  authored values — and *the caller* is now a named folder rather than whoever happened to be
  above.
- A theme that sets only some roles is not expressible, and would need a real decision rather than
  an edit, since a partial theme cannot be derived from one colour and two numbers.
