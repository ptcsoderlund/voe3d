# 24 — The game draws a Wall blocker's shadow
folder: game
after: 23
decisions: 0168, 0348, 0350
read: feature.md

## Change
The kind reaches the game window from the row: the game needs no code for it (the row is
described, drained in `voe_game_world_step`, cooked as a UINT32), so this card proves it. Read
the header of `game/tests/frame.c`, `scene/include/scene/light_blocker_component.h` and
`scene/include/scene/light_blocker_system.h`.

- `game/tests/frame.c`: a case with a lit ground shape under a low sun with a fill and a Wall
  blocker (kind `VOE_SCENE_LIGHT_BLOCKER_WALL`) floating above it; two frames as the file's cases,
  both true; the window read back: the ground in the box's shadow along the sun reads the fill,
  not black and below sunlit; ground clear of it reads sunlit; nothing is drawn on the box's edge.
  If the existing blocker case's samples now lie in that box's shadow along its sun, they are
  moved so they do not, and why is said there. Header point for the new case.
- `game/tests/tests.md`: the frame.c entry names the Wall case. At most 300 characters.

## Done when
The tests `game/frame` and `game/world` pass after the folder's build.

The human's, in the editor (bug 01's How to reproduce, all seven steps):
1. A directional light, Cast shadows off, over flat ground; a blocker, Kind Wall, raised into the
   air: a box-shaped patch on the ground in the sun's direction, with fill in it; it moves with
   the sun and with the blocker.
2. A point light, Cast shadows off, beside a Wall standing on the ground: none of its light on the
   far side, its light to the sides.
3. `examples/tank_game`: a house lined with Wall blockers inside its walls, windows and door open,
   one Indoors over its inside: dark inside but for sun through a window or the door; at the sun's
   Bounces 1+ that patch softly lights the room; outside unchanged.
4. A point light just outside a wall reaches inside only through a window or the door; one inside
   lights the room and shows outside only through them.
5. A Room around a cave: nothing from outside lights it; point lights inside never show outside;
   the directional light moved inside lights only the cave in its direction, outside goes dark.
6. Change a Kind, save, close, reopen: kept. A scene with blockers saved before: Rooms, as they
   looked. A scene with no blockers: as it was.
7. Play in each case: the game looks as the editor, the blockers invisible.
