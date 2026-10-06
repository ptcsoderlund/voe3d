# 076 — Glass

## What
A material can be glass. You see through it to what lies behind, bent by its index of refraction and
blurred by its roughness, so frosted glass is frosted. Thick glass tints what is seen through it more the
deeper it goes, by its colour and distance. Thin glass, like a window pane, does not bend the view. Glass
with dispersion splits light into a faint rainbow at its edges. Glass still reflects the environment and
the lights. It is seen through to everything that is not glass. Glass behind other glass is not seen
through the nearer pane, which is an accepted limit (0373). Glass costs little and nothing when no glass is
in view.

## Why
Glass is the most-asked-for look among the samples, and done like water (0305) it fits the hardware line.

## Models
From https://github.com/KhronosGroup/glTF-Sample-Assets/tree/main/Models: TransmissionTest, TransmissionRoughnessTest, TransmissionThinwallTestGrid, TransmissionOrderTest, CompareTransmission, CompareVolume, CompareIor, IORTestGrid, AttenuationTest, DispersionTest, CompareDispersion, DragonDispersion, DragonAttenuation, GlassBrokenWindow, GlassHurricaneCandleHolder, GlassVaseFlowers, MosquitoInAmber, ABeautifulGame, USDShaderBallForGltf, LightsPunctualLamp, FlightHelmet.

## How to test
1. With `footprint_court.hdr` as environment and background, place TransmissionTest, TransmissionRoughnessTest
   and CompareTransmission. Open them in the Khronos glTF Sample Viewer (https://github.khronos.org/glTF-Sample-Viewer-Release/). You see through them the same way, and the
   rough ones blur what is behind.
2. Place TransmissionThinwallTestGrid, AttenuationTest, CompareVolume, CompareIor and IORTestGrid. Thin glass
   does not bend the view. Thick glass bends it and tints deeper parts more, as in the viewer.
3. Place DispersionTest, CompareDispersion and DragonDispersion. A faint rainbow shows at the edges, as in the
   viewer.
4. Place TransmissionOrderTest. It matches the viewer apart from glass behind glass.
5. Place DragonAttenuation, GlassBrokenWindow, GlassHurricaneCandleHolder, GlassVaseFlowers, MosquitoInAmber,
   ABeautifulGame, USDShaderBallForGltf, LightsPunctualLamp and FlightHelmet. At a glance each looks like the
   viewer.
6. Open the frame breakdown with glass in view and then out of view. Glass costs little in view and nothing
   out of it.
7. Press Play: the same in the game window.
