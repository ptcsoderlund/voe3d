# 042 — Particles

## What
The engine draws particles: many small sprites that are born, move, fade, change size and colour,
and die. They are made by an emitter placed on a thing, set in the Inspector (how many, how fast,
how long they live, their colours and sizes over their life, and their texture, taken from the
Assets panel), and seen live in the editor. Game code can start and stop an emitter, or fire a
single burst. Particles are lit by the scene, and some can glow (fire). In the tank game there is a
muzzle flash at each shot, smoke and a burst of fire at each explosion, and dust behind the treads
while driving.

## Why
Milestone 9 of 0268. The game's hits and motion need to be felt, not just seen.

## How to test
1. In the editor, add a particle emitter to a thing. It starts at once in the views. Change its speed,
   colours and texture in the Inspector. The view changes live. Undo works.
2. Play the tank game. Driving leaves dust behind the tank, and none while it stands still.
3. Fire. Each shot has a flash at the barrel. Hit an enemy. It explodes in fire and smoke that rises
   and fades.
4. Blow up ten things quickly. The game stays smooth.
5. Put the sun low and orange. The smoke is lit orange on one side.
