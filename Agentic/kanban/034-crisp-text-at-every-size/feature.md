# 034 — Crisp text at every size

## What
Text is no longer grainy when it is small. Every letter has smooth edges at every size: small
text in a small window, the editor at a 50% text size, a game's interface. Large text stays as
sharp as it is now. The fix is in the one place text is drawn, so the editor and every game
improve together. Text looks the same as before in every other way: the same font, the same size,
the same position and spacing.

This covers the characters the font already draws. Gameplay text is ASCII only. Folder and file
names can be UTF-8 (ours and a project's). A name with characters the font cannot draw still shows
the missing-glyph box as it does today (0247), never a failure; drawing those characters as real
letters is a later feature.

## Why
Milestone 1 of 0.2 (0268): the sponsor wants text finished before the tank game's work starts.
Today small text looks grainy everywhere, because the editor draws through the same runtime as the
game.

## How to test
1. Open the editor in a small window, around 800×600. The top bar, Scene list, Inspector and menus
   have smooth letters, with no grain or jagged pixels on curves and diagonals (look at `s`, `o`,
   `w`, `2`).
2. In Preferences, drag the text size slider down to 50%. The small text is still smooth and as
   readable as its size allows.
3. Drag it up to 200%. Large text is as sharp as before, not blurry.
4. Maximise the window and compare with how it looked before. Nothing has moved: the same layout,
   spacing and size, only the edges are smoother.
5. Open the coin game project and press Play. The score and menu text are smooth too.
6. Open or make a project in a folder whose name has UTF-8 characters, such as `Spel_åäö` or
   `Игра`. The editor works. Where the name shows, the characters the font has are drawn smooth,
   and the others show the missing-glyph box, as before.
