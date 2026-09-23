# 0224 — A theme's text scale is a third number on its line in theme_scalars
date: 2026-09-23
by: planner

## Decision
The text size slider of 0219 is a **scale on the theme's own `text_size`**, 0.5 to 2.0 and 1.0 by default,
named `VOE_EDITOR_TEXT_SCALE_MIN` and `_MAX` in `editor/src/theme_scalars.h`. The palette is derived with
`inputs.text_size * text_scale`; the theme file's own `text_size` is never changed and is what 1.0 means.
The slider shows it as a whole percentage.

It is kept in 0197's file as a third number: `<contrast> <separation> <text_scale> <identity>`, each number
in `%.3f`. A line whose third token is not a number followed by a blank is an older two-number line, read
with a scale of 1.0 and the identity starting at that token; it is written back in the new shape. A line
whose scale is outside `MIN..MAX` is skipped like one whose scalars are. Reset drops the whole line, the
scale with the two others.

## Reasoning
A scale and not an absolute size, because 0219 says "half to twice the theme's own size" and a theme whose
file changes its size should move the text with it. The same file and line, because 0197 already remembers
per theme, written at the end of a drag and on Reset, and a second file would repeat that for one number.
Reading the old shape keeps what a person already set; the one ambiguity — a theme file whose name starts
with a number and a blank — is accepted as not worth a version field. The range lives in the editor, not in
`ui`, because `ui`'s `text_size` is millimetres per em and knows nothing of a theme's own size.

## Replaces
0197's line shape, `<contrast> <separation> <identity>`, which is still read.
