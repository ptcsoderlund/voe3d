# 03 — A bright column runs down the ground under the sun

## Seen
"There is a bright line in the landscape ground." In the editor's view of lumberhill_project, with the sun
low over the horizon (light colour white, intensity 3.142, fill 0.801, haze on), a straight vertical band
of near-white the width of the sun disc runs from the disc down the ground to the bottom of the view. It
keeps the same width the whole way and does not fade towards the camera, so it does not look like a glint
off the ground. The terrain on either side of it is shaded as normal.

A guess, not checked: the band sits exactly in the sun's direction left to right, as if something that
reads the sky's colour for a point on the ground — haze mixing towards the sky at the horizon is the
likeliest — tests the sun disc by its direction across the ground and ignores how far below the horizon
the point is.

## Expected
- The ground under the sun shades and hazes like the ground beside it; no band.
- The sun disc and glow appear only in the sky, above the horizon.

## How to reproduce
1. Open lumberhill_project in the editor; the light white, intensity 3.142, fill 0.801, the sky's haze on.
2. Turn the sun low over the horizon and look towards it in an editor view.
