// THE THREE PLACES AN INPUT LAYER LEAKS STATE, AND NONE OF THEM NEEDS A
// KEYBOARD OR A MOUSE TO CHECK.
//
// voe_platform_input_begin_poll, voe_platform_input_focus_lost and
// voe_platform_input_pointer_lost are the whole of what src/input.c decides.
// Everything else in this folder's input is a window system handing over an
// event, which cannot be tested without a compositor on one platform and a
// message queue on the other — but these three are arithmetic over one struct,
// and they are also the three that go wrong.
//
// FOCUS LOST WITH A KEY STILL DOWN IS THE ONE THE CARD NAMED. Alt-tab away while
// holding W and the release is delivered to whoever has focus now, which is not
// us: without the clear, W is held forever and a camera flies off with nobody
// touching anything. It is a bug that cannot be seen in a screenshot, does not
// crash, and only shows up when somebody switches windows — which is to say, not
// while it is being written. So it is a test.
//
// AND THE OTHER ONE IS THE MOTION ACCUMULATOR, WHICH IS DRAINED RATHER THAN
// OVERWRITTEN. A poll that brings no mouse movement must leave nought and not
// the previous frame's number; a poll that brings three events must leave their
// total. Getting that wrong gives a camera that keeps turning after the mouse
// has stopped, which reads as drift and gets blamed on the camera.
//
// AND THE THIRD IS THE POINTER GOING AWAY WITH A BUTTON STILL DOWN, which is the
// same leak as the keyboard's with a mouse in it: the release is delivered to
// whoever has the pointer now, and a button that never comes up is a drag that
// never ends. The position is deliberately kept through it — the public header
// promises a GUI the last place the pointer was seen — and a poll must leave the
// position, the buttons and the over-the-window flag exactly as they were,
// because a poll that cleared them would make every hover a single-frame one.
//
// IT INCLUDES platform's INTERNAL HEADER BY RELATIVE PATH, the same way render's
// tests reach that folder's internals: the struct these two functions operate on
// is not platform's public surface and must not become part of it so that a test
// can see it. Nothing here opens a window, so nothing here needs a display —
// this runs under ctest on a build box with no compositor anywhere.
#include "../src/input.h"

#include <testing/test.h>

// A poll leaves held keys alone, because a key does not stop being held because
// a frame went by. This is the half of begin_poll that is about what it must not
// do, and it is the easier one to get wrong in the other direction: clearing the
// keys here would make every key a single-frame press.
static void a_poll_keeps_held_keys(void)
{
	struct voe_platform_input input = { 0 };

	input.keys[VOE_PLATFORM_KEY_W] = true;
	input.keys[VOE_PLATFORM_KEY_SHIFT] = true;

	for (int frame = 0; frame < 3; frame++)
		voe_platform_input_begin_poll(&input);

	VOE_TEST_CHECK(input.keys[VOE_PLATFORM_KEY_W]);
	VOE_TEST_CHECK(input.keys[VOE_PLATFORM_KEY_SHIFT]);
	VOE_TEST_CHECK(!input.keys[VOE_PLATFORM_KEY_A]);
	VOE_TEST_CHECK(!input.keys[VOE_PLATFORM_KEY_ESCAPE]);
}

