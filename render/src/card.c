// Choosing the graphics card and saying which one was chosen. A step of startup;
// device.c's open_device calls voe_render_card_choose once the surface exists —
// see startup.h. voe_render_card_rank is the choice itself, pure, so
// tests/card.c proves it with no graphics card.
//
// THE FASTEST CARD, BY WHAT IT IS (0201, 0214). A card is ranked only if it
// claims Vulkan 1.3 and has a queue family that draws. Ranked cards order by
// kind — discrete, integrated, virtual, any other, a software rasteriser last —
// then by the size of the largest device-local heap, then by the earlier index.
// On a window the chosen card is the best-ranked one whose drawing family also
// presents to the surface; headless, presenting is not asked. Nothing is
// measured: kind and heap size are what every driver reports without a
// benchmark, and two cards alike in both are told apart by nothing better.
//
// THE `render` LINE AT STARTUP IS PRINTED HERE, where the choice is made, because
// this is the one place that knows why: the card's name, its kind in words, how
// many cards there were, the fastest or the next one down and which card could
// not present, the Vulkan version and the rate. It goes to stdout and is
// flushed, because it is what a person reads to know which card the engine is
// on. The rate is the present mode, not hertz: the device opens on FIFO
// (ADR-0131) and nothing in the engine knows the display's refresh.
#include "startup.h"

#include <base/report.h>

#include <assert.h>
#include <stdio.h>

// -------------------------------------------------------------- the ranking

// Lower is faster. A kind this list does not name ranks above a software
// rasteriser and below everything it does name.
static uint32_t kind_order(VkPhysicalDeviceType kind)
{
	switch (kind) {
	case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
		return 0;
	case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
		return 1;
	case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
		return 2;
	case VK_PHYSICAL_DEVICE_TYPE_CPU:
		return 4;
	default:
		return 3;
	}
}

// Strictly above, so that of two equal cards the one met first stays.
static bool ranks_above(const voe_render_card_facts *a,
			const voe_render_card_facts *b)
{
	assert(a != NULL);
	assert(b != NULL);
	if (kind_order(a->kind) != kind_order(b->kind))
		return kind_order(a->kind) < kind_order(b->kind);
	return a->memory > b->memory;
}

uint32_t voe_render_card_rank(const voe_render_card_facts *cards, uint32_t count,
			      bool headless, uint32_t *fastest)
{
	uint32_t chosen = UINT32_MAX;

	assert(cards != NULL || count == 0);
	assert(fastest != NULL);

	*fastest = UINT32_MAX;
	for (uint32_t i = 0; i < count; i++) {
		if (!cards[i].vulkan_1_3 || !cards[i].draws)
			continue;
		if (*fastest == UINT32_MAX || ranks_above(&cards[i], &cards[*fastest]))
			*fastest = i;
		if (!headless && !cards[i].presents)
			continue;
		if (chosen == UINT32_MAX || ranks_above(&cards[i], &cards[chosen]))
			chosen = i;
	}

	assert(chosen == UINT32_MAX || *fastest != UINT32_MAX);
	return chosen;
}

// ------------------------------------------------------------- the facts

