# 27 — The bounce stays off the shadow side
folder: render
after: none
decisions: 0168, 0310, 0311, 0312, 0313

## Change
Bug 03: the tank Obstacle's sun shadow at its foot is lit and tinted by the
bounce. 0312 says what must hold; 0313 is the fix. Read
`render/shaders/bounce.slang`, `render/shaders/lighting.slangh` (the gain),
`render/shaders/shaders.md`, `render/tests/bounce_scene.c`,
`render/tests/tests.md`, `examples/tank_game/main.scene` (Sun; Obstacle,
2 × 10 × 1 at (−4.09, 5, 6.01), lit on its −x face, its +x face in shadow)
and `editor/src/view.h` (view 0 orbits the Camera entity).

1. Measure today's build, before any change. Copy `examples/tank_game` to
   `build/debug/tank_sun_1` and `build/debug/tank_sun_pi`, Sun `intensity`
   1 and 3.14159; if view 0 does not show both the −x and +x feet of the
   Obstacle, move the Camera in both copies (never in `examples/`). Capture
   each as card 23 did (`--frames 60 --capture-view`), and again with
   `VOE_BOUNCE_GAIN` set to 0 for the build (bounce off; never committed).
   Read, by projecting from view 0's eye: ground 1 m, 4 m, 8 m out from the
   −x face (card 23's points), and S, ground 0.25 m out from the +x face at
   z 6.0, which must read near the fill in the bounce-off picture. The
   editor's render line must name the RTX 4070. Point 2 (S within 2/255 a
   channel of bounce off) should fail; if it holds at both suns, block with
   the numbers.
2. `render/tests/bounce_scene.c`, case SHADOW SIDE, at sun 1 and π, fill
   0.09: ground 0.25 m and 0.75 m out from the wall's shadowed face, inside
   its sun shadow, reads within 2/255 a channel of the same pixel with no
   update. The wall in it is no thicker than 1 m. Run it on today's
   shaders: it must fail; keep the numbers. Header gains the case.
3. `bounce.slang`, `gather`: drop the move behind the face; multiply each
   VPL's term by 0313's w. Header: the formula with w, ADR-0313, 0309 gone.
4. `lighting.slangh`: `VOE_BOUNCE_GAIN` becomes the lowest whole number at
   which both captures meet 0310's tint (blue at 1 m ≥ blue at 8 m + 12,
   4 m between) and S holds point 2. Header names the value.
5. Rerun step 1's captures and SHADOW SIDE; both pass.
6. Render tests whose numbers move (`bounce_scene.c` OFF THE ORIGIN's
   near and far, `bounce.c`): keep each near-over-far claim; a threshold
   the new weight moves below is reset to the measured lift less a margin,
   its header saying why (0313).
7. `shaders.md`: the `bounce.slang` entry loses "half a cell behind", says
   the standoff weight. `tests.md`: the `bounce_scene.c` entry names the
   shadow side, under its 300-character cap.

If no gain up to 50 meets the tint with S holding, or S's excess on the
fixed build is untinted (channels rising alike, as from Obstacle 2's lit
face, which 0312 allows), block with every number.

## Done when
The tests `render/bounce_scene`, `render/bounce`, `render/bounce_grid`,
`render/bounce_map` and `render/bounce_schedule` pass after the folder's
build. The commit message gives the card from the render line, the gain,
and for both suns, before and after: the 1 m, 4 m, 8 m and S pixels with
the bounce on and off, and SHADOW SIDE's failing then passing pixels.

## Blocked
No gain meets 0310's tint with S holding point 2, and "4 m between" fails at
every gain, because the standoff weight drops the 1 m point's near probe
(x −5, 0.09 m behind the −x face) to nothing. Measured on the RTX 4070 Laptop GPU,
view 0's Camera moved to x −2.84 in both copies (the tank hides S otherwise);
bounce off: sun 1 1 m/4 m/8 m (62,57,49), S (47,45,24); sun π (108,100,87),
S (47,45,24). Before the fix (gain 6): sun 1 1 m (79,69,64) 4 m (70,63,56)
8 m (62,57,50) S (47,45,29); sun π (135,119,111) (121,110,99) (108,100,89)
S (69,56,55). SHADOW SIDE before: sun 1 0.25 m 102 60 60, 0.75 m 88 60 60;
sun π 173 71 71, 150 68 68; all against 60 60 60. After the fix, 8 m stays
(62,57,49) / (108,100,88), sun 1 then sun π, 1 m; 4 m; S:
gain 6: (70,63,56) (71,64,57) (47,45,24); (121,110,99) (123,112,100) (47,45,24).
gain 10: (75,66,60) (77,69,62) (47,45,24); (129,116,106) (133,119,108) (47,45,25).
gain 11: (76,67,61) (78,70,63) (47,45,24); (131,117,108) (135,121,110) (47,45,27).
gain 12: (77,68,62) (80,71,64) (47,45,24); (133,118,109) (137,123,112) (47,45,28).
gain 20: (86,75,70) (89,79,72) (47,45,24); (147,129,121) (152,135,125) (50,45,38).
gain 50: (112,94,92) (117,102,96) (47,45,33); (189,161,156) (198,173,163) (79,66,62).
Gain 10 holds S but gives blue +11 at sun 1; 11 gives +12 but S's blue +3 at π
(blue rises first because the fill floor is weakest in blue). Committed: the fix
at gain 6 and SHADOW SIDE (passing, 60 60 60 at both suns and points); render
tests pass. Unblock: a decision on the near-face loss (e.g. w per receiver, or
an offset in w) or on dropping "4 m between" and the S margin.
