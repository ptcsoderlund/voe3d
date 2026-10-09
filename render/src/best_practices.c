// What the validation layer's Best Practices messages count for (ADR-0358,
// 0367 points 4, 5 and 7): the allowlist, which vendor a message is for, the
// classifier instance.c's messenger hands every message to, whether a device
// wants the checks at all (headless always, a window only by
// VOE_RENDER_BEST_PRACTICES=1, ADR-0398), and the one line device.c prints once
// the card is chosen. Declared in device_calls.h.
//
// A NEW MESSAGE NOW COUNTS. Every validation error, and every warning whose id
// name is not on the allowlist, adds one to the device's new_messages. The list
// only shrinks: a card may remove a row, and adding one is a decision (0358).
// A row may name an id only some layer releases raise, and a row the checks'
// own layer cannot raise is kept, not pruned; no layer release is required of
// anyone (0385).
//
// ANOTHER VENDOR'S MESSAGE IS DROPPED. The layer runs all four vendor sets, so
// a message whose id name carries a vendor tag that is not the chosen card's
// says nothing about this machine and is neither printed nor counted.
//
// A NEW MESSAGE IS NAMED AGAIN AT CLOSE (bug 03). The line printed when one
// arrives may be far above the close, or lost; the line above the gate's assert
// must say what it counts. So the classifier keeps each new message, one row per
// id name in the device, and _list_new prints those rows before the count.
//
// Constraints: the allowlist and the vendor tags are scanned in order, one
// strcmp or strstr a row, per message; a message is rare enough that this is
// nothing. A table long enough to matter would be sorted and searched.
#include "device_internal.h"

#include <base/report.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>

// ----------------------------------------------------------------- the lists

// One allowed warning: its id name and one line why it stands. Ends at the row
// with no id. Today's warnings as of 062, on an NVIDIA card: every test of
// every folder and the editor's capture of examples/tank_game. Each is a cost
// the breakdown has not yet ranked, so fixing it waits for a measurement (0358).
struct allowance {
	const char *id_name;
	const char *why;
};

static const struct allowance allowlist[] = {
	{ "BestPractices-ImageBarrierAccessLayout",
	  "barriers name MEMORY_READ|WRITE rather than the layout's own access; the barrier review is its own card" },
	{ "BestPractices-ImageMemoryBarrier-TransitionUndefinedToReadOnly",
	  "an image is settled from UNDEFINED straight to read-only so an unwritten slot is valid to bind (default texture, empty maps)" },
	{ "BestPractices-NVIDIA-AllocateMemory-ReuseAllocations",
	  "a resize or a volume freed and rebuilt reallocates; reuse needs an allocator the engine does not have yet" },
	{ "BestPractices-NVIDIA-AllocateMemory-SetPriority",
	  "no VK_EXT_memory_priority asked for; a hint whose worth the breakdown has not shown" },
	{ "BestPractices-NVIDIA-ClearColor-NotCompressed",
	  "the clear colour is the scene's own, not 0 or 1; changing it changes the picture" },
	{ "BestPractices-NVIDIA-CreateDevice-PageableDeviceLocalMemory",
	  "VK_EXT_pageable_device_local_memory is not asked for; an extension the engine has not decided to take" },
	{ "BestPractices-NVIDIA-CreateImage-Depth32Format",
	  "D32_SFLOAT is chosen on purpose for reversed depth's precision (device_internal.h)" },
	{ "BestPractices-NVIDIA-CreatePipelineLayout-LargePipelineLayout",
	  "one shared layout for every mesh and element pipeline; splitting it is a descriptor redesign" },
	{ "BestPractices-Pipeline-SortAndBind",
	  "a pipeline is rebound once per pass, as every pass opens its own rendering; draws are not sorted across passes" },
	{ "BestPractices-PushConstants",
	  "the range is shared by the element and mesh pipelines and a mesh draw pushes only its own part" },
	{ "BestPractices-pipeline-stage-flags2-compute",
	  "barriers name ALL_COMMANDS; the barrier review waits for the frame breakdown's measurement (0358)" },
	{ "BestPractices-vkAllocateMemory-small-allocation",
	  "every buffer and image has its own allocation; no sub-allocator yet, the same cause as the small-dedicated-allocation rows" },
	{ "BestPractices-vkBindBufferMemory-small-dedicated-allocation",
	  "every buffer has its own allocation; sub-allocation is the allocator the review asked for, not this feature" },
	{ "BestPractices-vkBindImageMemory-small-dedicated-allocation",
	  "every image has its own allocation; sub-allocation is the allocator the review asked for, not this feature" },
	{ "BestPractices-vkCmdDrawIndexed-many-small-indexed-drawcalls",
	  "an Arm and IMG check with no vendor tag in its id; tests draw tiny meshes, which is what they are for" },
	{ "BestPractices-vkCreateComputePipelines-multiple-pipelines-no-cache",
	  "no pipeline cache yet; startup cost is measured by prepare's steps, not here" },
	{ NULL, NULL },
};

// The four vendors the layer has checks for: the PCI vendor id, the tag their
// message id names carry, and the name the start line uses.
static const struct {
	uint32_t vendor_id;
	const char *tag;
	const char *name;
} vendors[4] = {
	{ 0x10DE, "-NVIDIA-", "NVIDIA" },
	{ 0x1002, "-AMD-", "AMD" },
	{ 0x13B5, "-Arm-", "Arm" },
	{ 0x1010, "-IMG-", "IMG" },
};

static uint32_t allowlist_length(void)
{
	uint32_t length = 0;
	const uint32_t rows = sizeof(allowlist) / sizeof(allowlist[0]);

	while (length < rows && allowlist[length].id_name != NULL)
		length++;
	assert(length < rows);
	return length;
}

