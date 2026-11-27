# 117 — Foliage groups

## What
The tree maker is gone (0520): my trees come from Blender as glbs, materials baked to PBR, and are painted
as foliage like any model. Instead of a foliage kind, a landscape has foliage groups, up to 16. A group holds
one or more glbs, each with its own LODs as any model has. The group is what I paint and what a material
grows: each painted spot takes one of the group's glbs, the same every time the project opens, so I paint a
stretch of hillside once with three spruces and a bush instead of once per model.

Each glb in a group has the settings a kind has today — cast shadows, base size, size, turn and tilt
scatter, blend with ground and its height — and a share, how often it is picked against the others, equal
to begin with. A group is painted at one density; things spaced alike go in one group.

I can remove a glb from a group: its spots are taken by the group's other glbs. I can remove a whole group
from a landscape without deleting the landscape: its copies go, the other groups stay as they were. Each is
one undo step. A project saved with kinds opens with each kind as a group of one glb, everything where it
was. The landscape file is taken in when it changes on disk (115) and `Docs/` describes groups (116).

Nothing of the tree maker is left: no tree species file, no Create tree, no species view, no tree settings.

## Why
I am about to make the hill's trees in Blender. A group lets one stroke lay down a forest that varies,
without a kind per model and without painting the same slope once for each.

## How to test
1. Open the hill as it is today. Every kind is now a group holding its one glb; every tree, rock and tuft
   stands where it stood and looks the same.
2. Make a group and drag three spruce glbs and a bush glb into it. Paint a stretch of hillside: all four
   mix, and looking along it I cannot pick out repeats.
3. Set one spruce's share to 3: it comes up about three times as often as each of the others. Set it to 0:
   it no longer comes up.
4. Turn shadows off on the bush alone, raise one spruce's base size, set tilt with slope on the bush: only
   that glb's copies change.
5. Close and reopen the project: every spot has the same glb as before.
6. Let a ground material grow the group: the same mix comes up on it.
7. Remove one spruce from the group: its spruces go and the other glbs fill their spots. Undo: it is back
   as it was.
8. Remove a whole group with the cross on its title: every copy goes, the other groups keep the same trees
   in the same spots. Undo: the group is back with every copy where it was. Redo, save, close and reopen:
   it is gone.
9. Add groups up to 16: a seventeenth is refused.
10. Fly away from the forest: each glb swaps to its own LODs without popping, and the frame cost stays under
    114's budget. The spruces cast shadows.
11. Edit a share in the landscape file by hand while the editor is open: the hillside changes. `Docs/`
    describes groups and their glbs' settings.
12. Assets has no Create tree, the Inspector no tree section, `Docs/` no species file.
13. Press Play, then ship: the forest looks the same in both.
