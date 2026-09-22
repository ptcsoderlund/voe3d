// Choosing the graphics card and saying which one was chosen. Steps of startup;
// device.c's open_device calls voe_render_card_choose once the surface exists
// and voe_render_card_say once the device is whole — see startup.h.
//
// WHAT QUALIFIES A CARD IS TWO THINGS. It claims Vulkan 1.3, and one of its
// queue families can draw and, unless the device is headless, present to the
// surface. Of the cards that qualify a discrete one is preferred, and past that
// the first one listed wins.
//
// THE `render` LINE AT STARTUP IS PRINTED HERE: card name, Vulkan version,
// queue family. It goes to stdout and is flushed, because it is what a person
// reads to know which card the engine is on.
#include "startup.h"

#include <base/report.h>

#include <stdio.h>

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
bool voe_render_card_choose(voe_render_device *device, voe_base_arena *arena)
{
	uint32_t count = 0;
	VkPhysicalDevice *cards;
	VkPhysicalDevice best = VK_NULL_HANDLE;
	uint32_t best_family = UINT32_MAX;
	bool best_is_discrete = false;

	if (voe_render_vk.enumerate_physical_devices(device->instance, &count,
						     NULL) != VK_SUCCESS ||
	    count == 0) {
		VOE_BASE_ERROR("render", "the Vulkan instance reports no graphics cards");
		return false;
	}

	cards = voe_base_arena_push(arena, (size_t)count * sizeof(*cards));
	if (voe_render_vk.enumerate_physical_devices(device->instance, &count,
						     cards) != VK_SUCCESS) {
		VOE_BASE_ERROR("render", "vkEnumeratePhysicalDevices failed");
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
		VOE_BASE_ERROR("render",
			       "none of the %u graphics cards on this machine offers Vulkan 1.3 and a queue that can draw%s",
			       count,
			       device->headless ? "" : " and present to this window");
		return false;
	}

	device->physical = best;
	device->queue_family = best_family;
	return true;
}

void voe_render_card_say(voe_render_device *device)
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
