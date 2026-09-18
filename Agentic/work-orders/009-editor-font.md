# 009 — Editor font

## What

The editor's own panels are drawn in Pixel Operator, a pixel-style typeface free to ship, instead
of Oxanium. The built-in theme takes Pixel Operator as its font, and the editor's Preferences can
override whichever theme is chosen with Oxanium — the logo font — or Pixel Operator. The override
belongs to the editor only; a game made with the engine is not affected.

## Why

The sponsor wants the editor's own panels in a pixel-style face and Oxanium kept as the logo font, with the choice belonging to the editor only. This was spec 005 and ADR-0167 calls it that; it is 009 because it is built after 006, whose Preferences panel and themes it sits on, and a work order is taken in number order.

The font itself is already in the tree on branch `feature/006-themes`: its card 02 embeds Pixel Operator beside Oxanium and makes a font asked for by typeface (ADR-0167). What is left here is the editor's default and the override in Preferences.

## How to test

1. **The new default.** Start the editor with no preference saved: every panel — top bar, Scene
   list, Inspector, file browser, Preferences — is in Pixel Operator.
2. **A font choice in Preferences.** Beside the list of themes, Preferences offers the font: the
   theme's own, Pixel Operator or Oxanium, the one in use marked.
3. **Switching is immediate.** Choosing Oxanium redraws every panel in Oxanium at once, whatever
   theme is chosen; choosing the theme's own font puts it back. Nothing else about the scene
   changes, and it does not count as an unsaved change.
4. **The choice is remembered.** Choose Oxanium, close the editor and start it again: the panels are
   in Oxanium.
5. **A theme's rows keep their own font.** In Preferences each theme's preview row still shows that
   theme's own font, so the override does not hide what a theme looks like.
6. **A picture shows it too.** `voe_editor --capture` draws the panels in the chosen font.
7. **Only the editor.** The dev program still draws its text in Oxanium.
8. `cmake -P check.cmake` exits zero on Linux.

## Out of scope

- Tahoma: it is Microsoft's and may not be shipped.
- Fonts in a game, or text inside a scene.
- Any other preference, and font sizes beyond what a theme says.

## Constraints

- Linux first: acceptance is on Linux.
- Pixel Operator ships inside the editor with its licence file beside it, as Oxanium does. If its
  licence turns out not to permit that, the build stops and the sponsor is asked.
- Built after 004 and 006: it uses 006's Preferences panel and themes.

## Defaults

- Pixel Operator's regular weight.
- The font override is kept per person on the machine, beside the remembered last project and
  theme. If it cannot be read, the editor uses the theme's own font.

## Open questions

- None.
