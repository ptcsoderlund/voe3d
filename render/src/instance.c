// The Vulkan instance: the extensions and the layer it is created with, and the
// validation messenger attached to it when there is one. A step of startup;
// device.c's open_device calls voe_render_instance_create once, after the
// loader is open and before the surface exists — see startup.h for the order.
//
// VALIDATION IS DEBUG-ONLY. A release build never asks for the layer. A debug
// build asks for it with Best Practices on (ADR-0358, 0367 point 4): through
// VK_EXT_layer_settings, asked of the layer by name, with the four vendor sets
// (AMD, Arm, IMG, NVIDIA) when the layer offers it, else core Best Practices
// through VkValidationFeaturesEXT; device->checks_on and ->vendor_checks say
// which. A debug build without the layer still runs, and records the checks as
// missing for best_practices.c's start line to say so.
//
// A NEW MESSAGE NOW COUNTS (0358). The messenger's user data is the device;
// every message goes to best_practices.c's classifier, which drops another
// vendor's and counts an error or a warning not on the allowlist as new. What
// is kept is printed through base's report, one line with its id name and
// whether it was allowed.
//
// DEBUG-UTILS IS ASKED FOR APART FROM VALIDATION, in every build, whenever the
// instance offers it (ADR-0367): a capture tool reads the names and labels
// debug_names.c gives buffers, images and passes, and a release build is the
// one worth capturing. The messenger still needs both the layer and it.
//
// A HEADLESS INSTANCE NAMES NO WINDOW SYSTEM. This is the first of the places a
// headless device differs (device.c's header lists them): no surface extension,
// so a build box with no compositor can still open one.
#include "startup.h"

#include "backend.h"

#include <base/report.h>

#include <assert.h>
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

// The Best Practices settings the layer is handed when it offers
// VK_EXT_layer_settings: core and the four vendor sets, each a VkBool32 true.
static const char *const best_practices_settings[5] = {
	"validate_best_practices",     "validate_best_practices_amd",
	"validate_best_practices_arm", "validate_best_practices_img",
	"validate_best_practices_nvidia",
};

