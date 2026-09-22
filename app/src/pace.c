// The pace's four cases, in the order they are asked. The reasoning is in
// app/src/pace.h.
#include "pace.h"

#include <base/assert.h>

voe_app_pace_step voe_app_pace_next(bool focused, bool visible, bool closing,
				    double now, double last_open)
{
	double left;

	VOE_BASE_ASSERT(now >= 0.0, "a clock reading is not negative");
	VOE_BASE_ASSERT(last_open >= 0.0, "a clock reading is not negative");

	if (closing)
		return (voe_app_pace_step){ VOE_APP_PACE_DRAW, 0.0 };
	if (!visible)
		return (voe_app_pace_step){ VOE_APP_PACE_WAIT, -1.0 };
	if (focused)
		return (voe_app_pace_step){ VOE_APP_PACE_DRAW, 0.0 };
	left = VOE_APP_HEARTBEAT_SECONDS - (now - last_open);
	if (left < VOE_APP_PACE_FLOOR_SECONDS)
		return (voe_app_pace_step){ VOE_APP_PACE_DRAW, 0.0 };
	return (voe_app_pace_step){ VOE_APP_PACE_WAIT, left };
}
