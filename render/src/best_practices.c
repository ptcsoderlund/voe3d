// What the validation layer's Best Practices messages count for (ADR-0358,
// 0367 points 4, 5 and 7): the allowlist, which vendor a message is for, the
// classifier instance.c's messenger hands every message to, and the one line
// device.c prints once the card is chosen. Declared in device_calls.h.
//
// A NEW MESSAGE NOW COUNTS. Every validation error, and every warning whose id
// name is not on the allowlist, adds one to the device's new_messages. The list
// only shrinks: a card may remove a row, and adding one is a decision (0358).
//
// ANOTHER VENDOR'S MESSAGE IS DROPPED. The layer runs all four vendor sets, so
// a message whose id name carries a vendor tag that is not the chosen card's
// says nothing about this machine and is neither printed nor counted.
//
// Constraints: the allowlist and the vendor tags are scanned in order, one
// strcmp or strstr a row, per message; a message is rare enough that this is
// nothing. A table long enough to matter would be sorted and searched.
#include "device_internal.h"

#include <base/report.h>

#include <assert.h>
#include <string.h>

// ----------------------------------------------------------------- the lists

// One allowed warning: its id name and one line why it stands. Ends at the row
// with no id; card 06 fills it with today's warnings.
struct allowance {
	const char *id_name;
	const char *why;
};

static const struct allowance allowlist[] = {
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

enum voe_render_message_verdict
voe_render_best_practices_classify(voe_render_device *device, bool error,
				   const char *id_name)
{
	assert(device != NULL);

	if (voe_render_best_practices_other_vendor(device->vendor_id, id_name))
		return VOE_RENDER_MESSAGE_DROPPED;
	if (!error && voe_render_best_practices_allowed(id_name))
		return VOE_RENDER_MESSAGE_ALLOWED;
	device->new_messages++;
	assert(device->new_messages > 0);
	return VOE_RENDER_MESSAGE_NEW;
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

	if (device->checks_missing)
		VOE_BASE_WARNING("render",
				 "best practices checks missing: this debug build could not load the validation layer, so no message is checked (allowlist of %u)",
				 allowlist_length());
	else if (device->checks_on)
		VOE_BASE_WARNING("render",
				 "best practices checks on, %s, vendor %s (0x%04X), allowlist of %u",
				 device->vendor_checks ?
					 "vendor checks on" :
					 "vendor checks off (the layer offers no VK_EXT_layer_settings)",
				 vendor_name(device->vendor_id), device->vendor_id,
				 allowlist_length());
}
