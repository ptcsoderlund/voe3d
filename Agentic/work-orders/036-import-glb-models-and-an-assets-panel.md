# 036 — Import .glb models, and an Assets panel

## What
The editor has an Assets panel that shows the project's `Assets/` folder and its subfolders. Import
in the panel lets you pick a `.glb` file anywhere on the disk and copies it into the folder the
panel shows. A `.glb` copied into `Assets/` outside the editor shows up in the panel by itself.
Dragging a model from the panel into a scene view places a new thing where it lands, and that thing
draws the model. Models draw with their own baked textures (colour, and the other channels Blender
bakes into the file: normal, roughness and metal, and emission) and are lit and cast and receive
shadows the same way the built-in shapes do. The Inspector shows which model a thing draws, and it
can be swapped for another. Clicking a model in a view selects it by its own shape. Re-exporting a
`.glb` from Blender over the same file updates it everywhere it is placed, with no restart. A model
too broken to read is named in the editor, and the thing draws as nothing. The game shows the same
models when played and shipped. This work order also starts `examples/tank_game/` (0272), and the
sponsor's first models are imported into it.

## Why
Milestone 3 of 0268, with the panel from 0270. The tank game is built from the sponsor's own
models, and the sponsor will import dozens of them.

## How to test
Have a tank hull `.glb` from Blender ready, with baked textures, and a second model of any kind.
1. Open `examples/tank_game` in the editor. The Assets panel is there, and it shows `Assets/`.
2. Import the hull. It shows in the panel. It is also in `examples/tank_game/Assets/` on disk.
3. Copy the second model into `Assets/` with the file manager. It shows in the panel without
   anything else done.
4. Drag the hull into a view. It stands where you dropped it, with its textures, lit by the sun,
   casting a shadow onto the ground and taking shadows from other things.
5. Click the hull in the view. It is selected and outlined, and the Inspector shows its model. Swap
   the model for the second one, then undo.
6. Place three more hulls. Change the hull's colour in Blender and export over the same file. All
   four change in the editor within a moment.
7. Save, close, reopen. The hulls are all there.
8. Press Play. The hulls are in the game, lit and shadowed the same way. Ship. The shipped game shows
   them too.
9. Put a text file renamed to `broken.glb` into `Assets/` and place it. The editor says it could not
   read the file, and nothing crashes.
