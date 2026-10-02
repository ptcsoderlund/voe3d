// Starting the GPU: the loader, the instance, the surface, the graphics card, the
// logical device and the pipelines, in that order, because each one is what the
// next is asked for. Everything here happens once. open_device below is the one
// place that order is written; the steps that grew too long for this file are
// beside it, declared in startup.h: the instance and validation in instance.c,
// the card and the `render` line in card.c, the three mesh pipelines, the shader
// bytes and depth in pipeline.c, the shadow maps in shadow.c and point_shadow.c. This file keeps the logical device, the format,
// timing, present modes, the frame objects and close-down. The parts that happen
// again live in target.c, swapchain.c and frame.c — see device_internal.h for
// why the split is where it is.
//
// WHY THE ARENA IS A PARAMETER AND WHY IT IS ONLY NEEDED HERE. Startup asks the
// driver four questions whose answers are arrays whose length is not known until
// it answers: which layers, which extensions, which graphics cards, which queue
// families, which surface formats. That is working memory, it comes from an
// arena the caller passes in, and the caller gets it all back the moment this
// returns — nothing the device keeps is pushed on it. Drawing a frame asks no
// such question, which is why the arena appears on _new and nowhere else.
//
// EVERY FAILURE HERE PRINTS ONE LINE AND RETURNS A CATEGORY. The category is all
// a caller can act on; the line is what a person needs. See base/error.h.
//
// THERE ARE TWO WAYS IN AND ONE OF THEM HAS NO WINDOW. voe_render_device_new
// opens a device on a window; voe_render_device_new_headless opens one on
// nothing, for the tests. They are the same function with one
// argument between them, and every place that argument is read says so — the
// instance extensions (instance.c), the surface (here), the queue family
// (card.c), the format, the device extensions and the swapchain. Six places, and
// there are no others: everything past the graphics card is identical, which is
// what makes a test on a headless device a test of the code that ships.
#include "device_internal.h"
#include "startup.h"

#include "backend.h"

#include <render/device.h>

#include <base/assert.h>
#include <base/report.h>

#include <stddef.h>
#include <stdlib.h>

// SRGB AND NOT UNORM, BECAUSE CARD 019 MADE THIS ENGINE RENDER IN LINEAR LIGHT.
// Every colour inside a frame is linear — a decoded texture, a light, a factor,
// the clear — and something has to apply the sRGB curve on the way to the
// screen. That something is this format: the target and the swapchain image both
// have it, so the shader writes linear values, the hardware encodes them once as
// it stores them, and the blit between two images of the same sRGB format is a
// straight copy. The alternative is writing the curve into the fragment shader
// by hand, which is the same arithmetic done less exactly in a place a later
// tone-mapping pass would have to undo it.
//
// IT WAS UNORM UNTIL THERE WAS A LIGHT, and that was right at the time: with
// nothing lit, a texture drawn straight to the screen through an sRGB view came
// out visibly pale against the same picture in an image viewer. The two halves —
// this format and the sRGB texture format in texture.c — had to change together
// and they did.
//
// It is what the surface is asked for and what a headless device takes without
// asking anyone. A surface that offers no sRGB format at all is handled where
// the asking happens, in voe_render_device_choose_format.
#define PREFERRED_FORMAT VK_FORMAT_B8G8R8A8_SRGB

static void report(voe_base_error *error, voe_base_error code)
{
	if (error != NULL)
		*error = code;
}

// ---------------------------------------------------------------------- device

