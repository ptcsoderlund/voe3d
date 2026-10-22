// The hitch rule and its line, driven with numbers chosen here. Needs no
// window and no graphics card. A frame of 50 ms after sixty of 16 is a hitch
// and names its largest stretch first; 20 ms is not, nor is 50 after only
// thirty; a card frame sums passes of one name with their count; a line lists
// five parts at most and stays inside a small room.
#include <app/hitches.h>

#include <testing/test.h>

#include <string.h>

static void warm_frames(voe_app_hitches *hitches, uint32_t frames)
{
	char line[VOE_APP_HITCH_LINE];

	for (uint32_t i = 0; i < frames; i++)
		VOE_TEST_CHECK(!voe_app_hitches_frame(hitches, 0.016, NULL, 0, line,
						      sizeof line));
}

static void check_a_stalled_frame_is_named(void)
{
	const voe_app_stretch stretches[] = {
		{ "open", 0.005 }, { "views", 0.040 }, { "end", 0.005 },
	};
	voe_app_hitches hitches = { 0 };
	char line[VOE_APP_HITCH_LINE];

	warm_frames(&hitches, 60);
	VOE_TEST_CHECK(voe_app_hitches_frame(&hitches, 0.050, stretches, 3, line,
					     sizeof line));
	VOE_TEST_CHECK(strncmp(line, "frame 50.0 ms", 13) == 0);
	VOE_TEST_CHECK(strstr(line, "views") != NULL);
	VOE_TEST_CHECK(strstr(line, "views") < strstr(line, "open"));
}

static void check_a_small_rise_is_not_a_hitch(void)
{
	voe_app_hitches hitches = { 0 };
	char line[VOE_APP_HITCH_LINE] = "untouched";

	warm_frames(&hitches, 60);
	VOE_TEST_CHECK(!voe_app_hitches_frame(&hitches, 0.020, NULL, 0, line,
					      sizeof line));
	VOE_TEST_CHECK(strcmp(line, "untouched") == 0);
}

static void check_too_few_samples_is_not_a_hitch(void)
{
	voe_app_hitches hitches = { 0 };
	char line[VOE_APP_HITCH_LINE];

	warm_frames(&hitches, 30);
	VOE_TEST_CHECK(!voe_app_hitches_frame(&hitches, 0.050, NULL, 0, line,
					      sizeof line));
}

static void check_a_card_frame_sums_passes_by_name(void)
{
	const voe_render_pass_time passes[] = {
		{ "bounce sun shadow", 0.010 },
		{ "bounce relight", 0.006 },
		{ "bounce sun shadow", 0.020 },
		{ "view target 0", 0.003 },
	};
	voe_app_hitches hitches = { 0 };
	char line[VOE_APP_HITCH_LINE];

	for (uint32_t i = 0; i < 60; i++)
		VOE_TEST_CHECK(!voe_app_hitches_card(&hitches, 0.009, NULL, 0,
						     line, sizeof line));
	VOE_TEST_CHECK(voe_app_hitches_card(&hitches, 0.041, passes, 4, line,
					    sizeof line));
	VOE_TEST_CHECK(strncmp(line, "card 41.0 ms (mean 9.0): ", 25) == 0);
	VOE_TEST_CHECK(strncmp(line + 25,
			       "bounce sun shadow 2x 30.0, bounce relight 6.0",
			       45) == 0);
}

static void check_a_line_lists_five(void)
{
	const voe_app_stretch stretches[] = {
		{ "a", 0.007 }, { "b", 0.006 }, { "c", 0.005 }, { "d", 0.004 },
		{ "e", 0.003 }, { "f", 0.002 }, { "g", 0.001 },
	};
	voe_app_hitches hitches = { 0 };
	char line[VOE_APP_HITCH_LINE];
	uint32_t commas = 0;

	warm_frames(&hitches, 60);
	VOE_TEST_CHECK(voe_app_hitches_frame(&hitches, 0.050, stretches, 7, line,
					     sizeof line));
	for (const char *at = line; *at != '\0'; at++)
		commas += *at == ',';
	VOE_TEST_CHECK_INT(commas, 4);
	VOE_TEST_CHECK(strstr(line, "f 2.0") == NULL);
}

static void check_a_small_room_is_kept(void)
{
	voe_app_hitches hitches = { 0 };
	char line[32];

	memset(line, 'x', sizeof line);
	warm_frames(&hitches, 60);
	VOE_TEST_CHECK(voe_app_hitches_frame(&hitches, 0.050, NULL, 0, line, 16));
	VOE_TEST_CHECK(memchr(line, '\0', 16) != NULL);
	for (size_t i = 16; i < sizeof line; i++)
		VOE_TEST_CHECK(line[i] == 'x');
}

int main(void)
{
	check_a_stalled_frame_is_named();
	check_a_small_rise_is_not_a_hitch();
	check_too_few_samples_is_not_a_hitch();
	check_a_card_frame_sums_passes_by_name();
	check_a_line_lists_five();
	check_a_small_room_is_kept();
	return voe_test_result();
}
