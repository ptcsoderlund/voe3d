# 101 — Clouds in the sky cast shadows on the world

## What
Decisions 0479 and 0480. "I want clouds in the sky and I want them to project shadows in the world." A Clouds
row under Rendering, like Sky and Haze, has a Kind: Flat or Volumetric. "Some games we want flat, but in this
demo we want volumetric." Flat clouds are a layer at one height. Volumetric clouds have depth: I can fly up to
them, and they look different from every side. Both kinds have coverage (clear sky to overcast), height, and
wind direction and speed that move them. Volumetric clouds also have thickness. The sun lights them, so a low
orange sun colours them. The haze veils them towards the horizon as it does the sky. Shiny surfaces and water
mirror them.

The same clouds cast shadows on the ground, grass, forest and buildings, and the shadows move with the
clouds. The Clouds row has its own Shadow strength for how dark a cloud's shadow is, separate from the light's
fill, which still sets how dark the shadows of objects are. The clouds look the same in the editor's view, in
Play and in a shipped build, and the hill stays smooth on my laptop.

## Why
"The hill is not a shipped product until we have clouds and a proper sky." Clouds are the only thing the sky
lacks: the colours and day and night are the game developer's to make (0480).

## How to test
1. Add a Clouds row to the hill. Set the Kind to Volumetric. Clouds fill the sky above the valley.
2. Slide coverage from clear to overcast and back: the sky clears and fills. Raise and lower the height, and
   make them thicker: they move up and down and grow taller.
3. Set a wind: the clouds drift that way. Their shadows sweep over the hill, through the forest and over the
   house, matching the clouds above.
4. Change Shadow strength: the cloud shadows get lighter and darker, while the house's and the trees' own
   shadows stay as they were. Change the light's fill: the object shadows change, and the cloud shadows do not.
5. Lower the sun and make it orange: the clouds turn orange where it lights them and darker on their far side.
6. Fly up to the clouds from the tower: they have depth, and I can fly round and into them.
7. Look at the water and at a shiny surface: the clouds show in them.
8. Set the Kind to Flat: a flat drifting layer replaces them, and it still casts shadows. Set it back.
9. Fly through the forest under the volumetric clouds: it stays smooth, and the frame breakdown shows what
   the clouds cost.
10. Save, close and reopen: the clouds are as I left them. Press Play, and ship: they look as they do in the
    editor's view.
