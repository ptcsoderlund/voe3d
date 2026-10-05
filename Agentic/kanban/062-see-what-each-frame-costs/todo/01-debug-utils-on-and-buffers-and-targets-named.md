# 01 — Debug-utils on whenever offered; buffers and target images named
folder: render
after: none
decisions: 0168, 0358, 0367

## Change
- `render/src/loader.h`: three instance-level entries resolved from the instance, NULL when
  `VK_EXT_debug_utils` was not enabled: `set_debug_utils_object_name`, `cmd_begin_debug_utils_label`,
  `cmd_end_debug_utils_label`. Fix the comment over the two messenger entries: the extension is now
  enabled apart from the layer.
- `render/src/loader.c`: resolve the three where the messenger pair is resolved.
- `render/src/instance.c`: enable `VK_EXT_debug_utils` whenever the instance offers it, in every build
  (0367 point 3); the layer and the messenger stay debug-only and still need it. Header: the
  extension is asked for apart from validation, and why (a capture tool reads the names).
- New `render/src/debug_names.c`, declared in `render/src/device_calls.h` under its own group:
  `void voe_render_debug_name(const voe_render_device *device, VkObjectType type, uint64_t handle,
  const char *name)`, and `voe_render_debug_label_begin(VkCommandBuffer commands, const char *name)` /
  `voe_render_debug_label_end(VkCommandBuffer commands)`. Each does nothing when its entry is NULL.
  Header: why one helper, and that names cost nothing without the extension.
- `render/src/buffer.c` and its declaration in `device_calls.h`: `voe_render_buffer_build` gains a
  last `const char *name` and names the buffer. Every caller passes a readable name: `buffer.c`,
  `descriptors.c`, `geometry.c`, `shading.c`, `target_read.c`, `texture.c`, `bounce_relight.c`
  (e.g. "vertex pool", "slot 1 uniforms", "staging").
- `render/src/target.c`: `voe_render_target_image_build` names its image and view with its existing
  `what`; `voe_render_target_depth_copy_build` likewise.
- `render/src/src.md`: an entry for `debug_names.c`; fix the `instance.c` and `buffer.c` entries.

## Done when
- The folder's build and tests pass.
- `grep -c voe_render_debug_name render/src/buffer.c render/src/target.c` prints at least 1 for each.
