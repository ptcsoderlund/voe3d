// What the Best Practices classifier keeps for the close to name (ADR-0358),
// proved on made-up messages: a zeroed device on an NVIDIA card (vendor_id
// 0x10DE) and no Vulkan opened, so this test needs no graphics card.
//
// The cases: an allowlisted warning allowed and kept nowhere; an AMD-tagged id
// dropped and kept nowhere; one new id three times with three texts one row,
// count 3, the first text; an error with an allowlisted id new and kept as an
// error; a NULL id and a NULL text kept under "(no id)"; a text longer than a
// row holds cut and terminated, the next row untouched; more distinct ids than
// rows, the rest counted as overflow and new_messages the rows' counts plus it;
// and _list_new returning on that device (its stderr is not read).
//
// It includes render's internal device_internal.h by relative path, as card.c
// includes startup.h: the kept rows are not part of render's surface.
#include "../src/device_internal.h"

#include <testing/test.h>

#include <stdio.h>
#include <string.h>

// Large, so static rather than on the stack; each case starts from it zeroed.
static voe_render_device device;

static voe_render_device *fresh_nvidia_device(void)
{
	memset(&device, 0, sizeof(device));
	device.vendor_id = 0x10DE;
	return &device;
}

static void an_allowed_or_dropped_message_keeps_no_row(void)
{
	voe_render_device *d = fresh_nvidia_device();

	VOE_TEST_CHECK(voe_render_best_practices_classify(
			       d, false, "BestPractices-PushConstants", "pushed") ==
		       VOE_RENDER_MESSAGE_ALLOWED);
	VOE_TEST_CHECK(voe_render_best_practices_classify(
			       d, false, "BestPractices-AMD-MadeUp-Message",
			       "amd") == VOE_RENDER_MESSAGE_DROPPED);
	VOE_TEST_CHECK_INT(d->new_messages, 0);
	VOE_TEST_CHECK_INT(d->kept[0].count, 0);
	VOE_TEST_CHECK_INT(d->kept_overflow, 0);
}

static void one_id_three_times_is_one_row_with_the_first_text(void)
{
	voe_render_device *d = fresh_nvidia_device();
	const char *texts[3] = { "first", "second", "third" };

	for (uint32_t i = 0; i < 3; i++)
		VOE_TEST_CHECK(voe_render_best_practices_classify(
				       d, false, "BestPractices-vkMadeUp-new",
				       texts[i]) == VOE_RENDER_MESSAGE_NEW);
	VOE_TEST_CHECK_INT(d->new_messages, 3);
	VOE_TEST_CHECK_INT(d->kept[0].count, 3);
	VOE_TEST_CHECK(!d->kept[0].error);
	VOE_TEST_CHECK(strcmp(d->kept[0].id_name, "BestPractices-vkMadeUp-new") ==
		       0);
	VOE_TEST_CHECK(strcmp(d->kept[0].text, "first") == 0);
	VOE_TEST_CHECK_INT(d->kept[1].count, 0);
}

static void an_error_with_an_allowed_id_is_new_and_an_error(void)
{
	voe_render_device *d = fresh_nvidia_device();

	VOE_TEST_CHECK(voe_render_best_practices_classify(
			       d, true, "BestPractices-PushConstants", "err") ==
		       VOE_RENDER_MESSAGE_NEW);
	VOE_TEST_CHECK_INT(d->new_messages, 1);
	VOE_TEST_CHECK_INT(d->kept[0].count, 1);
	VOE_TEST_CHECK(d->kept[0].error);
	VOE_TEST_CHECK(strcmp(d->kept[0].id_name, "BestPractices-PushConstants") ==
		       0);
}

static void no_id_and_no_text_are_kept_under_no_id(void)
{
	voe_render_device *d = fresh_nvidia_device();

	VOE_TEST_CHECK(voe_render_best_practices_classify(d, false, NULL, NULL) ==
		       VOE_RENDER_MESSAGE_NEW);
	VOE_TEST_CHECK_INT(d->new_messages, 1);
	VOE_TEST_CHECK_INT(d->kept[0].count, 1);
	VOE_TEST_CHECK(strcmp(d->kept[0].id_name, "(no id)") == 0);
	VOE_TEST_CHECK(d->kept[0].text[0] != '\0');
}

static void a_long_text_is_cut_and_terminated(void)
{
	voe_render_device *d = fresh_nvidia_device();
	static char text[2 * VOE_RENDER_KEPT_TEXT_BYTES];

	memset(text, 'x', sizeof(text) - 1);
	text[sizeof(text) - 1] = '\0';
	VOE_TEST_CHECK(voe_render_best_practices_classify(
			       d, false, "BestPractices-vkMadeUp-long", text) ==
		       VOE_RENDER_MESSAGE_NEW);
	VOE_TEST_CHECK_INT(strlen(d->kept[0].text),
			   VOE_RENDER_KEPT_TEXT_BYTES - 1);
	VOE_TEST_CHECK(d->kept[0].text[VOE_RENDER_KEPT_TEXT_BYTES - 1] == '\0');
	VOE_TEST_CHECK_INT(d->kept[1].count, 0);
	VOE_TEST_CHECK(d->kept[1].id_name[0] == '\0');
}

// Ends with _list_new on the full device: it must return, overflow and all.
static void more_ids_than_rows_overflow_and_still_list(void)
{
	voe_render_device *d = fresh_nvidia_device();
	const uint32_t ids = VOE_RENDER_KEPT_MESSAGES + 3;
	uint32_t kept = 0;
	char id[64];

	for (uint32_t i = 0; i < ids; i++) {
		snprintf(id, sizeof(id), "BestPractices-vkMadeUp-%u", i);
		VOE_TEST_CHECK(voe_render_best_practices_classify(d, false, id,
								  "text") ==
			       VOE_RENDER_MESSAGE_NEW);
	}
	// One more of the first id lands on its row, not the overflow.
	VOE_TEST_CHECK(voe_render_best_practices_classify(
			       d, false, "BestPractices-vkMadeUp-0", "again") ==
		       VOE_RENDER_MESSAGE_NEW);
	for (uint32_t i = 0; i < VOE_RENDER_KEPT_MESSAGES; i++)
		kept += d->kept[i].count;
	VOE_TEST_CHECK_INT(d->kept[0].count, 2);
	VOE_TEST_CHECK_INT(d->kept_overflow, 3);
	VOE_TEST_CHECK_INT(d->new_messages, ids + 1);
	VOE_TEST_CHECK_INT(d->new_messages, kept + d->kept_overflow);

	voe_render_best_practices_list_new(d);
}

int main(void)
{
	an_allowed_or_dropped_message_keeps_no_row();
	one_id_three_times_is_one_row_with_the_first_text();
	an_error_with_an_allowed_id_is_new_and_an_error();
	no_id_and_no_text_are_kept_under_no_id();
	a_long_text_is_cut_and_terminated();
	more_ids_than_rows_overflow_and_still_list();
	return voe_test_result();
}
