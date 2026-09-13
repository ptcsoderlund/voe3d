// The Wayland backend. See backend.h for what a backend is.
//
// HOW THIS NAMES OS TYPES WITHOUT INCLUDING AN OS HEADER — the card's trap, and
// on this platform there is nothing to it. VkWaylandSurfaceCreateInfoKHR has two
// fields spelled `struct wl_display *` and `struct wl_surface *`, and C declares
// a struct tag it has not seen before on the spot: an incomplete type is all a
// pointer needs. vulkan_wayland.h therefore compiles with no wayland-client.h
// behind it, and the handles platform hands over as uintptr_t cast straight into
// those fields. The Windows side of this is the one that has real work in it.
//
// The loader's file name lives here too. It is Linux knowledge rather than
// Wayland knowledge, and this file is where the build puts Linux-only code —
// see the note in platform/src/library_wayland.c about the suffix.
//
// vkCreateWaylandSurfaceKHR is resolved here rather than kept in the table in
// loader.h, because its type does not exist on the other platform. It is the one
// entry point that could not be in the shared table, and it is still render's
// and still resolved through vkGetInstanceProcAddr.
#include "backend.h"

#include "../vulkan/vulkan_wayland.h"

#include <base/report.h>

const char *voe_render_backend_library(void)
{
	return "libvulkan.so.1";
}

const char *voe_render_backend_extension(void)
{
	return VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME;
}

VkSurfaceKHR voe_render_backend_surface_new(VkInstance instance,
					    voe_platform_native native)
{
	PFN_vkCreateWaylandSurfaceKHR create;
	VkWaylandSurfaceCreateInfoKHR info = {
		.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
		.display = (struct wl_display *)native.context,
		.surface = (struct wl_surface *)native.window,
	};
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	VkResult result;

	create = (PFN_vkCreateWaylandSurfaceKHR)
		voe_render_vk.get_instance_proc_addr(instance,
						     "vkCreateWaylandSurfaceKHR");
	if (create == NULL) {
		VOE_BASE_ERROR("render",
			       "the instance has no vkCreateWaylandSurfaceKHR");
		return VK_NULL_HANDLE;
	}

	result = create(instance, &info, NULL, &surface);
	if (result != VK_SUCCESS) {
		VOE_BASE_ERROR("render",
			       "vkCreateWaylandSurfaceKHR failed (VkResult %d)",
			       (int)result);
		return VK_NULL_HANDLE;
	}

	return surface;
}
