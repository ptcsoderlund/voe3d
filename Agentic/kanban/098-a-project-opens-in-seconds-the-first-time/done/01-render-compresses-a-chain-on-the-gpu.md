# 01 — render compresses a chain on the GPU
folder: render
after: none
decisions: 0168, 0484, 0486, 0477, 0370

## Change
0486 points 1–5. The folder may not include `assets`; the CPU encoder is read as a reference only.

- `render/include/render/device.h` — beside `voe_render_texture_create_blocks`, a section:
  - `typedef struct voe_render_compressing voe_render_compressing;`
  - `[[nodiscard]] voe_render_compressing *voe_render_compress_start(voe_render_device *device,
    voe_render_blocks blocks, uint32_t width, uint32_t height, uint32_t level_count,
    const uint8_t *rgba, voe_base_error *error)` — the chain laid out as `_create_levels` takes
    it, copied in, the work submitted, no wait; NULL with `error` when memory or the pipeline
    will not make.
  - `bool voe_render_compress_ready(voe_render_device *device, const voe_render_compressing *job)`
    — no wait.
  - `void voe_render_compress_finish(voe_render_device *device, voe_render_compressing *job,
    uint8_t *out)` — waits if not ready, copies every level's blocks into `out` laid out as
    `_create_blocks` takes them (NULL drops them), frees the job.
  - Header points: what the encoder searches (0486 point 3), the edge rule, that a blocking
    caller is start then finish, the memory a job holds until finish, that a device closes with
    no job live (asserted). The threading paragraph at the file's top adds these three calls to
    what the one other thread may call.
- `render/shaders/compress.slang` — new: entry points `bc7` and `bc5`, one thread a block, the
  level's sides and its first texel and block in a push block; bytes as the formats store them.
  Port `assets/src/image_bc7.c` and `assets/src/image_bc5.c` (read their headers and bodies)
  block for block. `render/shaders/shaders.md` gets its entry.
- `render/src/compress.c` — new: the set layout, pool and two pipelines built under the guard at
  the first start, as `render/src/bounce_relight_build.c` builds its compute pipelines (read its
  header and its build); per job one host-visible input buffer, one host-visible output buffer,
  a descriptor set, a command buffer of one dispatch a level and a fence; the guard held to
  record and submit and to free, the fence waited outside it. Its header says why buffers and
  not an image.
- `render/src/device_internal.h`, `device_calls.h`, `device_parts.h` — the device's compress
  record (pipelines, layout, pool, live job count) and the teardown call `device.c`'s close
  makes; `render/src/device.c` — that call at close.
- `render/src/src.md`, `render/render.md`, `render/include/render/render.md` — `compress.c`'s
  entry and the device.h line naming the compress calls.

## Done when
`render/tests/compress.c` (new, headless, entry in `render/tests/tests.md`) passes. It draws each
result through `voe_render_texture_create_blocks` beside its RGBA8 source, read back as
`render/tests/texture_blocks.c` does, and checks:
- a smooth colour gradient and a noise picture, 64×64 with a full chain, as BC7 within the bounds
  `assets/tests/image_bc7.c` holds the CPU encoder to;
- a normal map's x and y as BC5 within the bounds `assets/tests/image_blocks.c` holds it to;
- a 13×9 picture's edge blocks repeat their edge texels;
- two jobs started together are both ready after a bounded wait, and one dropped with NULL leaves
  the other's blocks right.
