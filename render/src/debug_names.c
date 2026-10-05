// Names and labels a capture tool reads: an object's name, and a labelled span
// of a command buffer. Declared in device_calls.h; buffer.c names every buffer,
// target.c every target image and view, and a pass is labelled with its name.
//
// ONE HELPER, SO NO CALLER ASKS WHETHER THE EXTENSION IS THERE. VK_EXT_debug_utils
// is enabled whenever the instance offers it (instance.c), and its three entries
// are NULL otherwise; each call here checks its own entry and does nothing
// without it. So a name costs nothing on a machine without the extension, and
// a caller names unconditionally.
//
// The name is copied by the driver, so a caller's string need only live for the
// call. A NULL handle is the caller's bug and asserts.
#include "device_internal.h"

#include <base/assert.h>

void voe_render_debug_name(const voe_render_device *device, VkObjectType type,
			   uint64_t handle, const char *name)
{
	VkDebugUtilsObjectNameInfoEXT info = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
		.objectType = type,
		.objectHandle = handle,
		.pObjectName = name,
	};

	VOE_BASE_DEBUG_ASSERT(device != NULL, "naming an object on no device");
	VOE_BASE_DEBUG_ASSERT(handle != 0, "naming no object");
	VOE_BASE_DEBUG_ASSERT(name != NULL, "naming an object nothing");

	if (voe_render_vk.set_debug_utils_object_name == NULL)
		return;
	// A name that will not set costs a capture its label and nothing else.
	(void)voe_render_vk.set_debug_utils_object_name(device->device, &info);
}

void voe_render_debug_label_begin(VkCommandBuffer commands, const char *name)
{
	VkDebugUtilsLabelEXT label = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
		.pLabelName = name,
	};

	VOE_BASE_DEBUG_ASSERT(commands != VK_NULL_HANDLE,
			      "labelling no command buffer");
	VOE_BASE_DEBUG_ASSERT(name != NULL, "labelling with no name");

	if (voe_render_vk.cmd_begin_debug_utils_label == NULL)
		return;
	voe_render_vk.cmd_begin_debug_utils_label(commands, &label);
}

void voe_render_debug_label_end(VkCommandBuffer commands)
{
	VOE_BASE_DEBUG_ASSERT(commands != VK_NULL_HANDLE,
			      "ending a label on no command buffer");

	if (voe_render_vk.cmd_end_debug_utils_label == NULL)
		return;
	voe_render_vk.cmd_end_debug_utils_label(commands);
}
