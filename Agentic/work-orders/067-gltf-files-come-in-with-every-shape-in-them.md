# 067 — `.gltf` files come in, with every shape in them

## What
Import accepts a `.gltf` file as well as a `.glb`. A `.gltf` brings the `.bin` and image files it names
along with it into `Assets/`, keeping their folder layout. Data embedded in the file itself works too, and so
do names with spaces or characters outside English. Re-exporting or replacing any of those files updates the
model everywhere, as a `.glb` does today.

Every shape a glTF file can hold comes in at the right place, size and turn: triangles with or without an
index list, strips and fans, lines, line loops and strips, and points. Interleaved and sparse data, packed
(quantized) positions, normals and texture coordinates, and mirrored parts (negative scale) all work. A file
with several scenes shows its default one. A model with animation shows still in its rest pose, and one with
morph targets shows at its default weights. Cameras in the file are ignored. A node the file marks invisible
is not drawn.

A file that requires something 0.3 does not read is refused, and the Errors panel names the extension. An
optional extension the engine does not read is ignored, and the model still draws (0373).

## Why
0.3 starts with reading every sample model's file. Without that, nothing after it can be compared.

## Models
From https://github.com/KhronosGroup/glTF-Sample-Assets/tree/main/Models: Box, Box With Spaces, BoxInterleaved, Triangle, TriangleWithoutIndices, SimpleMeshes, SimpleSparseAccessor, MeshPrimitiveModes, PrimitiveModeNormalsTest, OrientationTest, NegativeScaleTest, MultipleScenes, Cameras, Unicode❤♻Test, Cube, Suzanne, BoomBoxWithAxes, Avocado, Duck, MorphPrimitivesTest, NodeVisibilityTest, XmpMetadataRoundedCube, BoxAnimated, CesiumMilkTruck, MeshoptCubeTest, BrainStem.

## How to test
1. Download Box, Box With Spaces, Triangle and Unicode❤♻Test. Import the `.gltf` from each one's `glTF`
   folder, and from `glTF-Embedded` where there is one. Each lands in Assets with its `.bin` and images
   beside it, and drags into a view.
2. Place SimpleMeshes, TriangleWithoutIndices, BoxInterleaved and SimpleSparseAccessor. Each looks like
   its own screenshot in its folder.
3. Place MeshPrimitiveModes. Every row of points, lines and triangles shows, as in its screenshot.
4. Place OrientationTest and NegativeScaleTest. Every part points and mirrors as in their screenshots,
   with no face turned inside out.
5. Import Avocado and Duck from their `glTF-Quantized` folders. They look the same as their plain `glTF`
   versions placed beside them.
6. Place MultipleScenes, Cameras, MorphPrimitivesTest, NodeVisibilityTest, BoxAnimated and CesiumMilkTruck.
   Each shows a still model matching its screenshot's first frame. Hidden nodes stay hidden.
7. Import BrainStem's `glTF-Meshopt` file. It is refused, and the Errors panel says it needs
   `KHR_meshopt_compression`, which comes in 072. Import MeshoptCubeTest's `glTF` file: it uses the
   same extension only optionally, so it comes in and draws.
8. Re-save Box's `.gltf` with a changed colour in a text editor. The placed box updates within a second.
9. Save the scene, close and reopen the editor. Everything is as it was. Press Play: the same in the game
   window.