static bool create_device(voe_render_device *device)
{
	const char *extensions[1] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	float priority = 1.0f;
	VkDeviceQueueCreateInfo queue = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = device->queue_family,
		.queueCount = 1,
		.pQueuePriorities = &priority,
	};
	// Slang lowers SV_VertexID to gl_VertexIndex minus gl_BaseVertex, which
	// makes the compiled shader declare the DrawParameters capability, which
	// needs this feature turned on or the module is invalid. The subtraction
	// is of no use to us — nothing here draws with a first vertex that is not
	// zero — but there is no way to ask slangc for the builtin without it,
	// and a feature that every 1.3 driver measured reports is a smaller price
	// than a vertex buffer this card says not to add.
	VkPhysicalDeviceVulkan11Features features11 = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
		.shaderDrawParameters = VK_TRUE,
	};
	// Core in 1.3 and supported by every 1.3 implementation, but still off
	// until asked for: a feature that is core is guaranteed available, not
	// guaranteed enabled.
	VkPhysicalDeviceVulkan13Features features13 = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
		.pNext = &features11,
		.synchronization2 = VK_TRUE,
		.dynamicRendering = VK_TRUE,
	};
	// The second half of the same question, and it is a different feature
	// from the one below rather than a stronger spelling of it. See the
	// paragraph under it.
	//
	// The probe volumes at bindings 6 and 10 are unsized arrays whose entries
	// for volumes not built are unwritten or name freed images (ADR-0326):
	// reading them needs runtimeDescriptorArray and
	// descriptorBindingPartiallyBound, both among the descriptor-indexing
	// features Vulkan 1.3 requires, so not queried.
	VkPhysicalDeviceVulkan12Features features12 = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
		.pNext = &features13,
		.runtimeDescriptorArray = VK_TRUE,
		.descriptorBindingPartiallyBound = VK_TRUE,
	};
	// What the same query has to be handed in order to answer about a 1.2
	// feature: get_physical_device_features2 fills only what is chained onto
	// it, so asking about shaderSampledImageArrayNonUniformIndexing means
	// chaining a 1.2 block onto the query as well as onto the create.
	VkPhysicalDeviceVulkan12Features available12 = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
	};
	// WHAT THE FRAGMENT STAGE NEEDS IN ORDER TO PICK A TEXTURE AT RUN TIME.
	// It samples one element of an array of 1024, and which element is
	// a number that came out of a buffer — the same number the CPU handed
	// out as a texture id (ADR-0018). Indexing a sampled-image array with
	// anything but a compile-time constant is what this feature permits, and
	// a device created without it is undefined behaviour that happens to
	// work on most drivers, which is the worst kind.
	//
	// IT IS QUERIED AND NOT ASSUMED, because enabling a feature a card does
	// not have fails vkCreateDevice outright — which would turn a card that
	// can very nearly do this into a card that cannot start the engine. Every
	// Vulkan 1.3 implementation measured offers it; the message below is for
	// the one that does not, so that the wrong picture has an explanation
	// sitting above it in the log.
	VkPhysicalDeviceFeatures2 available = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
		.pNext = &available12,
	};
	VkPhysicalDeviceFeatures2 features = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
		.pNext = &features12,
	};
	// The one extension, and a headless device does not enable it: there is
	// no surface for a swapchain to be made from and nothing to present to.
	VkDeviceCreateInfo info = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = &features,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &queue,
		.enabledExtensionCount = device->headless ? 0 : 1,
		.ppEnabledExtensionNames = extensions,
	};
	VkResult result;

	voe_render_vk.get_physical_device_features2(device->physical, &available);
	if (available.features.shaderSampledImageArrayDynamicIndexing)
		features.features.shaderSampledImageArrayDynamicIndexing =
			VK_TRUE;
	else
		VOE_BASE_ERROR("render",
			       "this graphics card cannot index a texture array with a value from a buffer; textures will be wrong");

	// AND WHAT THE ELEMENT PIPELINE NEEDS ON TOP OF IT, WHICH IS A SECOND
	// FEATURE AND NOT A STRONGER ONE. The dynamic feature above permits an
	// index that is the same value everywhere in the draw, which is what a
	// mesh draw's is: one shading record, picked by a push constant. An
	// element draw is one draw for every element the frame submitted, and
	// each one names its own sheet out of its own record — so the index
	// differs between fragments of one draw, which is precisely what
	// "non-uniform" means and precisely what the feature above does not
	// cover. Enabling this is what makes shaders/elements.slang's
	// NonUniformResourceIndex mean anything; without the pair, a panel with
	// two sheets in it is undefined behaviour that happens to look right on
	// most drivers.
	//
	// QUERIED AND NOT ASSUMED, for the same reason the one above is: asking
	// for a feature a card does not have fails vkCreateDevice outright, and
	// a card that cannot do this can still run everything else in the
	// engine. The message is for that card, so the wrong text has an
	// explanation sitting above it in the log.
	if (available12.shaderSampledImageArrayNonUniformIndexing)
		features12.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
	else
		VOE_BASE_ERROR("render",
			       "this graphics card cannot index a texture array differently per element; text drawn as elements will be wrong where a frame uses more than one sheet");

	// What the point shadow pass's vertex stage needs to pick a layer
	// (ADR-0325). Queried for the same reason: a card without it runs
	// everything else, with the point shadow maps' side taken as nought.
	device->output_layer = available12.shaderOutputLayer == VK_TRUE;
	features12.shaderOutputLayer = available12.shaderOutputLayer;
	device->point_shadow_size = device->output_layer ?
					    device->capacities.point_shadow_size :
					    0;
	if (!device->output_layer && device->capacities.point_shadow_size > 0)
		VOE_BASE_ERROR("render",
			       "this graphics card cannot write a layer from a vertex shader (shaderOutputLayer); point lights will cast no shadow");

	result = voe_render_vk.create_device(device->physical, &info, NULL,
					     &device->device);
	if (result != VK_SUCCESS) {
		VOE_BASE_ERROR("render", "vkCreateDevice failed (VkResult %d)",
			       (int)result);
		return false;
	}

	voe_render_loader_device(device->device, !device->headless);
	voe_render_vk.get_device_queue(device->device, device->queue_family, 0,
				       &device->queue);
	return true;
}