// Vulkan calls this, so it is shaped the way Vulkan wants rather than the way
// this engine writes functions. `user` is the device. It classifies, prints
// what is kept and returns VK_FALSE, which is the only value a messenger
// callback is allowed to return outside a layer's own tests.
static VKAPI_ATTR VkBool32 VKAPI_CALL
debug_message(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	      VkDebugUtilsMessageTypeFlagsEXT types,
	      const VkDebugUtilsMessengerCallbackDataEXT *data, void *user)
{
	const bool error = severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	enum voe_render_message_verdict verdict;

	(void)types;
	assert(user != NULL);
	assert(data != NULL);

	verdict = voe_render_best_practices_classify(user, error,
						     data->pMessageIdName);
	if (verdict == VOE_RENDER_MESSAGE_DROPPED)
		return VK_FALSE;

	// DEVIATION: card 062 "every one of these is VOE_BASE_ERROR", read as
	// applying to the layer's own warnings too, so the layer's severity stays a
	// word in the message; mapping it onto the report's level is a judgement the
	// card reserves.
	VOE_BASE_ERROR("render", "vulkan %s %s (%s): %s",
		       error ? "error" : "warning",
		       data->pMessageIdName != NULL ? data->pMessageIdName : "(no id)",
		       verdict == VOE_RENDER_MESSAGE_ALLOWED ? "allowed" : "new",
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

// `layer` NULL asks the implementation and the implicit layers; a layer's name
// asks that layer for the extensions it provides itself.
static bool has_instance_extension(voe_base_arena *arena, const char *layer,
				   const char *name)
{
	uint32_t count = 0;
	VkExtensionProperties *extensions;

	if (voe_render_vk.enumerate_instance_extensions(layer, &count, NULL) !=
	    VK_SUCCESS)
		return false;
	if (count == 0)
		return false;

	extensions = voe_base_arena_push(arena,
					 (size_t)count * sizeof(*extensions));
	if (voe_render_vk.enumerate_instance_extensions(layer, &count,
							extensions) != VK_SUCCESS)
		return false;

	for (uint32_t i = 0; i < count; i++) {
		if (strcmp(extensions[i].extensionName, name) == 0)
			return true;
	}
	return false;
}

// Debug builds only, and only when the layer and the extension are both
// installed. A debug build without them still draws the same picture; the
// caller records the checks as missing and best_practices.c says so.
static bool wants_validation(voe_base_arena *arena)
{
	// A constant and not an #ifdef around the body, so that both calls below
	// stay compiled and type-checked in a release build. Wrapping the body
	// instead leaves two functions nothing calls, which -Werror is right to
	// object to and which would have been fixed by deleting the check.
	if (!VALIDATION_IN_THIS_BUILD)
		return false;

	return has_layer(arena, VALIDATION_LAYER) &&
	       has_instance_extension(arena, NULL,
				      VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
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
		.pUserData = device,
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

// The Best Practices half of the create info, chained onto `info` (ADR-0367
// point 4): layer settings with the vendor sets when the layer offers them,
// else validation features with the one bit. The structs are the caller's, so
// they outlive the vkCreateInstance they are chained into.
static void turn_on_best_practices(voe_render_device *device,
				   voe_base_arena *arena,
				   VkInstanceCreateInfo *info,
				   const char **extensions,
				   uint32_t *extension_count,
				   VkLayerSettingEXT *settings,
				   VkLayerSettingsCreateInfoEXT *layer_settings,
				   VkValidationFeaturesEXT *features)
{
	static const VkBool32 on = VK_TRUE;
	static const VkValidationFeatureEnableEXT best_practices =
		VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT;

	assert(device != NULL);
	assert(info->pNext == NULL);

	device->checks_on = true;
	device->vendor_checks =
		has_instance_extension(arena, VALIDATION_LAYER,
				       VK_EXT_LAYER_SETTINGS_EXTENSION_NAME);
	if (device->vendor_checks) {
		for (uint32_t i = 0; i < 5; i++)
			settings[i] = (VkLayerSettingEXT){
				.pLayerName = VALIDATION_LAYER,
				.pSettingName = best_practices_settings[i],
				.type = VK_LAYER_SETTING_TYPE_BOOL32_EXT,
				.valueCount = 1,
				.pValues = &on,
			};
		*layer_settings = (VkLayerSettingsCreateInfoEXT){
			.sType = VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT,
			.settingCount = 5,
			.pSettings = settings,
		};
		extensions[(*extension_count)++] =
			VK_EXT_LAYER_SETTINGS_EXTENSION_NAME;
		info->pNext = layer_settings;
		return;
	}

	*features = (VkValidationFeaturesEXT){
		.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
		.enabledValidationFeatureCount = 1,
		.pEnabledValidationFeatures = &best_practices,
	};
	if (has_instance_extension(arena, VALIDATION_LAYER,
				   VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME))
		extensions[(*extension_count)++] =
			VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME;
	info->pNext = features;
	assert(info->pNext != NULL);
}

bool voe_render_instance_create(voe_render_device *device, voe_base_arena *arena)
{
	const char *extensions[4];
	uint32_t extension_count = 0;
	const char *layers[1] = { VALIDATION_LAYER };
	bool validate = wants_validation(arena);
	bool debug_utils = validate ||
			   has_instance_extension(arena, NULL,
						  VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	VkLayerSettingEXT settings[5];
	VkLayerSettingsCreateInfoEXT layer_settings;
	VkValidationFeaturesEXT features;
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
	if (debug_utils)
		extensions[extension_count++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

	if (validate)
		turn_on_best_practices(device, arena, &info, extensions,
				       &extension_count, settings,
				       &layer_settings, &features);
	device->checks_missing = VALIDATION_IN_THIS_BUILD && !validate;

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
