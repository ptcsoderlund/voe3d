# 38 — The game says a world with no light draws black
folder: game
after: 37
decisions: 0168, 0287

## Change
Bug 04. After card 37 a game frame of a world with no light draws lit surfaces black; the game's
own code does not change, only what it says.

- `game/include/game/frame.h`, the Constraints paragraph: "draws every surface in its material
  colour, unshaded (0238)" becomes lit surfaces black, the background colour and the GUI as
  before (0287).
- `game/tests/frame.c` header, the "TWO CASES" paragraph: the lightless world no longer asserts
  and now draws black (0287), not unshaded. The test body does not change.

## Done when
`ctest --test-dir build/debug -R '^game/frame$'` passes, and
`grep -rln 0238 game/include game/src game/tests` prints nothing.
