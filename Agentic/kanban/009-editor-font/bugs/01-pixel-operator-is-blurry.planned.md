# 01 — Pixel Operator is blurry

## Seen
"The font rendering is not good. The precision seems off somehow. It is not crisp at all. In full
screen it's ok on Oxanium but not Pixel Operator. It's very hard to read on smaller sizes."

The editor's text is drawn in 3D space and scales with the window's height (1 pixel is 1 millimetre
at 1 m from the camera), so a smaller window means smaller text. No anti-aliasing was decided early
on, yet the letters look smudged. The sponsor suspects the texture sampling or shader smooths them.

## Expected
- Pixel Operator is crisp at every window height: every edge of a letter is hard, with no grey
  smear between the letter and the panel behind it.
- At full screen it is at least as sharp as Oxanium is today.
- In a small window it is still readable. At a height where the font's pixels do not line up
  with the screen's, some strokes may come out one screen pixel wider than others; that is
  accepted, blur is not.
- The text keeps scaling smoothly with the window. It does not jump between sizes, and the
  1 pixel = 1 mm rule stands.
- Oxanium looks no worse than it does now.

## How to reproduce
1. Start the editor with Pixel Operator (the default for both built-in themes).
2. Look at any panel's text at full screen, then make the window smaller in steps.
3. The letters are soft at full screen and get hard to read as the window shrinks.
4. Choose Oxanium in Preferences and compare at full screen.
