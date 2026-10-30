# 0503 — A section is a bordered box whose caller holds its fold, and a heading is a bigger label
date: 2026-10-30
by: planner

## Decision
For 108, how 0496 and 0502 are built:
1. **`ui`'s section holds no state.** `voe_ui_section_begin(ui, name, index, open)` opens a box
   (a panel on `surface` with its hairline border, growing to its parent's width) and its header row
   with the fold button first, and hands back the box and the fold button. The caller composes the
   title into the header row, closes it, draws the body only while open, closes the box, and flips
   its own state when the fold button fires. Saving is the caller's.
2. **The fold button reads `+` folded and `-` open**, as the Scene list's does. Oxanium has `‹` and
   `›` but no down-pointing arrow or triangle, so an arrow would need a glyph the sheet cannot give.
3. **A heading is `VOE_UI_TEXT_ROLE_HEADING`**: `text_primary` at 1.25 times the theme's `text_size`.
   No palette role is added; roles stay lightnesses (0194) and the heading differs by size.
4. **Only the fold button folds, on every section.** A component section's title does nothing; a
   foliage kind's title is its choice row and picks it (0496 point 2). A section's Remove sits on
   its header line, so it shows while folded.
5. **The fold lines** in `editor_settings` are `fold section <component key | sculpt | layers |
   foliage | brush | lod>` and `fold kind <k> <landscape path>`, the path project-relative as the
   model row names it. The editor's `folds.h` keeps them, writes the file at once when one flips,
   and is never undone (feature 108 test 8).
6. **The Scene list stays as it is**: its own fold button on the identity's `folded` (0302, 0355),
   saved in the scene and undone. It does not move onto the section.

## Reasoning
Folds must be saved in the editor's settings, which `ui` cannot see, so `ui` remembering them
(as it does scroll offsets) would keep two copies to sync. A caller-held flag is one copy. Putting
the title in the caller's hands lets one box serve a plain heading and a kind's picking row. A
flip is rare, so writing at once costs nothing and survives a crash. A project-relative path keeps
the Foliage draw free of the project folder. The cost is that two projects with the same landscape
path share kind folds, which is harmless. The Scene list's fold rides undo and the scene file by
0302. Moving it would change where that state lives, which is not the planner's to change.

## Replaces
Nothing. Carries out 0496 and 0502.
