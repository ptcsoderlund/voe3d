# 058 — Several directional lights

## What
A scene can have several directional lights at once, for example a sun and a moon. Each one has its own
direction, colour, intensity, fill, Bounces and Cast shadows, and their light and fill add up. Game code can
change any of them while the game runs, so a day and night cycle can turn the sun's shadows off and the moon's
on at night, and back again in the day, leaving the moon as a faint light without shadows in the day. A new
directional light casts no shadows. Every directional light that casts shadows costs its own, so it is the
scene's choice how many do. Light blockers treat every directional light the same: Walls stop each one's
light, Indoors and Rooms keep each one's fill out, and one placed inside a Room lights only that Room
(0348, 0349).

## Why
Day and night with a sun and a moon, and caves with their own light inside while the sun lights the world
outside.

## How to test
1. Open a scene with a sun. Add a second directional light, name it moon, turn it to come from another side
   and make it faint and blue. Both light the scene: surfaces facing the moon get a faint blue light, and the
   fill is both lights' fills together.
2. Turn on Cast shadows on both. Things cast two shadows, one from each light, each in its own direction.
   Turn the sun's off: only the moon's shadows stay. The frame rate with one casting light is as it was with
   one directional light before.
3. Set Bounces on the moon to 1: its light bounces too, as the sun's does.
4. Press Play in a small test game whose code turns the sun's intensity down and its shadows off over a few
   seconds, then turns the moon's shadows on, and back. The light and shadows change smoothly and nothing
   flickers when a light's shadows switch.
5. Make a cave with a Room blocker. Put a dim directional light inside it: the cave is lit by it alone, in its
   own direction, and the sun still lights the world outside. Neither shows on the other side.
6. Save, close and reopen: both lights are there with their settings. Open a scene with one directional light
   saved before this: it looks exactly as it did.
7. Delete every directional light: the editor shows its preview light as today, and the game draws black.
