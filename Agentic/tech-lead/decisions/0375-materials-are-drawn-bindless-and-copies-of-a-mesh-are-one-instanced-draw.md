# 0375 — Materials are drawn bindless, and copies of a mesh are one instanced draw
date: 2026-10-07
by: tech-lead

## Decision
1. **Bindless.** Every texture a material reads sits in the one texture array the shaders index
   (0278's slots, grown when a scene needs more). A material is a record in a buffer that names its
   textures by slot and holds its values. A draw binds nothing per material. There are no texture
   atlases.
2. **Materials share shaders.** A material is an instance of a shader (0189). An imported model's
   materials become instances of the engine's shaders, not shaders of their own. So models from
   different files that share a shader draw with one pipeline and change only the record they read.
3. **Instancing.** The same mesh, at the same LOD level and with the same shader, placed many times
   (trees, grass, rocks, a file's own instancing) is one instanced draw. Each copy's place and
   material are read by its instance index.
4. **Fewer draws than that waits.** Indirect and multi-draw, culling on the GPU, and meshes merged by
   the cooker come when 0358's frame breakdown names draws as the cost.
5. **Play shows exactly what the shipped game shows.** Whatever the cooker works out once (LOD
   levels, compressed textures, any merging) is kept in the project's cache and used by Play too. The
   editor's own views aim for the same picture. While such work is still running they may show a
   stand-in, but never a different look once it is done.

## Reasoning
The sponsor's call (2026-10-07), for 0374's hill: thousands of trees and grass blades of a few kinds,
and Blender models that each bring their own materials. The engine is already most of the way to
bindless. Shaders read a 1024-entry texture array by index, and each object reads its own record from a
buffer by number (`render/src/draw.c`). Descriptor indexing is core in Vulkan 1.2, on every card in
0318's range.
- Texture atlases: tiling ground textures cannot repeat inside one, mips bleed between neighbours, and
  bindless makes them unnecessary.
- Texture arrays grouped by size: every layer must share a size and format, which bindless does not ask.
- Each imported material its own shader: nothing batches across files.
- Indirect drawing and GPU culling now: before the breakdown shows draws cost anything.
- Editor views drawn differently from the game: "looks fine in the editor" would stop meaning anything.

## Replaces
nothing. Extends 0189 and 0278.
