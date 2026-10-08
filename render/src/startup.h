// The steps of startup that live beside device.c: create the instance, choose
// the graphics card and say which, learn the card's clock and the surface's
// present modes (pacing.c), create the pipeline layout. Internal to
// render; device.c's open_device is the only caller of each, once per device.
// voe_render_card_rank is also called by tests/card.c, which needs no card.
//
// OPEN_DEVICE IS STILL THE ONE PLACE THE ORDER IS. These are in files of their
// own because device.c had grown past reading, not because they are independent:
// each is a step that needs what the one before it made. The instance comes
// straight after the loader is open, because every later call goes through it.
// The card is chosen after the surface exists, because a card qualifies only if
// one of its queue families presents to that surface. The pipeline layout comes
// after voe_render_descriptors_build, because it names the descriptor set layout
// that makes — built first, it would name a null handle. The element pipeline
// (element.c) comes after the layout, because it shares it. The mesh pipelines
// and the relight's startup are not opened here: voe_render_device_prepare
// builds them later, a step a call (pipeline.c).
//
// Each returns false after printing one line of why; open_device turns that
// into the category a caller sees.
#pragma once

#include "device_internal.h"

#include <base/arena.h>

// Vulkan 1.3 is the floor: dynamic rendering and synchronization2 are core in
// it, and both are what this folder is written against. The instance asks for
// it and a card that does not claim it is passed over.
#define REQUIRED_VERSION VK_API_VERSION_1_3

// Fills device->instance, and device->messenger in a debug build that has the
// validation layer. The arena is for the driver's lists of layers and extensions.
[[nodiscard]] bool voe_render_instance_create(voe_render_device *device,
					      voe_base_arena *arena);

// What the ranking knows of one card, gathered by card.c from the driver.
typedef struct {
	VkPhysicalDeviceType kind;
	uint64_t memory;      // bytes in the largest DEVICE_LOCAL heap
	bool vulkan_1_3;
	bool draws;           // a queue family with graphics
	bool presents;        // that family can present (ignored headless)
} voe_render_card_facts;

// The index of the chosen card by 0214's ranking, or UINT32_MAX when none
// qualifies. *fastest is the best-ranked card that has Vulkan 1.3 and draws,
// presenting or not, so a caller can tell it fell back; UINT32_MAX if none.
uint32_t voe_render_card_rank(const voe_render_card_facts *cards, uint32_t count,
			      bool headless, uint32_t *fastest);

// Fills device->physical and device->queue_family and prints the one `render`
// line saying which card and why. Needs device->surface unless headless.
[[nodiscard]] bool voe_render_card_choose(voe_render_device *device,
					  voe_base_arena *arena);

// Sets device->timestamps with the clock's period and bits when the card can time.
void voe_render_timing_learn(voe_render_device *device, voe_base_arena *arena);

// Sets device->mailbox_offered when the surface offers MAILBOX; never headless.
void voe_render_present_modes_learn(voe_render_device *device,
				    voe_base_arena *arena);

// Fills device->layout, the one every graphics pipeline here shares. The mesh
// pipelines themselves are voe_render_device_prepare's, pipeline.c's too.
[[nodiscard]] bool voe_render_pipeline_layout_create(voe_render_device *device);
