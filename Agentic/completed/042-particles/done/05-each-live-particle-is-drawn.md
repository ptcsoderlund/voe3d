# 05 — Each live particle is drawn
folder: 3d
after: 04
decisions: 0168, 0298

## Change
0298 point 6. Read `3d/include/3d/draw_system.h`, `3d/src/draw_system.c`,
`3d/src/draw_group.h`, `3d/include/3d/emitter_component.h` and
`3d/include/3d/models.h`.

- `3d/src/draw_particles.h` and `3d/src/draw_particles.c`, new, internal:
  a function that `voe_3d_draw_system_run` calls with the frame, adding one
  draw per live particle to the world layer's blended group:
  - the picture is `frame.models`' entry for the emitter's texture, or the
    one at "" for an empty texture; no models, or no loaded entry, draws
    nothing for that emitter;
  - part 1 when the emitter glows, else part 0;
  - the world matrix faces the camera (the view's right and up axes), is
    scaled by size, and sits at the particle's position, relative to the eye
    the way the other drawables are;
  - `t` is age over life; size lerps from start to end, colour from start to
    end, alpha from start to end; colour and alpha go into the object's
    colour;
  - a particle of a hidden entity (`frame.hidden`) is not drawn.
- `3d/src/draw_system.c`: calls it; the arena room it takes counts live
  particles as drawables too. Particles go to no shadow pass
  (`3d/src/draw_shadows.c` is unchanged).
- `3d/include/3d/draw_system.h`: its header says particles are drawn from
  `frame.models`, one blended draw each, sorted with the rest, casting no
  shadow, and what that needs in capacities: `VOE_3D_EMITTER_PARTICLES`
  objects per emitter per world pass.
- `3d/tests/draw_particles.c`, new, headless as `3d/tests/draw_system.c`
  is: an emitter of burst 5 run once through the emitter system, with the dot
  loaded, gives five more blended draws than without the emitter; with no
  models it gives none; a glowing one draws with part 1's shading. Read
  `3d/tests/draw_system.c`'s header for how draws are counted.
- `3d/include/3d/3d.md`, `3d/src/src.md`, `3d/tests/tests.md`: the entries.

## Done when
`ctest --test-dir build/debug -R '^3d/(draw_particles|draw_system)$'`
passes, after the folder's build.
