# 080 — Material variants, and the showcase

## What
A model whose file has material variants shows a Variant dropdown in the Inspector, listing the file's
variants by name. Picking one swaps the model's materials at once. It is saved with the scene, undoable, and
game code can change it. With that, every showcase model in 0.3's scope looks like the Khronos viewer at a
glance, and 0.3 is done (0373).

## Why
Variants are cheap and the last feature the showcase models need; the showcase is 0.3's finish line.

## Models
From https://github.com/KhronosGroup/glTF-Sample-Assets/tree/main/Models: MaterialsVariantsShoe, SheenChair, GlamVelvetSofa, DragonAttenuation, StainedGlassLamp, ChronographWatch, CarConcept, ToyCar, CommercialRefrigerator, AnisotropyBarnLamp, IridescentDishWithOlives, PotOfCoals, DamagedHelmet, Sponza.

## How to test
1. Place MaterialsVariantsShoe. The Inspector shows Variant with the file's names. Pick each in turn: the
   shoe changes at once. Undo goes back to the previous one.
2. Do the same with SheenChair, GlamVelvetSofa, DragonAttenuation, StainedGlassLamp and ChronographWatch.
3. Save, close and reopen. Each model keeps its variant. Press Play: the same in the game window.
4. Give a new scene `footprint_court.hdr` and place CarConcept, ToyCar, CommercialRefrigerator,
   AnisotropyBarnLamp, IridescentDishWithOlives, PotOfCoals, StainedGlassLamp and DamagedHelmet. Open each in
   the Khronos glTF Sample Viewer (https://github.khronos.org/glTF-Sample-Viewer-Release/). At a glance each looks the same.
5. Add Sponza around them and fly through on your laptop. It stays smooth.
6. Ship the project and run it. It looks the same as in the editor.
