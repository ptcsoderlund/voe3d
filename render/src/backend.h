// The three things that differ between the two window systems, and the whole of
// what render knows about there being two. Everything else in this folder is
// written once.
//
//   - what the operating system calls the Vulkan loader,
//   - which instance extension creates a surface on it,
//   - and how to make that surface out of the two handles platform hands over.
//
// One implementation per platform, in backend_wayland.c and backend_win32.c, and
// the build compiles exactly one of them. There is no #ifdef anywhere else in
// render, and there should not be one: a third thing that differs gets a fourth
// function here rather than a preprocessor branch at its call site.
#pragma once

#include "loader.h"

#include <platform/window.h>

// "libvulkan.so.1" or "vulkan-1.dll". The name is here rather than in platform
// because it is knowledge about Vulkan, and platform does not know what Vulkan
// is.
const char *voe_render_backend_library(void);

// The window-system half of the surface extensions. VK_KHR_surface is the other
// half and is the same everywhere, so it is not here.
const char *voe_render_backend_extension(void);

// VK_NULL_HANDLE if the surface could not be created, with a line at the site
// saying what the driver said.
[[nodiscard]] VkSurfaceKHR voe_render_backend_surface_new(VkInstance instance,
							  voe_platform_native native);
