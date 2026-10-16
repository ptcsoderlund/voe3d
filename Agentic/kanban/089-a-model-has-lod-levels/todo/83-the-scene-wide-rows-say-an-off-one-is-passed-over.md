# 83 — The scene-wide rows say an off one is passed over
folder: scene
after: none
decisions: 0168, 0427, 0440

## Change
Comments only; no code changes. 0440 point 1 changes which Sky, Haze and
Picture row is used: the first in table order whose entity is on, not
simply the first.

- `scene/include/scene/sky_component.h`: the paragraph at line 16 ("THE
  FIRST ROW IN TABLE ORDER IS THE ONE DRAWN") and the `_rows` comment at
  line 63 ("rows[0] is the sky drawn") say: the first row whose entity is
  on (`voe_scene_identity_off`, 0440); an off row is as no row (0427).
- `scene/include/scene/haze_component.h`: the same for line 7's paragraph
  and line 56's "rows[0] is the one used"; line 4's "No row, no haze" also
  covers an off row.
- `scene/include/scene/picture_component.h`: the same for line 10's
  paragraph and line 71's comment; line 7's "NO ROW IS x1 AND CLIP" also
  covers no row on.

Each header comment stays no longer than it is now: reword in place.
`scene/scene.md` and `scene/include/scene/scene.md` change only if their
entries for these three headers say "first row".

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --header-lines scene/include/scene/sky_component.h scene/include/scene/haze_component.h scene/include/scene/picture_component.h`
prints no "over cap", and
`grep -c "rows\[0\]" scene/include/scene/{sky,haze,picture}_component.h`
prints 0 for each.
