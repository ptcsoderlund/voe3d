// What src/gamepad.h promises, checked with no device: four attaches fill
// slots 0 to 3 and a fifth is refused; a slot freed and taken again gets a
// higher id than any before and a freed slot is all zero; an axis and a trigger
// map both ends, the middle and past both ends, and a range of no width is 0.
//
// Needs no window, no display and no pad.
#include "../src/gamepad.h"

#include <testing/test.h>

#include <string.h>

static void check_slots(void)
{
	struct voe_platform_gamepads pads = { 0 };
	voe_platform_gamepad zero = { 0 };
	uint32_t highest = 0;

	for (int slot = 0; slot < VOE_PLATFORM_GAMEPAD_SLOTS; slot++) {
		VOE_TEST_CHECK_INT(voe_platform_gamepad_attach(&pads), slot);
		VOE_TEST_CHECK(pads.slots[slot].connected);
		VOE_TEST_CHECK(pads.slots[slot].id > highest);
		highest = pads.slots[slot].id;
	}
	VOE_TEST_CHECK_INT(voe_platform_gamepad_attach(&pads), -1);

	pads.slots[1].left_x = 0.5f;
	pads.slots[1].buttons[VOE_PLATFORM_GAMEPAD_BUTTON_SOUTH] = true;
	voe_platform_gamepad_detach(&pads, 1);
	VOE_TEST_CHECK(memcmp(&pads.slots[1], &zero, sizeof(zero)) == 0);

	VOE_TEST_CHECK_INT(voe_platform_gamepad_attach(&pads), 1);
	VOE_TEST_CHECK(pads.slots[1].connected);
	VOE_TEST_CHECK(pads.slots[1].id > highest);
	VOE_TEST_CHECK(pads.slots[1].left_x == 0.0f);
	VOE_TEST_CHECK(!pads.slots[1].buttons[VOE_PLATFORM_GAMEPAD_BUTTON_SOUTH]);
}

static void check_axis(void)
{
	VOE_TEST_CHECK_FLOAT(voe_platform_gamepad_axis(-32768, -32768, 32767),
			     -1.0f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(voe_platform_gamepad_axis(32767, -32768, 32767),
			     1.0f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(voe_platform_gamepad_axis(128, 0, 256), 0.0f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(voe_platform_gamepad_axis(64, 0, 256), -0.5f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(voe_platform_gamepad_axis(-10, 0, 255), -1.0f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(voe_platform_gamepad_axis(300, 0, 255), 1.0f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(voe_platform_gamepad_axis(5, 7, 7), 0.0f, 0.0f);
}

static void check_trigger(void)
{
	VOE_TEST_CHECK_FLOAT(voe_platform_gamepad_trigger(0, 0, 255), 0.0f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(voe_platform_gamepad_trigger(255, 0, 255), 1.0f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(voe_platform_gamepad_trigger(128, 0, 256), 0.5f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(voe_platform_gamepad_trigger(-5, 0, 255), 0.0f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(voe_platform_gamepad_trigger(999, 0, 255), 1.0f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(voe_platform_gamepad_trigger(3, 3, 3), 0.0f, 0.0f);
}

int main(void)
{
	check_slots();
	check_axis();
	check_trigger();
	return voe_test_result();
}