// The first queue family that draws and, when presenting is asked, also presents
// to the surface. This engine has one queue and no reason yet to have two.
// UINT32_MAX is the "none of them" answer, which is safe because it is not a
// family index anything could return.
static uint32_t graphics_family(voe_render_device *device,
				VkPhysicalDevice physical,
				voe_base_arena *arena, bool presenting)
{
	uint32_t count = 0;
	VkQueueFamilyProperties *families;

	assert(device != NULL);
	assert(!presenting || device->surface != VK_NULL_HANDLE);

	voe_render_vk.get_queue_family_properties(physical, &count, NULL);
	if (count == 0)
		return UINT32_MAX;

	families = voe_base_arena_push(arena, (size_t)count * sizeof(*families));
	voe_render_vk.get_queue_family_properties(physical, &count, families);

	for (uint32_t i = 0; i < count; i++) {
		VkBool32 presents = VK_FALSE;

		if ((families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0)
			continue;
		if (!presenting)
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

// The largest heap, not their sum: a card's own memory is one heap, and the
// device-local heaps beside it on some drivers are windows onto the same bytes.
static uint64_t largest_local_heap(VkPhysicalDevice physical)
{
	VkPhysicalDeviceMemoryProperties memory;
	uint64_t largest = 0;

	assert(physical != VK_NULL_HANDLE);
	voe_render_vk.get_memory_properties(physical, &memory);
	assert(memory.memoryHeapCount <= VK_MAX_MEMORY_HEAPS);

	for (uint32_t i = 0; i < memory.memoryHeapCount; i++) {
		if ((memory.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) &&
		    memory.memoryHeaps[i].size > largest)
			largest = memory.memoryHeaps[i].size;
	}
	return largest;
}

static voe_render_card_facts gather_facts(voe_render_device *device,
					  VkPhysicalDevice physical,
					  const VkPhysicalDeviceProperties *properties,
					  voe_base_arena *arena)
{
	voe_render_card_facts facts = {
		.kind = properties->deviceType,
		.memory = largest_local_heap(physical),
		.vulkan_1_3 = properties->apiVersion >= REQUIRED_VERSION,
	};

	assert(device != NULL);
	assert(arena != NULL);
	facts.draws = graphics_family(device, physical, arena, false) != UINT32_MAX;
	facts.presents = facts.draws && !device->headless &&
			 graphics_family(device, physical, arena, true) != UINT32_MAX;
	return facts;
}

// ---------------------------------------------------------------- the line

static const char *kind_in_words(VkPhysicalDeviceType kind)
{
	switch (kind) {
	case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
		return "a discrete card";
	case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
		return "an integrated card";
	case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
		return "a virtual card";
	case VK_PHYSICAL_DEVICE_TYPE_CPU:
		return "a software rasteriser";
	default:
		return "a card of another kind";
	}
}

static void say_choice(const voe_render_device *device,
		       const VkPhysicalDeviceProperties *properties,
		       uint32_t count, uint32_t chosen, uint32_t fastest)
{
	const VkPhysicalDeviceProperties *card = &properties[chosen];

	assert(chosen < count);
	assert(fastest < count);

	printf("render     %s, %s, ", card->deviceName, kind_in_words(card->deviceType));
	if (chosen != fastest)
		printf("the next one down of %u cards because %s, the fastest, cannot present to this window",
		       count, properties[fastest].deviceName);
	else if (count == 1)
		printf("the only card");
	else
		printf("the fastest of %u cards", count);
	printf(", Vulkan %u.%u.%u, %s\n",
	       VK_API_VERSION_MAJOR(card->apiVersion),
	       VK_API_VERSION_MINOR(card->apiVersion),
	       VK_API_VERSION_PATCH(card->apiVersion),
	       device->headless ? "headless, nothing presented"
				: "drawing in step with the display");
	fflush(stdout);
}

// ------------------------------------------------------------- the choice

bool voe_render_card_choose(voe_render_device *device, voe_base_arena *arena)
{
	uint32_t count = 0;
	uint32_t chosen;
	uint32_t fastest;
	VkPhysicalDevice *cards;
	VkPhysicalDeviceProperties *properties;
	voe_render_card_facts *facts;

	assert(device != NULL);
	assert(arena != NULL);

	if (voe_render_vk.enumerate_physical_devices(device->instance, &count,
						     NULL) != VK_SUCCESS ||
	    count == 0) {
		VOE_BASE_ERROR("render", "the Vulkan instance reports no graphics cards");
		return false;
	}

	cards = voe_base_arena_push(arena, (size_t)count * sizeof(*cards));
	properties = voe_base_arena_push(arena, (size_t)count * sizeof(*properties));
	facts = voe_base_arena_push(arena, (size_t)count * sizeof(*facts));
	if (voe_render_vk.enumerate_physical_devices(device->instance, &count,
						     cards) != VK_SUCCESS) {
		VOE_BASE_ERROR("render", "vkEnumeratePhysicalDevices failed");
		return false;
	}

	for (uint32_t i = 0; i < count; i++) {
		voe_render_vk.get_physical_device_properties(cards[i], &properties[i]);
		facts[i] = gather_facts(device, cards[i], &properties[i], arena);
	}

	chosen = voe_render_card_rank(facts, count, device->headless, &fastest);
	if (chosen == UINT32_MAX) {
		VOE_BASE_ERROR("render",
			       "none of the %u graphics cards on this machine offers Vulkan 1.3 and a queue that can draw%s",
			       count,
			       device->headless ? "" : " and present to this window");
		return false;
	}

	device->physical = cards[chosen];
	device->queue_family = graphics_family(device, cards[chosen], arena,
					       !device->headless);
	say_choice(device, properties, count, chosen, fastest);
	return true;
}
