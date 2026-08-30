// Starting the GPU: the loader, the instance, the surface, the graphics card and
// the logical device, in that order, because each one is what the next is asked
// for. Everything here happens once. The parts that happen again live in
// swapchain.c and frame.c — see device_internal.h for why the split is where it
// is.
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
#include "device_internal.h"

#include "backend.h"

#include <render/device.h>

#include <base/assert.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VALIDATION_LAYER "VK_LAYER_KHRONOS_validation"

// Debug builds only. The layer costs real time on every call, and a release
// build is what a person runs to see how fast the engine is.
#ifdef NDEBUG
#define VALIDATION_IN_THIS_BUILD false
#else
#define VALIDATION_IN_THIS_BUILD true
#endif

// Vulkan 1.3 is the floor: dynamic rendering and synchronization2 are core in
// it, and both are what this folder is written against.
#define REQUIRED_VERSION VK_API_VERSION_1_3

static void report(voe_base_error *error, voe_base_error code)
{
	if (error != NULL)
		*error = code;
}

// ------------------------------------------------------------------ validation

// Vulkan calls this, so it is shaped the way Vulkan wants rather than the way
// this engine writes functions. It prints and returns VK_FALSE, which is the
// only value a messenger callback is allowed to return outside a layer's own
// tests.
static VKAPI_ATTR VkBool32 VKAPI_CALL
debug_message(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	      VkDebugUtilsMessageTypeFlagsEXT types,
	      const VkDebugUtilsMessengerCallbackDataEXT *data, void *user)
{
	(void)types;
	(void)user;

	fprintf(stderr, "vulkan %s: %s\n",
		severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT ? "error" :
		severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT ? "warning" :
									     "info",
		data->pMessage);
	return VK_FALSE;
}

static bool has_layer(voe_base_arena *arena, const char *name)
{
	uint32_t count = 0;
	VkLayerProperties *layers;

	if (voe_render_vk.enumerate_instance_layers(&count, NULL) != VK_SUCCESS)
		return false;
	if (count == 0)
		return false;

	layers = voe_base_arena_push(arena, (size_t)count * sizeof(*layers));
	if (voe_render_vk.enumerate_instance_layers(&count, layers) != VK_SUCCESS)
		return false;

	for (uint32_t i = 0; i < count; i++) {
		if (strcmp(layers[i].layerName, name) == 0)
			return true;
	}
	return false;
}

static bool has_instance_extension(voe_base_arena *arena, const char *name)
{
	uint32_t count = 0;
	VkExtensionProperties *extensions;

	if (voe_render_vk.enumerate_instance_extensions(NULL, &count, NULL) !=
	    VK_SUCCESS)
		return false;
	if (count == 0)
		return false;

	extensions = voe_base_arena_push(arena,
					 (size_t)count * sizeof(*extensions));
	if (voe_render_vk.enumerate_instance_extensions(NULL, &count,
							extensions) != VK_SUCCESS)
		return false;

	for (uint32_t i = 0; i < count; i++) {
		if (strcmp(extensions[i].extensionName, name) == 0)
			return true;
	}
	return false;
}

// Debug builds only, and only when the layer and the extension are both
// installed. Neither is required of anyone: they come with the Vulkan SDK, the
// SDK is optional, and a machine without one draws exactly the same picture. So
// this is silent when it finds nothing — an absent layer is not a warning, it is
// the normal case.
static bool wants_validation(voe_base_arena *arena)
{
	// A constant and not an #ifdef around the body, so that both calls below
	// stay compiled and type-checked in a release build. Wrapping the body
	// instead leaves two functions nothing calls, which -Werror is right to
	// object to and which would have been fixed by deleting the check.
	if (!VALIDATION_IN_THIS_BUILD)
		return false;

	return has_layer(arena, VALIDATION_LAYER) &&
	       has_instance_extension(arena, VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
}

static void attach_messenger(voe_render_device *device)
{
	VkDebugUtilsMessengerCreateInfoEXT info = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
		.messageSeverity =
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
		.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			       VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
		.pfnUserCallback = debug_message,
	};

	if (voe_render_vk.create_debug_messenger == NULL)
		return;

	// A messenger that will not attach costs us the messages and nothing
	// else, so it is not a failure to open a device over.
	if (voe_render_vk.create_debug_messenger(device->instance, &info, NULL,
						 &device->messenger) != VK_SUCCESS)
		device->messenger = VK_NULL_HANDLE;
}

// -------------------------------------------------------------------- instance

static bool create_instance(voe_render_device *device, voe_base_arena *arena)
{
	const char *extensions[3];
	uint32_t extension_count = 0;
	const char *layers[1] = { VALIDATION_LAYER };
	bool validate = wants_validation(arena);
	VkApplicationInfo application = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = "voe3d",
		.apiVersion = REQUIRED_VERSION,
	};
	VkInstanceCreateInfo info = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &application,
	};
	VkResult result;

	extensions[extension_count++] = VK_KHR_SURFACE_EXTENSION_NAME;
	extensions[extension_count++] = voe_render_backend_extension();
	if (validate)
		extensions[extension_count++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

	info.enabledExtensionCount = extension_count;
	info.ppEnabledExtensionNames = extensions;
	info.enabledLayerCount = validate ? 1 : 0;
	info.ppEnabledLayerNames = validate ? layers : NULL;

	result = voe_render_vk.create_instance(&info, NULL, &device->instance);
	if (result != VK_SUCCESS) {
		fprintf(stderr,
			"render: vkCreateInstance failed (VkResult %d) with %s enabled\n",
			(int)result, voe_render_backend_extension());
		return false;
	}

	voe_render_loader_instance(device->instance);
	if (validate)
		attach_messenger(device);
	return true;
}

