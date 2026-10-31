# 01 — Fog thickness runs to 4
folder: scene
after: none
decisions: 0168, 0499, 0498

## Change
The fog volume's `thickness` runs 0 to 4: its range for a tool and the system's refusal both move
from 1 to 4. Patchiness and godrays_strength stay 0 to 1; the default row (thickness 0.4) and every
other field are unchanged. Nothing outside `scene/` changes: the Inspector's slider takes its ends
from the range, and the render squares the thickness with no bound.

- `scene/include/scene/fog_volume_component.h`: add `VOE_SCENE_FOG_VOLUME_THICKNESS_MAX` (4), as
  the light header's `VOE_SCENE_LIGHT_DISC_SIZE_MAX` is, so the range and the refusal cannot drift
  apart. In `VOE_SCENE_FOG_VOLUME_RANGES`, `thickness` runs 0 to that constant, unit still empty.
  Header, fields paragraph: thickness is 0 to the constant, 0 no fog, the top the thickest
  (0499 point 1); cite 0499 beside 0493 in the opening line.
- `scene/include/scene/fog_volume_system.h`: the REFUSED paragraph separates thickness (outside 0 to
  the constant) from patchiness and godrays strength (outside 0 to 1).
- `scene/src/fog_volume_system.c`: in `fog_volume_valid()`, thickness is no longer checked by
  `unit_valid`; it is refused when below 0 or above `VOE_SCENE_FOG_VOLUME_THICKNESS_MAX` (finite is
  already checked). The other two fields keep `unit_valid`.
- `scene/tests/fog_volumes.c`: `check_range` for `thickness` expects 0 and 4; the refused-row case
  that uses thickness 1.1 uses a value just above 4 instead; add a case that a row with thickness 4
  is added and a replace to 4 lands on a run (the old top is now an ordinary value). Header: the
  thickness range is 0 to 4.
- `scene/tests/tests.md`: the `fog_volumes.c` entry gives thickness 0 to 4, patchiness and godrays
  strength 0 to 1.

## Meaning
- Density stays 0.1 × thickness² per metre, so 1 looks as now and 4 is 1.6/m; every saved volume
  reads unchanged (0499 point 2). Do not touch the mapping, colour or lighting (set aside until 106).
- The refusal is only in `fog_volume_valid()`; scene text read and the cook add rows raw, and the
  editor only clamps the shown value to the range (scout).

## Done when
- `ctest --test-dir build/debug -R '^scene/fog_volumes$'` exits 0, with the range 0–4, the refusal
  above 4 and the accepted row at 4 each checked.
- `grep -n "0 to 1: 0 no fog" scene/include/scene/fog_volume_component.h` prints nothing.
- Human, in the editor (How to test 1–5 of feature.md): open the tank project, select Fog; the
  Thickness slider runs 0–4 and shows 1.00, the fog as before; at 4 nothing shows through the box's
  length; it thins smoothly to nothing at 0; 3 survives save and reopen; Play matches the editor.
