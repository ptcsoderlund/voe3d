# 086 — The ground is painted in layers

## What
A terrain has a list of layers, each one a material from 085: dirt, grass, moss, rock, or whatever I
make. I paint layers onto the ground with a brush that has a radius, a strength and a soft edge, and the
layers blend where they meet. The blend looks like grass growing between stones, not a blurry smear.
The first layer covers the whole terrain from the start. The ground's textures repeat without an
obvious grid of copies when seen from far. Painting is undoable stroke by stroke and saved with the
scene.

## Why
Blended ground is half of what makes the hill look like a place, and grass (091) grows from the grass
layer.

## How to test
1. Make Dirt, Grass, Moss and Rock materials. Add them as layers to the hill's terrain. The hill is all
   dirt.
2. Paint grass over the lower slopes and rock over the top. Each shows where I paint.
3. Paint moss lightly at the edge of the rock. The moss creeps in between the rock, and the edge does not
   look smeared.
4. Fly up high. The grass does not show a visible tiled pattern across the slopes.
5. Undo one stroke, then redo it.
6. Change the Grass material's tint in the Inspector. The painted grass changes at once.
7. Save, close and reopen. Press Play, and ship: the same ground in all three.