// -------------------------------------------------------------- graphics cards

// A queue family that can do both, because this engine has one queue and no
// reason yet to have two. VK_QUEUE_MAX_ENUM is the "none of them" answer, which
// is safe because it is not a family index anything could return.
static uint32_t present_and_graphics_family(voe_render_device *device,
					    VkPhysicalDevice physical,
					    voe_base_arena *arena)
{
	uint32_t count = 0;
	VkQueueFamilyProperties *families;

	voe_render_vk.get_queue_family_properties(physical, &count, NULL);
	if (count == 0)
		return UINT32_MAX;

	families = voe_base_arena_push(arena, (size_t)count * sizeof(*families));
	voe_render_vk.get_queue_family_properties(physical, &count, families);

	for (uint32_t i = 0; i < count; i++) {
		VkBool32 presents = VK_FALSE;

		if ((families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0)
			continue;
		if (voe_render_vk.get_surface_support(physical, i,
						      device->surface,
						      &presents) != VK_SUCCESS)
			continue;
		if (presents)
			return i;
	}
	return UINT32_MAX;
}

// Discrete first, then anything else that qualifies. "Qualifies" is the whole of
// what this engine requires and it is two things: the card claims Vulkan 1.3, and
// one of its queue families can both render and present to our surface.
static bool choose_physical_device(voe_render_device *device,
				   voe_base_arena *arena)
{
	uint32_t count = 0;
	VkPhysicalDevice *cards;
	VkPhysicalDevice best = VK_NULL_HANDLE;
	uint32_t best_family = UINT32_MAX;
	bool best_is_discrete = false;

	if (voe_render_vk.enumerate_physical_devices(device->instance, &count,
						     NULL) != VK_SUCCESS ||
	    count == 0) {
		fprintf(stderr, "render: the Vulkan instance reports no graphics cards\n");
		return false;
	}

	cards = voe_base_arena_push(arena, (size_t)count * sizeof(*cards));
	if (voe_render_vk.enumerate_physical_devices(device->instance, &count,
						     cards) != VK_SUCCESS) {
		fprintf(stderr, "render: vkEnumeratePhysicalDevices failed\n");
		return false;
	}

	for (uint32_t i = 0; i < count; i++) {
		VkPhysicalDeviceProperties properties;
		uint32_t family;
		bool discrete;

		voe_render_vk.get_physical_device_properties(cards[i], &properties);
		if (properties.apiVersion < REQUIRED_VERSION)
			continue;

		family = present_and_graphics_family(device, cards[i], arena);
		if (family == UINT32_MAX)
			continue;

		// Keep the first card that qualifies, and give it up only for a
		// discrete one. Two discrete cards: the first wins, because
		// there is nothing here that could tell them apart.
		discrete = properties.deviceType ==
			   VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
		if (best != VK_NULL_HANDLE && (best_is_discrete || !discrete))
			continue;

		best = cards[i];
		best_family = family;
		best_is_discrete = discrete;
	}

	if (best == VK_NULL_HANDLE) {
		fprintf(stderr,
			"render: none of the %u graphics cards on this machine offers Vulkan 1.3 and a queue that can present to this window\n",
			count);
		return false;
	}

	device->physical = best;
	device->queue_family = best_family;
	return true;
}

static void say_which_card(voe_render_device *device)
{
	VkPhysicalDeviceProperties properties;

	voe_render_vk.get_physical_device_properties(device->physical, &properties);
	printf("render     %s, Vulkan %u.%u.%u, queue family %u\n",
	       properties.deviceName,
	       VK_API_VERSION_MAJOR(properties.apiVersion),
	       VK_API_VERSION_MINOR(properties.apiVersion),
	       VK_API_VERSION_PATCH(properties.apiVersion),
	       device->queue_family);
	fflush(stdout);
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
	// Core in 1.3 and supported by every 1.3 implementation, but still off
	// until asked for: a feature that is core is guaranteed available, not
	// guaranteed enabled.
	VkPhysicalDeviceVulkan13Features features13 = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
		.synchronization2 = VK_TRUE,
		.dynamicRendering = VK_TRUE,
	};
	VkDeviceCreateInfo info = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = &features13,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &queue,
		.enabledExtensionCount = 1,
		.ppEnabledExtensionNames = extensions,
	};
	VkResult result;

	result = voe_render_vk.create_device(device->physical, &info, NULL,
					     &device->device);
	if (result != VK_SUCCESS) {
		fprintf(stderr, "render: vkCreateDevice failed (VkResult %d)\n",
			(int)result);
		return false;
	}

	voe_render_loader_device(device->device);
	voe_render_vk.get_device_queue(device->device, device->queue_family, 0,
				       &device->queue);
	return true;
}

