# 04 — Pixel Operator is dropped; Oxanium is the default and the fallback

## Seen
"I re-ran editor and the font looks phenomenal. Even on smaller sizes. I believe oxanium is the
only font we need. Lets drop pixel operator and we do oxanium as default and fallback."

Today the editor starts in Pixel Operator and Preferences offers a font choice (theme's own, Pixel
Operator, Oxanium). Per decision 0185 neither is wanted any more.

## Expected
Per decision 0185:
- Every panel of the editor is drawn in Oxanium, in both Near black and Near white.
- Preferences shows the list of themes and no font choice.
- Pixel Operator is gone from the tree: no embedded face, no licence file, no name for it.
- A theme that asks for any other font, `pixel_operator` included, draws in Oxanium.
- A leftover remembered font setting from an earlier run changes nothing.
- Small-window text stays as it is now, after bug 03.

## How to reproduce
1. Start the editor: the panels are in Pixel Operator.
2. Open Preferences: a font choice sits beside the themes.
