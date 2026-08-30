// Opening the Vulkan loader and filling the table. See loader.h for why the
// table exists and what the rules on it are.
//
// Three passes, in this order, because each one needs the thing the previous one
// made:
//
//   library   vkGetInstanceProcAddr, by name, and the handful of calls a NULL
//             instance answers — the version, the layers, the extensions, and
//             vkCreateInstance itself.
//   instance  everything that takes a VkInstance or a VkPhysicalDevice.
//   device    everything that takes a VkDevice or a VkQueue, resolved through
//             vkGetDeviceProcAddr so the driver can hand back the direct entry
//             point rather than one that dispatches on every call.
//
// A function missing in the first pass is a machine with no usable Vulkan, and
// it is reported. A function missing in the second or third is a driver that
// answered vkCreateInstance and then denied something core to the version it
// claims to be; that is not a condition a caller can do anything about, so it
// aborts.
#include "loader.h"

#include "backend.h"

#include <base/assert.h>
#include <platform/library.h>

#include <stdio.h>
#include <string.h>

voe_render_vk_table voe_render_vk;

static voe_platform_library *loader_library;

// #vkname twice over: once as the string the driver is asked for, once inside
// the assert message, so a driver that is missing a function names it.
#define INSTANCE_FUNCTION(field, vkname)                                       \
	do {                                                                   \
		voe_render_vk.field = (PFN_##vkname)                           \
			voe_render_vk.get_instance_proc_addr(instance, #vkname); \
		VOE_BASE_ASSERT(voe_render_vk.field != NULL,                   \
				"the Vulkan driver does not export " #vkname); \
	} while (0)

#define OPTIONAL_INSTANCE_FUNCTION(field, vkname)                              \
	do {                                                                   \
		voe_render_vk.field = (PFN_##vkname)                           \
			voe_render_vk.get_instance_proc_addr(instance, #vkname); \
	} while (0)

#define DEVICE_FUNCTION(field, vkname)                                         \
	do {                                                                   \
		voe_render_vk.field = (PFN_##vkname)                           \
			voe_render_vk.get_device_proc_addr(device, #vkname);    \
		VOE_BASE_ASSERT(voe_render_vk.field != NULL,                   \
				"the Vulkan driver does not export " #vkname); \
	} while (0)

bool voe_render_loader_open(void)
{
	voe_platform_symbol entry;

	if (loader_library != NULL)
		return true;

	loader_library = voe_platform_library_new(voe_render_backend_library());
	if (loader_library == NULL) {
		fprintf(stderr, "render: %s is not installed on this machine\n",
			voe_render_backend_library());
		return false;
	}

	entry = voe_platform_library_symbol(loader_library,
					    "vkGetInstanceProcAddr");
	if (entry == NULL) {
		fprintf(stderr,
			"render: %s exports no vkGetInstanceProcAddr\n",
			voe_render_backend_library());
		voe_render_loader_close();
		return false;
	}
	voe_render_vk.get_instance_proc_addr = (PFN_vkGetInstanceProcAddr)entry;

	// A NULL instance is what the specification says to pass for the calls
	// that exist before one does. vkEnumerateInstanceVersion is among them
	// only from Vulkan 1.1, so a loader that does not answer it is older
	// than anything this engine can use, and saying that is more useful than
	// failing later on a version comparison.
	voe_render_vk.enumerate_instance_version = (PFN_vkEnumerateInstanceVersion)
		voe_render_vk.get_instance_proc_addr(NULL,
						     "vkEnumerateInstanceVersion");
	voe_render_vk.enumerate_instance_layers = (PFN_vkEnumerateInstanceLayerProperties)
		voe_render_vk.get_instance_proc_addr(NULL,
						     "vkEnumerateInstanceLayerProperties");
	voe_render_vk.enumerate_instance_extensions = (PFN_vkEnumerateInstanceExtensionProperties)
		voe_render_vk.get_instance_proc_addr(NULL,
						     "vkEnumerateInstanceExtensionProperties");
	voe_render_vk.create_instance = (PFN_vkCreateInstance)
		voe_render_vk.get_instance_proc_addr(NULL, "vkCreateInstance");

	if (voe_render_vk.enumerate_instance_version == NULL ||
	    voe_render_vk.enumerate_instance_layers == NULL ||
	    voe_render_vk.enumerate_instance_extensions == NULL ||
	    voe_render_vk.create_instance == NULL) {
		fprintf(stderr,
			"render: %s is older than Vulkan 1.1 and cannot be used\n",
			voe_render_backend_library());
		voe_render_loader_close();
		return false;
	}

	return true;
}

void voe_render_loader_close(void)
{
	if (loader_library != NULL)
		voe_platform_library_destroy(loader_library);
	loader_library = NULL;

	// Emptied rather than left holding pointers into a library that is no
	// longer mapped, so that a use after this is a null dereference at the
	// call site instead of a jump into freed pages.
	memset(&voe_render_vk, 0, sizeof(voe_render_vk));
}

void voe_render_loader_instance(VkInstance instance)
{
	VOE_BASE_ASSERT(instance != VK_NULL_HANDLE,
			"resolving instance functions without an instance");

	INSTANCE_FUNCTION(destroy_instance, vkDestroyInstance);
	INSTANCE_FUNCTION(get_device_proc_addr, vkGetDeviceProcAddr);
	INSTANCE_FUNCTION(enumerate_physical_devices, vkEnumeratePhysicalDevices);
	INSTANCE_FUNCTION(get_physical_device_properties,
			  vkGetPhysicalDeviceProperties);
	INSTANCE_FUNCTION(get_queue_family_properties,
			  vkGetPhysicalDeviceQueueFamilyProperties);
	INSTANCE_FUNCTION(create_device, vkCreateDevice);

	// VK_KHR_surface's, and required because the instance was created with
	// that extension enabled or not created at all.
	INSTANCE_FUNCTION(destroy_surface, vkDestroySurfaceKHR);
	INSTANCE_FUNCTION(get_surface_support,
			  vkGetPhysicalDeviceSurfaceSupportKHR);
	INSTANCE_FUNCTION(get_surface_capabilities,
			  vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
	INSTANCE_FUNCTION(get_surface_formats,
			  vkGetPhysicalDeviceSurfaceFormatsKHR);
	INSTANCE_FUNCTION(get_surface_present_modes,
			  vkGetPhysicalDeviceSurfacePresentModesKHR);

	// Absent whenever the debug-utils extension was not enabled, which is
	// the normal case on a machine with no Vulkan SDK. Left NULL and never
	// called.
	OPTIONAL_INSTANCE_FUNCTION(create_debug_messenger,
				   vkCreateDebugUtilsMessengerEXT);
	OPTIONAL_INSTANCE_FUNCTION(destroy_debug_messenger,
				   vkDestroyDebugUtilsMessengerEXT);
}

void voe_render_loader_device(VkDevice device)
{
	VOE_BASE_ASSERT(device != VK_NULL_HANDLE,
			"resolving device functions without a device");

	DEVICE_FUNCTION(destroy_device, vkDestroyDevice);
	DEVICE_FUNCTION(device_wait_idle, vkDeviceWaitIdle);
	DEVICE_FUNCTION(get_device_queue, vkGetDeviceQueue);

	DEVICE_FUNCTION(create_swapchain, vkCreateSwapchainKHR);
	DEVICE_FUNCTION(destroy_swapchain, vkDestroySwapchainKHR);
	DEVICE_FUNCTION(get_swapchain_images, vkGetSwapchainImagesKHR);
	DEVICE_FUNCTION(acquire_next_image, vkAcquireNextImageKHR);
	DEVICE_FUNCTION(queue_present, vkQueuePresentKHR);

	DEVICE_FUNCTION(create_image_view, vkCreateImageView);
	DEVICE_FUNCTION(destroy_image_view, vkDestroyImageView);

	DEVICE_FUNCTION(create_command_pool, vkCreateCommandPool);
	DEVICE_FUNCTION(destroy_command_pool, vkDestroyCommandPool);
	DEVICE_FUNCTION(allocate_command_buffers, vkAllocateCommandBuffers);
	DEVICE_FUNCTION(begin_command_buffer, vkBeginCommandBuffer);
	DEVICE_FUNCTION(end_command_buffer, vkEndCommandBuffer);
	DEVICE_FUNCTION(reset_command_buffer, vkResetCommandBuffer);

	// Both core in 1.3, which is the version the physical device had to
	// claim to be picked at all.
	DEVICE_FUNCTION(cmd_pipeline_barrier2, vkCmdPipelineBarrier2);
	DEVICE_FUNCTION(cmd_begin_rendering, vkCmdBeginRendering);
	DEVICE_FUNCTION(cmd_end_rendering, vkCmdEndRendering);
	DEVICE_FUNCTION(queue_submit2, vkQueueSubmit2);

	DEVICE_FUNCTION(create_semaphore, vkCreateSemaphore);
	DEVICE_FUNCTION(destroy_semaphore, vkDestroySemaphore);
	DEVICE_FUNCTION(create_fence, vkCreateFence);
	DEVICE_FUNCTION(destroy_fence, vkDestroyFence);
	DEVICE_FUNCTION(wait_for_fences, vkWaitForFences);
	DEVICE_FUNCTION(reset_fences, vkResetFences);
}
