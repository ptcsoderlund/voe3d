# 33 — The game's blocker test says Direct
folder: game
after: 30, 31, 32
decisions: 0168, 0352, 0353
read: feature.md

## Change
game follows card 30's rename; no game code spells the field. Read the header of
`game/tests/frame.c` and `scene/include/scene/light_blocker_component.h`.

- `game/tests/frame.c`: the blocker case's row is `.block = VOE_SCENE_LIGHT_BLOCKER_DIRECT`; the
  header and comments say a Direct blocker in place of a Wall one, and the block in place of its
  kind. Claims and samples unchanged.
- `game/tests/tests.md`: the frame.c entry says Direct. At most 300 characters.

## Done when
The tests `game/frame` and `game/world` pass after the folder's build, and
`grep -rnE "LIGHT_BLOCKER_(ROOM|INDOORS|WALL)" game editor dev` prints nothing.

The human's, in the editor (bug 02's How to reproduce):
1. Open a scene, add a light blocker: the Inspector field reads Block, All selected.
2. Open the dropdown: All, Fill, Direct.
3. Open a scene saved before the change holding a Room, an Indoors and a Wall blocker: they show
   as All, Fill and Direct and the scene looks exactly as it did; save, reopen: still so.
4. feature.md's How to test still holds; Play looks as the editor.