bool voe_render_best_practices_allowed(const char *id_name)
{
	const uint32_t length = allowlist_length();

	if (id_name == NULL)
		return false;
	for (uint32_t i = 0; i < length; i++) {
		assert(allowlist[i].why != NULL);
		if (strcmp(allowlist[i].id_name, id_name) == 0)
			return true;
	}
	return false;
}

bool voe_render_best_practices_other_vendor(uint32_t vendor_id,
					    const char *id_name)
{
	if (id_name == NULL)
		return false;
	for (uint32_t i = 0; i < 4; i++) {
		if (strstr(id_name, vendors[i].tag) != NULL)
			return vendors[i].vendor_id != vendor_id;
	}
	return false;
}

// ------------------------------------------------------------ the classifier

// `from` into `to` of `size` bytes, cut to fit and always terminated. snprintf
// rather than a copy loop: it neither allocates nor reads past the cut.
static void copy_cut(char *to, size_t size, const char *from)
{
	assert(to != NULL && size > 0);
	if (snprintf(to, size, "%s", from) < 0)
		to[0] = '\0';
	assert(strlen(to) < size);
}

// One new message into the device's kept rows: the row with its id name, or the
// first free one, counted, the first text kept; no row left counts the overflow.
// A cut id name matches on what was kept of it.
static void keep_new(voe_render_device *device, bool error,
		     const char *id_name, const char *message)
{
	const char *name = id_name != NULL ? id_name : "(no id)";
	uint32_t row = 0;

	assert(device != NULL);
	while (row < VOE_RENDER_KEPT_MESSAGES && device->kept[row].count > 0 &&
	       strncmp(device->kept[row].id_name, name,
		       VOE_RENDER_KEPT_ID_BYTES - 1) != 0)
		row++;
	if (row == VOE_RENDER_KEPT_MESSAGES) {
		device->kept_overflow++;
		return;
	}
	if (device->kept[row].count == 0) {
		copy_cut(device->kept[row].id_name, VOE_RENDER_KEPT_ID_BYTES,
			 name);
		copy_cut(device->kept[row].text, VOE_RENDER_KEPT_TEXT_BYTES,
			 message != NULL ? message : "(no text)");
	}
	device->kept[row].error = device->kept[row].error || error;
	device->kept[row].count++;
	assert(device->kept[row].count > 0);
}

enum voe_render_message_verdict
voe_render_best_practices_classify(voe_render_device *device, bool error,
				   const char *id_name, const char *message)
{
	assert(device != NULL);

	if (voe_render_best_practices_other_vendor(device->vendor_id, id_name))
		return VOE_RENDER_MESSAGE_DROPPED;
	if (!error && voe_render_best_practices_allowed(id_name))
		return VOE_RENDER_MESSAGE_ALLOWED;
	device->new_messages++;
	keep_new(device, error, id_name, message);
	assert(device->new_messages > 0);
	return VOE_RENDER_MESSAGE_NEW;
}

void voe_render_best_practices_list_new(const voe_render_device *device)
{
	assert(device != NULL);
	assert(device->new_messages > 0);

	for (uint32_t i = 0;
	     i < VOE_RENDER_KEPT_MESSAGES && device->kept[i].count > 0; i++)
		VOE_BASE_ERROR("render", "new vulkan %s %s, %u times, first: %s",
			       device->kept[i].error ? "error" : "warning",
			       device->kept[i].id_name, device->kept[i].count,
			       device->kept[i].text);
	if (device->kept_overflow > 0)
		VOE_BASE_ERROR("render",
			       "%u more new messages whose id name found no free row (%u rows kept)",
			       device->kept_overflow,
			       (unsigned)VOE_RENDER_KEPT_MESSAGES);
}

// ------------------------------------------------------------- whether at all

// ADR-0398: a window's GPU times are slowed about fivefold by these checks, so
// only a headless device has them by default; a window only by exactly "1".
bool voe_render_best_practices_wanted(bool headless, const char *setting)
{
	const bool wanted =
		headless || (setting != NULL && strcmp(setting, "1") == 0);

	assert(!headless || wanted);
	return wanted;
}

// -------------------------------------------------------------- the line

static const char *vendor_name(uint32_t vendor_id)
{
	for (uint32_t i = 0; i < 4; i++) {
		if (vendors[i].vendor_id == vendor_id)
			return vendors[i].name;
	}
	return "another vendor";
}

void voe_render_best_practices_announce(const voe_render_device *device)
{
	assert(device != NULL);
	assert(!(device->checks_on && device->checks_missing));
	assert(!(device->checks_on && device->checks_off_for_window));
	assert(!(device->checks_missing && device->checks_off_for_window));

	if (device->checks_missing) {
		VOE_BASE_WARNING("render",
				 "best practices checks missing: this debug build could not load the validation layer, so no message is checked (allowlist of %u)",
				 allowlist_length());
	} else if (device->checks_off_for_window) {
		VOE_BASE_WARNING("render",
				 "best practices checks off for a window, so GPU times are true; VOE_RENDER_BEST_PRACTICES=1 turns them on (allowlist of %u)",
				 allowlist_length());
	} else if (device->checks_on) {
		VOE_BASE_WARNING("render",
				 "best practices checks on, %s, vendor %s (0x%04X), allowlist of %u",
				 device->vendor_checks ?
					 "vendor checks on" :
					 "vendor checks off (the layer offers no VK_EXT_layer_settings)",
				 vendor_name(device->vendor_id), device->vendor_id,
				 allowlist_length());
		if (!device->headless)
			VOE_BASE_WARNING("render",
					 "GPU times are slowed by these checks; the frame breakdown is not the engine's cost");
	}
}
