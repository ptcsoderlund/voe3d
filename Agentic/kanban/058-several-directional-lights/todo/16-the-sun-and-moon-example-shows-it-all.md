# 16 — The sun and moon example shows it all
folder: examples
after: 03, 04, 14, 15
decisions: 0168, 0357, 0349, 0350
read: feature.md

## Change
The project that `## How to test` is walked through. `examples/sun_and_moon/` exists already with
its `Code` (card 04); add its files beside it, each modelled on `examples/capsule`'s. Read
`examples/capsule/project.voe3d`, `examples/capsule/capsule.md`, the first 100 lines of
`examples/capsule/main.scene` for the section shape, and `examples/tank_game/main.scene` lines
~295–311 for a light blocker's sections.

- `examples/sun_and_moon/project.voe3d`: as capsule's, naming `main.scene`.
- `examples/sun_and_moon/main.scene`, every row written out (literals cast nothing, 0324):
  - a camera looking at the middle from about 12 m;
  - a grey ground 40 m square, and three boxes on it, every shape `cast_shadows = true`;
  - "Sun": white, intensity 1, from high on +x, fill 0.1, `cast_shadows = true`, `bounces = 1`,
    with a `day_night` row: day 1, night 0, day shadows true, night shadows false, period 20;
  - "Moon": faint blue `[0.6, 0.7, 1]`, from low on −x, fill 0.03, `cast_shadows = true`,
    `bounces = 0`, with a `day_night` row: day 0, night 0.3, day shadows false, night shadows true,
    period 20;
  - "Cave": walls and a roof of boxes at one side of the ground, closed but for a door, with a light
    blocker of Block All (`block = 0`) filling its inside;
  - "Cave Light": a directional light placed inside the blocker, warm, intensity 0.4, no shadows,
    no bounces.
- `examples/sun_and_moon/sun_and_moon.md`: what the project shows, with an entry each for
  `main.scene` and `Code`, as capsule's.
- `examples/examples.md`: a `sun_and_moon` entry.

## Done when
`[ $(grep -c '^\[[0-9]*\.voe_scene_light\]' examples/sun_and_moon/main.scene) -eq 3 ] && [ $(grep -c
'^\[[0-9]*\.day_night\]' examples/sun_and_moon/main.scene) -eq 2 ] && grep -q
'^\[[0-9]*\.voe_scene_light_blocker\]' examples/sun_and_moon/main.scene && grep -q sun_and_moon
examples/examples.md` exits 0.

The human, in the editor with `examples/sun_and_moon` open:
1. Both the sun and the moon light the scene, from their two sides, and their fills add up.
2. With Cast Shadows on both there are two shadows. With the sun's off, only the moon's stay. With
   one casting light the frame rate is as before.
3. The moon's Bounces at 1 bounces its light.
4. Play: the sun fades out and loses its shadows, and the moon fades in with its own, then back.
   It is smooth, with no flicker.
5. The cave is lit by Cave Light alone, and the sun lights outside it. Neither shows on the other
   side.
6. Save, close and reopen: every light is kept. `examples/capsule`, a one-light scene, looks as it
   did.
7. Delete every light: the views show the editor's preview light, and Play shows a black game.
