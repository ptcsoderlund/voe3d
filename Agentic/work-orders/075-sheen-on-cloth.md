# 075 — Sheen on cloth

## What
A material can have sheen: the soft bright rim that velvet and fabric get at grazing angles, in its own
colour and roughness. Each cloth model looks like the Khronos viewer at a glance and costs little.

## Why
Cloth is common in real scenes, and sheen is cheap.

## Models
From https://github.com/KhronosGroup/glTF-Sample-Assets/tree/main/Models: SheenTestGrid, CompareSheen, SheenCloth, SheenChair, ChairDamaskPurplegold, SpecularSilkPouf, GlamVelvetSofa.

## How to test
1. With `footprint_court.hdr` as the environment, place SheenTestGrid and CompareSheen. Open both in
   the Khronos glTF Sample Viewer (https://github.khronos.org/glTF-Sample-Viewer-Release/). Every cell's rim matches.
2. Place SheenCloth, SheenChair, ChairDamaskPurplegold, SpecularSilkPouf, and GlamVelvetSofa. Each looks like the viewer at a glance. Variants come in 080, so each shows its first one.
3. Turn the view around a sofa. The fabric's rim brightens at its edges, as velvet does.
4. Press Play: the same in the game window.
