# 39 — Import and Make levels ask how many and how hard
folder: editor
after: 36, 38
decisions: 0168, 0429, 0430

## Change
0430 point 6: the question of card 38 (`editor/src/levels_ask.h`) opens after a `.glb`'s Import and on
Make levels; its Make saves the model's levels and runs the prepare that Import and Make levels ran
straight away until now (`session.prepare_due`, `prepare_path`, `prepare_remake`).

- `editor/src/session.h`: the session gains `voe_editor_levels_ask levels_asking`, commented beside
  `asking`. The prepare fields' comment: asked by the levels question's Make, always with
  `prepare_remake`. The IMPORT_FILE sentence: a `.glb` opens the levels question in place of
  setting `prepare_due`.
- `editor/src/session.c`, `session_import` (line 565): for a `.glb`, `voe_editor_levels_ask_open` on
  the imported path and the project folder, in place of setting `prepare_due` and `prepare_path`.
- `editor/src/interface_assets.c`: `voe_editor_interface_assets_levels_do` (around line 185) opens
  the question on the joined path when it fits, in place of setting the prepare. After the Delete
  question's read, the same shape for the levels question when the new `levels_asking` argument is
  set: MAKE saves (`voe_editor_levels_ask_save`); on true `prepare_path` copied from the question,
  `prepare_due` and `prepare_remake` set; on false the session's notice says the levels could not
  be saved; MAKE and CANCEL close it. The header's Make levels sentence says it asks first.
- `editor/src/interface_assets.h`: `voe_editor_interface_assets_read` gains `bool levels_asking`
  after `asking`; its comment names the levels question.
- `editor/src/interface.c`: beside `asking` (around line 197) `levels_asking =
  session->levels_asking.open`, the browser showing closing it as it closes the Delete question;
  everywhere `asking` keeps the menu closed or the pointer off the dock, `levels_asking` does too;
  drawn beside the Delete question's draw (around line 336); passed to the Assets read.
- `editor/src/interface.h`: a paragraph after THE ASSETS PANEL'S DELETE QUESTION counting the
  levels question's nodes and elements as that one counts its own (the title cut at its kept room,
  nine buttons each a button and a label), and `VOE_EDITOR_INTERFACE_NODES` and `_ELEMENTS` raised
  by them.
- `editor/src/src.md`: the `session.h`, `interface_assets.h`, `interface_assets.c` and
  `interface.h` entries name the levels question where they name the Delete one, each entry still
  one sentence.

## Done when
`voe_editor` builds in the folder's check with no warning, and
`grep -n 'levels_ask' editor/src/session.c editor/src/interface.c editor/src/interface_assets.c`
prints lines in all three. The human, in the lumberhill project:
1. Right-click `buildings/wooden_stable.glb` in Assets → Make levels: the question shows Levels 4 and
   Medium lit. Make: the splash shows "Preparing wooden_stable.glb" once.
2. Place the stable and fly away: the Inspector shows 3 or 4 levels and Level reaches the last; the
   Frame panel's Triangles far is well under half of near (it stopped at 70 % before).
3. Make levels again with Levels 2 and Strong: the Inspector says "of 2"; reopen the question: 2 and
   Strong are lit.
4. Import a downloaded `.glb`: the question shows after the copy; Cancel makes nothing, and placing
   it still gets its levels.
