# 020 — A text size slider in the theme

## What
Beside the contrast and surface separation sliders there is a **text size** slider, from half to twice the
theme's own size, starting at 100%. Dragging it makes all the editor's text bigger or smaller at once. Each
theme remembers its own text size, and Reset sets it back with the others. Only the text is scaled: rows,
buttons, fields and menus grow and shrink because the text in them does, and when something no longer fits
its panel it follows the overflow rules — it scrolls or stops at the edge — instead of being cut in half or
drawn over its neighbour.

## Why
Decision 0219. Someone who makes a theme with a font that is too small can fix it without editing the theme.

## How to test
1. Open Preferences. A text size slider sits with the other theme sliders, at 100%.
2. Drag it up to 200%. All the editor's text grows as you drag: top bar, Scene list, Inspector, menus,
   dropdowns. Rows and buttons are taller to fit it; no text is cut or overlaps another.
3. With the Inspector narrow, look at a long field name. It does not spill over the next column or out of the
   panel.
4. Drag it down to 50%. Everything shrinks to fit the smaller text and stays readable as far as the font allows.
5. Switch to another theme. Its text is at its own size, 100% if never changed. Set it to something else.
6. Switch back. The first theme has the size you left it at.
7. Press Reset. The text size goes back to 100% with the other sliders.
8. Close and reopen the editor. Each theme still has the text size you gave it.
