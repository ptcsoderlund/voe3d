# 009 — Editor font

## What

The editor's own panels are drawn in Oxanium, the logo font, and it is the only font the engine
carries. Both built-in themes, Near black and Near white, take Oxanium, and Oxanium is the
fallback for any font a theme asks for that the engine does not have. Text stays whole and even
when the window is made small.

## Why

This began as Pixel Operator for the editor with a font choice in Preferences (spec 005,
ADR-0167, ADR-0179). After bug 03 fixed small-window text, the sponsor found Oxanium good at every
size and dropped Pixel Operator and the choice (ADR-0185). What is left is one font drawn well.

## How to test

1. **One font.** Start the editor: every panel (top bar, Scene list, Inspector, file browser,
   Preferences) is in Oxanium. Choose Near white: still Oxanium.
2. **No font choice.** Preferences lists the themes and offers no font.
3. **Small windows.** Make the window smaller in steps: no letter loses a stroke, and the text
   stays readable. Full-screen text looks as good as before.
4. **Fallback.** A theme file with `font=pixel_operator` draws in Oxanium, with no error.
5. **A picture shows it too.** `voe_editor --capture` draws the panels in Oxanium.
6. **The dev program** still draws its text in Oxanium.
7. `cmake -P check.cmake` exits zero on Linux.

## Out of scope

- Tahoma: it is Microsoft's and may not be shipped.
- Fonts in a game, or text inside a scene.
- Any other preference, and font sizes beyond what a theme says.

## Constraints

- Linux first: acceptance is on Linux.
- Built after 004 and 006: it uses 006's Preferences panel and themes.
- ADR-0178 stands as first written: both built-in themes take Oxanium.

## Defaults

- Oxanium's regular weight.
- A leftover remembered font setting from an earlier build is ignored.

## Open questions

- None.
