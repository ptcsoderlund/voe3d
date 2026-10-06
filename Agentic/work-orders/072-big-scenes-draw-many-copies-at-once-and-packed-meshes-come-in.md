# 072 — Big scenes draw many copies at once, and packed meshes come in

## What
A model that places the same mesh many times, whether through the file's own instancing or through many
nodes, is drawn as a handful of draws instead of one per copy. The frame breakdown shows how many draws the
frame has. A model packed with meshopt compression imports and looks the same as its plain version.
A scene with thousands of objects stays smooth to fly through in the editor, and the Scene list stays
responsive while it holds it.

## Why
Instancing and packed meshes are exactly what the GPU does in hardware, and big scenes need them before 0.3's showcase models.

## Models
From https://github.com/KhronosGroup/glTF-Sample-Assets/tree/main/Models: SimpleInstancing, NodePerformanceTest, VirtualCity, Sponza, MeshoptCubeTest, BrainStem.

## How to test
1. Place SimpleInstancing. It matches its screenshot.
2. Place NodePerformanceTest. Open the frame breakdown: the draw count is far below its number of nodes,
   and the view flies smoothly.
3. Place VirtualCity and Sponza and fly through both. It is smooth, and the draw count stays low.
4. Import MeshoptCubeTest's `glTF-Meshopt` file and BrainStem's `glTF-Meshopt` file. Both come in and look
   the same as their plain `glTF` versions placed beside them, BrainStem still in its rest pose.
5. Scroll the Scene list and select things in it and in the view. The editor does not stall.
6. Press Play: the same in the game window.