// The format the surface will take, which the offscreen targets then take too —
// see the format field in device_internal.h for why those are the same thing
// today and will not stay that way.
bool voe_render_device_choose_format(voe_render_device *device,
				     voe_base_arena *arena)
{
	uint32_t count = 0;
	VkSurfaceFormatKHR *formats;

	// Nothing to ask and nothing to satisfy: a headless device presents to
	// no screen, so it takes the format the loop below would have preferred
	// and is done. The colour space is a property of a surface and means
	// nothing here; it is filled in so the field is never read unset.
	if (device->headless) {
		device->format.format = PREFERRED_FORMAT;
		device->format.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
		return true;
	}

	if (voe_render_vk.get_surface_formats(device->physical, device->surface,
					      &count, NULL) != VK_SUCCESS ||
	    count == 0) {
		VOE_BASE_ERROR("render", "the surface offers no formats");
		return false;
	}

	formats = voe_base_arena_push(arena, (size_t)count * sizeof(*formats));
	if (voe_render_vk.get_surface_formats(device->physical, device->surface,
					      &count, formats) != VK_SUCCESS) {
		VOE_BASE_ERROR("render", "vkGetPhysicalDeviceSurfaceFormatsKHR failed");
		return false;
	}

	// The preferred one, or any other sRGB one, or whatever the surface put
	// first. The second choice matters: a driver that offers R8G8B8A8_SRGB
	// and not B8G8R8A8_SRGB is offering exactly what this engine wants with
	// the channels the other way round, and the swizzle is the driver's
	// problem rather than something to fall off a cliff over.
	device->format = formats[0];
	for (uint32_t i = 0; i < count; i++) {
		if (formats[i].colorSpace != VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			continue;
		if (formats[i].format == PREFERRED_FORMAT) {
			device->format = formats[i];
			return true;
		}
		if (formats[i].format == VK_FORMAT_R8G8B8A8_SRGB)
			device->format = formats[i];
	}

	// A surface with no sRGB format on it draws a frame that is too dark,
	// because nothing then applies the curve the screen expects. It is said
	// out loud rather than refused: the picture is wrong and the program
	// still runs, which is the more useful of the two on a machine nobody
	// here has seen. Every desktop compositor either platform supports
	// offers one.
	if (device->format.format != PREFERRED_FORMAT &&
	    device->format.format != VK_FORMAT_R8G8B8A8_SRGB)
		VOE_BASE_ERROR("render",
			       "this surface offers no sRGB format (taking %d), so the frame will reach the screen too dark",
			       (int)device->format.format);
	return true;
}

// ---------------------------------------------------------- timing and pacing

// What the card's clock is worth, asked once. Two numbers and they are separate
// questions: the period is the card's, out of its limits, and the valid bits are
// the queue family's — a card can write timestamps and the family this device
// took can still be one that does not.
//
// A CARD THAT CANNOT TIME IS NOT A FAILURE AND IS NOT WORTH A MESSAGE AT
// STARTUP. Every desktop card measured writes timestamps; the ones that do not
// are compute-only queues and virtualised drivers, and the answer there is a
// frame loop that reports CPU time and says the GPU number is missing. So this
// returns nothing: it sets three fields and the caller carries on either way.
static void learn_timing(voe_render_device *device, voe_base_arena *arena)
{
	VkPhysicalDeviceProperties properties;
	VkQueueFamilyProperties *families;
	uint32_t count = 0;

	voe_render_vk.get_physical_device_properties(device->physical,
						     &properties);

	// Zero means the card cannot do it at all, and the specification says so
	// in exactly those words.
	if (properties.limits.timestampPeriod == 0.0f)
		return;

	voe_render_vk.get_queue_family_properties(device->physical, &count, NULL);
	if (count == 0 || device->queue_family >= count)
		return;

	families = voe_base_arena_push(arena, (size_t)count * sizeof(*families));
	voe_render_vk.get_queue_family_properties(device->physical, &count,
						  families);

	if (families[device->queue_family].timestampValidBits == 0)
		return;

	device->timestamp_period = properties.limits.timestampPeriod;
	device->timestamp_valid_bits =
		families[device->queue_family].timestampValidBits;
	device->timestamps = true;
}

// Whether this surface offers MAILBOX, asked once for the same reason the format
// is: it is a property of a physical device and a surface, and a resize changes
// neither. A headless device has no surface and presents nothing.
//
// FALSE IS AN ORDINARY ANSWER AND NOT A FAILURE. MAILBOX is optional in the
// specification; only FIFO is required of everyone. So a query that will not
// answer is read as "no mailbox here" and the device stays on the mode every
// driver has to support.
static void learn_present_modes(voe_render_device *device, voe_base_arena *arena)
{
	VkPresentModeKHR *modes;
	uint32_t count = 0;

	if (device->headless)
		return;

	if (voe_render_vk.get_surface_present_modes(device->physical,
						    device->surface, &count,
						    NULL) != VK_SUCCESS ||
	    count == 0)
		return;

	modes = voe_base_arena_push(arena, (size_t)count * sizeof(*modes));
	if (voe_render_vk.get_surface_present_modes(device->physical,
						    device->surface, &count,
						    modes) != VK_SUCCESS)
		return;

	for (uint32_t i = 0; i < count; i++) {
		if (modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
			device->mailbox_offered = true;
			return;
		}
	}
}

void voe_render_present_set(voe_render_device *device, voe_render_present mode)
{
	VOE_BASE_ASSERT(device != NULL, "asking no device to present differently");
	VOE_BASE_ASSERT(mode == VOE_RENDER_PRESENT_FIFO ||
				mode == VOE_RENDER_PRESENT_MAILBOX,
			"asking for a present mode that is not one of the two");

	device->present_wanted = mode;

	// The swapchain is what carries the mode, so changing it is building
	// another one — and that happens at the top of a frame, where every
	// other rebuild happens, because the images the presentation engine is
	// still reading are not ours to destroy from here.
	if (device->present_wanted != device->present_in_force)
		device->rebuild = true;
}

voe_render_present voe_render_present_get(const voe_render_device *device)
{
	VOE_BASE_ASSERT(device != NULL, "asking no device how it presents");

	return device->present_in_force;
}

bool voe_render_frame_gpu_time(const voe_render_device *device, double *seconds)
{
	VOE_BASE_ASSERT(device != NULL, "asking no device what the card's clock said");
	VOE_BASE_ASSERT(seconds != NULL,
			"asking for a GPU time with nowhere to put it");

	if (!device->gpu_measured)
		return false;

	*seconds = device->gpu_seconds;
	return true;
}

// ------------------------------------------------------------- the frame's own

// One of everything per frame slot, and one pool behind all of it. Nothing here
// is indexed by anything but a slot, and the loop below is the whole of that: a
// per-frame resource added by a later card gets a line in it and needs no array
// of its own.
static bool create_frame_objects(voe_render_device *device)
{
	VkCommandPoolCreateInfo pool = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = device->queue_family,
	};
	VkCommandBufferAllocateInfo commands = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = VOE_RENDER_FRAMES_IN_FLIGHT,
	};
	VkCommandBuffer buffers[VOE_RENDER_FRAMES_IN_FLIGHT];
	VkSemaphoreCreateInfo semaphore = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
	};
	// Created signalled, so the first frame on every slot waits on a fence
	// that is already up rather than on a submit that has not happened.
	VkFenceCreateInfo fence = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT,
	};
	// A query pool is created with its queries in an undefined state, not an
	// empty one, which is why every frame resets its own before writing —
	// see frame.c.
	VkQueryPoolCreateInfo queries = {
		.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
		.queryType = VK_QUERY_TYPE_TIMESTAMP,
		.queryCount = VOE_RENDER_TIMESTAMPS_PER_FRAME,
	};

	if (voe_render_vk.create_command_pool(device->device, &pool, NULL,
					      &device->pool) != VK_SUCCESS) {
		VOE_BASE_ERROR("render", "vkCreateCommandPool failed");
		return false;
	}

	// One call for every slot, because vkAllocateCommandBuffers takes a
	// count and there is nothing to be had from asking it that many times.
	// The handles are spread one to a slot below, and the pool takes every
	// one of them back when it is destroyed.
	commands.commandPool = device->pool;
	if (voe_render_vk.allocate_command_buffers(device->device, &commands,
						   buffers) != VK_SUCCESS) {
		VOE_BASE_ERROR("render", "vkAllocateCommandBuffers failed");
		return false;
	}

	for (uint32_t i = 0; i < VOE_RENDER_FRAMES_IN_FLIGHT; i++) {
		device->frames[i].commands = buffers[i];

		if (voe_render_vk.create_semaphore(device->device, &semaphore,
						   NULL,
						   &device->frames[i].acquired) !=
		    VK_SUCCESS) {
			VOE_BASE_ERROR("render", "vkCreateSemaphore failed");
			return false;
		}

		if (voe_render_vk.create_fence(device->device, &fence, NULL,
					       &device->frames[i].submitted) !=
		    VK_SUCCESS) {
			VOE_BASE_ERROR("render", "vkCreateFence failed");
			return false;
		}

		// The two timestamps this slot's frame writes. Only on a card
		// that can write them: everything that touches one of these
		// reads device->timestamps first, so a null handle here is a
		// state and not something to guard against later.
		//
		// A REFUSED QUERY POOL TURNS TIMING OFF RATHER THAN STOPPING
		// STARTUP. A device that draws and cannot say how long it took
		// is worth having; refusing to open one over a measurement would
		// be the tail wagging the dog.
		if (device->timestamps &&
		    voe_render_vk.create_query_pool(device->device, &queries,
						    NULL,
						    &device->frames[i].timestamps) !=
			    VK_SUCCESS) {
			VOE_BASE_ERROR("render",
				       "vkCreateQueryPool failed, so there will be no GPU timings");
			device->timestamps = false;
		}
	}

	return true;
}

