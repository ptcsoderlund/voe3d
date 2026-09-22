# 05 — Text is too thick on the desktop monitor

## Seen
"I am testing on windows, the font is rendered much thicker then on my linux machine. Small or
large screen doesnt matter, still thicker (too thick)."

Then: "If i run on wsl and windows, its the same thick font. But it looked slimmer on my fedora kde
laptop."

So this is not Windows against Linux. On the sponsor's desktop machine the editor draws heavy
Oxanium strokes whether it runs natively on Windows or under WSL, and it does so at every window
size. On the Fedora KDE laptop, where feature 009 and bug 03 were accepted, the same text is slim.

The sponsor then captured the same commit on both sides of the desktop machine, 2026-09-22:
`voe_editor --capture` natively on Windows (NVIDIA) and under WSL (llvmpipe), both 1920x1080. The
tech-lead compared them pixel by pixel:
- The text is the same picture on both. The "Preferences" label lights 1501 pixels on one and
  1502 on the other, with identical letter shapes, hard edges and two grey levels only.
- Everything else differs by at most 15 of 255 in a channel, plus 11 pixels on the cylinder's
  edge: the two Vulkan drivers shading the scene slightly apart. No panel or text differs.
- In that capture the stem of the "P" in "Preferences" is 4 pixels wide at a cap height of 23
  pixels. That is a heavy stem for Oxanium's regular weight at this size.

So the weight is in the drawing itself, identically on both operating systems, and the thing that
differs from the laptop is the display the drawing lands on. Working hypothesis: the edge cutoff of
decision 0182, tuned on the laptop so that no stroke vanishes, widens every stroke by a fixed
fraction of a pixel. On the laptop's dense panel the letters hold more pixels and the widening is a
small share of each stem; on the desktop monitor the letters hold fewer pixels and the same
widening reads as bold.

A separate observation to keep apart from this bug: on the desktop machine the live window's
colours differ from the PNG, and the PNG looks better. Not a font matter; noted here so it is not
lost, to be filed against its own feature once 009 is off the board.

## Expected
- Oxanium in the editor has the stroke weight of Oxanium's regular weight, within a pixel, at
  every window size and on every display. A stroke may be one pixel wider than the design where
  it must be to stay visible (bug 01, decision 0182); it may not be heavier than that everywhere.
- Decision 0182 still holds: hard edges, text scales with the window, no stroke vanishes. Thinner
  strokes here must not bring back bug 03's missing strokes on the laptop.
- The capture shows it: in a 1920x1080 capture the stems of the top-bar labels are visibly
  lighter than today's 4 pixels at 23 pixels cap height, and the sponsor judges the picture slim
  on the desktop monitor.

## How to reproduce
1. On the desktop machine, start `voe_editor` natively on Windows or under WSL: strokes are
   heavier than on the laptop. Shrink and enlarge the window: heavier at every size.
2. `voe_editor --capture capture.png --size 1920x1080` on either side of the desktop machine, open
   the PNG at 100 percent, and measure the "P" of "Preferences": stem 4 pixels, cap height 23.
3. On the Fedora laptop, start the same commit: strokes look slim. Its display's resolution and
   scale factor, and the desktop monitor's, are to be noted here when the sponsor has them.
