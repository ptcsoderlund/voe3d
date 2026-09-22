// The pace's decision, driven with numbers chosen here. Needs no window and no
// graphics card, which is why the decision is a pure function in app/src/pace.h.
#include "../src/pace.h"

#include <testing/test.h>

#define NEARLY 1e-12

static void check_focused_never_waits(void)
{
	VOE_TEST_CHECK_FLOAT(voe_app_pace_wait(true, true, 100.0, 100.0), 0.0,
			     NEARLY);
	VOE_TEST_CHECK_FLOAT(voe_app_pace_wait(true, true, 100.0, 0.0), 0.0,
			     NEARLY);
}

static void check_hidden_waits_with_no_timeout(void)
{
	VOE_TEST_CHECK(voe_app_pace_wait(false, false, 100.0, 100.0) < 0.0);
	VOE_TEST_CHECK(voe_app_pace_wait(true, false, 100.0, 100.0) < 0.0);
}

static void check_unfocused_waits_out_the_heartbeat(void)
{
	VOE_TEST_CHECK_FLOAT(voe_app_pace_wait(false, true, 100.1, 100.0),
			     VOE_APP_HEARTBEAT_SECONDS - 0.1, 1e-9);
	VOE_TEST_CHECK_FLOAT(voe_app_pace_wait(false, true,
					       100.0 + VOE_APP_HEARTBEAT_SECONDS,
					       100.0),
			     0.0, NEARLY);
	VOE_TEST_CHECK_FLOAT(voe_app_pace_wait(false, true, 102.0, 100.0), 0.0,
			     NEARLY);
}

int main(void)
{
	check_focused_never_waits();
	check_hidden_waits_with_no_timeout();
	check_unfocused_waits_out_the_heartbeat();
	return voe_test_result();
}