// ------------------------------------------------------------ open and shut

// Every failure below lands here, so a half-open device is torn down by the same
// code that tears down a whole one. Order is the reverse of creation, and every
// step checks for the handle being absent because that is what a failure halfway
// leaves behind.
static void close_down(voe_render_device *device)
{
	if (device->device != VK_NULL_HANDLE) {
		voe_render_vk.device_wait_idle(device->device);
		voe_render_swapchain_teardown(device);
		voe_render_target_teardown(device);
		// Before the table that holds the targets' volumes goes.
		voe_render_bounce_volume_teardown(device, &device->window_volume);
		for (uint32_t i = 0; device->targets != NULL &&
				     i < device->capacities.targets;
		     i++)
			voe_render_bounce_volume_teardown(device,
							  &device->targets[i].volume);
		voe_render_targets_shutdown(device);

		// Before the layout below, which it shares.
		voe_render_element_shutdown(device);

		if (device->pipeline_capture != VK_NULL_HANDLE)
			voe_render_vk.destroy_pipeline(device->device,
						       device->pipeline_capture,
						       NULL);
		if (device->pipeline_point_shadow != VK_NULL_HANDLE)
			voe_render_vk.destroy_pipeline(device->device,
						       device->pipeline_point_shadow,
						       NULL);
		if (device->pipeline_bounce != VK_NULL_HANDLE)
			voe_render_vk.destroy_pipeline(device->device,
						       device->pipeline_bounce,
						       NULL);
		if (device->pipeline_shadow != VK_NULL_HANDLE)
			voe_render_vk.destroy_pipeline(device->device,
						       device->pipeline_shadow,
						       NULL);
		if (device->pipeline_blended != VK_NULL_HANDLE)
			voe_render_vk.destroy_pipeline(device->device,
						       device->pipeline_blended,
						       NULL);
		if (device->pipeline != VK_NULL_HANDLE)
			voe_render_vk.destroy_pipeline(device->device,
						       device->pipeline, NULL);
		if (device->layout != VK_NULL_HANDLE)
			voe_render_vk.destroy_pipeline_layout(device->device,
							      device->layout, NULL);
		// After the pipeline and its layout, because the layout named the
		// descriptor set layout this takes away. Before the command pool,
		// because nothing in here needs one and the order still reads the
		// way the build order reversed.
		voe_render_texture_shutdown(device);
		voe_render_geometry_shutdown(device);
		voe_render_shading_shutdown(device);
		voe_render_descriptors_teardown(device);
		voe_render_shadow_shutdown(device);
		voe_render_point_shadow_shutdown(device);
		voe_render_bounce_capture_shutdown(device);
		voe_render_bounce_grid_shutdown(device);
		voe_render_bounce_shutdown(device);
		// The command buffers are not freed one at a time: destroying the
		// pool below takes every one of them with it.
		for (uint32_t i = 0; i < VOE_RENDER_FRAMES_IN_FLIGHT; i++) {
			if (device->frames[i].timestamps != VK_NULL_HANDLE)
				voe_render_vk.destroy_query_pool(device->device,
								 device->frames[i].timestamps,
								 NULL);
			if (device->frames[i].submitted != VK_NULL_HANDLE)
				voe_render_vk.destroy_fence(device->device,
							    device->frames[i].submitted,
							    NULL);
			if (device->frames[i].acquired != VK_NULL_HANDLE)
				voe_render_vk.destroy_semaphore(device->device,
								device->frames[i].acquired,
								NULL);
		}
		if (device->pool != VK_NULL_HANDLE)
			voe_render_vk.destroy_command_pool(device->device,
							   device->pool, NULL);
		voe_render_vk.destroy_device(device->device, NULL);
	}

	if (device->surface != VK_NULL_HANDLE)
		voe_render_vk.destroy_surface(device->instance, device->surface,
					      NULL);
	if (device->messenger != VK_NULL_HANDLE)
		voe_render_vk.destroy_debug_messenger(device->instance,
						      device->messenger, NULL);
	if (device->instance != VK_NULL_HANDLE)
		voe_render_vk.destroy_instance(device->instance, NULL);

	free(device);
	voe_render_loader_close();
}

