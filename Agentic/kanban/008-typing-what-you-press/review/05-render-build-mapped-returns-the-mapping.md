# 05 — render: build_mapped returns the mapping instead of writing through a double pointer
folder: render
decisions: 0168

## Change

`build_mapped` in `src/descriptors.c` line 210 breaks rule 6 (no `**`, ADR-0043):

```c
static bool build_mapped(voe_render_device *device,
			 struct voe_render_buffer *buffer, void **mapped,
			 VkDeviceSize size, VkBufferUsageFlags usage)
```

Make it return the mapping instead:

```c
[[nodiscard]] static void *build_mapped(voe_render_device *device,
					struct voe_render_buffer *buffer,
					VkDeviceSize size,
					VkBufferUsageFlags usage)
```

`NULL` is the one way it fails (rule 13); a successful mapping is never `NULL`, so the two cannot be confused —
say that in the function's own comment. Update the three calls at lines 333, 339 and 343, each of which becomes
an assignment to the `*_mapped` field it already fills, tested for `NULL` where it tested the `bool`. The
function is `static` and no header declares it, so nothing outside this file changes and no folder downstream is
touched. `render/vulkan/` is not touched.

## Done when

`grep -nE '\*[[:space:]]*\*' render/src/descriptors.c` prints nothing outside comments, and
`cmake --preset debug && cmake --build --preset debug --target voe_render && ctest --test-dir build/debug -R '^render/'`
exits 0. If `slangc` is not found, it is installed at `~/voe3d-scratch/tools/slang/bin/` — put that on PATH for
the session; the tool is the programmer's (ADR-0021), so do not teach `check.cmake` or the build to look for it.
