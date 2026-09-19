# 011 — Monochrome themes and contrast sliders

## What
The editor never uses colour to mean anything (decision 0194). Every part of it is one hue in
different lightnesses, like an old green-screen or amber terminal. Near black and Near white are
pure grey. What is selected, pressed, focused, hovered or being dragged stands out the way it did
on a console: inverted, a heavier background, a border, an underline, brighter or dimmer. It is
never a different colour. The selected entity's name in `Scene` is one of these, not blue.

A theme file names its one hue with `hue=` where it used to say `accent=`. `hue="#D4A02B"` gives
an amber editor, and a grey gives a neutral one. A file that still says `accent=` is refused with a
notice naming the line, like any other bad line.

Preferences shows two sliders for the theme in force: contrast (how far text stands from what it
sits on) and surface separation (how far panels and surfaces stand from each other). Moving one
changes the whole editor as you drag. The range is limited so text always stays readable. The
sliders work for Near black and Near white too. What you set is remembered for that theme across a
restart, and the theme file itself is not changed. A reset puts the theme's own values back.

## Why
Colour that carries meaning is lost when a theme changes it or when someone cannot tell colours
apart, and the look was agreed as monochrome. The sliders let a person make any theme easier on
their eyes without writing a file.

## How to test
1. Start the editor in Near black and select an entity in `Scene`. Its row is clearly marked, for
   example inverted or on a heavier background, and nothing on screen has any colour but grey.
2. Press and hold a button, and drag a number box in the Inspector. Both are clearly marked, still
   only in grey.
3. Switch to Near white in Preferences. Same as steps 1 and 2: grey only, and selected, pressed and
   dragged are still easy to tell.
4. Write `amber.theme` in `~/.config/voe3d/themes/` with `hue="#D4A02B"` and choose it. The whole
   editor turns amber. Selected, pressed and dragged are marked the same way as in grey, and no
   part is a different hue from the rest.
5. Change `hue=` to `accent=` in that file and save. The editor keeps drawing and shows a notice
   naming the file and the line.
6. Put `hue=` back. In Preferences, drag contrast all the way down, then all the way up. Text moves
   closer to and further from its background as you drag, and at both ends it is still readable.
7. Drag surface separation to both ends. Panels and surfaces move closer together and further
   apart in lightness as you drag.
8. Quit and start the editor again. The theme and both slider positions are as you left them, and
   `amber.theme` on disk has not changed.
9. Choose Near black. Its sliders show their own values, not amber's. Reset puts them back to
   Near black's own values.
