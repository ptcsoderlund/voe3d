# 13 — The `run.h` entry in game.md fits its cap
folder: game/include/game
after: none
decisions: 0168

## Change
Text only. Read `game/include/game/game.md` and the header comment of
`game/include/game/run.h`.

- `game/include/game/game.md`: the `run.h` entry is over the 300-character
  entry cap. Cut it to one sentence: the game's whole run in the window it is
  handed, until the window closes or the interface ends it. The list of what
  the run builds (world, types, mixer, sound device, scene, prefabs, systems)
  and the pause and restart leave the entry.
- `game/include/game/run.h`: only if its header comment does not already say
  something the entry drops, add that point to the header. Its header already
  gives the order and the pause and restart (0333); expect no change.

## Done when
`awk '/^- .run\.h./{ok = length($0) < 290} END{exit !ok}' game/include/game/game.md`
exits 0.
