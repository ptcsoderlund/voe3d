# vulkan

The Khronos Vulkan headers, vendored unmodified from Vulkan-Headers 1.4.341
(`VK_HEADER_VERSION` 341). Declarations only — nothing in this repository links
against Vulkan, and every entry point is resolved at run time through
`vkGetInstanceProcAddr`. Not a place to edit: a fix goes upstream and comes back
as a new copy.

Upstream keeps `vk_video/` beside `vulkan/` rather than inside it. Here it is
inside, because `vulkan_core.h` includes it by a quoted relative path and that
resolves against this directory — which is what lets `render` include these
headers with no include path of its own and no CMake edit.

- `vulkan_core.h` — the API.
- `vk_platform.h` — the calling conventions and integer types `vulkan_core.h`
  needs.
- `vulkan_wayland.h` — `VK_KHR_wayland_surface`.
- `vulkan_win32.h` — `VK_KHR_win32_surface`, and the Windows types it expects
  someone else to have declared.
- `vk_video/` — the video-codec headers `vulkan_core.h` includes. Nothing here
  uses them; they are carried because that header will not compile without them.
