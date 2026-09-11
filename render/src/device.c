// Starting the GPU: the loader, the instance, the surface, the graphics card, the
// logical device and the pipeline, in that order, because each one is what the
// next is asked for. Everything here happens once. The parts that happen again
// live in target.c, swapchain.c and frame.c — see device_internal.h for why the
// split is where it is.
//
// THE SHADER IS IN THIS FILE, AS BYTES. slangc compiles shaders/draw.slang into
// the build tree and #embed puts the result in the binary below; nothing is read
// from disk at run time and there is no shader path to get wrong on someone
// else's machine. The pipeline is here rather than in a file of its own because
// this file is everything with a startup lifetime, and a pipeline that no resize
// touches has one.
//
// THE PIPELINE NAMES THINGS THE OTHER STARTUP FILES OWN. Its layout names the
// descriptor set layout descriptors.c builds, and its vertex input describes
// voe_render_vertex — which is why voe_render_descriptors_build runs before
// create_pipelines in open_device below and not after it, and why that order is
// commented there rather than left to be rediscovered.
//
// THERE ARE TWO OF THEM AND THEY DIFFER IN THREE LINES. The solid one writes
// depth and does not blend; the blended one tests depth the same way, writes
// none, and blends premultiplied. Everything else — the shader module, the
// vertex input, the raster state, the layout — is one description built once and
// handed to both, which is what keeps the two from drifting apart: see
// create_pipeline's `blended` parameter, which is the whole of the difference.
//
// A THIRD PIPELINE IS NOT BUILT HERE AND IT IS NOT A VARIANT OF THESE TWO.
// element.c builds it: no vertex input at all, a triangle strip, nothing culled
// and its own shader, which is four differences and nothing left of the shared
// description. What it does share is the layout below, which is why
// voe_render_element_startup runs after create_pipelines in open_device and why
// this file's push constant range is a matrix wide rather than a word.
//
// DEPTH IS SET UP HERE AND IT RUNS BACKWARDS. GREATER, not LESS, because the
// near plane is at 1.0 and the far plane at 0.0. The clear that goes with it is
// in frame.c and the projection matrix that produces those planes is 3d's;
// change any one of the three alone and the picture is wrong in a way that still
// looks plausible.
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
// instance extensions, the surface, the queue family, the format, the device
// extensions and the swapchain. Six places, and there are no others: everything
// past the graphics card is identical, which is what makes a test on a headless
// device a test of the code that ships.
#include "device_internal.h"

#include "backend.h"

#include <render/device.h>

#include <base/assert.h>

#include <stddef.h>
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
		fprintf(stderr,
			"render: vkCreateInstance failed (VkResult %d) with these extensions asked for:\n",
			(int)result);
		for (uint32_t i = 0; i < extension_count; i++)
			fprintf(stderr, "render:     %s\n", extensions[i]);
		return false;
	}

	voe_render_loader_instance(device->instance, !device->headless);
	if (validate)
		attach_messenger(device);
	return true;
}

// -------------------------------------------------------------- graphics cards

