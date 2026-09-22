// Which graphics card the engine takes, proved on made-up facts: the ranking of
// 0214 is a pure function, so this test needs no graphics card and never opens
// Vulkan. Each claim is one a first-listed or kind-only choice would get wrong.
//
// It includes render's internal startup.h by relative path, as the other tests
// include device_internal.h: the ranking is not part of render's surface.
#include "../src/startup.h"

#include <testing/test.h>

#include <stdint.h>

#define GIB (1024ull * 1024ull * 1024ull)

static voe_render_card_facts card(VkPhysicalDeviceType kind, uint64_t memory)
{
	return (voe_render_card_facts){
		.kind = kind,
		.memory = memory,
		.vulkan_1_3 = true,
		.draws = true,
		.presents = true,
	};
}

static void the_discrete_card_wins_wherever_it_is_listed(void)
{
	const voe_render_card_facts cards[] = {
		card(VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU, 32 * GIB),
		card(VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, 12 * GIB),
		card(VK_PHYSICAL_DEVICE_TYPE_CPU, 64 * GIB),
	};
	uint32_t fastest;

	VOE_TEST_CHECK_INT(voe_render_card_rank(cards, 3, false, &fastest), 1);
	VOE_TEST_CHECK_INT(fastest, 1);
}

static void a_discrete_card_that_cannot_present_falls_back(void)
{
	voe_render_card_facts cards[] = {
		card(VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU, 32 * GIB),
		card(VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, 12 * GIB),
		card(VK_PHYSICAL_DEVICE_TYPE_CPU, 64 * GIB),
	};
	uint32_t fastest;

	cards[1].presents = false;
	VOE_TEST_CHECK_INT(voe_render_card_rank(cards, 3, false, &fastest), 0);
	VOE_TEST_CHECK_INT(fastest, 1);

	// Headless nothing is presented, so not presenting costs nothing.
	VOE_TEST_CHECK_INT(voe_render_card_rank(cards, 3, true, &fastest), 1);
	VOE_TEST_CHECK_INT(fastest, 1);
}

static void a_software_rasteriser_alone_is_taken(void)
{
	const voe_render_card_facts cards[] = {
		card(VK_PHYSICAL_DEVICE_TYPE_CPU, 8 * GIB),
	};
	uint32_t fastest;

	VOE_TEST_CHECK_INT(voe_render_card_rank(cards, 1, false, &fastest), 0);
	VOE_TEST_CHECK_INT(fastest, 0);
}

static void of_two_discrete_cards_the_larger_memory_wins(void)
{
	const voe_render_card_facts cards[] = {
		card(VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, 8 * GIB),
		card(VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, 16 * GIB),
		card(VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, 16 * GIB),
	};
	uint32_t fastest;

	// The two 16 GiB cards tie, and the earlier one stays.
	VOE_TEST_CHECK_INT(voe_render_card_rank(cards, 3, false, &fastest), 1);
	VOE_TEST_CHECK_INT(fastest, 1);
}

static void a_card_without_1_3_or_a_drawing_queue_is_never_taken(void)
{
	voe_render_card_facts cards[] = {
		card(VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, 24 * GIB),
		card(VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, 16 * GIB),
		card(VK_PHYSICAL_DEVICE_TYPE_CPU, 1 * GIB),
	};
	uint32_t fastest;

	cards[0].vulkan_1_3 = false;
	cards[1].draws = false;
	VOE_TEST_CHECK_INT(voe_render_card_rank(cards, 3, false, &fastest), 2);
	VOE_TEST_CHECK_INT(fastest, 2);
	VOE_TEST_CHECK_INT(voe_render_card_rank(cards, 3, true, &fastest), 2);
}

static void none_qualifying_is_uint32_max(void)
{
	voe_render_card_facts cards[] = {
		card(VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, 24 * GIB),
		card(VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU, 16 * GIB),
	};
	uint32_t fastest;

	cards[0].vulkan_1_3 = false;
	cards[1].presents = false;
	VOE_TEST_CHECK(voe_render_card_rank(cards, 2, false, &fastest) == UINT32_MAX);
	VOE_TEST_CHECK_INT(fastest, 1);

	cards[1].draws = false;
	VOE_TEST_CHECK(voe_render_card_rank(cards, 2, true, &fastest) == UINT32_MAX);
	VOE_TEST_CHECK(fastest == UINT32_MAX);
	VOE_TEST_CHECK(voe_render_card_rank(cards, 0, false, &fastest) == UINT32_MAX);
}

int main(void)
{
	the_discrete_card_wins_wherever_it_is_listed();
	a_discrete_card_that_cannot_present_falls_back();
	a_software_rasteriser_alone_is_taken();
	of_two_discrete_cards_the_larger_memory_wins();
	a_card_without_1_3_or_a_drawing_queue_is_never_taken();
	none_qualifying_is_uint32_max();
	return voe_test_result();
}
