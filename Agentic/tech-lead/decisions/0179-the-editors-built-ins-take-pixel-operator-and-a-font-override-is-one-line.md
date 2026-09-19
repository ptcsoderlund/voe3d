# 0179 — The editor's built-in themes take Pixel Operator, and a font override is one line beside the theme
date: 2026-09-18
by: planner

## Decision
Near black and Near white are derived with Pixel Operator (`typeface = VOE_TEXT_TYPEFACE_PIXEL_OPERATOR`).
Everything else in ADR-0178 stands.

The editor may override the font of whichever theme is in force. The override is one of three choices:
the theme's own, Pixel Operator, or Oxanium. It is remembered per person in `<settings>/voe3d/font`, one line,
read and written the way `<settings>/voe3d/theme` is: empty or absent for the theme's own, `pixel_operator` or
`oxanium` otherwise, the same spellings a `.theme` file's `font=` uses. Any other line, or a file that cannot be
read, means the theme's own font, and nothing is reported.

`editor/src/themes.h` owns the override. It keeps one palette in force: a copy of the chosen entry's palette
whose `font` is replaced by the override. The copy is refreshed whenever the chosen theme, its re-read palette or
the override changes. The interface is set with that copy and never with an entry's own palette. The entries are
left as they are, so each Preferences row, pushed in its own entry's palette, still shows the theme's own font.

## Reasoning
Spec 009 (spec 005 in ADR-0167) asks for Pixel Operator as the editor's default, for an override kept beside the
remembered theme, and for the theme rows to keep their own font.
- **A sibling one-line file**: ADR-0172 expected it ("three one-line files ... once 005 lands its font
  override"). Moving the three into one settings document is still its own decision, and nothing here needs it.
- **Changing the entries' palettes in place**: rejected. The Preferences rows would lose their own fonts.
- **Putting the override in `ui` or `theme`**: rejected. The override belongs to the editor, and a game must not
  see it.

## Replaces
ADR-0178's "Oxanium" for both built-in themes. Both now use Pixel Operator.
