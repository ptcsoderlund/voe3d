// The pace's three cases. The reasoning is in app/src/pace.h.
#include "pace.h"

#include <base/assert.h>

double voe_app_pace_wait(bool focused, bool visible, double now,
			 double last_open)
{
	double left;

	VOE_BASE_ASSERT(now >= 0.0, "a clock reading is not negative");
	VOE_BASE_ASSERT(last_open >= 0.0, "a clock reading is not negative");

	if (!visible)
		return -1.0;
	if (focused)
		return 0.0;
	left = VOE_APP_HEARTBEAT_SECONDS - (now - last_open);
	return left > 0.0 ? left : 0.0;
}
