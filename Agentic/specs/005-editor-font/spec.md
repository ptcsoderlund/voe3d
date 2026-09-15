# 005 Editor font

Status: approved
Approved: 2026-09-15
Accepted: -

The editor's own panels are drawn in Pixel Operator, a pixel-style typeface free to ship, instead
of Oxanium. Oxanium stays one choice away in the editor's Preferences, because it is the logo
font. The setting belongs to the editor only; a game made with the engine is not affected.

## Acceptance criteria

1. **The new default.** Start the editor with no preference saved: every panel — top bar, Scene
   list, Inspector, file browser — is in Pixel Operator.
2. **Preferences.** A Preferences button in the top bar opens a small Preferences panel listing the
   two fonts, the one in use marked. It can be closed again.
3. **Switching is immediate.** Choosing Oxanium redraws every panel in Oxanium at once; choosing
   Pixel Operator switches back. Nothing else about the scene changes, and it does not count as an
   unsaved change.
4. **The choice is remembered.** Choose Oxanium, close the editor and start it again: the panels are
   in Oxanium.
5. **A picture shows it too.** `voe_editor --capture` draws the panels in the chosen font.
6. **Only the editor.** The dev program still draws its text in Oxanium.
7. `cmake -P check.cmake` exits zero on Linux.

## Out of scope

- Tahoma: it is Microsoft's and may not be shipped.
- Fonts in a game, or text inside a scene.
- Any other preference, and font sizes.

## Constraints

- Linux first: acceptance is on Linux.
- Pixel Operator ships inside the editor with its licence file beside it, as Oxanium does. If its
  licence turns out not to permit that, the build stops and the sponsor is asked.
- Built after 004, whose top bar it uses.

## Defaults

- Pixel Operator's regular weight.
- The font preference is kept per person on the machine, beside the remembered last project. If it
  cannot be read, the editor starts in Pixel Operator.

## Open questions

- None.
