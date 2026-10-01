# 052 — The editor fits a PC monitor

## What
The editor looks made for a PC monitor, not a touch panel or a smartphone. Its
text is about a fifth smaller by default, and the padding, buttons, rows and
gaps between things are clearly tighter, so more fits in each panel and
nothing looks oversized. Everything stays readable and clickable with a mouse.

The text-size slider still reads 100 % by default; 100 % is simply the new,
smaller size, and whatever I set it to before scales down with it.

The panels keep the widths they have, and a game's own interface is not
changed. Both are sized later, with docking and a GUI editor.

## Why
Today the editor has large buttons and wide spaces, as if it were made for touch.
I use it on PC monitors.

## How to test
1. Start the editor on `examples/tank_game`. The text is clearly smaller than
   before, and the padding and gaps shrink more than the text: the top bar is
   thinner, and the Scene list and Inspector show more rows in the same height.
2. Select the tank in the Scene list. Every Inspector field, label, button and
   dropdown is still readable at normal desk distance, and nothing is clipped,
   overlaps or runs out of its box.
3. Open a dropdown, the Add component menu and the file browser (Open). All
   are tighter in the same way, and every entry is easy to hit with the mouse.
4. Open the theme settings. The text-size slider reads 100 %. Drag it to 200 %
   and back to 50 %: the editor grows and shrinks as before, and at 50 %
   nothing breaks. Press Reset: back to the new default.
5. The Scene list and Inspector are as wide as before, and a panel width I
   dragged earlier is kept.
6. Switch between Near black and Near white. Both themes get the new sizes.
7. Press Play. The game looks exactly as it did before.
