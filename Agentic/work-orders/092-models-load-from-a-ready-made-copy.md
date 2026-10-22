# 092 — Models load from a ready-made copy

## What
The first time the editor meets a model, it makes a ready-to-draw copy of it in the project's `Cache/`: the
meshes, the LODs and the pictures with their mips, laid out as they sit in memory. It does this under the
splash. It also makes the copy again when the `.glb` or a picture changes. After
that, opening the project, loading a scene and pressing Play read these copies straight in, with nothing
decoded or rebuilt, and the shipped game uses the same copies. The splash's last line says how long the load
took. Deleting `Cache/` only means the copies are made again (0454).

## Why
Loading models is slow, and the forest has brought many more, with grass next. A copy made once and read raw costs
almost nothing to load after the first time, in the editor, in Play and in the shipped game.

## How to test
1. Open the hill and note the load time on the splash. Press Play and note that load time too.
2. Close the editor, delete the project's `Cache/` folder and open the hill again. The splash says it is
   preparing the models and finishes faster than step 1 did.
3. Close and open again. The load is much faster than step 1, with nothing being prepared. Press Play: that
   load is also much faster than in step 1.
4. Re-export one model from Blender with a visible change. Within a second or so the change shows. Close and
   open: only that model is prepared again.
5. Run Import textures on a model at a smaller size. The model shows the new pictures, and the next open
   prepares only that model.
6. Ship the project and run it. It starts without preparing anything and looks as it does in Play.