// UNORM and not SRGB, so that the colour a clear is given is the colour that
// reaches the screen. An SRGB swapchain would convert it, which is right the day
// this engine renders in linear light and wrong today, when the only thing it
// draws is one colour that a person is looking at to see whether the GPU is
// working.
bool voe_render_device_choose_format(voe_render_device *device,
				     voe_base_arena *arena)
{
	uint32_t count = 0;
	VkSurfaceFormatKHR *formats;

	if (voe_render_vk.get_surface_formats(device->physical, device->surface,
					      &count, NULL) != VK_SUCCESS ||
	    count == 0) {
		fprintf(stderr, "render: the surface offers no formats\n");
		return false;
	}

	formats = voe_base_arena_push(arena, (size_t)count * sizeof(*formats));
	if (voe_render_vk.get_surface_formats(device->physical, device->surface,
					      &count, formats) != VK_SUCCESS) {
		fprintf(stderr, "render: vkGetPhysicalDeviceSurfaceFormatsKHR failed\n");
		return false;
	}

	device->format = formats[0];
	for (uint32_t i = 0; i < count; i++) {
		if (formats[i].format == VK_FORMAT_B8G8R8A8_UNORM &&
		    formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			device->format = formats[i];
			break;
		}
	}
	return true;
}

// ------------------------------------------------------------- the frame's own

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
		.commandBufferCount = 1,
	};
	VkSemaphoreCreateInfo semaphore = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
	};
	// Created signalled, so the first frame's wait returns immediately
	// rather than waiting for a submit that has not happened.
	VkFenceCreateInfo fence = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT,
	};

	if (voe_render_vk.create_command_pool(device->device, &pool, NULL,
					      &device->pool) != VK_SUCCESS) {
		fprintf(stderr, "render: vkCreateCommandPool failed\n");
		return false;
	}

	commands.commandPool = device->pool;
	if (voe_render_vk.allocate_command_buffers(device->device, &commands,
						   &device->commands) != VK_SUCCESS) {
		fprintf(stderr, "render: vkAllocateCommandBuffers failed\n");
		return false;
	}

	if (voe_render_vk.create_semaphore(device->device, &semaphore, NULL,
					   &device->acquired) != VK_SUCCESS) {
		fprintf(stderr, "render: vkCreateSemaphore failed\n");
		return false;
	}

	if (voe_render_vk.create_fence(device->device, &fence, NULL,
				       &device->submitted) != VK_SUCCESS) {
		fprintf(stderr, "render: vkCreateFence failed\n");
		return false;
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

		if (device->submitted != VK_NULL_HANDLE)
			voe_render_vk.destroy_fence(device->device,
						    device->submitted, NULL);
		if (device->acquired != VK_NULL_HANDLE)
			voe_render_vk.destroy_semaphore(device->device,
							device->acquired, NULL);
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

voe_render_device *voe_render_device_new(voe_base_arena *arena,
					 voe_platform_native native,
					 voe_platform_size size,
					 voe_base_error *error)
{
	voe_render_device *device;

	VOE_BASE_ASSERT(arena != NULL, "opening a device without an arena");
	VOE_BASE_DEBUG_ASSERT(native.window != 0,
			      "opening a device on a window that is not there");

	report(error, VOE_BASE_OK);

	device = calloc(1, sizeof(*device));
	VOE_BASE_ASSERT(device != NULL, "out of memory opening a device");

	// No Vulkan on the machine at all. The one failure a person can fix by
	// installing something, and the reason this function returns a pointer
	// that can be NULL rather than asserting.
	if (!voe_render_loader_open())
		return open_failed(device, error, VOE_BASE_ERROR_UNAVAILABLE);

	if (!create_instance(device, arena))
		return open_failed(device, error, VOE_BASE_ERROR_REFUSED);

	device->surface = voe_render_backend_surface_new(device->instance, native);
	if (device->surface == VK_NULL_HANDLE)
		return open_failed(device, error, VOE_BASE_ERROR_REFUSED);

	// The machine has Vulkan and no card on it can do what we need. That is
	// a different answer from the two above and a caller may want to say so
	// differently, which is why it is a category of its own.
	if (!choose_physical_device(device, arena))
		return open_failed(device, error, VOE_BASE_ERROR_UNSUPPORTED);

	if (!create_device(device))
		return open_failed(device, error, VOE_BASE_ERROR_REFUSED);
	if (!voe_render_device_choose_format(device, arena))
		return open_failed(device, error, VOE_BASE_ERROR_REFUSED);
	if (!create_frame_objects(device))
		return open_failed(device, error, VOE_BASE_ERROR_REFUSED);
	if (!voe_render_swapchain_build(device, size))
		return open_failed(device, error, VOE_BASE_ERROR_REFUSED);

	say_which_card(device);
	return device;
}

void voe_render_device_destroy(voe_render_device *device)
{
	VOE_BASE_DEBUG_ASSERT(device != NULL, "destroying a NULL device");

	close_down(device);
}
