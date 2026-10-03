# 05 — The tank cannot back out past the screen's bottom
folder: examples/tank_game/Code
after: 04
decisions: 0168, 0334

## Change
0334 point 2. Read `tank_hull.h`, `tank_hull_system.c`, `tank_scroll.h` and `Code.md`.

- `examples/tank_game/Code/tank_hull_system.c`: with a `tank_scroll` row
  (`tank_scroll_get`), a hull's driven z is held to at most the larger of its z before the drive
  and the row's `bottom` less `TANK_HULL_MARGIN`, 3 m (about half the hull's length, so the
  whole tank stays on screen). x and the turn are untouched; with no row nothing changes. The
  file header says why the clamp and why one already past is not pulled in.
- `examples/tank_game/Code/tank_hull.h`: the header says a hull cannot back out of the screen
  once the scroll row exists.
- `examples/tank_game/Code/Code.md`: the hull system entry says the clamp.

## Done when
`(for f in examples/tank_game/Code/*.c; do clang -std=c23 -fsyntax-only -DVOE_BASE_DESCRIPTIONS=1
$(for d in */include; do printf -- "-I%s " $d; done) "$f" || exit 1; done)` exits 0, and `grep -q tank_scroll_get examples/tank_game/Code/tank_hull_system.c` exits 0.
