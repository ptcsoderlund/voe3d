# 0377 — The engine's own assets are made and managed in the Assets panel
date: 2026-10-07
by: tech-lead

## Decision
1. **Our own asset kinds.** Materials, shaders, landscapes and the kinds that follow (foliage sets
   and so on) are files of the project's own in `Assets/`, each kind with its own extension, written
   as text per 0236. A landscape is such an asset, placed into a scene, not part of the scene's text.
   Its heights and painted layers may be kept in outside formats (pictures) beside it, which 0236
   allows. Prefabs (0283) are one of these kinds already.
2. **Made from Create.** Right-clicking an empty part of the Assets panel opens Create, which makes
   a new asset of a kind in the folder shown, with its name ready to type. Create → Folder comes
   first (work order 081). Each kind adds itself to Create in the work order that brings it: Material
   in 084, Landscape in 082. A shader made in the editor waits for a work order of its own. 0.3's
   materials use the engine's shaders (0375 point 2).
3. **Managed in the panel.** A row's menu has Rename (also F2), Duplicate and Delete (also the Delete
   key). Dragging a row onto a folder, or onto Up, moves it. A rename or move is followed by every
   scene, prefab and asset of our own that names the file, so nothing breaks. A project's C code is
   never rewritten. Delete asks first, names what uses the file, and sends it to the desktop's trash.
   A taken or empty name is refused with a message and changes nothing.
4. **The road.** The panel work order goes before the hill's. 0376's 081–091 are renumbered 082–092
   in the same order, and the new 081 is "The Assets panel creates, renames and deletes".

## Reasoning
The sponsor's call (2026-10-07): "Go with A, we also introduce create -> folder. Custom assets will be
materials, shaders, landscape and so on." From 084 on, most of what the sponsor makes in the hill is an
asset of our own and not an import. One way to make and tidy them, before the first arrives, keeps
each later kind's work order small. Paths are how scenes name files (0277), so a rename that does not
follow them would break the hill.
- Folding the panel into 084 (materials): too big to test in one sitting.
- Waiting, with a bare New material button: renaming would break scenes by 089's forest.
- The landscape as part of the scene's text: its heights and layers are large, and one hill may be
  shared by several scenes (day, night).

## Replaces
nothing. Extends 0270 and 0283. Amends 0376's numbering and its "the hill is a terrain the editor owns"
to a landscape asset.
