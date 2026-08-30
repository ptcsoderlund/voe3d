// The no-SDK claim, checked. Everything else in render needs a window, a
// compositor and a person looking at the result; this needs none of the three,
// and what it proves is the one thing the whole folder rests on: that a machine
// with a graphics driver and no Vulkan SDK can reach Vulkan through a library
// opened by name, with nothing linked and no header installed.
//
// It includes render's internal loader header directly, by relative path. That
// is deliberate: the loader is not part of render's public surface and must not
// become part of it just so a test can see it, and a test that could only reach
// this through a public function would be asking for a public function that
// exists for the test's sake — the thing CLAUDE.md rule 10 exists to prevent.
//
// A MACHINE WITH NO DRIVER SKIPS AND SAYS SO. There is no Vulkan on a headless
// build box, and that is not a broken checkout. The skip is a pass and it prints
// its reason, because a test that quietly passes without running is worse than
// one that fails.
#include "../src/loader.h"

#include <testing/test.h>

#include <stdio.h>

int main(void)
{
	uint32_t version = 0;
	VkApplicationInfo application = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = "voe3d loader test",
		.apiVersion = VK_API_VERSION_1_3,
	};
	VkInstanceCreateInfo info = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &application,
	};
	VkInstance instance = VK_NULL_HANDLE;
	PFN_vkDestroyInstance destroy_instance;
	VkResult result;

	if (!voe_render_loader_open()) {
		printf("skip: no Vulkan loader on this machine\n");
		return 0;
	}

	// Reported and sane: the call goes through a pointer resolved out of the
	// library, and what comes back is at least the 1.1 that being able to
	// make the call at all implies.
	VOE_TEST_CHECK(voe_render_vk.enumerate_instance_version != NULL);
	VOE_TEST_CHECK(voe_render_vk.enumerate_instance_version(&version) ==
		       VK_SUCCESS);
	VOE_TEST_CHECK(VK_API_VERSION_MAJOR(version) >= 1);
	VOE_TEST_CHECK(VK_API_VERSION_MINOR(version) >= 1);
	printf("instance version %u.%u.%u\n", VK_API_VERSION_MAJOR(version),
	       VK_API_VERSION_MINOR(version), VK_API_VERSION_PATCH(version));

	// An instance, from nothing but a dlopen. No extensions, so no window
	// system is involved and this runs where there is no display.
	result = voe_render_vk.create_instance(&info, NULL, &instance);
	if (result == VK_ERROR_INCOMPATIBLE_DRIVER) {
		printf("skip: no driver here offers Vulkan 1.3\n");
		voe_render_loader_close();
		return voe_test_result();
	}

	VOE_TEST_CHECK_INT(result, VK_SUCCESS);
	VOE_TEST_CHECK(instance != VK_NULL_HANDLE);

	// Resolved from the instance by hand rather than through
	// voe_render_loader_instance, which fills in the surface functions too
	// and rightly aborts when they are missing — this instance was created
	// with no extensions at all, which is what lets the test run where there
	// is no display.
	if (instance != VK_NULL_HANDLE) {
		destroy_instance = (PFN_vkDestroyInstance)
			voe_render_vk.get_instance_proc_addr(instance,
							     "vkDestroyInstance");
		VOE_TEST_CHECK(destroy_instance != NULL);
		if (destroy_instance != NULL)
			destroy_instance(instance, NULL);
	}

	voe_render_loader_close();
	VOE_TEST_CHECK(voe_render_vk.get_instance_proc_addr == NULL);

	return voe_test_result();
}
