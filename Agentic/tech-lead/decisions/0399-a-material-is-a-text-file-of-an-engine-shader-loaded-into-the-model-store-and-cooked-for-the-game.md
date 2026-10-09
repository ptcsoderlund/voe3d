# 0399 — A material is a text file of an engine shader, loaded into the model store and cooked for the game
date: 2026-10-09
by: planner

## Decision
For 085, carrying out 0189, 0375 and 0377 point 2 for materials:
1. **The file.** `<name>.material` is sectioned text (`assets/sectioned.h`), one `[Material]` section:
   `shader` (`lit` or `unlit`), `colour` (three linear floats), `roughness`, `metal`, `repeat`, and
   `colour_map`, `normal_map`, `roughness_map` as project-relative paths, empty for none. A missing key
   reads its default: lit, colour 1 1 1, roughness 0.5, metal 0, repeat 1, no maps. Parsed and written
   in `assets/material.h`. Create → Material writes the defaults.
2. **The roughness map fills the metal-roughness slot**: roughness from green, metal from blue times
   Metal, as glTF reads it. A greyscale roughness map with Metal 0 is exact; no shader flag is added.
3. **Repeat is in the shading record** (the word `reserved_c`, a float): the mesh's texture
   coordinates are multiplied by it before any map is read, before the base colour rectangle; 0 reads as
   1, so every record built before is unchanged.
4. **Live edits**: `render` writes a shading record inside a frame, before its first pass, recorded as
   a buffer update between barriers as heights are (0396 point 5). Factors and repeat go live the frame
   after a drag moves them; a changed map reloads the material between frames.
5. **The store**: a material is an entry of 3d's model store keyed by its path, one part with no
   geometry holding its record, its blended twin and its own three textures (colour COLOUR, the others
   DATA, all mipped). Two materials naming one picture upload it twice; deduplication waits for need.
6. **Given to things**: the shape row gains `material`, one 128-byte path; the model row gains
   `materials`, eight 128-byte paths for its first eight parts in bake order. Empty, or a path the store
   holds no loaded material for, draws as before. A shape wearing a material draws with colour white.
7. **One table, two sources**: `game/materials.h` is a table of path and values. The game's is cooked
   by the editor's game tree into `materials.c` (floats in hex), so the game reads no project text
   (0236); the editor's is read from every `.material` under `Assets/`, read again on project open and
   after every Assets command. `game/models.h` loads every material path a row names from the table,
   its maps read from the folder as models are.
8. **The editor**: a pressed `.material` row opens it in the Inspector in place of the entity until an
   entity is selected; shader as Lit / Unlit buttons, Colour as a swatch with the picker, Roughness and
   Metal sliders 0–1, Repeat a number box 0.01–1000, and Colour, Normal and Roughness map slots, each a
   file name with ×, filled by dropping a picture on it. A `.material` dropped on the Inspector gives
   it to the selected shape, or to the model's materials row under the pointer. Import takes `.png`,
   `.jpg` and `.jpeg` as well as `.glb`.
9. **Saved and undone**: a material edit is written to its file when the editor comes to rest (0204),
   not by Save and not marking the scene unsaved. That rest pushes an undo state carrying the path and
   the values before and after beside the scene text, as a stroke does (0379 point 5); a step writes
   the values into the table, the file and the store. `.material` joins the follow list (0378 point
   1), so renames of it and of its maps follow; the open material follows a rename and closes on delete.

## Reasoning
- Written at rest rather than at Save: Duplicate copies the file on disk, so a duplicated material must
  already hold its edits (feature 085, How to test 5); a landscape's megabytes are why it waits for Save,
  and a material is a few hundred bytes.
- The model store rather than a store of its own: reload, rename, clear and the faded twin already live
  there, and a model's part and a material are both "a record a draw wears".
- A recreated record per drag frame: creating waits for the card to go idle, a hitch 0386 forbids.
- A roughness flag in the shader: one more branch for a case Metal 0 already makes exact.
- Material paths as a component of their own: a model's parts are not entities, and the shape's and
  model's rows already carry paths through text, undo, follow and cook.

## Replaces
nothing. Carries out 0377 point 2 for materials (renumbered 084 → 085 by 0392).
