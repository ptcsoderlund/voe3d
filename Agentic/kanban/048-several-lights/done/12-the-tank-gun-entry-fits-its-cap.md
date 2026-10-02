# 12 — The tank gun entry fits its cap
folder: examples/tank_game/Code
after: none
decisions: 0168, 0320

## Change
The `tank_gun_system.c` entry in `examples/tank_game/Code/Code.md` is 329
characters; the cap is 300. Read `examples/tank_game/Code/Code.md` and the
header comment of `examples/tank_game/Code/tank_gun_system.c`, nothing else.

- `examples/tank_game/Code/Code.md`: shorten the `tank_gun_system.c` entry to
  one sentence under 300 characters. Keep: fire held fires each ready gun's
  prefab at its muzzle along the barrel; each shot flashes the muzzle flash and
  its light, and plays the shot sound. Drop the detail the header already
  carries (the three fire inputs, the owner, that missing flash and light are
  added at run time).
- `examples/tank_game/Code/tank_gun_system.c`: header only, and only if a
  dropped point is not already in it; it already states the inputs come from
  the control row's `fire`, the owner, and the run-time flash and light, so
  most likely no change. No code change.

## Done when
`checks.sh --folder examples/tank_game/Code` prints no finding naming
`examples/tank_game/Code/Code.md`.
