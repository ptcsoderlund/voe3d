# 04 — Render takes BC blocks and counts what material textures take
folder: render
after: none
decisions: 0168, 0359, 0470, 0477

## Change
0477 points 1, 6 and 7 in `render`.

- `render/include/render/device.h`, textures section:
  - `voe_render_blocks`: `VOE_RENDER_BLOCKS_BC7`, `VOE_RENDER_BLOCKS_BC5`.
  - `[[nodiscard]] bool voe_render_texture_create_blocks(voe_render_device *device,
    voe_render_texture_kind kind, voe_render_blocks blocks, uint32_t width, uint32_t height,
    uint32_t level_count, const uint8_t *bytes, voe_render_texture *out, voe_base_error *error)`.
    It works as `_create_levels` does: always SMOOTH, the levels as given, the bytes copied, the GPU
    waited for. The bytes are 16 a 4×4 block, each level ⌈w/4⌉ × ⌈h/4⌉ blocks, back to back. BC7 is
    SRGB for COLOUR and UNORM for DATA; BC5 is UNORM for either kind. Without BC on the card it fails
    REFUSED with one line.
  - `voe_render_texture_memory { uint64_t bytes; uint32_t count; uint32_t compressed; }` and
    `voe_render_texture_memory voe_render_texture_memory_smooth(const voe_render_device *device)`:
    the SMOOTH textures alive, their images' memory as the card sized it, how many there are, and how
    many are blocks. On the owner's thread.
- `render/src/device_internal.h` — a texture slot keeps its image's memory size and whether it holds
  blocks; the device keeps whether BC is on.
- `render/src/device.c` — `textureCompressionBC` queried and turned on when the card has it, as the
  features beside it are; one line when it has not.
- `render/src/texture.c` — a slot's size is taken where its memory is bound and cleared on destroy.
  The file is near 800 lines: anything more than a few lines goes in the new files.
- `render/src/texture_blocks.c` (new) — `_create_blocks` through texture.c's upload, as
  `texture_given.c` does: one region per level, each level's extent its own size, not rounded to
  blocks.
- `render/src/texture_memory.c` (new) — `_memory_smooth`, one walk of the slot table.
- `render/tests/texture_blocks.c` (new; headless, skips without a card as `mips.c` does):
  - `bc7_is_drawn`: a 16×16 chain of hand-built mode-6 blocks of one opaque colour, as COLOUR, drawn
    as `mips.c`'s `given_levels_are_drawn` draws, reads what an RGBA8 texture of that colour reads,
    within 2;
  - `bc5_is_made`: a BC5 DATA chain is made;
  - `memory_counts`: an RGBA8 SMOOTH texture and a BC7 one each raise `count` and `bytes`, the BC7
    one `compressed` too; destroying both gives the first reading back.
- `render/src/src.md`, `render/tests/tests.md`, `render/include/render/render.md` — entries.

## Done when
The `render/texture_blocks` test passes.
