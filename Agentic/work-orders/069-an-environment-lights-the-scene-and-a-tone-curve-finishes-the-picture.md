# 069 — An environment lights the scene, and a tone curve finishes the picture

## What
A scene can have an environment: an `.hdr` panorama from the Assets panel, chosen in the scene's settings,
with a strength. It lights everything in the scene from all around. Rough surfaces take its soft light and
shiny ones reflect it, blurring as they get rougher. Metal reflects with its own colour, and the edges of
anything shiny catch more of the light. A model's occlusion texture darkens the environment's light in creases.
The environment can also be drawn as the background, which is a separate choice. A new scene has no
environment (0316, 0373). With none, lighting works as it does today.

A scene also has a tone curve and an exposure. The tone curve is Khronos PBR Neutral in a new scene and
none in a scene saved before this feature, so the tank and coin games keep their look. With the curve on,
very bright light rolls off smoothly instead of clipping to white, and colours keep their hue. The editor
views, Play and a shipped game all show the same picture.

Changing the environment costs nothing while it stays the same, and lighting with it is cheap every frame.

## Why
Every Khronos reference picture is lit by an environment and finished by a tone curve. Without both, no material can be compared fairly.

## Models
From https://github.com/KhronosGroup/glTF-Sample-Assets/tree/main/Models: EnvironmentTest, MetalRoughSpheres, MetalRoughSpheresNoTextures, CompareMetallic, CompareRoughness, CompareAmbientOcclusion, DamagedHelmet.

## How to test
1. Download `footprint_court.hdr` and `neutral.hdr` from https://github.com/KhronosGroup/glTF-Sample-Environments
   and import both into Assets.
2. Make a new scene. It has no environment, and the tone curve shows as Khronos PBR Neutral. Give it
   `footprint_court.hdr` with the background on. The court surrounds the view, and turning the view turns
   the court.
3. Place MetalRoughSpheres and EnvironmentTest. Open them in the Khronos glTF Sample Viewer (https://github.khronos.org/glTF-Sample-Viewer-Release/) with Footprint Court and its
   Khronos PBR Neutral tone mapping. At a glance they look the same: the same reflections, the same blur
   from smooth to rough, the same metal colours.
4. Do the same with MetalRoughSpheresNoTextures, CompareMetallic, CompareRoughness and DamagedHelmet.
5. Place CompareAmbientOcclusion. The creases darken as in the viewer.
6. Switch the environment to `neutral.hdr` and turn the background off. The models are relit at once, and
   the view's background is the plain one again.
7. Raise exposure, then drop it. The picture brightens and darkens. White highlights roll off softly and do
   not clip into flat patches.
8. Open the tank game's scene. It looks exactly as before this feature.
9. Open the frame breakdown. Environment lighting is a small share of the frame.
10. Save, close and reopen. The environment, strength, background, curve and exposure are kept. Press
    Play, and ship the scene's project: the same picture in both.
