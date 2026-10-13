# 57 — The sky row holds no haze
folder: scene
after: 56
decisions: 0168, 0416, 0417

## Change
0416 point 1: the sky row loses `haze` and `haze_start`, which card 56 put on the Haze row.
`3d` and `game` still name them until cards 59 and 60; do not touch them. Read the headers of
`scene/include/scene/sky_component.h`, `scene/include/scene/sky_system.h`,
`scene/src/sky_system.c`, `scene/tests/sky.c` and `scene/include/scene/haze_component.h` (card 56).

- `scene/include/scene/sky_component.h`: `VOE_SCENE_SKY_FIELDS` ends at `strength`. Header: the
  first line's decisions add 0416; the field list and the bounds comment above the fields drop
  the haze; `strength` scales the backdrop only, never a disc, a glow or the haze; a new short
  paragraph: haze is the Haze row's (`scene/haze_component.h`), it takes this row's horizon hue
  but none of its brightness or strength (0416 point 3). The 0415 sentence goes.
- `scene/include/scene/sky_system.h`: the default row's comment, the usage example and the refusal
  paragraph drop haze and haze start.
- `scene/src/sky_system.c`: the default row and the drain's and `add`'s checks drop the two
  fields; its header follows if it names them.
- `scene/tests/sky.c`: the description lists five fields, `strength` last; the rows built drop the
  two fields; the refusals of an infinite haze and of haze start −1 and infinite go; the header's
  list follows.
- `scene/scene.md`, `scene/include/scene/scene.md`, `scene/src/src.md`, `scene/tests/tests.md`: the
  sky entries drop the haze where they name it (the field count in `tests.md`), each one sentence
  of at most 300 characters.

## Done when
The tests `scene/sky` and `scene/haze` pass after the folder's build, and
`! grep -qi haze scene/src/sky_system.c` exits 0.
