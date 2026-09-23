# 0219 — Text size is a slider remembered per theme, and elements fit their content
date: 2026-09-23
by: tech-lead

## Decision
The theme sliders gain **text size**, a scale on the theme's text from half to twice its size, 100% by default.
Like contrast and surface separation (0197) it is **remembered per theme** in the same settings file, set back
by the same Reset, and the theme file is never written. It scales text only. **Nothing else is scaled with
it: elements size themselves to their content**, the way an HTML flexbox does, so a row with bigger text is a
taller row because its content is taller; where content does not fit the space it is given, the overflow rules
apply rather than the text being cut or overlapping. A UI scale for everything, and overriding a theme setting
on one element, are later (see `ideas.md`).

## Reasoning
The sponsor's call (2026-09-23): per theme, because a theme someone makes with a font that is too small is
fixed by scaling that theme; and layout that follows content rather than a second scale. Rejected: one text
size for all themes, which cannot fix one theme's small font without growing every other; rows and buttons
scaled by the same slider, which is a UI scale by another name.

## Replaces
nothing
