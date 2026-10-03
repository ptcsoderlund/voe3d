# 0329 — The probe volume stands on the eye, and the relight shadows the sun with a map of its own
date: 2026-10-03
by: planner

## Decision
For 051 bug 01, carrying out 0328 and amending 0326 points 2, 6 and 8:
1. **Placement by the eye alone.** `voe_3d_bounce_grid_fit` takes the eye and no view. The lowest cell
   is the eye's cell at `VOE_RENDER_BOUNCE_SPACING` less 12 along x and z (half of
   `VOE_RENDER_BOUNCE_PROBES_XZ`) and less `VOE_3D_BOUNCE_BELOW` 8 along y: the volume reaches about
   16 m below the eye and 8 m above it. `VOE_3D_BOUNCE_AHEAD` goes. Turning the camera never moves the
   volume, so it never empties or recaptures a probe.
2. **The relight's sun map.** Per frame slot a D32 map of `VOE_RENDER_BOUNCE_SHADOW_TEXELS` 1024 a side,
   built at startup beside the capture scratch, only with `shaderOutputLayer`. 3d fits its light view
   to a sphere about the volume's centre of the volume's half-diagonal plus `VOE_RENDER_BOUNCE_REACH`
   (60 m, so every surface a probe can see), in the sun's basis, snapped to whole texels in double and
   pulled back `VOE_3D_SHADOW_CASTER_REACH`, through `light_box.h` as a cascade is.
   `voe_render_bounce_shadow_pass_begin(device, light, &opened)` opens it after the capture passes,
   `opened` only when this frame will relight and the begun sun bounces; 3d opens it only for a casting
   sun and draws the casters into it. The relight shadows the sun by that map, lit outside it, and
   unshadowed on a frame that drew none. The cascades no longer reach the relight;
   `voe_render_bounce_frame.shadow` goes.
3. **Cost.** At most one more pass, and one more object per caster, on a frame that relights a casting
   sun; nothing on a settled frame. The game's and the editor's capacities grow by one pass and one
   object factor. 4 MB of depth a frame slot.

## Reasoning
The volume stood 24 m ahead of the eye, so turning moved it: probes left, came in empty and were
captured again over seconds, and the room faded at the volume's edge: bug 01, against 0328. Centring on
the eye is the one placement turning cannot move. Eight cells below, not six: the tank camera stands
10.9 m up and the ground must stay out of the faded outer cell; a standing eye still keeps a room's
ceiling. The cascades follow the view too, so a relight while looking away found the room behind the
camera outside them and lit it as unshadowed; a map fitted to the volume answers the same whichever
way the camera faces. 1024 texels over 120 m is 12 cm, finer than an 8-texel probe face sees.
- Widen the cascades to cover the volume: changes the direct shadows 0258 set.
- Relight from the last cascade only: its centre still moves with the view.
- Keep the grid ahead and recapture faster: still changes with a turn, and costs every turn.
- A larger grid to keep the tank view's far ground: 1.8× the probes for 32 a side; if the volume's
  edge shows about 20 m ahead in the tank game, that is a later work order.

## Replaces
Nothing. Amends 0326 points 2 (the placement), 6 (the sun's shadow in the relight) and 8 (the calls).
