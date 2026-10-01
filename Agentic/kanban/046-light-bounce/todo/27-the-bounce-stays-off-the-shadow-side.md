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
