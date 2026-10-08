// Opening the Vulkan loader and filling the table. See loader.h for why the
// table exists and what the rules on it are.
//
// There is no abstraction over Vulkan here or anywhere in this folder: there is
// one graphics API and there will not be a second.
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
#include <base/report.h>
#include <platform/library.h>

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
		VOE_BASE_ERROR("render", "%s is not installed on this machine",
			       voe_render_backend_library());
		return false;
	}

	entry = voe_platform_library_symbol(loader_library,
					    "vkGetInstanceProcAddr");
	if (entry == NULL) {
		VOE_BASE_ERROR("render",
			       "%s exports no vkGetInstanceProcAddr",
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
		VOE_BASE_ERROR("render",
			       "%s is older than Vulkan 1.1 and cannot be used",
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

void voe_render_loader_instance(VkInstance instance, bool surface)
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
	INSTANCE_FUNCTION(get_memory_properties,
			  vkGetPhysicalDeviceMemoryProperties);
	INSTANCE_FUNCTION(get_physical_device_format_properties,
			  vkGetPhysicalDeviceFormatProperties);
	INSTANCE_FUNCTION(get_physical_device_features2,
			  vkGetPhysicalDeviceFeatures2);
	INSTANCE_FUNCTION(create_device, vkCreateDevice);

	// VK_KHR_surface's, and required whenever that extension was enabled —
	// which is every instance opened on a window. A headless one enables it
	// nowhere and asks for none of these, so they stay null and the loader
	// is not accused of missing something it was never asked for.
	if (surface) {
		INSTANCE_FUNCTION(destroy_surface, vkDestroySurfaceKHR);
		INSTANCE_FUNCTION(get_surface_support,
				  vkGetPhysicalDeviceSurfaceSupportKHR);
		INSTANCE_FUNCTION(get_surface_capabilities,
				  vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
		INSTANCE_FUNCTION(get_surface_formats,
				  vkGetPhysicalDeviceSurfaceFormatsKHR);
		INSTANCE_FUNCTION(get_surface_present_modes,
				  vkGetPhysicalDeviceSurfacePresentModesKHR);
	}

	// Absent whenever the debug-utils extension was not enabled, which is
	// when the instance did not offer it. Left NULL and never called.
	OPTIONAL_INSTANCE_FUNCTION(create_debug_messenger,
				   vkCreateDebugUtilsMessengerEXT);
	OPTIONAL_INSTANCE_FUNCTION(destroy_debug_messenger,
				   vkDestroyDebugUtilsMessengerEXT);
	OPTIONAL_INSTANCE_FUNCTION(set_debug_utils_object_name,
				   vkSetDebugUtilsObjectNameEXT);
	OPTIONAL_INSTANCE_FUNCTION(cmd_begin_debug_utils_label,
				   vkCmdBeginDebugUtilsLabelEXT);
	OPTIONAL_INSTANCE_FUNCTION(cmd_end_debug_utils_label,
				   vkCmdEndDebugUtilsLabelEXT);
}

void voe_render_loader_device(VkDevice device, bool swapchain)
{
	VOE_BASE_ASSERT(device != VK_NULL_HANDLE,
			"resolving device functions without a device");

	DEVICE_FUNCTION(destroy_device, vkDestroyDevice);
	DEVICE_FUNCTION(device_wait_idle, vkDeviceWaitIdle);
	DEVICE_FUNCTION(get_device_queue, vkGetDeviceQueue);

	// VK_KHR_swapchain's, and the same story as the surface functions
	// above: enabled and required on a window, never asked for headless.
	if (swapchain) {
		DEVICE_FUNCTION(create_swapchain, vkCreateSwapchainKHR);
		DEVICE_FUNCTION(destroy_swapchain, vkDestroySwapchainKHR);
		DEVICE_FUNCTION(get_swapchain_images, vkGetSwapchainImagesKHR);
		DEVICE_FUNCTION(acquire_next_image, vkAcquireNextImageKHR);
		DEVICE_FUNCTION(queue_present, vkQueuePresentKHR);
	}

	// The offscreen target and the copy out of it. Core, so they are here
	// whether or not there is a swapchain to copy into.
	DEVICE_FUNCTION(create_image, vkCreateImage);
	DEVICE_FUNCTION(destroy_image, vkDestroyImage);
	DEVICE_FUNCTION(get_image_memory_requirements,
			vkGetImageMemoryRequirements);
	DEVICE_FUNCTION(bind_image_memory, vkBindImageMemory);
	DEVICE_FUNCTION(allocate_memory, vkAllocateMemory);
	DEVICE_FUNCTION(free_memory, vkFreeMemory);
	DEVICE_FUNCTION(cmd_blit_image, vkCmdBlitImage);
	DEVICE_FUNCTION(cmd_copy_image, vkCmdCopyImage);

	DEVICE_FUNCTION(create_sampler, vkCreateSampler);
	DEVICE_FUNCTION(destroy_sampler, vkDestroySampler);
	DEVICE_FUNCTION(cmd_copy_buffer_to_image, vkCmdCopyBufferToImage);
	DEVICE_FUNCTION(cmd_copy_image_to_buffer, vkCmdCopyImageToBuffer);
	DEVICE_FUNCTION(create_image_view, vkCreateImageView);
	DEVICE_FUNCTION(destroy_image_view, vkDestroyImageView);

	DEVICE_FUNCTION(create_command_pool, vkCreateCommandPool);
	DEVICE_FUNCTION(destroy_command_pool, vkDestroyCommandPool);
	DEVICE_FUNCTION(reset_command_pool, vkResetCommandPool);
	DEVICE_FUNCTION(allocate_command_buffers, vkAllocateCommandBuffers);
	DEVICE_FUNCTION(free_command_buffers, vkFreeCommandBuffers);
	DEVICE_FUNCTION(begin_command_buffer, vkBeginCommandBuffer);
	DEVICE_FUNCTION(end_command_buffer, vkEndCommandBuffer);

	// Both core in 1.3, which is the version the physical device had to
	// claim to be picked at all.
	DEVICE_FUNCTION(cmd_pipeline_barrier2, vkCmdPipelineBarrier2);
	DEVICE_FUNCTION(cmd_begin_rendering, vkCmdBeginRendering);
	DEVICE_FUNCTION(cmd_end_rendering, vkCmdEndRendering);
	// Core 1.0, and the overlay layer's whole cost on this side: one command
	// that clears depth in the middle of the block the two above open.
	DEVICE_FUNCTION(cmd_clear_attachments, vkCmdClearAttachments);
	DEVICE_FUNCTION(cmd_clear_color_image, vkCmdClearColorImage);
	DEVICE_FUNCTION(queue_submit2, vkQueueSubmit2);

	// The timestamps. Core, so they resolve on a headless device too, and
	// the offscreen test drives the same recording path a window does.
	DEVICE_FUNCTION(create_query_pool, vkCreateQueryPool);
	DEVICE_FUNCTION(destroy_query_pool, vkDestroyQueryPool);
	DEVICE_FUNCTION(cmd_reset_query_pool, vkCmdResetQueryPool);
	DEVICE_FUNCTION(cmd_write_timestamp2, vkCmdWriteTimestamp2);
	DEVICE_FUNCTION(get_query_pool_results, vkGetQueryPoolResults);

	// The pipeline and the draw. The shader module is made and thrown away
	// inside device.c, which is why its two live next to the pipeline's
	// rather than anywhere else.
	DEVICE_FUNCTION(create_shader_module, vkCreateShaderModule);
	DEVICE_FUNCTION(destroy_shader_module, vkDestroyShaderModule);
	DEVICE_FUNCTION(create_pipeline_layout, vkCreatePipelineLayout);
	DEVICE_FUNCTION(destroy_pipeline_layout, vkDestroyPipelineLayout);
	DEVICE_FUNCTION(create_graphics_pipelines, vkCreateGraphicsPipelines);
	DEVICE_FUNCTION(create_pipeline_cache, vkCreatePipelineCache);
	DEVICE_FUNCTION(destroy_pipeline_cache, vkDestroyPipelineCache);
	DEVICE_FUNCTION(get_pipeline_cache_data, vkGetPipelineCacheData);
	DEVICE_FUNCTION(create_compute_pipelines, vkCreateComputePipelines);
	DEVICE_FUNCTION(cmd_dispatch, vkCmdDispatch);
	DEVICE_FUNCTION(destroy_pipeline, vkDestroyPipeline);
	DEVICE_FUNCTION(cmd_bind_pipeline, vkCmdBindPipeline);
	DEVICE_FUNCTION(cmd_set_viewport, vkCmdSetViewport);
	DEVICE_FUNCTION(cmd_set_scissor, vkCmdSetScissor);
	DEVICE_FUNCTION(cmd_draw, vkCmdDraw);

	// Buffers and the memory under them, and the copy that fills a
	// device-local one from a staging one. All core 1.0.
	DEVICE_FUNCTION(create_buffer, vkCreateBuffer);
	DEVICE_FUNCTION(destroy_buffer, vkDestroyBuffer);
	DEVICE_FUNCTION(get_buffer_memory_requirements,
			vkGetBufferMemoryRequirements);
	DEVICE_FUNCTION(bind_buffer_memory, vkBindBufferMemory);
	DEVICE_FUNCTION(map_memory, vkMapMemory);
	DEVICE_FUNCTION(unmap_memory, vkUnmapMemory);
	DEVICE_FUNCTION(cmd_copy_buffer, vkCmdCopyBuffer);

	// The cube's draw. cmd_draw above is still here because two shaders bind
	// no vertex buffer at all and count their own vertices instead:
	// matrix_probe.slang's three, and elements.slang's four per element.
	DEVICE_FUNCTION(cmd_bind_vertex_buffers, vkCmdBindVertexBuffers);
	DEVICE_FUNCTION(cmd_bind_index_buffer, vkCmdBindIndexBuffer);
	DEVICE_FUNCTION(cmd_draw_indexed, vkCmdDrawIndexed);

	// The per-object matrix. Core 1.0, and the only thing in this engine that
	// reaches a shader without a buffer under it.
	DEVICE_FUNCTION(cmd_push_constants, vkCmdPushConstants);

	// The camera's descriptor.
	DEVICE_FUNCTION(create_descriptor_set_layout,
			vkCreateDescriptorSetLayout);
	DEVICE_FUNCTION(destroy_descriptor_set_layout,
			vkDestroyDescriptorSetLayout);
	DEVICE_FUNCTION(create_descriptor_pool, vkCreateDescriptorPool);
	DEVICE_FUNCTION(destroy_descriptor_pool, vkDestroyDescriptorPool);
	DEVICE_FUNCTION(allocate_descriptor_sets, vkAllocateDescriptorSets);
	DEVICE_FUNCTION(update_descriptor_sets, vkUpdateDescriptorSets);
	DEVICE_FUNCTION(cmd_bind_descriptor_sets, vkCmdBindDescriptorSets);

	DEVICE_FUNCTION(create_semaphore, vkCreateSemaphore);
	DEVICE_FUNCTION(destroy_semaphore, vkDestroySemaphore);
	DEVICE_FUNCTION(create_fence, vkCreateFence);
	DEVICE_FUNCTION(destroy_fence, vkDestroyFence);
	DEVICE_FUNCTION(wait_for_fences, vkWaitForFences);
	DEVICE_FUNCTION(reset_fences, vkResetFences);
	DEVICE_FUNCTION(get_fence_status, vkGetFenceStatus);
}
