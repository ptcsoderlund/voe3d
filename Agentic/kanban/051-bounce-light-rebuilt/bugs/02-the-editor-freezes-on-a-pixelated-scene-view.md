# 02 — The editor freezes on a pixelated scene view

## Seen
"editor freeze in a pixelated state (not gui, scene view is pixelated)."

The editor had aborted on a failed assert in its first frames; under CLion's debugger the window stays on the
last image. Run again under gdb with no input, it aborts the same way:

```
voe_base_assert_fail (expression="!frame->bounce_shadow.drawn",
    file="render/src/bounce_shadow.c", line=173,
    message="opening a second bounce shadow pass in one frame")
#4 voe_render_bounce_shadow_pass_begin   render/src/bounce_shadow.c:172
#5 draw_sun_map                          3d/src/draw_bounce.c:151
#6 voe_3d_draw_bounce                    3d/src/draw_bounce.c:199
#7 voe_3d_draw_system_shadows            3d/src/draw_shadows.c:254
#8 voe_editor_view_passes_draw           editor/src/view_passes.c:163
#9 main                                  editor/src/main.c:620
```

## Expected
The editor opens and draws every view it shows, each with its own bounce (0326 point 8) and each relight
shadowing the sun by a sun map of its own (0329 point 2), however many views are shown at once.

## How to reproduce
1. Build `voe_editor` (debug) at the end of card 32.
2. Start it with no arguments; it opens the last project, `examples/tank_game`, with its usual layout showing
   more than one view.
3. In its first frames the scene view freezes, pixelated, and the process aborts with the assert above. No
   input is needed.
