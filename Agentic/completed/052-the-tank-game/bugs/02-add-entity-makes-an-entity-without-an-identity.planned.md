# 02 — Add entity makes an entity without an identity

## Seen
"What happened to the identity component? When i click "add entity" its not there. We always want
identity component for editor entities."

## Expected
Decided: 0335. A new entity from Add entity has an identity component, with its name ("Entity", or
"Entity N" when taken) and its id, as 0300 and 0193 already said. The Inspector shows its Identity
section, with no Remove. The same holds for every other way an entity comes into the editor:
Duplicate, a placed prefab, a scene file opened, undo and redo. An entity in a scene file that has no
identity gets one when the file is read.

## How to reproduce
1. Open `examples/tank_game` in the editor.
2. Click Add entity.
3. Select the new entity and look in the Inspector: there is no Identity section.
