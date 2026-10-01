# 31 — The lit foot keeps its near probe, within 0315's bounds
folder: render
after: none
decisions: 0168, 0310, 0311, 0312, 0313, 0314, 0315

## Change
Bug 03, after card 29 blocked: 0314's allowance o is added and chosen
against 0315's looser shadow bound; card 29 measured o 0.25 at gain 10
passing it, so this card confirms that and takes it. Read
`render/shaders/bounce.slang`, `render/shaders/lighting.slangh`,
`render/shaders/shaders.md`, `render/tests/bounce_scene.c`,
`render/tests/bounce.c`, `render/tests/tests.md`,
`examples/tank_game/main.scene` and `editor/src/view.h` (view 0 orbits the
Camera entity).

1. Captures as card 29 made them: copies of `examples/tank_game` at
   `build/debug/tank_sun_1` and `build/debug/tank_sun_pi` (make them if
   absent: Sun `intensity` 1 and 3.14159, Camera moved to x −2.84 so view 0
   shows both feet; never edit `examples/`), editor `--frames 60
   --capture-view`. Points by projecting from view 0's eye: ground 1 m, 4 m
   (z 6.0) and 8 m (z 5.0) out from the Obstacle's −x face, and S, ground
   0.25 m out from its +x face at z 6.0. Capture once with `VOE_BOUNCE_GAIN`
   0 (bounce off, never committed) for the reference. The render line must
   name the RTX 4070. Card 29's numbers, for a sanity check, off: sun 1
   1 m/4 m/8 m (62,57,49), S (47,45,24); sun π (108,100,87) (108,100,87)
   (108,100,88), S (47,45,24). At o 0.25, gain 10: sun 1 (76,67,62)
   (77,69,62) (62,57,49) S (47,45,24); sun π (132,117,108) (133,120,108)
   (108,100,88) S (47,45,31).
2. `bounce.slang`: constant `VOE_BOUNCE_STANDOFF`, o in metres; `gather`'s
   w becomes 0314's, saturate((n·(probe − VPL) + o)/spacing). Header: the
   formula with o, its value, ADR-0314 and ADR-0315.
3. `bounce_scene.c` SHADOW SIDE: its bound becomes 4/255 a channel
   (ADR-0315), header saying so; its points and claim otherwise unchanged.
4. Search: start at o 0.25, gain 10 (card 29 found gains 6–9 short of the
   tint there); then gains upward to 50; then o 0.5, 0.75, 1, each from gain
   1 upward to 50. Take the first (o, gain) where, at both suns, blue at 1 m
   is at least blue at 8 m + 12, 4 m's blue lies between them (inclusive),
   S is within 8/255 of bounce off in every channel, and `render/bounce_scene`
   SHADOW SIDE passes at 4/255. Set both constants to it;
   `lighting.slangh`'s header names the gain.
5. Render tests whose numbers move (`bounce_scene.c` OFF THE ORIGIN's near
   and far, `bounce.c`): keep each near-over-far claim; a threshold the new
   weight moves below is reset to the measured lift less a margin, its
   header saying why (ADR-0314).
6. `shaders.md`: the `bounce.slang` entry names the allowance. `tests.md`:
   the `bounce_scene.c` entry names SHADOW SIDE's 4/255 if it states a
   bound, and any other entry whose claim changed, each under its cap.

If no (o, gain) meets step 4, block with each o's best gain and its 1 m,
4 m, 8 m and S pixels at both suns and SHADOW SIDE's pixels, and commit
nothing of steps 2–6.

## Done when
The tests `render/bounce_scene`, `render/bounce`, `render/bounce_grid`,
`render/bounce_map` and `render/bounce_schedule` pass after the folder's
build. The commit message gives the card from the render line, o, the gain
and, at both suns, the 1 m, 4 m, 8 m and S pixels with the bounce on and
off, SHADOW SIDE's pixels against its reference, plus each (o, gain) tried
that failed and which condition it failed.
