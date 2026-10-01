# 046 — needs decision: the bounce under a sun that clips

## Question
Card 21 (blocked) found that the scene in bug 01's screenshot, `examples/tank_game/main.scene`, has a sun
of intensity 9. In 8-bit sRGB with no tone map, its sunlit beige ground clips: at 1 m from the red box
it reads 255 254 219, and 8 m out 255 253 219. Any bounce added on top of that is cut off, so the ground
cannot get redder however right the bounce is. Bug 01's Expected ("the ground beside the red box reads
visibly redder") cannot hold in that scene as the frame works now. With the sun at π and no fill, the
same spot measures 195 159 136 against 191 158 136: +0.017 red over green, a real but faint tint.

Under the bounce pass and update there is no fault left in `3d` or `render` that the cases can show
(cards 19, 20 pass; card 21 fixed the casters drawing white). What is open is the look: how a sunlit
surface gets bright enough for the bounce to show without clipping. That decides what the frame's
colour is, in every scene and both programs, so it reaches past this feature (0024, 0051 and 0083 put
tone mapping on the *later* list, and 0083 says it changes the frame's colour format).

## Options
1. **Scenes keep the sun in range.** No engine change. The tank game's sun goes to about π (the new
   scene's light already is π, `editor/src/project.c`) and its fill is set again to suit. Card 21's TINT
   is measured at sun π, fill 0, with a threshold set from that measurement (0.01). Cheap; the sponsor's
   own scenes can still clip if lit hotter, and the bounce is faint in full sun.
2. **A tone map in `render` now.** The offscreen target goes to a float format and the frame's last
   step maps it (e.g. ACES or AgX) to the window's sRGB. Sun 9 stops clipping, every scene changes look,
   every pixel-reading test in the tree is re-measured. A work order of its own before 046 can close.
3. **Accept the clip.** The bounce shows only where the sun does not saturate: in shade and on darker
   ground. Card 21's TINT moves to the shaded ground beside the box; bug 01's Expected is amended.

## Recommendation
Option 1 for 046, so the feature closes with bounce visible in the game's scene, and tone mapping
(option 2) put on the 0268 road as its own work order, since that is where a sun of 9 belongs.

## Also open, smaller
`3d/shadows`' 100 KM OUT case reads 47 (32 with card 21's fix) under the cube where it reads 0 at the
origin, failing before card 21 too: bounce light from earlier frames on the same device's window grid
reaching the shadowed floor. Recommendation: that case compares like with like, so each of its two
pictures is drawn with the bounce off (the shadows call naming a target with no grid, as card 21's
reference frame does), and its claim stays "the cascades lose nothing at 100 km"; the bounce's own
reach far out is `3d/bounce`'s and `render/bounce_scene`'s to test.