static voe_render_device *open_failed(voe_render_device *device,
				      voe_base_error *error, voe_base_error code)
{
	report(error, code);
	close_down(device);
	return NULL;
}

// Both entry points below, and the one difference between them is headless. Read
// the six places it is asked about from the note at the top of this file; every
// other line here runs identically either way.
static voe_render_device *open_device(voe_base_arena *arena,
				      voe_platform_native native,
				      voe_platform_size size,
				      voe_render_capacities capacities,
				      voe_base_error *error, bool headless)
{
	voe_render_device *device;

	VOE_BASE_ASSERT(arena != NULL, "opening a device without an arena");
	VOE_BASE_ASSERT(capacities.vertices > 0 && capacities.indices > 0 &&
				capacities.geometries > 0 &&
				capacities.objects > 0 &&
				capacities.shadings > 0,
			"opening a device with room for nothing — every capacity is a number the caller has to choose");
	VOE_BASE_ASSERT(capacities.passes > 0,
			"opening a device with room for no passes — a frame draws nothing outside one, so `passes` is at least one");

	report(error, VOE_BASE_OK);

	device = calloc(1, sizeof(*device));
	VOE_BASE_ASSERT(device != NULL, "out of memory opening a device");
	device->headless = headless;
	device->capacities = capacities;

	// No Vulkan on the machine at all. The one failure a person can fix by
	// installing something, and the reason this function returns a pointer
	// that can be NULL rather than asserting.
	if (!voe_render_loader_open())
		return open_failed(device, error, VOE_BASE_ERROR_UNAVAILABLE);

	if (!voe_render_instance_create(device, arena))
		return open_failed(device, error, VOE_BASE_ERROR_REFUSED);

	if (!headless) {
		device->surface = voe_render_backend_surface_new(device->instance,
								 native);
		if (device->surface == VK_NULL_HANDLE)
			return open_failed(device, error, VOE_BASE_ERROR_REFUSED);
	}

	// The machine has Vulkan and no card on it can do what we need. That is
	// a different answer from the two above and a caller may want to say so
	// differently, which is why it is a category of its own.
	if (!voe_render_card_choose(device, arena))
		return open_failed(device, error, VOE_BASE_ERROR_UNSUPPORTED);

	if (!create_device(device))
		return open_failed(device, error, VOE_BASE_ERROR_REFUSED);
	if (!voe_render_device_choose_format(device, arena))
		return open_failed(device, error, VOE_BASE_ERROR_REFUSED);

	// Both ask the physical device and the surface questions whose answers
	// do not change while this device is open, so they are asked here rather
	// than on the frame or the resize that wants them. Neither can fail: a
	// card that cannot time and a surface with no mailbox are answers.
	// learn_timing is before create_frame_objects because that is what
	// decides whether there are query pools to make.
	learn_timing(device, arena);
	learn_present_modes(device, arena);
	// THE REST OF STARTUP IS IN THIS ORDER AND THE ORDER IS FORCED, NOT
	// PREFERRED. The frame objects come first because the command pool is one
	// of them and every staging upload below records into a command buffer
	// out of it. The descriptors come next, because the sets are what the
	// shading buffer's and the textures' descriptors are written into — so
	// both of those have to exist after the sets do, and the two writes after
	// that. The pipelines come last because their layout names the descriptor
	// set layout: build them first and it names a handle that is still null.
	if (!create_frame_objects(device))
		return open_failed(device, error, VOE_BASE_ERROR_REFUSED);
	// The shadow maps are settled through the command pool and named by the
	// descriptors, so they sit between the two.
	if (!voe_render_shadow_startup(device))
		return open_failed(device, error, VOE_BASE_ERROR_REFUSED);
	if (!voe_render_point_shadow_startup(device))
		return open_failed(device, error, VOE_BASE_ERROR_REFUSED);
	if (!voe_render_bounce_capture_startup(device))
		return open_failed(device, error, VOE_BASE_ERROR_REFUSED);
	if (!voe_render_bounce_startup(device))
		return open_failed(device, error, VOE_BASE_ERROR_REFUSED);
	if (!voe_render_bounce_grid_startup(device))
		return open_failed(device, error, VOE_BASE_ERROR_REFUSED);

	if (!voe_render_descriptors_build(device))
		return open_failed(device, error, VOE_BASE_ERROR_REFUSED);
	if (!voe_render_shading_startup(device))
		return open_failed(device, error, VOE_BASE_ERROR_REFUSED);
	if (!voe_render_geometry_startup(device))
		return open_failed(device, error, VOE_BASE_ERROR_REFUSED);
	if (!voe_render_texture_startup(device))
		return open_failed(device, error, VOE_BASE_ERROR_REFUSED);
	for (uint32_t i = 0; i < VOE_RENDER_FRAMES_IN_FLIGHT; i++) {
		voe_render_descriptors_write_shadings(device,
						      device->frames[i].descriptor);
		voe_render_texture_write_descriptors(device, i);
	}
	voe_render_targets_startup(device);
	if (!voe_render_pipelines_create(device))
		return open_failed(device, error, VOE_BASE_ERROR_REFUSED);
	// After them, and not beside them: the element pipeline shares the
	// layout the two above make, so it cannot be built until one of them
	// has.
	if (!voe_render_element_startup(device))
		return open_failed(device, error, VOE_BASE_ERROR_REFUSED);
	// Targets before the swapchain, because the targets are the resolution
	// and the swapchain is only where a frame is copied at the end. A device
	// whose targets could not be made cannot draw, and there would be
	// nothing to present.
	if (!voe_render_target_build(device, size))
		return open_failed(device, error, VOE_BASE_ERROR_REFUSED);
	if (!voe_render_swapchain_build(device, size))
		return open_failed(device, error, VOE_BASE_ERROR_REFUSED);

	return device;
}

voe_render_device *voe_render_device_new(voe_base_arena *arena,
					 voe_platform_native native,
					 voe_platform_size size,
					 voe_render_capacities capacities,
					 voe_base_error *error)
{
	VOE_BASE_DEBUG_ASSERT(native.window != 0,
			      "opening a device on a window that is not there");

	return open_device(arena, native, size, capacities, error, false);
}

voe_render_device *voe_render_device_new_headless(voe_base_arena *arena,
						  voe_platform_size size,
						  voe_render_capacities capacities,
						  voe_base_error *error)
{
	// A native window that is not there, and nothing reads it: the surface
	// is the one thing it would have been for and headless does not make
	// one.
	voe_platform_native native = { 0 };

	return open_device(arena, native, size, capacities, error, true);
}

void voe_render_device_destroy(voe_render_device *device)
{
	VOE_BASE_DEBUG_ASSERT(device != NULL, "destroying a NULL device");

	close_down(device);
}
