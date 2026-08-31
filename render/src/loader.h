// THE FUNCTION-POINTER TABLE. This is the one place in this engine where
// function pointers are expected, and it is a deliberate exception rather than
// an oversight — so it is written down here, where the next reader meets it.
//
// Nothing in this repository links against Vulkan. There is no -l flag, no
// import library and no SDK: the loader ships with the graphics driver, is
// opened by name at run time, and every entry point in the API is resolved
// through vkGetInstanceProcAddr. That is not a style choice, it is what makes a
// fresh clone build on a machine with nothing installed but a compiler.
//
// So there is a table, and it has rules:
//
//   - It is render's alone. No other folder sees this header.
//   - It is filled once at startup, in three passes — what the library exports,
//     then what the instance exports, then what the device exports — and never
//     written again after that.
//   - It is never passed as a parameter and never stored in anything. It is one
//     file-scope object with a name, reached by naming it.
//
// The last of those is why voe_render_vk is a global in an engine that has no
// others. Threading it through every call would put a parameter on every
// function in the folder to say something that is true of the whole folder, and
// a second copy could not exist for any reason worth having.
//
// VK_NO_PROTOTYPES IS DEFINED HERE AND THIS IS THE ONLY PLACE VULKAN'S HEADERS
// ENTER render. Without it the vendored headers declare prototypes, the folder
// links against symbols that do not exist, and the failure is a link error that
// says nothing about the cause. Include this header before any other Vulkan one.
//
// The field names are the Vulkan names with the vk prefix taken off, because the
// object already carries it: voe_render_vk.create_device is vkCreateDevice.
#pragma once

#ifndef VK_NO_PROTOTYPES
#define VK_NO_PROTOTYPES
#endif

#include "../vulkan/vulkan_core.h"

typedef struct {
	// Resolved from the library. The only symbol looked up by name.
	PFN_vkGetInstanceProcAddr get_instance_proc_addr;

	// Resolved from a NULL instance, which is what you are allowed to ask
	// before there is one.
	PFN_vkEnumerateInstanceVersion enumerate_instance_version;
	PFN_vkEnumerateInstanceLayerProperties enumerate_instance_layers;
	PFN_vkEnumerateInstanceExtensionProperties enumerate_instance_extensions;
	PFN_vkCreateInstance create_instance;

	// Resolved from the instance.
	PFN_vkDestroyInstance destroy_instance;
	PFN_vkGetDeviceProcAddr get_device_proc_addr;
	PFN_vkEnumeratePhysicalDevices enumerate_physical_devices;
	PFN_vkGetPhysicalDeviceProperties get_physical_device_properties;
	PFN_vkGetPhysicalDeviceQueueFamilyProperties get_queue_family_properties;
	PFN_vkCreateDevice create_device;
	PFN_vkDestroySurfaceKHR destroy_surface;
	PFN_vkGetPhysicalDeviceSurfaceSupportKHR get_surface_support;
	PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR get_surface_capabilities;
	PFN_vkGetPhysicalDeviceSurfaceFormatsKHR get_surface_formats;
	PFN_vkGetPhysicalDeviceSurfacePresentModesKHR get_surface_present_modes;

	// Both NULL unless the debug-utils extension was there to enable. Every
	// use of them is guarded on that, and nothing fails when they are absent.
	PFN_vkCreateDebugUtilsMessengerEXT create_debug_messenger;
	PFN_vkDestroyDebugUtilsMessengerEXT destroy_debug_messenger;

	// Resolved from the device, which is where a driver can hand out the
	// direct entry point rather than one that has to dispatch.
	PFN_vkDestroyDevice destroy_device;
	PFN_vkDeviceWaitIdle device_wait_idle;
	PFN_vkGetDeviceQueue get_device_queue;
	PFN_vkCreateSwapchainKHR create_swapchain;
	PFN_vkDestroySwapchainKHR destroy_swapchain;
	PFN_vkGetSwapchainImagesKHR get_swapchain_images;
	PFN_vkAcquireNextImageKHR acquire_next_image;
	PFN_vkQueuePresentKHR queue_present;
	PFN_vkCreateImageView create_image_view;
	PFN_vkDestroyImageView destroy_image_view;
	PFN_vkCreateCommandPool create_command_pool;
	PFN_vkDestroyCommandPool destroy_command_pool;
	PFN_vkAllocateCommandBuffers allocate_command_buffers;
	PFN_vkBeginCommandBuffer begin_command_buffer;
	PFN_vkEndCommandBuffer end_command_buffer;
	PFN_vkResetCommandBuffer reset_command_buffer;
	PFN_vkCmdPipelineBarrier2 cmd_pipeline_barrier2;
	PFN_vkCmdBeginRendering cmd_begin_rendering;
	PFN_vkCmdEndRendering cmd_end_rendering;
	PFN_vkCreateShaderModule create_shader_module;
	PFN_vkDestroyShaderModule destroy_shader_module;
	PFN_vkCreatePipelineLayout create_pipeline_layout;
	PFN_vkDestroyPipelineLayout destroy_pipeline_layout;
	PFN_vkCreateGraphicsPipelines create_graphics_pipelines;
	PFN_vkDestroyPipeline destroy_pipeline;
	PFN_vkCmdBindPipeline cmd_bind_pipeline;
	PFN_vkCmdSetViewport cmd_set_viewport;
	PFN_vkCmdSetScissor cmd_set_scissor;
	PFN_vkCmdDraw cmd_draw;
	PFN_vkCreateSemaphore create_semaphore;
	PFN_vkDestroySemaphore destroy_semaphore;
	PFN_vkCreateFence create_fence;
	PFN_vkDestroyFence destroy_fence;
	PFN_vkWaitForFences wait_for_fences;
	PFN_vkResetFences reset_fences;
	PFN_vkQueueSubmit2 queue_submit2;
} voe_render_vk_table;

extern voe_render_vk_table voe_render_vk;

// Opens the platform's Vulkan loader and fills in everything above that does not
// need an instance. False means there is no Vulkan on this machine — which is a
// thing to report, not to abort on. Opening an already-open loader succeeds and
// changes nothing.
[[nodiscard]] bool voe_render_loader_open(void);

// Closes the library and empties the table. Every pointer taken out of it before
// this is dangling afterwards.
void voe_render_loader_close(void);

// The second and third passes. Each is called once, in this order, and each
// aborts if the driver does not export something core to the version we asked
// for — a loader that answered vkCreateDevice and then does not know
// vkDestroyDevice is broken in a way no caller can act on.
void voe_render_loader_instance(VkInstance instance);
void voe_render_loader_device(VkDevice device);
