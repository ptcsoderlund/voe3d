# 070 — Surfaces look the way the file describes them

## What
Every core material setting of a glTF model draws as it should, under the environment (069) and the
scene's lights alike. Normal maps use the tangents the file gives, or the standard ones when it gives
none, so mirrored halves are lit the same way. Base colour, metal and roughness match the file. Emission
glows, and a material's emission strength makes it glow brighter than white. Alpha works in all three modes:
opaque ignores it, mask cuts hard edges that stay solid when far away, and blend is see-through and sorted
so near glass covers far glass. A surface marked double-sided is drawn and lit from both sides, and a
one-sided surface is culled. An unlit material shows its colour flat, untouched by any light.

## Why
With the light right (069), this is the core of every sample model and of every game's imported model.

## Models
From https://github.com/KhronosGroup/glTF-Sample-Assets/tree/main/Models: NormalTangentTest, NormalTangentMirrorTest, CompareNormal, AlphaBlendModeTest, CompareAlphaCoverage, TwoSidedPlane, EmissiveStrengthTest, CompareEmissiveStrength, UnlitTest, CompareBaseColor, BoomBox, WaterBottle, Lantern, Corset, BarramundiFish, AntiqueCamera, SciFiHelmet, FlightHelmet, Sponza.

## How to test
1. Give a new scene `footprint_court.hdr`. Place NormalTangentTest and NormalTangentMirrorTest. Every
   sphere's bump is lit from the same side, as in their screenshots.
2. Place AlphaBlendModeTest. The opaque, masked and blended rows match its screenshot. Fly back until it is
   small: the masked edges stay solid instead of thinning away. Do the same with CompareAlphaCoverage.
3. Place TwoSidedPlane and fly around it. It is visible and lit from both sides.
4. Place EmissiveStrengthTest and CompareEmissiveStrength. The stronger cubes glow brighter, as in the viewer.
5. Place UnlitTest. Its colours stay flat whatever the light and the environment do.
6. Place BoomBox, WaterBottle, Lantern, Corset, BarramundiFish, AntiqueCamera, SciFiHelmet, CompareBaseColor
   and FlightHelmet. Open each in the Khronos glTF Sample Viewer (https://github.khronos.org/glTF-Sample-Viewer-Release/) with Footprint Court. At a glance they look the same. FlightHelmet's
   goggle lenses look like glass only after 076.
7. Place Sponza and fly through it lit by today's directional light. The hanging cloth and plants are cut
   cleanly, and both sides of the cloth show.
8. Press Play: the same in the game window.
