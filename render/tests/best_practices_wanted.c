// Whether a device with the validation layer turns Best Practices on
// (ADR-0398), proved on voe_render_best_practices_wanted alone: no device is
// opened and no environment is read, so this test needs no graphics card.
//
// The cases: a headless device wants the checks whatever the setting says
// (NULL, "0", "1"); a window wants them for exactly "1"; a window with NULL,
// "", "0" or "yes" does not.
//
// It includes render's internal device_internal.h by relative path, as
// new_messages.c does: the call is not part of render's surface.
#include "../src/device_internal.h"

#include <testing/test.h>

static void a_headless_device_always_wants_them(void)
{
	VOE_TEST_CHECK(voe_render_best_practices_wanted(true, NULL));
	VOE_TEST_CHECK(voe_render_best_practices_wanted(true, "0"));
	VOE_TEST_CHECK(voe_render_best_practices_wanted(true, "1"));
}

static void a_window_wants_them_only_for_one(void)
{
	VOE_TEST_CHECK(voe_render_best_practices_wanted(false, "1"));
	VOE_TEST_CHECK(!voe_render_best_practices_wanted(false, NULL));
	VOE_TEST_CHECK(!voe_render_best_practices_wanted(false, ""));
	VOE_TEST_CHECK(!voe_render_best_practices_wanted(false, "0"));
	VOE_TEST_CHECK(!voe_render_best_practices_wanted(false, "yes"));
}

int main(void)
{
	a_headless_device_always_wants_them();
	a_window_wants_them_only_for_one();
	return voe_test_result();
}