// A queue family that can do both, because this engine has one queue and no
// reason yet to have two. A headless device has no surface to present to, so
// graphics alone is the whole of what it asks for. UINT32_MAX is the "none of
// them" answer, which is safe because it is not a family index anything could
// return.
static uint32_t graphics_family(voe_render_device *device,
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
		if (device->headless)
			return i;
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

		family = graphics_family(device, cards[i], arena);
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
			"render: none of the %u graphics cards on this machine offers Vulkan 1.3 and a queue that can draw%s\n",
			count,
			device->headless ? "" : " and present to this window");
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
	VkPhysicalDeviceVulkan12Features features12 = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
		.pNext = &features13,
	};
	// What the same query has to be handed in order to answer about a 1.2
	// feature: get_physical_device_features2 fills only what is chained onto
	// it, so asking about shaderSampledImageArrayNonUniformIndexing means
	// chaining a 1.2 block onto the query as well as onto the create.
	VkPhysicalDeviceVulkan12Features available12 = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
	};
	// WHAT THE FRAGMENT STAGE NEEDS IN ORDER TO PICK A TEXTURE AT RUN TIME.
	// It samples one element of an array of sixty-four, and which element is
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
		fprintf(stderr,
			"render: this graphics card cannot index a texture array with a value from a buffer; textures will be wrong\n");

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
		fprintf(stderr,
			"render: this graphics card cannot index a texture array differently per element; text drawn as elements will be wrong where a frame uses more than one sheet\n");

	result = voe_render_vk.create_device(device->physical, &info, NULL,
					     &device->device);
	if (result != VK_SUCCESS) {
		fprintf(stderr, "render: vkCreateDevice failed (VkResult %d)\n",
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
		fprintf(stderr, "render: the surface offers no formats\n");
		return false;
	}

	formats = voe_base_arena_push(arena, (size_t)count * sizeof(*formats));
	if (voe_render_vk.get_surface_formats(device->physical, device->surface,
					      &count, formats) != VK_SUCCESS) {
		fprintf(stderr, "render: vkGetPhysicalDeviceSurfaceFormatsKHR failed\n");
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
		fprintf(stderr,
			"render: this surface offers no sRGB format (taking %d), so the frame will reach the screen too dark\n",
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

// ------------------------------------------------------------------- pipeline

// The compiled shader, in the binary. slangc writes draw.spv into the build tree
// and cmake/voe.cmake puts that directory on this file's include path, so the
// quoted name below resolves to a generated file and never to one in the source
// tree. There is no fallback path and no file to ship beside the binary.
//
// alignas because vkCreateShaderModule takes a const uint32_t *, and #embed can
// only fill an array of bytes. A char array is aligned for a char; handing a
// misaligned pointer to the driver is undefined behaviour that happens to work
// until the day it does not.
static alignas(uint32_t) const unsigned char draw_spv[] = {
#embed "draw.spv"
};

// Both entry points live in the one module above, spelled exactly as the shader
// spells them — see -fvk-use-entrypoint-name in cmake/voe.cmake, which is what
// keeps these two strings true.
#define DRAW_VERTEX_ENTRY "voe_render_draw_vertex"
#define DRAW_FRAGMENT_ENTRY "voe_render_draw_fragment"

// Both pipelines, from one description. `blended` is the only thing that differs
// between the two and the three lines it touches are marked below.
static bool create_pipeline(voe_render_device *device, bool blended,
			    VkPipeline *out)
{
	VkShaderModuleCreateInfo module_info = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = sizeof(draw_spv),
		.pCode = (const uint32_t *)draw_spv,
	};
	VkShaderModule module = VK_NULL_HANDLE;
	VkPipelineShaderStageCreateInfo stages[2];
	// One buffer, read one vertex at a time. The stride is the struct's own
	// size rather than a number written out, so a field added to
	// voe_render_vertex cannot leave this behind.
	VkVertexInputBindingDescription binding = {
		.binding = 0,
		.stride = sizeof(voe_render_vertex),
		.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
	};
	// THESE THREE LOCATIONS AND draw.slang's THREE vk::location NUMBERS ARE ONE
	// FACT IN TWO PLACES. Both are stated rather than counted, and both
	// offsets come from offsetof rather than from adding up sizes — which is
	// what makes reordering the struct's fields harmless and renaming one of
	// them a compile error instead of a wrong picture.
	VkVertexInputAttributeDescription attributes[3] = {
		{
			.location = 0,
			.binding = 0,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = offsetof(voe_render_vertex, position),
		},
		{
			.location = 1,
			.binding = 0,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = offsetof(voe_render_vertex, normal),
		},
		{
			.location = 2,
			.binding = 0,
			// Two floats, not three: a texture coordinate.
			.format = VK_FORMAT_R32G32_SFLOAT,
			.offset = offsetof(voe_render_vertex, uv),
		},
	};
	VkPipelineVertexInputStateCreateInfo vertex_input = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = &binding,
		.vertexAttributeDescriptionCount = 3,
		.pVertexAttributeDescriptions = attributes,
	};
	VkPipelineInputAssemblyStateCreateInfo assembly = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
	};
	// One of each, and what they are is decided per frame — see the dynamic
	// state below. Counts here, values in frame.c.
	VkPipelineViewportStateCreateInfo viewport = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.scissorCount = 1,
	};
	// FRONT FACE IS COUNTER-CLOCKWISE, WHICH IS THE SAME WORD THE WORLD USES,
	// AND THAT IS THE WHOLE POINT OF THE FLIP. This engine winds front faces
	// counter-clockwise with +Y up, as glTF does. Vulkan decides facing from
	// framebuffer coordinates, so without the negative viewport height it
	// would want the opposite constant — the flip is what puts the two
	// systems into agreement, and once they agree the constant here is
	// spelled the way the convention is spelled and no translation happens
	// anywhere.
	//
	// IT SAID CLOCKWISE ONCE, AND THAT WAS THE DOUBLE NEGATIVE CLAUDE.md
	// WARNS ABOUT. The reasoning written here was that the flip reverses the
	// winding so the constant must be reversed too; the flip is what removes
	// the reversal, and applying both left a front face Vulkan called a back
	// one. Nothing culled, so nothing showed it, which is exactly the failure
	// mode the rule about proving this with a test names.
	//
	// THESE TWO LINES AND THE VIEWPORT'S SIGN ARE ONE FACT IN THREE PLACES,
	// AND render/tests/offscreen.c IS WHAT HOLDS THEM TOGETHER. That test
	// draws a cube into an offscreen image through the engine's own viewport
	// and reads the centre pixel back, then draws it again through the mirror
	// of that viewport — every face wound the other way — and requires the
	// first to show the near face and the second to show the far one. The
	// cube's near and far faces carry the same picture the other way round
	// by construction, so the two cases cannot be confused. Change either line here, or the
	// sign in voe_render_frame_viewport, and it fails. Change all three and it
	// still fails, which is the point: flipping twice looks exactly like
	// flipping none until something is culled.
	VkPipelineRasterizationStateCreateInfo raster = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_BACK_BIT,
		.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
		.lineWidth = 1.0f,
	};
	VkPipelineMultisampleStateCreateInfo multisample = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
	};
	// GREATER, AND THAT IS THE WHOLE OF THIS ENGINE'S REVERSED DEPTH ON THE
	// PIPELINE'S SIDE. A fragment survives when it is *nearer*, and nearer
	// means a larger depth here, because voe_3d_projection puts the near
	// plane at 1.0 and the far plane at 0.0. LESS_OR_EQUAL is what every
	// tutorial writes and it would keep the farthest fragment instead — on a
	// convex shape like a cube that still draws something, which is why the
	// card asks for the comparison to be flipped on purpose and looked at.
	//
	// depthBoundsTestEnable stays off: it clips against a depth range and
	// this engine has nothing that wants that. The two bounds below are the
	// full range and are ignored while the test is off; they are stated so
	// that a reader does not have to wonder whether a zero here means the
	// near plane.
	//
	// THE BLENDED PIPELINE TESTS DEPTH AND DOES NOT WRITE IT, AND NEITHER
	// HALF OF THAT IS OPTIONAL. The test stays on so that a see-through
	// thing behind a solid one is still hidden by it. The write goes off so
	// that two see-through things do not hide each other — which is what
	// makes the caller's furthest-first order load-bearing rather than
	// cosmetic. Leave the write on and the sort appears to work while doing
	// nothing at all.
	VkPipelineDepthStencilStateCreateInfo depth = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = VK_TRUE,
		.depthWriteEnable = blended ? VK_FALSE : VK_TRUE,
		.depthCompareOp = VK_COMPARE_OP_GREATER,
		.depthBoundsTestEnable = VK_FALSE,
		.stencilTestEnable = VK_FALSE,
		.minDepthBounds = 0.0f,
		.maxDepthBounds = 1.0f,
	};
	// THE BLEND IS PREMULTIPLIED AND THAT IS THE ENGINE'S CONTRACT, NOT A
	// TASTE. Source factor ONE, destination factor ONE_MINUS_SRC_ALPHA, for
	// colour and for alpha both, because a fragment leaves the shader with
	// its colour already multiplied by its own alpha — draw.slang's last act.
	// The textbook non-premultiplied pair (SRC_ALPHA, ONE_MINUS_SRC_ALPHA)
	// would multiply by alpha a second time and everything see-through would
	// come out too dark.
	//
	// THE ALPHA CHANNEL IS BLENDED THE SAME WAY AND NOT LEFT ALONE. The
	// target's alpha is what a later compositing step would read; keeping it
	// consistent costs nothing and an inconsistent one is invisible until
	// something reads it.
	//
	// The solid pipeline writes rather than blends: everything it draws is
	// fully solid and there is nothing underneath it but the clear.
	VkPipelineColorBlendAttachmentState attachment = {
		.blendEnable = blended ? VK_TRUE : VK_FALSE,
		.srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		.colorBlendOp = VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		.alphaBlendOp = VK_BLEND_OP_ADD,
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
				  VK_COLOR_COMPONENT_G_BIT |
				  VK_COLOR_COMPONENT_B_BIT |
				  VK_COLOR_COMPONENT_A_BIT,
	};
	VkPipelineColorBlendStateCreateInfo blend = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.attachmentCount = 1,
		.pAttachments = &attachment,
	};
	// So that a resize rebuilds the targets and the swapchain and nothing
	// else. A pipeline baked at one size would have to be built again on
	// every resize, which is a lot of driver work to say a number that
	// changed.
	VkDynamicState dynamic_states[2] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
	};
	VkPipelineDynamicStateCreateInfo dynamic = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = 2,
		.pDynamicStates = dynamic_states,
	};
	// Dynamic rendering has no render pass, so the pipeline is told the
	// attachment format here instead. It is the target's, which is the
	// surface's, chosen once and unchanged by a resize — which is what makes
	// this a startup decision and not a per-resize one. The day the target
	// stops sharing the swapchain's format, this line follows the target and
	// not the screen.
	// Dynamic rendering has no render pass, so both attachment formats are
	// declared here instead. They have to match what frame.c attaches, and a
	// depth format declared with no depth attachment — or the other way
	// round — is invalid rather than merely wrong.
	VkPipelineRenderingCreateInfo rendering = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
		.colorAttachmentCount = 1,
		.pColorAttachmentFormats = &device->format.format,
		.depthAttachmentFormat = VOE_RENDER_DEPTH_FORMAT,
	};
	// One set holding everything the shader reads, and one push constant
	// range holding the one number that differs between two draws in a
	// frame. descriptors.c made the set layout, which is why it has to have
	// run before this function does; the range is described here because it
	// belongs to the pipeline layout and to nothing else.
	//
	// BOTH STAGES, because both read the object's record: the vertex stage
	// wants its world matrix and the fragment stage wants the shading index
	// in it. A range that named one stage would make the other's read
	// invalid.
	//
	// IT IS A MATRIX WIDE THOUGH A MESH DRAW STILL PUSHES FOUR BYTES, AND
	// THAT IS THE ELEMENT PIPELINE'S DOING. shaders/elements.slang pushes a
	// sixty-four-byte surface transform through this same range, because
	// that pipeline shares this layout — and it shares it so that the
	// descriptor set frame.c binds once at the top of a frame stays bound
	// across an element draw. Two layouts differing only in their push
	// constant ranges are incompatible, and binding a pipeline with an
	// incompatible layout disturbs the set for everything drawn afterwards.
	//
	// THE TWO BLOCKS ALIAS AND NEITHER EVER READS THE OTHER'S BYTES. Both
	// start at offset nought, and each pipeline pushes its own immediately
	// before its own draw — voe_render_frame_draw pushes the object number
	// for every mesh it draws, and voe_render_element's draw pushes the
	// transform for its one draw. There is no ordering in which a shader
	// reads bytes the other left. Widening it further is free until 128,
	// which is the smallest range Vulkan guarantees.
	VkPushConstantRange push = {
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT |
			      VK_SHADER_STAGE_FRAGMENT_BIT,
		.offset = 0,
		.size = sizeof(voe_math_float4x4),
	};
	VkPipelineLayoutCreateInfo layout = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 1,
		.pSetLayouts = &device->descriptor_layout,
		.pushConstantRangeCount = 1,
		.pPushConstantRanges = &push,
	};
	VkGraphicsPipelineCreateInfo info = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.pNext = &rendering,
		.stageCount = 2,
		.pStages = stages,
		.pVertexInputState = &vertex_input,
		.pInputAssemblyState = &assembly,
		.pViewportState = &viewport,
		.pRasterizationState = &raster,
		.pMultisampleState = &multisample,
		.pDepthStencilState = &depth,
		.pColorBlendState = &blend,
		.pDynamicState = &dynamic,
	};
	VkResult result;

	VOE_BASE_DEBUG_ASSERT(device->descriptor_layout != VK_NULL_HANDLE,
			      "building the pipeline before the descriptor layout exists");

	if (voe_render_vk.create_shader_module(device->device, &module_info, NULL,
					       &module) != VK_SUCCESS) {
		fprintf(stderr, "render: vkCreateShaderModule failed on draw.spv\n");
		return false;
	}

	stages[0] = (VkPipelineShaderStageCreateInfo){
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_VERTEX_BIT,
		.module = module,
		.pName = DRAW_VERTEX_ENTRY,
	};
	stages[1] = (VkPipelineShaderStageCreateInfo){
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
		.module = module,
		.pName = DRAW_FRAGMENT_ENTRY,
	};

	// ONE LAYOUT FOR BOTH PIPELINES, MADE BY WHICHEVER GETS HERE FIRST. The
	// two describe the same set and the same push constant, and the probe
	// shares it as well — see device_internal.h. A second one would be a
	// second handle for the same description and a leak the day only one of
	// them was destroyed.
	if (device->layout == VK_NULL_HANDLE &&
	    voe_render_vk.create_pipeline_layout(device->device, &layout, NULL,
						 &device->layout) != VK_SUCCESS) {
		fprintf(stderr, "render: vkCreatePipelineLayout failed\n");
		voe_render_vk.destroy_shader_module(device->device, module, NULL);
		return false;
	}
	info.layout = device->layout;

	result = voe_render_vk.create_graphics_pipelines(device->device,
							 VK_NULL_HANDLE, 1, &info,
							 NULL, out);

	// The module is the compiler's input and the pipeline has finished
	// reading it, so it goes away here whether or not the pipeline was made.
	// Keeping it would be keeping a copy of the shader for nobody.
	voe_render_vk.destroy_shader_module(device->device, module, NULL);

	if (result != VK_SUCCESS) {
		fprintf(stderr,
			"render: vkCreateGraphicsPipelines failed on the %s pipeline (VkResult %d)\n",
			blended ? "blended" : "solid", (int)result);
		*out = VK_NULL_HANDLE;
		return false;
	}
	return true;
}

