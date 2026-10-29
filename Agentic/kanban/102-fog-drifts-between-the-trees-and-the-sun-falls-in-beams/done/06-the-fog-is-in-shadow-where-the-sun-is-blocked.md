# 06 — The fog is in shadow where the sun is blocked
folder: render
after: 05
decisions: 0168, 0480, 0492, 0493

## Change
0493 point 6 proved on the picture: beams are the sun's light in the fog, shadowed by what casts
and by the clouds, as strong as `godrays` says, and shown with the sun behind the eye. Read the
headers of `render/shaders/fog.slangh` and `render/tests/fog.c` (card 05), and
`render/tests/shadow.c` and `render/tests/cloud_shadows.c` as the patterns for a casting sun's
shadow passes and for clouds in a headless test.

- `render/tests/godrays.c` (new), headless. A target looking along −Z at a fog volume 40 m wide,
  10 m tall and 20 m deep centred 30 m ahead, density 0.05, patchiness 0, no sky drawn so the fog
  is all that lights the picture; one casting light from straight above with a fill; an opaque
  slab drawn into the cascades only (out of the camera's view, high above) over the volume's left
  half:
  - godrays 1: the mean of a patch on the picture's left half is darker than one on its right by
    more than 4/255;
  - godrays 0: the two patches are within 2/255 of each other, and both are visible fog (more
    than 8/255 from no fog);
  - godrays 0.5: the difference lies between those two;
  - the light turned to shine from behind the eye, low, the slab moved to shade the left half
    again: godrays 1 still makes the left darker by more than 4/255;
  - no slab, godrays 1, clouds Volumetric coverage 1 above at bottom 500, thickness 500, shadow
    strength 1: the fog is darker than with no clouds by more than 4/255; shadow strength 0 is
    within 1/255 of no clouds.
  Patch means over 8 × 8 pixels, so the march's jitter does not decide a case.
- `render/shaders/fog.slangh`: where a case fails because the fog's lighting is wrong, fix it
  there and say so in its header; nothing else changes.
- `render/tests/tests.md`: an entry, at most 300 characters.

## Done when
The tests `render/godrays` and `render/fog` pass after the folder's build.
