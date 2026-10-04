# 32 — 3d sorts blockers by Block
folder: 3d
after: 30
decisions: 0168, 0350, 0352, 0353

## Change
3d follows card 30's rename; behaviour unchanged (0353 point 4: Direct's bit goes in `walls`,
Fill's in `indoors`, All's in neither). Read the headers of `3d/src/draw_light_blockers.c`,
`voe_3d_draw_system_light_blockers` in `3d/include/3d/draw_system.h`, and
`scene/include/scene/light_blocker_component.h`.

- `3d/src/draw_light_blockers.c`: `rows[i].block` against `VOE_SCENE_LIGHT_BLOCKER_DIRECT` and
  `_FILL`; header points name Block and the values, and that render's words keep their names.
- `3d/include/3d/draw_system.h`: the call's comment names Block and its values for the kinds.
- `3d/tests/light_blockers.c`: `.block` and the new constants throughout; its helper and case
  names and comments say block, All, Fill, Direct in place of kind, Room, Indoors, Wall. Claims
  and samples unchanged.
- `3d/src/src.md`, `3d/tests/tests.md`: entries naming Room, Indoors or Wall for a blocker say
  All, Fill, Direct. Each at most 300 characters.

## Done when
The tests `3d/light_blockers` and `3d/bounce_scene` pass after the folder's build, and
`grep -rnE "LIGHT_BLOCKER_(ROOM|INDOORS|WALL)|\.kind = kind" 3d` prints nothing.
