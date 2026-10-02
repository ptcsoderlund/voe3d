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
	PFN_vkGetPhysicalDeviceMemoryProperties get_memory_properties;
	// Core since Vulkan 1.1, and the floor here is 1.3. Asked one question:
	// whether this card can index the shader's texture array with a number
	// that is not a constant — see create_device in device.c.
	PFN_vkGetPhysicalDeviceFeatures2 get_physical_device_features2;
	PFN_vkCreateDevice create_device;

	// VK_KHR_surface's, and all five are absent on a headless instance,
	// which is created without that extension. Nothing on that path calls
	// one: see the surface argument to voe_render_loader_instance below.
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

	// VK_KHR_swapchain's, and all five are absent on a headless device for
	// the same reason the surface functions are absent from a headless
	// instance. See the swapchain argument to voe_render_loader_device.
	PFN_vkCreateSwapchainKHR create_swapchain;
	PFN_vkDestroySwapchainKHR destroy_swapchain;
	PFN_vkGetSwapchainImagesKHR get_swapchain_images;
	PFN_vkAcquireNextImageKHR acquire_next_image;
	PFN_vkQueuePresentKHR queue_present;

	// The offscreen target: an image of our own, the memory under it, and
	// the copy that puts what was drawn into the swapchain image at the end
	// of a frame.
	PFN_vkGetPhysicalDeviceFormatProperties get_physical_device_format_properties;

	PFN_vkCreateImage create_image;
	PFN_vkDestroyImage destroy_image;
	PFN_vkGetImageMemoryRequirements get_image_memory_requirements;
	PFN_vkBindImageMemory bind_image_memory;
	PFN_vkAllocateMemory allocate_memory;
	PFN_vkFreeMemory free_memory;
	PFN_vkCmdBlitImage cmd_blit_image;
	// A pass's depth copied into its sampled twin; see
	// voe_render_frame_copy_depth in pass.c.
	PFN_vkCmdCopyImage cmd_copy_image;

	PFN_vkCreateImageView create_image_view;
	PFN_vkDestroyImageView destroy_image_view;

	// Textures. The sampler is how a shader reads one, the buffer-to-image
	// copy is how the pixels get there, and the format properties are asked
	// because generating mipmaps is a blit and a blit needs the format to
	// filter linearly — which is a property of the card, not of the format
	// on paper. See texture.c.
	PFN_vkCreateSampler create_sampler;
	PFN_vkDestroySampler destroy_sampler;
	PFN_vkCmdCopyBufferToImage cmd_copy_buffer_to_image;
	// The other direction, and the whole of what a target read back costs
	// this table: a finished target's colour image copied into a
	// host-visible buffer. Core 1.0, so it resolves on a headless device
	// too — which is the device the picture is usually taken on. See
	// voe_render_target_read in target_read.c.
	PFN_vkCmdCopyImageToBuffer cmd_copy_image_to_buffer;
	PFN_vkCreateCommandPool create_command_pool;
	PFN_vkDestroyCommandPool destroy_command_pool;
	PFN_vkAllocateCommandBuffers allocate_command_buffers;
	// Only the staging upload in buffer.c frees one: every other command
	// buffer in the engine lives as long as the pool it came from.
	PFN_vkFreeCommandBuffers free_command_buffers;
	PFN_vkBeginCommandBuffer begin_command_buffer;
	PFN_vkEndCommandBuffer end_command_buffer;
	PFN_vkResetCommandBuffer reset_command_buffer;
	PFN_vkCmdPipelineBarrier2 cmd_pipeline_barrier2;
	PFN_vkCmdBeginRendering cmd_begin_rendering;
	PFN_vkCmdEndRendering cmd_end_rendering;
	// The overlay's depth clear, recorded into the block the two above open.
	// It is here rather than beside the draws because what it clears is an
	// attachment of that block and it is meaningless outside one.
	PFN_vkCmdClearAttachments cmd_clear_attachments;
	// The probe volumes' clear to nought, once, as each volume is built.
	PFN_vkCmdClearColorImage cmd_clear_color_image;
	PFN_vkCreateShaderModule create_shader_module;
	PFN_vkDestroyShaderModule destroy_shader_module;
	PFN_vkCreatePipelineLayout create_pipeline_layout;
	PFN_vkDestroyPipelineLayout destroy_pipeline_layout;
	PFN_vkCreateGraphicsPipelines create_graphics_pipelines;
	// The relight's compute pipelines and their dispatches.
	PFN_vkCreateComputePipelines create_compute_pipelines;
	PFN_vkCmdDispatch cmd_dispatch;
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
	// Asked in debug builds only, by the assert in frame.c that a slot's
	// fence really is the one that was just waited on. Nothing in a release
	// build calls it.
	PFN_vkGetFenceStatus get_fence_status;
	PFN_vkQueueSubmit2 queue_submit2;

	// Timestamp queries: the graphics card's own clock, written into a pool
	// at the top and the bottom of a frame's command buffer and read back
	// once that frame's fence says the card has finished with it. This is
	// the only way there is to measure GPU time — the CPU can see when it
	// submitted work and when a fence came back, and neither of those is how
	// long the card spent. cmd_write_timestamp2 is core in 1.3, which is the
	// version the physical device had to claim; the other four are core 1.0.
	// See frame.c.
	PFN_vkCreateQueryPool create_query_pool;
	PFN_vkDestroyQueryPool destroy_query_pool;
	PFN_vkCmdResetQueryPool cmd_reset_query_pool;
	PFN_vkCmdWriteTimestamp2 cmd_write_timestamp2;
	PFN_vkGetQueryPoolResults get_query_pool_results;

	// Buffers, and the memory under them. The vertex and index buffers live
	// in device-local memory and are filled through a host-visible staging
	// buffer, which is what map_memory and cmd_copy_buffer are for; the
	// uniform buffers are host-visible and stay mapped for their whole life.
	// See buffer.c.
	PFN_vkCreateBuffer create_buffer;
	PFN_vkDestroyBuffer destroy_buffer;
	PFN_vkGetBufferMemoryRequirements get_buffer_memory_requirements;
	PFN_vkBindBufferMemory bind_buffer_memory;
	PFN_vkMapMemory map_memory;
	PFN_vkUnmapMemory unmap_memory;
	PFN_vkCmdCopyBuffer cmd_copy_buffer;

	// Drawing the cube: two buffers bound, and a draw that reads indices
	// rather than counting vertices.
	PFN_vkCmdBindVertexBuffers cmd_bind_vertex_buffers;
	PFN_vkCmdBindIndexBuffer cmd_bind_index_buffer;
	PFN_vkCmdDrawIndexed cmd_draw_indexed;

	// The per-object matrix, pushed straight into the command buffer rather
	// than into a buffer of its own. There are two cubes and a draw each, so
	// the model matrix is the one thing that differs between two draws in one
	// frame — see cube.c for why that makes it a push constant.
	PFN_vkCmdPushConstants cmd_push_constants;

	// The one descriptor in this engine: set 0, binding 0, the uniform buffer
	// holding the camera's two matrices. One layout, one pool, and one set
	// per frame slot — see cube.c.
	PFN_vkCreateDescriptorSetLayout create_descriptor_set_layout;
	PFN_vkDestroyDescriptorSetLayout destroy_descriptor_set_layout;
	PFN_vkCreateDescriptorPool create_descriptor_pool;
	PFN_vkDestroyDescriptorPool destroy_descriptor_pool;
	PFN_vkAllocateDescriptorSets allocate_descriptor_sets;
	PFN_vkUpdateDescriptorSets update_descriptor_sets;
	PFN_vkCmdBindDescriptorSets cmd_bind_descriptor_sets;
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
//
// surface and swapchain say whether the instance and the device were created
// with VK_KHR_surface and VK_KHR_swapchain enabled. A function belonging to an
// extension that was never enabled is not there to resolve, and asking for it is
// not a broken driver — so it is skipped and left NULL rather than aborted on.
// Both are true for a device opened on a window and both are false for a
// headless one; there is no third combination.
void voe_render_loader_instance(VkInstance instance, bool surface);
void voe_render_loader_device(VkDevice device, bool swapchain);
