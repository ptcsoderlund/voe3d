// The Vulkan instance: the extensions and the layer it is created with, and the
// validation messenger attached to it when there is one. A step of startup;
// device.c's open_device calls voe_render_instance_create once, after the
// loader is open and before the surface exists — see startup.h for the order.
//
// VALIDATION IS DEBUG-ONLY AND NEVER REQUIRED. The layer and the debug-utils
// extension come with the Vulkan SDK, which nobody has to install. A debug build
// asks for both when both are there and says nothing when they are not; a
// release build never asks. What the layer reports is printed through base's
// report, one line per message.
//
// A HEADLESS INSTANCE NAMES NO WINDOW SYSTEM. This is the first of the places a
// headless device differs (device.c's header lists them): no surface extension,
// so a build box with no compositor can still open one.
#include "startup.h"

#include "backend.h"

#include <base/report.h>

#include <string.h>

#define VALIDATION_LAYER "VK_LAYER_KHRONOS_validation"

// Debug builds only. The layer costs real time on every call, and a release
// build is what a person runs to see how fast the engine is.
#ifdef NDEBUG
#define VALIDATION_IN_THIS_BUILD false
#else
#define VALIDATION_IN_THIS_BUILD true
#endif

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

	// DEVIATION: card 062 "every one of these is VOE_BASE_ERROR", read as
	// applying to the layer's own warnings too, so the layer's severity stays a
	// word in the message; mapping it onto the report's level is a judgement the
	// card reserves.
	VOE_BASE_ERROR("render", "vulkan %s: %s",
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

bool voe_render_instance_create(voe_render_device *device, voe_base_arena *arena)
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

	// A headless instance names no window system at all. It opens no surface,
	// so these two would be enabled and never called — and on a build box
	// with no compositor the platform's one is not there to enable, which is
	// what would otherwise stop the offscreen test running headless.
	if (!device->headless) {
		extensions[extension_count++] = VK_KHR_SURFACE_EXTENSION_NAME;
		extensions[extension_count++] = voe_render_backend_extension();
	}
	if (validate)
		extensions[extension_count++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

	info.enabledExtensionCount = extension_count;
	info.ppEnabledExtensionNames = extensions;
	info.enabledLayerCount = validate ? 1 : 0;
	info.ppEnabledLayerNames = validate ? layers : NULL;

	result = voe_render_vk.create_instance(&info, NULL, &device->instance);
	if (result != VK_SUCCESS) {
		// Named one per line rather than summarised, because which of
		// them the driver would not have is the whole of the answer.
		VOE_BASE_ERROR("render",
			       "vkCreateInstance failed (VkResult %d) with these extensions asked for:",
			       (int)result);
		for (uint32_t i = 0; i < extension_count; i++)
			VOE_BASE_ERROR("render", "    %s", extensions[i]);
		return false;
	}

	voe_render_loader_instance(device->instance, !device->headless);
	if (validate)
		attach_messenger(device);
	return true;
}
