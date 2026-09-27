# 0277 — A model is a path on a thing, drawn from a store its program fills
date: 2026-09-27
by: planner

## Decision
For 036, filling in what 0270 and the feature leave to the planner:

1. **The component is a path.** `3d/model_component.h`: `voe_3d_model`, one CHAR field `path` of
   128 bytes, relative to the project folder with `/` separators (`Assets/hull.glb`). Empty draws
   nothing, silently. It needs a transform, sits at `Rendering / Model` in Add component, and its
   intent is its replace, drained by `voe_3d_model_system_run` in `game`'s world step.
2. **Loaded models live in a store, keyed by path.** `3d/models.h`: `voe_3d_models`, at most
   `VOE_3D_MODELS` (128) entries, each loaded or failed. Loading bakes each node's world matrix
   into its vertices (normals by the normal matrix) and merges the primitives into one part per
   material, at most `VOE_3D_MODEL_PARTS` (16), more refused as unsupported. A part is a geometry
   and a `voe_3d_material`. The whole model is also one `voe_3d_shape_geometry` on the CPU, edges
   welded, for picking and the outline. Device room: `VOE_3D_MODELS_VERTICES` 2²¹,
   `_INDICES` 3·2²¹, `_GEOMETRIES` 512, `_SHADINGS` 512.
3. **What draws walks the rows through the store it is handed**: `voe_3d_frame.models`,
   `voe_3d_outlined.models` and a parameter of `voe_3d_pick`, NULL for none. A part draws in the
   world layer, white in its object record, grouped and cast by its material as a mesh would be.
4. **Files are read by `game/models.h`**, which may name `platform`: a path a row names and the
   store lacks is read from a folder and loaded; a file that will not read or parse is kept as a
   failed entry, said once on stderr and handed back to the caller. The editor reads against the
   project folder, the game against its program's folder as 0266 point 3 does, and
   `cmake/game.cmake` copies and installs every project `.glb` as it does `.wav`.
5. **A re-export is found by the file's stamp** (`platform`: modification time and size). The
   editor looks at every entry's stamp once a second; a changed one is re-read. Success replaces
   the parts and frees the old ones (0278); failure keeps the old parts and says so; a failed
   entry is tried again when its stamp changes. The store is emptied when a different project is
   in place.
6. **The Assets panel** sits under the Scene list in the left column, held at 70 mm, not dragged
   in 036. It lists `<project>/Assets/` — folders first, then files, by name, hidden ones left out
   — enters a folder row, goes Up but never above `Assets/`, and is listed again once a second. A
   `.glb` row is a model; other files are listed only. With no `Assets/` it says so.
7. **Import shows the editor's own browser** (0164: no system dialog) in an IMPORT mode listing
   folders and `.glb` files. Choosing one copies it, read whole and written atomically, into the
   folder the panel shows, `Assets/` made if missing; a file of that name is overwritten, which the
   stamp then reloads everywhere.
8. **Dragging**: press a model row and release over a scene view to place a new thing, named
   after the file without `.glb`, with that model, at the first thing the view's ray meets, else
   where it crosses y = 0 in front of the eye, else 10 m along it. Release over the Inspector while
   the selected thing has a model swaps its path. Either is one undo step and marks unsaved;
   elsewhere nothing happens. The path stays typeable in the Inspector.
9. **A broken file** puts `Could not read <path>` and the reason's category in the session notice;
   the thing draws as nothing.

## Reasoning
A path in a CHAR field is what the Inspector, the scene text, undo and the cook already carry
(0266), so a model survives save, undo and ship with no new format (0236). Rows that point into a
store keep one upload per file however many things wear it, which is what makes a re-export
update every placed copy at once. Baking nodes and merging by material keeps a Blender export with
baked textures to one draw. Reading files in `game` keeps `3d` off `platform` (no new edge) and
gives the editor and the game one loader. Rejected: one entity per primitive (needs parenting,
milestone 4, and would leak parts into the scene file); a mesh component holding a model (one
geometry and one material per entity); a system dialog (0164); a file watcher API per platform
(a stamp once a second is enough for a person exporting).

## Replaces
nothing. Fills in 0270.
