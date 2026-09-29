# Needs decision: which way does the preview light shine?

For bug 04 (`bugs/04-a-prefab-with-no-light-is-drawn-flat.md`), decision 0287.

## Question
0287 says the preview light has "exactly the settings a newly added directional light gets (its
direction, ...)", and that the two change together "because they are one set of values". A newly
added light has no direction of its own. The only way to add one is Add component "Rendering /
Light" (`scene/include/scene/light_system.h`). That gives the default row: white, strength 1, fill
0. The light then shines along its entity's transform's -Z (0273). A new entity, or the transform
the light's `needs` adds, has the identity rotation. So a new light shines along -Z, which is
horizontal (0033: +Y up, -Z forward).

A preview light built literally from those values grazes the ground and every top face at 90
degrees. With fill 0 they draw black, and shadows stretch sideways. In the tank game's top-down
view, a prefab or level with no light would look worse than it does now, not "shaded and casting
shadows" as the bug expects. The only well-lit default in the tree is the untitled scene's light
(`editor/src/project.c`): shining down and from the front-right, strength pi. It is the editor's,
and Add component does not use it.

## Options
1. **Literal.** The preview is the default row plus -Z. This matches 0287's wording and needs no
   new values, but the preview draws upward faces black.
2. **The untitled scene's light is "the new light".** Its direction and strength move into one set
   of values that the untitled scene and the preview both use. Add component still gives white,
   strength 1, and the entity's own rotation. This stays inside the editor and fits this feature.
3. **Give every new light a tilted default.** A light added to an entity with no rotation of its
   own, or a light registered with a default facing, shines down and from the front-right, and the
   preview uses that value. This reaches `scene` and maybe `ecs` (a dependent's default per needing
   type), and changes what Add component does everywhere.

## Recommendation
Option 2. It gives a preview that looks right in the tank game with one editor-local set of values.
It also keeps 0287's "one set of values" if "a newly added directional light" is read as the one
the editor makes for a new scene. Option 3 is right only if a new light added through Add
component should also shine down. That is a product question for the sponsor, beyond this bug.
