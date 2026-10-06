# 079 — Anisotropy

## What
A material can be anisotropic: brushed metal and carbon fibre stretch their highlights along a direction the
file sets, by a strength and a turn, including a direction drawn per pixel by a texture. It looks like the
Khronos viewer at a glance and costs little.

## Why
Brushed metal is a classic look, and the last single-material extension in 0.3.

## Models
From https://github.com/KhronosGroup/glTF-Sample-Assets/tree/main/Models: AnisotropyStrengthTest, AnisotropyRotationTest, AnisotropyDiscTest, CompareAnisotropy, CarbonFibre, AnisotropyBarnLamp.

## How to test
1. With `footprint_court.hdr` as the environment, place AnisotropyStrengthTest and AnisotropyRotationTest.
   Open them in the Khronos glTF Sample Viewer (https://github.khronos.org/glTF-Sample-Viewer-Release/). The highlights stretch the same amount and in the same direction.
2. Place AnisotropyDiscTest and CompareAnisotropy. The circular brushing matches the viewer.
3. Place CarbonFibre and AnisotropyBarnLamp. Each looks like the viewer at a glance.
4. Press Play: the same in the game window.
