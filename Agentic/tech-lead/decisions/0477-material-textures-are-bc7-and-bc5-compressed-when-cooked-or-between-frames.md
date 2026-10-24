# 0477 — Material textures are BC7 and BC5, compressed when cooked or between frames
date: 2026-10-24
by: planner

## Decision
For 096, how material textures become GPU-compressed.

1. **Formats.** Base colour and emission maps are BC7 read as sRGB; ORM and every other data map BC7
   read as numbers; normal maps BC5, x and y only. Every normal map, compressed or not, is read with
   its z made again from x and y, so one that waits for compression draws the same. One byte a texel,
   a quarter of RGBA8, mips included.
2. **The encoder is ours** (ADR-0023), in `assets`, on the CPU, one 4×4 block at a time. BC7 uses mode
   6 alone: one pair of endpoints along the block's main axis, 16 levels, alpha with the colour. More
   modes (partitions) come only if the sponsor sees blocks. BC5 is two BC4 blocks. Nothing is encoded
   on the GPU.
3. **When.** A copy is compressed wherever 0470 point 4 cooks one: under a splash, in Play's and Ship's
   cook and the game's first load. The splash shows "Preparing models" for a model and "Compressing
   textures" done/total for the materials' maps. Outside a splash, in the editor, a map with no fresh
   copy is mipped on the CPU, uploaded uncompressed, and queued. This covers a changed map, a material
   newly named, and a map changed in the Inspector. Between frames, on the main thread, the queue
   encodes blocks for at most 2 ms a frame. Then it writes the map's copy, and every material naming
   that map loads again from it. This amends 0470 point 4: the queue writes a map's copy outside a
   splash. A re-exported `.glb`'s pictures stay uncompressed until its copy is cooked at the next open,
   Play or Ship. A model with no fresh copy placed outside a splash cooks inline, as it does today.
4. **One thread.** No worker compresses (0456, 0386). The queue holds at most 16 maps. One past that
   is dropped with a line, and that map waits for the next open.
5. **The editor watches the maps** the project's materials name, once a second, as it watches models
   (0277 point 5). A map whose stamp changed loads its materials again.
6. **The card.** `render` turns on `textureCompressionBC` when the card has it. Every card in 0318's
   range does. Without it, a compressed upload fails with one line, and the material fails to load.
7. **The Frame panel** shows the video memory of the material textures, the SMOOTH ones, as the card
   sized them, and how many of them are compressed. Landscape heights and paint, targets, shadow maps,
   glyphs and sheets are not counted.
8. **The copies' versions rise**, the cooked picture's and the model's, so old copies are stale and are
   made again.
9. The glyph atlas, sprite sheets, emitter textures, picture entries and the splash are not compressed
   (0359, 0470 point 9).

## Reasoning
- BC7 is the quarter the work order asks for. It keeps alpha for cut-outs and has an sRGB form. BC1 is
  half that size, but its 5:6:5 colour bands and it has no proper alpha.
- BC5 is what engines use for normals: each channel gets its own eight levels in a block, so slopes stay
  smooth. BC7 would mix them with a third channel nobody needs.
- Mode 6 alone is the smallest encoder that looks the same at a glance. Partitioned modes can come
  later behind the same call.
- Pacing on the main thread keeps 0456's one thread. A worker for compression would be a new threading
  decision for a few seconds of waiting, which the uncompressed upload already hides.
- A GPU compute encoder is rejected. It needs a readback for the cache, a large shader, and a device in
  the cook before Play, which has none.
- Reloading the materials from the written copy reuses the load path, so the store needs no call that
  swaps a texture in place.

## Replaces
Amends 0470 point 4 (the queue writes a map's copy outside a splash) and 0454 point 1 ("RGBA8 until
compressed"). Carries out 0359's "compression later".
