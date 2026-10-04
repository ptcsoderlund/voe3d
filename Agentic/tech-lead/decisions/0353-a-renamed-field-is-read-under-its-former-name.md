# 0353 — A renamed field is read under its former name
date: 2026-10-04
by: planner

## Decision
How 0352 is carried out, for 056 bug 02:
1. **The row.** `voe_scene_light_blocker`'s `kind` becomes `block`, still a UINT32 with the same numbers:
   `VOE_SCENE_LIGHT_BLOCKER_ALL` 0, `_FILL` 1, `_DIRECT` 2, named "All", "Fill", "Direct". The Inspector
   shows a field by its name, so the field's name is what changes; the default row is still 0, All.
2. **ecs keeps a type's former field names**, as it keeps the unsaid row (0324): a described type may set,
   once, a list of pairs (a former key, the field it now is), the strings and the list the declaring
   folder's and outliving the world. ecs stores them and never reads them. The light blocker sets
   ("kind", "block").
3. **The scene reader reads a former key as its field**, quietly: a key naming no field is looked up in the
   type's former names before it is ignored. A section holding both the field's name and a former one
   keeps the current name's value and warns about the former line. The writer writes only current names,
   so a file is upgraded on its next save. A prefab is read by the same pass.
4. **Render keeps its words.** `walls` and `indoors` in `voe_render_light_blockers` and the shaders are
   render's own names for the masks; 3d puts a Direct blocker's bit in `walls` and a Fill one's in
   `indoors`. No render or shader change.

## Reasoning
- Keeping the field `kind` and naming it "Block" only on screen: the Inspector labels a field by its
  description's name and has no other label; a display name per field is a wider mechanism than this.
- Renaming the key and relying on the unsaid row: one row for every blocker cannot say which was a Wall.
- Rewriting old files by hand: a scene saved with blockers on this branch is the human's, not the repo's.
- Renaming render's masks too: dozens of lines across C and Slang, for words no person sees.

## Replaces
Nothing. Carries out 0352; extends 0324's reader fallbacks.
