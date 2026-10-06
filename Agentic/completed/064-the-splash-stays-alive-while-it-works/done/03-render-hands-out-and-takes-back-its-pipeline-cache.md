# 03 — The device's pipeline cache as bytes out and in
folder: render
after: 02
decisions: 0168, 0362, 0370

## Change
- `render/include/render/device.h`, after prepare:
  - `[[nodiscard]] bool voe_render_device_cache_seed(voe_render_device *device, const void *bytes,
    size_t size)`: before the first prepare step only (assert); true when the bytes are this
    engine's on this card and the cache now holds them; false, starting empty, on any mismatch.
  - `const void *voe_render_device_cache_bytes(voe_render_device *device, voe_base_arena *arena,
    size_t *size)`: the cache behind render's header, pushed into arena; NULL when the driver will
    not hand it out.
  Their comment says what the header holds and that a stale cache is never an error (0370 point 6).
- `render/src/device_internal.h`, `render/src/device.c`: the device keeps a `VkPipelineCache`, made
  empty at open and destroyed at close. Add the cache entry points to the loader table in
  `render/src/loader.c` if missing (read `render/src/loader.h`'s header first).
- `render/src/pipeline_cache.c` (new): seed and bytes. The header: magic, a version, the card's
  vendor id, device id and `pipelineCacheUUID`, a 64-bit hash of every shader the device embeds
  (find them where `pipeline.c` and `element.c` `#embed` them), the payload's size and its
  checksum; any field off, or a short buffer, is a mismatch.
- `render/src/pipeline.c`: every pipeline build passes the device's cache.
- `render/src/src.md`: entry for `pipeline_cache.c`; the `pipeline.c` entry mentions the cache.
- `render/tests/pipeline_cache.c` (new): device A prepares and hands out bytes; headless device B
  seeds them (true) and prepares; device C seeds them with one payload byte flipped (false) and
  still prepares; a 10-byte buffer is false; NULL/0 is false and harmless. Entry in `tests/tests.md`.

## Done when
`ctest --test-dir build/debug -R '^render/pipeline_cache$'` passes.