// A poll zeroes the motion, so that a frame with no mouse movement in it reports
// none rather than repeating the frame before.
static void a_poll_drains_the_motion(void)
{
	struct voe_platform_input input = { 0 };

	// What a backend does when the window system reports movement: it adds.
	input.motion_x += 4.0f;
	input.motion_y += -2.0f;
	input.motion_x += 1.0f;
	VOE_TEST_CHECK_FLOAT(input.motion_x, 5.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(input.motion_y, -2.0f, 0.0f);

	// The next frame begins and nothing has moved since.
	voe_platform_input_begin_poll(&input);
	VOE_TEST_CHECK_FLOAT(input.motion_x, 0.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(input.motion_y, 0.0f, 0.0f);

	// And a second empty poll leaves it at nought rather than doing anything
	// clever.
	voe_platform_input_begin_poll(&input);
	VOE_TEST_CHECK_FLOAT(input.motion_x, 0.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(input.motion_y, 0.0f, 0.0f);
}

// A poll does not touch the lock. It is the window system's answer, not
// something a frame boundary can change, and a poll that cleared it would make
// voe_platform_input_pointer_locked report false on every other frame.
static void a_poll_keeps_the_lock(void)
{
	struct voe_platform_input input = { 0 };

	input.pointer_locked = true;
	voe_platform_input_begin_poll(&input);
	VOE_TEST_CHECK(input.pointer_locked);
}

// EVERY KEY GOES UP AT ONCE WHEN FOCUS GOES, AND THAT IS THE CARD'S CASE. Every
// key is set, not just one, because a clear that got the size of the array wrong
// would pass with one key held at the front of it.
static void focus_lost_releases_every_key(void)
{
	struct voe_platform_input input = { 0 };

	for (int key = 0; key < VOE_PLATFORM_KEY_COUNT; key++)
		input.keys[key] = true;

	voe_platform_input_focus_lost(&input);

	for (int key = 0; key < VOE_PLATFORM_KEY_COUNT; key++)
		VOE_TEST_CHECK(!input.keys[key]);
}

// The motion goes with the keys, because alt-tab drags the pointer across the
// surface on some compositors and a camera that snapped round on the way out
// would look exactly like a bug in the camera.
static void focus_lost_throws_away_the_motion(void)
{
	struct voe_platform_input input = { 0 };

	input.motion_x = 900.0f;
	input.motion_y = -900.0f;

	voe_platform_input_focus_lost(&input);

	VOE_TEST_CHECK_FLOAT(input.motion_x, 0.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(input.motion_y, 0.0f, 0.0f);
}

// THE LOCK IS NOT CLEARED BY LOSING FOCUS, AND THAT IS DELIBERATE RATHER THAN AN
// OVERSIGHT IN THE CLEAR. Whether the pointer is still locked is the window
// system's to say: Wayland sends an unlocked event and may hand the lock back
// when focus returns without being asked, and the Win32 side sets the flag from
// its own apply_lock. A clear here would be this file overruling both of them.
static void focus_lost_leaves_the_lock_to_the_window_system(void)
{
	struct voe_platform_input input = { 0 };

	input.pointer_locked = true;
	input.keys[VOE_PLATFORM_KEY_W] = true;

	voe_platform_input_focus_lost(&input);

	VOE_TEST_CHECK(!input.keys[VOE_PLATFORM_KEY_W]);
	VOE_TEST_CHECK(input.pointer_locked);
}

// A poll leaves the pointer alone: where it is, whether it is over the window
// and which buttons are held are facts about now and not sums over the frame.
// This is the pointer's half of a_poll_keeps_held_keys, and it is the one that
// would go wrong if somebody widened begin_poll's clear to "everything mouse".
static void a_poll_keeps_the_pointer(void)
{
	struct voe_platform_input input = { 0 };

	input.pointer_x = 120.5f;
	input.pointer_y = 30.0f;
	input.pointer_over = true;
	input.buttons[VOE_PLATFORM_BUTTON_LEFT] = true;
	input.buttons[VOE_PLATFORM_BUTTON_MIDDLE] = true;

	for (int frame = 0; frame < 3; frame++)
		voe_platform_input_begin_poll(&input);

	VOE_TEST_CHECK_FLOAT(input.pointer_x, 120.5f, 0.0f);
	VOE_TEST_CHECK_FLOAT(input.pointer_y, 30.0f, 0.0f);
	VOE_TEST_CHECK(input.pointer_over);
	VOE_TEST_CHECK(input.buttons[VOE_PLATFORM_BUTTON_LEFT]);
	VOE_TEST_CHECK(input.buttons[VOE_PLATFORM_BUTTON_MIDDLE]);
	VOE_TEST_CHECK(!input.buttons[VOE_PLATFORM_BUTTON_RIGHT]);
}

// Focus is the keyboard's. A pointer resting over an unfocused window is still
// over it, still reports where, and a button held through an alt-tab is the
// pointer's business — see voe_platform_input_pointer_lost for when it lifts.
static void focus_lost_leaves_the_pointer_alone(void)
{
	struct voe_platform_input input = { 0 };

	input.pointer_x = 7.0f;
	input.pointer_y = 9.0f;
	input.pointer_over = true;
	input.buttons[VOE_PLATFORM_BUTTON_RIGHT] = true;
	input.keys[VOE_PLATFORM_KEY_W] = true;

	voe_platform_input_focus_lost(&input);

	VOE_TEST_CHECK(!input.keys[VOE_PLATFORM_KEY_W]);
	VOE_TEST_CHECK_FLOAT(input.pointer_x, 7.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(input.pointer_y, 9.0f, 0.0f);
	VOE_TEST_CHECK(input.pointer_over);
	VOE_TEST_CHECK(input.buttons[VOE_PLATFORM_BUTTON_RIGHT]);
}

// EVERY BUTTON GOES UP WHEN THE POINTER GOES, AND THE POSITION STAYS. Every
// button is set, not just one, for the same reason every key is set in the
// keyboard's test: a clear that got the size wrong would pass with one held at
// the front.
static void pointer_lost_releases_every_button_and_keeps_the_position(void)
{
	struct voe_platform_input input = { 0 };

	input.pointer_x = 400.0f;
	input.pointer_y = 300.0f;
	input.pointer_over = true;
	for (int button = 0; button < VOE_PLATFORM_BUTTON_COUNT; button++)
		input.buttons[button] = true;

	voe_platform_input_pointer_lost(&input);

	for (int button = 0; button < VOE_PLATFORM_BUTTON_COUNT; button++)
		VOE_TEST_CHECK(!input.buttons[button]);
	VOE_TEST_CHECK(!input.pointer_over);
	VOE_TEST_CHECK_FLOAT(input.pointer_x, 400.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(input.pointer_y, 300.0f, 0.0f);
}

// The pointer going away does not lift a finger off the keyboard, and does not
// touch the lock or the motion — a pointer that leaves the window while the
// camera is looking round is a compositor's business, and the motion it brought
// this frame is still this frame's.
static void pointer_lost_leaves_the_keyboard_the_lock_and_the_motion(void)
{
	struct voe_platform_input input = { 0 };

	input.keys[VOE_PLATFORM_KEY_W] = true;
	input.pointer_locked = true;
	input.motion_x = 3.0f;
	input.motion_y = -1.0f;
	input.buttons[VOE_PLATFORM_BUTTON_LEFT] = true;

	voe_platform_input_pointer_lost(&input);

	VOE_TEST_CHECK(input.keys[VOE_PLATFORM_KEY_W]);
	VOE_TEST_CHECK(input.pointer_locked);
	VOE_TEST_CHECK_FLOAT(input.motion_x, 3.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(input.motion_y, -1.0f, 0.0f);
	VOE_TEST_CHECK(!input.buttons[VOE_PLATFORM_BUTTON_LEFT]);
}

int main(void)
{
	a_poll_keeps_held_keys();
	a_poll_drains_the_motion();
	a_poll_keeps_the_lock();
	a_poll_keeps_the_pointer();
	focus_lost_releases_every_key();
	focus_lost_throws_away_the_motion();
	focus_lost_leaves_the_lock_to_the_window_system();
	focus_lost_leaves_the_pointer_alone();
	pointer_lost_releases_every_button_and_keeps_the_position();
	pointer_lost_leaves_the_keyboard_the_lock_and_the_motion();
	return voe_test_result();
}
