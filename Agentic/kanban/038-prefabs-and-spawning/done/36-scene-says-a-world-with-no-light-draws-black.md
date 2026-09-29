# 36 — Scene says a world with no light draws black
folder: scene
after: none
decisions: 0168, 0287

## Change
Bug 04. Only a comment changes.

`scene/include/scene/light_system.h`, the paragraph on `voe_scene_light_register` "A WORLD THAT
IS DRAWN NEEDS THIS EVEN IF IT HOLDS NO SUN": replace "a world with no light draws unshaded
(0238)" with its lit surfaces drawing black in a game, and the editor's views lighting it with a
preview light (0287). Say "directional light" rather than "sun" in the lines you touch (0288).

## Done when
`! grep -q 0238 scene/include/scene/light_system.h && grep -q 0287 scene/include/scene/light_system.h`
exits 0.
