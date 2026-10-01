# 29 — The lit foot keeps its near probe
folder: render
after: none
decisions: 0168, 0310, 0311, 0312, 0313, 0314

## Change
Bug 03, after card 27 blocked: 0313's w held the shadow side but dropped the
probe 0.09 m behind the tank Obstacle's lit −x face, so the ground 1 m out
reads darker than 4 m out at every gain. 0314 adds an allowance o. Read
`render/shaders/bounce.slang`, `render/shaders/lighting.slangh`,
`render/shaders/shaders.md`, `render/tests/bounce_scene.c`,
`render/tests/bounce.c`, `render/tests/tests.md`,
`examples/tank_game/main.scene` and `editor/src/view.h` (view 0 orbits the
Camera entity).

1. Captures as card 27 made them: copies of `examples/tank_game` at
   `build/debug/tank_sun_1` and `build/debug/tank_sun_pi` (make them if
   absent: Sun `intensity` 1 and 3.14159, Camera moved to x −2.84 so view 0
   shows both feet; never edit `examples/`), editor `--frames 60
   --capture-view`. Points by projecting from view 0's eye: ground 1 m, 4 m,
   8 m out from the Obstacle's −x face, and S, ground 0.25 m out from its +x
   face at z 6.0. Capture once with `VOE_BOUNCE_GAIN` 0 (bounce off, never
   committed) for the reference. The render line must name the RTX 4070.
   Card 27's numbers, for a sanity check: off, 1 m/4 m/8 m (62,57,49) and
   S (47,45,24) at sun 1; (108,100,87) and (47,45,24) at π.
2. `bounce.slang`: constant `VOE_BOUNCE_STANDOFF`, o in metres; `gather`'s
   w becomes 0314's. Header: the formula with o, its value, ADR-0314.
3. Search, o from 0.25 m upward in 0.25 m steps to 1 m; at each o, gains
   from 1 upward: the first (o, gain) where, at both suns, blue at 1 m is at
   least blue at 8 m + 12, 4 m's blue lies between them, S is within 2/255
   of bounce off in every channel, and `render/bounce_scene` SHADOW SIDE
   passes. Stop at gain 50 for an o and go to the next. Set both constants
   to it; `lighting.slangh`'s header names the gain.
4. Render tests whose numbers move (`bounce_scene.c` OFF THE ORIGIN's near
   and far, `bounce.c`): keep each near-over-far claim; a threshold the new
   weight moves below is reset to the measured lift less a margin, its
   header saying why (ADR-0314). SHADOW SIDE keeps its claim unchanged.
5. `shaders.md`: the `bounce.slang` entry names the allowance. `tests.md`
   only if an entry's claim changed, under its cap.

If no (o, gain) up to 1 m and 50 meets step 3, block with each o's best
gain and its 1 m, 4 m, 8 m and S pixels at both suns, and commit nothing
of steps 2–5.

## Done when
The tests `render/bounce_scene`, `render/bounce`, `render/bounce_grid`,
`render/bounce_map` and `render/bounce_schedule` pass after the folder's
build. The commit message gives the card from the render line, o, the gain
and, at both suns, the 1 m, 4 m, 8 m and S pixels with the bounce on and
off, plus each (o, gain) tried that failed and which condition it failed.
