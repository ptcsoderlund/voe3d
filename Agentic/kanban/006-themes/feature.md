# 006 — Themes

## What

Every piece of interface — in the editor and in a game — takes its look from a theme: flat, square,
hairline borders in the manner of Windows 10, with one accent colour. A theme is a small text file
with four inputs (an accent colour, contrast strength, surface separation, light or dark) plus its
font and text size, and every other colour is worked out from them, so making your own theme is
easy and hard to make ugly. A theme set on a panel applies to everything inside it unless something
nearer sets its own. The editor dogfoods it first; a game editor for interfaces comes later.

## Why

Making your own theme should be easy and hard to make ugly, and every piece of interface in the editor and in a game should take its look from one place. The editor dogfoods it first.

Work on this began under the earlier workflow. Cards 01 to 04 are done and committed on branch `feature/006-themes`, which carries this feature's folder under `Agentic/kanban/` with those cards in `review/` and the rest of the old plan in `blocked/` for the planner to re-cut. Resume there: switch to the branch, merge `dev`, and run `/drive`. Built after 008 by the sponsor's choice on 2026-09-17.

## How to test

1. **The default look.** The editor starts in the built-in almost-black theme: near-black
   surfaces, flat square controls with hairline borders, light text, and one accent colour. The
   selected entity in the Scene list is shown in the accent instead of today's `> ` marker; a
   button being pressed and a number being dragged show the accent too.
2. **Preferences.** A Preferences button in the top bar opens a Preferences panel listing the
   themes, the one in use marked. It can be closed again.
3. **Your own theme.** Put a theme file with an accent, the two sliders and a mode into the editor's
   themes folder (on Linux `~/.config/voe3d/themes/`): it appears in Preferences. Choosing it
   restyles every panel at once, and the choice is remembered on the next start.
4. **Few inputs are enough.** Setting the mode to light turns the whole editor light with readable
   text; changing only the accent recolours everything that uses it; the sliders visibly change
   how much surfaces and text stand apart.
5. **Live editing.** With the editor running on a theme, change its file and save: the editor
   shows the change within about a second, without restarting.
6. **A broken theme keeps the last good one.** Save a theme file with a mistake: the editor keeps
   drawing the last good theme and shows a notice naming the file, the line and what is wrong.
7. **A theme inherits downwards.** In Preferences each theme's row is drawn in that theme — its
   label and button take it from their row — while the rest of the editor stays in the chosen
   theme.
8. **Font and size are part of a theme.** A theme file naming a different font or text size draws
   the panels in it when chosen.
9. **A picture shows it.** `voe_editor --capture` draws the panels in the chosen theme.
10. **Not only for the editor.** The theme system is part of the engine's interface pieces, which a
    game uses; a check proves a theme read from a file decides the colours of a panel, label and
    button with no editor involved.
11. `cmake -P check.cmake` exits zero on Linux.

## Out of scope

- A built-in light theme; light is a file anyone can write.
- Changing theme while a finished (cooked) game runs. A game's theme is fixed at cooking unless its
  developer deliberately exposes a choice.
- An editor for game interfaces, and showing themes in the dev program.
- Colours with a meaning (error, warning, success).
- Overriding a single derived colour by name.
- Rounded corners, shadows, animation.

## Constraints

- Linux first: acceptance is on Linux.
- Built after 004 (it uses the top bar) and before 009 (editor font, was 005), which then adds a font choice
  on top of the theme.

## Defaults

- The built-in theme is dark, with a blue accent; its font is Oxanium until 009 makes it Pixel
  Operator.
- A theme can name only fonts the program carries.
- Theme files use the engine's existing text format. The editor's own themes live in the per-person
  settings folder; a game's live in its project.
- Live editing watches the chosen theme's file only.

## Open questions

- None.
