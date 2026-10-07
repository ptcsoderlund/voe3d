# 081 — The Assets panel creates, renames and deletes

## What
The Assets panel looks after the project's files, so I never need the file manager. Right-clicking an
empty part of the panel opens a menu with Create, and Create → Folder makes a new folder in the folder
shown, with its name ready to type. Later asset kinds (material, landscape, shader and so on) are added
to the same Create menu. Right-clicking a row opens Rename, Duplicate and Delete. F2 renames the
selected row and the Delete key deletes it. Dragging a row onto a folder row, or onto Up, moves it
there. When I rename or move a model, prefab, picture or sound, every scene and prefab that uses it
follows, so nothing breaks. Names in my own C code are not changed. Delete asks first, and if anything
uses the file it names what does. A deleted file goes to the desktop's trash. A name that is already
taken, or empty, is refused with a message, and nothing changes.

## Why
0377: our own assets (materials, landscapes, shaders and more) are made and managed in the Assets
panel. That needs a panel that can make, name and tidy files before the first of them arrives in 084.

## How to test
1. Open the tank game. Right-click an empty part of the Assets panel and choose Create → Folder. A
   new folder appears with its name ready to type. Name it `Rocks` and press Enter.
2. Make a folder inside `Rocks` the same way. Go into it, then Up.
3. Try to make a second folder named `Rocks`, then one with an empty name. Each is refused with a
   message, and nothing changes.
4. Select a model that the level uses and press F2. Rename it. The level still shows it. Save, close
   and reopen: it is still there, and the file has its new name in the file manager.
5. Drag a prefab that the level uses onto `Rocks`. It moves there, and its copies in the level stay.
   Press Play: the game runs as before.
6. Duplicate a prefab. A copy appears beside it with a new name, and the original is unchanged.
7. Delete a model that the level uses. A question names the level as using it. Cancel, and nothing
   happens. Delete an unused folder instead and confirm: it goes, and it is in the desktop's trash.
8. Right-click a row and choose Rename, Duplicate and Delete from the menu. Each does the same as above.
