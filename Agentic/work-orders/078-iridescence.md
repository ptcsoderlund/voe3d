# 078 — Iridescence

## What
A material can be iridescent: a thin film like soap bubbles, oil or a beetle's shell, whose colour shifts
as you look at it from different angles, over metals and non-metals alike. It looks like the Khronos viewer
at a glance and costs little.

## Why
A distinctive look several showcase models need, and cheap done the game way.

## Models
From https://github.com/KhronosGroup/glTF-Sample-Assets/tree/main/Models: IridescenceSuzanne, IridescenceDielectricSpheres, IridescenceMetallicSpheres, CompareIridescence, IridescenceAbalone, IridescenceLamp, IridescentDishWithOlives, SunglassesKhronos.

## How to test
1. With `footprint_court.hdr` as the environment, place IridescenceDielectricSpheres, IridescenceMetallicSpheres
   and CompareIridescence. Open them in the Khronos glTF Sample Viewer (https://github.khronos.org/glTF-Sample-Viewer-Release/). The colour bands match at a glance.
2. Place IridescenceSuzanne and turn the view around it. The colours shift with the angle.
3. Place IridescenceAbalone, IridescenceLamp, IridescentDishWithOlives and SunglassesKhronos. Each looks like
   the viewer at a glance.
4. Press Play: the same in the game window.