// The solid one first, because it is the one that makes the layout and the one
// every draw that is not see-through goes through.
static bool create_pipelines(voe_render_device *device)
{
	return create_pipeline(device, false, &device->pipeline) &&
	       create_pipeline(device, true, &device->pipeline_blended);
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
		fprintf(stderr, "render: vkCreateCommandPool failed\n");
		return false;
	}

	// One call for every slot, because vkAllocateCommandBuffers takes a
	// count and there is nothing to be had from asking it that many times.
	// The handles are spread one to a slot below, and the pool takes every
	// one of them back when it is destroyed.
	commands.commandPool = device->pool;
	if (voe_render_vk.allocate_command_buffers(device->device, &commands,
						   buffers) != VK_SUCCESS) {
		fprintf(stderr, "render: vkAllocateCommandBuffers failed\n");
		return false;
	}

	for (uint32_t i = 0; i < VOE_RENDER_FRAMES_IN_FLIGHT; i++) {
		device->frames[i].commands = buffers[i];

		if (voe_render_vk.create_semaphore(device->device, &semaphore,
						   NULL,
						   &device->frames[i].acquired) !=
		    VK_SUCCESS) {
			fprintf(stderr, "render: vkCreateSemaphore failed\n");
			return false;
		}

		if (voe_render_vk.create_fence(device->device, &fence, NULL,
					       &device->frames[i].submitted) !=
		    VK_SUCCESS) {
			fprintf(stderr, "render: vkCreateFence failed\n");
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
			fprintf(stderr,
				"render: vkCreateQueryPool failed, so there will be no GPU timings\n");
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

		// Before the layout below, which it shares.
		voe_render_element_shutdown(device);

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

	if (!create_instance(device, arena))
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
	if (!choose_physical_device(device, arena))
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
		voe_render_texture_write_descriptors(device,
						     device->frames[i].descriptor);
	}
	if (!create_pipelines(device))
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

	say_which_card(device);
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
