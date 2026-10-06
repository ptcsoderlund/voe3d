# 074 — Specular, IOR and clearcoat

## What
A material can set how strongly and in what colour non-metals reflect (specular), how much a surface bends
light (its index of refraction), and a clear lacquer layer over it (clearcoat) with its own roughness and
its own bumps. A car's paint shows a sharp clear reflection over a soft coloured base. Each looks like the
Khronos viewer at a glance and costs little.

## Why
Three cheap extensions that most showcase models use, and that glass (076) builds on.

## Models
From https://github.com/KhronosGroup/glTF-Sample-Assets/tree/main/Models: SpecularTest, CompareSpecular, CompareIor, ClearCoatTest, CompareClearcoat, ClearCoatCarPaint, ClearcoatWicker, PotOfCoals.

## How to test
1. With `footprint_court.hdr` as the environment, place SpecularTest and CompareSpecular. Open both in
   the Khronos glTF Sample Viewer (https://github.khronos.org/glTF-Sample-Viewer-Release/). At a glance they look the same.
2. Place CompareIor. Its spheres' reflections match the viewer. The see-through parts come in 076.
3. Place ClearCoatTest and CompareClearcoat. Every row matches its labels and the viewer.
4. Place ClearCoatCarPaint, ClearcoatWicker and PotOfCoals. The coat's sharp reflection sits over the base,
   as in the viewer.
5. Open the frame breakdown. These materials cost about the same as the plain ones from 070.
6. Press Play: the same in the game window.
