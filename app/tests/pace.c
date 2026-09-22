// The pace's decision, driven with numbers chosen here. Needs no window and no
// graphics card, which is why the decision is a pure function in app/src/pace.h.
//
// The storms are the point (ADR-0216, bug 017/01). They run the loop app.c's
// wait_for_pace runs against a wait that returns after one millisecond however
// long the step asked for — which is what an unfocused Wayland window's own
// connection traffic does — and prove that the heartbeat is still a quarter
// second of clock and that the loop ends.
#include "../src/pace.h"

#include <testing/test.h>

// A window the loop can ask, and the clock the storm advances.
typedef struct {
	bool focused;
	bool visible;
	bool closing;
	double now;
	double last_open;
} storm;

// One turn of wait_for_pace: ask the step, and on a wait spend a millisecond of
// clock and poll (which here changes nothing the caller did not change itself).
// True when the step said draw.
static bool storm_turn(storm *s)
{
	voe_app_pace_step step;

	step = voe_app_pace_next(s->focused, s->visible, s->closing, s->now,
				 s->last_open);
	if (step.kind == VOE_APP_PACE_DRAW)
		return true;
	s->now += 0.001;
	return false;
}

// The turns taken before the loop drew, or -1 if it was still waiting after
// `limit` of them.
static int storm_turns_to_draw(storm *s, int limit)
{
	for (int i = 0; i < limit; i++) {
		if (storm_turn(s))
			return i;
	}
	return -1;
}

static void check_focused_never_waits(void)
{
	VOE_TEST_CHECK_INT(voe_app_pace_next(true, true, false, 100.0, 100.0).kind,
			   VOE_APP_PACE_DRAW);
	VOE_TEST_CHECK_INT(voe_app_pace_next(true, true, false, 100.0, 0.0).kind,
			   VOE_APP_PACE_DRAW);
}

static void check_hidden_waits_with_no_timeout(void)
{
	voe_app_pace_step hidden = voe_app_pace_next(false, false, false, 100.0,
						     100.0);
	voe_app_pace_step focused = voe_app_pace_next(true, false, false, 100.0,
						      100.0);

	VOE_TEST_CHECK_INT(hidden.kind, VOE_APP_PACE_WAIT);
	VOE_TEST_CHECK(hidden.seconds < 0.0);
	VOE_TEST_CHECK_INT(focused.kind, VOE_APP_PACE_WAIT);
	VOE_TEST_CHECK(focused.seconds < 0.0);
}

static void check_unfocused_waits_out_the_heartbeat(void)
{
	voe_app_pace_step early = voe_app_pace_next(false, true, false, 100.1,
						    100.0);

	VOE_TEST_CHECK_INT(early.kind, VOE_APP_PACE_WAIT);
	VOE_TEST_CHECK_FLOAT(early.seconds, VOE_APP_HEARTBEAT_SECONDS - 0.1,
			     1e-9);
	VOE_TEST_CHECK_INT(voe_app_pace_next(false, true, false,
					     100.0 + VOE_APP_HEARTBEAT_SECONDS,
					     100.0).kind,
			   VOE_APP_PACE_DRAW);
	VOE_TEST_CHECK_INT(voe_app_pace_next(false, true, false, 102.0, 100.0).kind,
			   VOE_APP_PACE_DRAW);
}

// A rest below one millisecond is a draw, not a wait that would be truncated to
// nothing and spun through.
static void check_the_last_sliver_is_a_draw(void)
{
	VOE_TEST_CHECK_INT(voe_app_pace_next(false, true, false, 100.2495,
					     100.0).kind,
			   VOE_APP_PACE_DRAW);
	VOE_TEST_CHECK_INT(voe_app_pace_next(false, true, false, 100.248,
					     100.0).kind,
			   VOE_APP_PACE_WAIT);
}

static void check_a_storm_cannot_beat_the_heartbeat(void)
{
	storm s = { .visible = true, .now = 100.0, .last_open = 100.0 };
	int turns = storm_turns_to_draw(&s, 10000);

	VOE_TEST_CHECK(turns >= 0);
	VOE_TEST_CHECK(s.now - s.last_open >=
		       VOE_APP_HEARTBEAT_SECONDS - 2.0 * VOE_APP_PACE_FLOOR_SECONDS);
	VOE_TEST_CHECK(turns <= 260);
}

static void check_a_hidden_storm_draws_nothing_until_shown(void)
{
	storm s = { .visible = false, .now = 100.0, .last_open = 100.0 };

	VOE_TEST_CHECK_INT(storm_turns_to_draw(&s, 1000), -1);
	VOE_TEST_CHECK(s.now - s.last_open > VOE_APP_HEARTBEAT_SECONDS);
	s.visible = true;
	VOE_TEST_CHECK_INT(storm_turns_to_draw(&s, 1000), 0);
}

static void check_a_hidden_storm_draws_nothing_until_closing(void)
{
	storm s = { .focused = true, .visible = false, .now = 100.0,
		    .last_open = 100.0 };

	VOE_TEST_CHECK_INT(storm_turns_to_draw(&s, 1000), -1);
	s.closing = true;
	VOE_TEST_CHECK_INT(storm_turns_to_draw(&s, 1000), 0);
}

static void check_closing_draws_from_every_state(void)
{
	VOE_TEST_CHECK_INT(voe_app_pace_next(true, true, true, 100.0, 100.0).kind,
			   VOE_APP_PACE_DRAW);
	VOE_TEST_CHECK_INT(voe_app_pace_next(false, true, true, 100.0, 100.0).kind,
			   VOE_APP_PACE_DRAW);
	VOE_TEST_CHECK_INT(voe_app_pace_next(false, false, true, 100.0, 100.0).kind,
			   VOE_APP_PACE_DRAW);
}

int main(void)
{
	check_focused_never_waits();
	check_hidden_waits_with_no_timeout();
	check_unfocused_waits_out_the_heartbeat();
	check_the_last_sliver_is_a_draw();
	check_a_storm_cannot_beat_the_heartbeat();
	check_a_hidden_storm_draws_nothing_until_shown();
	check_a_hidden_storm_draws_nothing_until_closing();
	check_closing_draws_from_every_state();
	return voe_test_result();
}
