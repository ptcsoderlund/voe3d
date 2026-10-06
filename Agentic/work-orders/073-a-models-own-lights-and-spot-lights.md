# 073 — A model's own lights, and spot lights

## What
Lights in a glTF file come in with its model: directional, point and spot lights, with their colour,
strength, range and cone. They shine where the model is placed and move with it. They cast no shadows
(0316, 0373). A point or spot light fades with distance as the file intends. A lamp model's bulb lights
its surroundings as it does in the Khronos viewer.

The editor gains the spot light as a light type of its own, beside directional and point lights, with a
cone and a soft edge and Cast shadows, in the same Inspector, marker and picking as the other lights.

## Why
Lamps that carry their own light are half of what a lit model is, and the spot light has been waiting since 0321.

## Models
From https://github.com/KhronosGroup/glTF-Sample-Assets/tree/main/Models: DirectionalLight, PointLightIntensityTest, PlaysetLightTest, LightsPunctualLamp.

## How to test
1. Place DirectionalLight in a scene with no other light and no environment. It is lit by its own light, as in
   its screenshot.
2. Place PointLightIntensityTest. The pattern of lit and dark matches its screenshot.
3. Place PlaysetLightTest. Open it in the Khronos glTF Sample Viewer (https://github.khronos.org/glTF-Sample-Viewer-Release/). Its lamps light the room in the same places, at the same
   strength at a glance.
4. Place LightsPunctualLamp and move it with the gizmo. Its light moves with it. Its glass shade looks
   like glass only after 076.
5. Add a spot light from Add component. Its marker shows its cone. Turn it with the rings: the lit spot
   follows. Widen the cone and soften the edge, and the spot changes. Turn on Cast shadows: things in the
   cone cast shadows.
6. Save, close and reopen. Press Play: the same in the game window.
