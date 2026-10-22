# 03 — The foliage brush hides its size in the Sculpt section

## Seen
It is confusing to use the sculpt radius as the brush size for foliage painting. The Foliage section's
Scatter and Erase take their radius and softness from the Sculpt section's sliders (`editor/src/sculpt.c`
builds the foliage brush from `sculpt->radius` and `sculpt->softness`), so I have to go to Sculpt to
size a foliage brush. Strength is on the same row of sliders but does nothing to foliage.

## Expected
A Brush section of its own, holding what every brush shares: Radius (m) and Softness. It is shown
whenever a sculpt, paint or foliage brush is in use. Strength stays with Sculpt (and paint), Density
with Foliage. One radius and one softness for all brushes, as today; the ring drawn in the view does
not change.

## How to reproduce
In the editor, open a landscape in the inspector, pick a foliage kind and Scatter: the brush's size is
set only by Radius (m) in the Sculpt section.
