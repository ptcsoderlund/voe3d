// voe_dev — the one program a person runs to see what the engine can currently
// do. Today it opens a window with two cubes in it and a camera that either
// orbits them or is flown by hand. There is one of these and it always shows the
// current state, so what is here now is expected to be deleted rather than kept
// behind a flag when the next thing lands.
//
// NO ENGINE LOGIC LIVES HERE. This file is a call site. Anything in it that
// starts to look worth keeping belongs in a folder, with a test — the moment it
// is worth testing it is in the wrong place. Nothing below decides where the
// camera is or how fast it goes: that is render's. What is below is which key
// means which direction, which is a binding and belongs at a call site, and the
// one bool that says whether the camera is being flown.
//
// NOT ONE #ifdef. If this file ever needs to know which operating system it is
// on, the API in platform/window.h, platform/input.h or render/device.h has a
// hole and that is the finding, not a reason to reach for the preprocessor.
//
// WHAT IT SHOULD LOOK LIKE. A flat blue-green background with two one-metre
// cubes standing on nothing, each corner of each cube a different colour so that
// every face is a gradient and no two faces read the same. It prints a line
// whenever something changes — the window's size, who is drawing its frame,
// whether the camera is being flown, whether the pointer is locked — and exits
// zero when the window is closed.
//
// TAB FLIES IT AND TAB HANDS IT BACK, AND BOTH STATES ARE WORTH LOOKING AT.
// There are two things to check here and they need different cameras: whether
// the rendering is right, which wants a camera nobody is touching, and whether
// the input is right, which wants a hand on it. Tab switches, Escape always
// hands back, and the orbit is what the program starts in.
//
// ---- ORBITING: THREE MOTIONS, AND ALL THREE HAVE TO BE THERE ----
//
// This is the thing to look at for the rendering, and the reason there are two
// cubes rather than one:
//
//   - The camera orbits, once every twelve seconds or so. What says so is the
//     spinning cube: it starts to the right of its neighbour, swings across the
//     frame passing in front of it, ends up on the left, and comes back round
//     behind. Parallax is the only thing that can move one object past another,
//     so if that happens the camera is moving and not the cubes.
//   - One cube stands still, in the middle of the frame, and stays there. Its
//     faces turn because the camera goes round it. It never drifts, never
//     changes size and never leaves the centre — the camera looks straight at
//     it, from wherever it is.
//   - The other cube spins, three times as fast as the camera orbits, about a
//     tilted axis so that it cannot be mistaken for a second orbit. It brings
//     its top and bottom faces round as it goes, and its own centre never moves.
//
// With one cube none of this would be checkable: a camera orbiting a cube and a
// cube rotating in front of a still camera draw the same picture.
//
// WHAT IS WRONG IF IT LOOKS WRONG. Each of the three motions fails in its own
// way, which is the whole point of having three:
//
//   - Nothing on screen, or a cube inside out — the depth test or the winding.
//     render/tests/offscreen.c is the automated form of that one.
//   - Both cubes drifting or growing — the projection or the aspect ratio.
//   - The picture upside down — the one Y flip went the wrong way or happened
//     twice. A cube's top face is the bright one; it should be up.
//   - The still cube not still, or not centred — the model matrix or the look-at.
//     render/tests/matrix.c checks both on the CPU, so this should have failed
//     before it got here.
//
// ---- FLYING: W A S D, SPACE, CTRL, SHIFT, AND THE MOUSE ----
//
// Tab, then: W and S forwards and back along where the camera is looking, A and
// D left and right, Space and Ctrl straight up and down whatever the camera is
// looking at, Shift to go four times as fast, and the mouse to look around. Tab
// again or Escape to give it back.
//
// WHAT IS WRONG IF IT FEELS WRONG, AND EACH OF THESE IS A DIFFERENT MISTAKE:
//
//   - The view jumps the moment Tab is pressed — the handover is not seeding the
//     flown camera from where the orbit had reached. It should be seamless: the
//     picture holds still and then answers the keyboard.
//   - The mouse turns the wrong way, or up looks down — a sign on one of the two
//     angles in render/src/cube.c.
//   - Looking straight up or straight down and everything vanishes — the pitch
//     clamp. Try to look further up than you can; it should simply stop.
//   - Strafing while looking at the floor sinks into it — right is being taken
//     from the camera's own frame rather than kept horizontal.
//   - A diagonal is faster than a straight line — the movement direction is not
//     being normalized. Hold W, then hold W and D, and the speed should not
//     change.
//   - The camera keeps flying with nobody touching anything — a held key that
//     was never released. This is the one to look for after alt-tabbing away
//     with W down; see below.
//
// WHAT THERE IS TO TRY:
//
//   - Alt-tab away while holding W, and come back. It must not still be flying
//     when focus is gone, and it must not need a fresh press of W to notice it
//     is still held on the way back. Both window systems say when focus goes and
//     what is held when it returns, and this is where an input layer leaks state.
//   - Look around, a lot, in one direction. It should not slow down, drift or
//     stick — mouse look with no pointer lock walks the cursor out of the window
//     and stops, which is what the `locked` line is for.
//   - Watch the `locked` line. Asking to fly asks for the pointer; a compositor
//     may say no, and then mouse look works only while the cursor happens to be
//     over the window. That is not a failure and nothing here treats it as one.
//   - Resize it. A `size` line should follow, the background should still reach
//     every corner with no border and no tearing at the edge, and the cubes
//     should stay the same shape rather than stretching — a wider window shows
//     more of the scene, it does not squash it. That is the swapchain being
//     rebuilt and the aspect ratio following it.
//   - Make it very wide or very tall. The cubes stay cubes. A projection that
//     multiplied by the aspect ratio instead of dividing would fail here and
//     nowhere else.
//   - Toggle the frame off and on. On KWin: right-click the titlebar ->
//     More Actions -> No Borders, or Alt+F3 for the same menu. A `size` line
//     follows and a `decorated` line does not. That is the measured answer and
//     not a gap: KWin does re-answer on every toggle, and every answer is the
//     same one, because it is responsible for the frame whether or not it draws
//     it. The `decorated` check is live — it sees the events — it just never
//     sees the value move. A compositor that answers differently when it draws
//     nothing would print here, which is what makes this worth keeping until a
//     second compositor has been tried.
//   - Minimise it. Nothing should happen and nothing should crash: a window with
//     no area has no frame to draw and the frame is skipped. Neither the scene
//     nor the camera advances while it is away, because both move on frames
//     drawn.
//   - Close it. It should print `closed` and exit zero. Alt+F4 works with or
//     without a titlebar, and it works while flying — the Alt keystroke is
//     recorded and then handed straight back to the window system.
//
// It will spin a core while it is open. Presenting waits for the display, so a
// visible window costs one frame's worth of work per refresh — but a minimised
// one presents nothing, and _poll returns immediately because platform has no way
// to wait yet. That is also why both the orbit's speed and the flying speed
// follow the refresh rate: the engine counts frames rather than measuring them,
// and card 020 is the card that changes that.
#include <base/arena.h>
#include <base/error.h>
#include <platform/input.h>
#include <platform/window.h>
#include <render/device.h>

#include <stdio.h>

// Scratch for the questions starting the GPU asks the driver — how many cards,
// which queue families, which surface formats. It is handed over, used and
// destroyed here, because nothing the device keeps comes out of it.
#define STARTUP_SCRATCH (64 * 1024)

// The bindings, and the only thing in this file that decides anything. Which key
// means forward is a call site's business — the engine's job is to know what
// forward does, not which finger asks for it — so this function is where a
// remappable set of keys would arrive, and it is the reason render never sees a
// key.
//
// OPPOSITE KEYS HELD TOGETHER CANCEL, WHICH IS WHAT ADDING AND SUBTRACTING GIVES
// FOR FREE. W and S together is nought and not "whichever was pressed last",
// which needs an order this file does not keep and would need a queue to keep.
//
// NOT FLYING MEANS AN INPUT OF NOTHING, AND THE MOUSE IS NOT READ. render reads
// no field but `fly` in that case, but leaving the motion out is what makes the
// difference visible here rather than relying on that: a camera that is not
// being flown is not being asked anything.
static voe_render_camera_input camera_input(voe_platform_window *window,
					    bool flying)
{
	voe_render_camera_input look = { .fly = flying };
	voe_platform_motion motion;

	if (!flying)
		return look;

	if (voe_platform_input_key_down(window, VOE_PLATFORM_KEY_W))
		look.forward += 1.0f;
	if (voe_platform_input_key_down(window, VOE_PLATFORM_KEY_S))
		look.forward -= 1.0f;
	if (voe_platform_input_key_down(window, VOE_PLATFORM_KEY_D))
		look.right += 1.0f;
	if (voe_platform_input_key_down(window, VOE_PLATFORM_KEY_A))
		look.right -= 1.0f;
	if (voe_platform_input_key_down(window, VOE_PLATFORM_KEY_SPACE))
		look.up += 1.0f;
	if (voe_platform_input_key_down(window, VOE_PLATFORM_KEY_CONTROL))
		look.up -= 1.0f;

	look.fast = voe_platform_input_key_down(window, VOE_PLATFORM_KEY_SHIFT);

	motion = voe_platform_input_motion(window);
	look.look_x = motion.x;
	look.look_y = motion.y;

	return look;
}

int main(void)
{
	voe_platform_window *window;
	voe_base_arena *scratch;
	voe_render_device *gpu;
	voe_base_error error;
	voe_platform_size size;
	bool decorated;
	bool flying = false;
	bool was_flying = false;
	bool locked = false;
	// Last frame's Tab, because a toggle is an edge and platform hands out
	// state. Two bools at a call site is what include/platform/input.h says
	// this costs instead of an event queue, and this is that call site.
	bool tab_was_down = false;

	window = voe_platform_window_new(960, 540, "voe3d — two cubes, one camera");
	if (window == NULL) {
		fprintf(stderr, "could not open a window\n");
		return 1;
	}

	scratch = voe_base_arena_new(STARTUP_SCRATCH);
	gpu = voe_render_device_new(scratch, voe_platform_window_native(window),
				    voe_platform_window_size(window), &error);
	voe_base_arena_destroy(scratch);
	if (gpu == NULL) {
		fprintf(stderr, "could not start the GPU: %s\n",
			voe_base_error_string(error));
		voe_platform_window_destroy(window);
		return 1;
	}

	size = voe_platform_window_size(window);
	decorated = voe_platform_window_decorated(window);
	printf("opened     %dx%d\n", size.width, size.height);
	printf("decorated  %s\n", decorated ? "yes" : "no");
	printf("camera     orbit — Tab to fly, Escape to hand it back\n");
	fflush(stdout);

	while (!voe_platform_window_should_close(window)) {
		voe_platform_size now_size;
		bool now_decorated;
		bool now_locked;
		bool tab_down;

		voe_platform_window_poll(window);

		// Poll, then report what changed. Everything is asked every
		// frame because platform hands out state, not events — there is
		// nothing to subscribe to and nothing that would tell us.
		now_size = voe_platform_window_size(window);
		if (now_size.width != size.width ||
		    now_size.height != size.height) {
			size = now_size;
			printf("size       %dx%d\n", size.width, size.height);
			fflush(stdout);
		}

		now_decorated = voe_platform_window_decorated(window);
		if (now_decorated != decorated) {
			decorated = now_decorated;
			printf("decorated  %s\n", decorated ? "yes" : "no");
			fflush(stdout);
		}

		// Tab toggles on the press and not while held, which is the one
		// place this file has to turn state back into an edge. Escape
		// only ever hands control back, so that there is a key that
		// means "let me out" whatever else is going on.
		tab_down = voe_platform_input_key_down(window,
						       VOE_PLATFORM_KEY_TAB);
		if (tab_down && !tab_was_down)
			flying = !flying;
		tab_was_down = tab_down;

		if (voe_platform_input_key_down(window, VOE_PLATFORM_KEY_ESCAPE))
			flying = false;

		if (flying != was_flying) {
			was_flying = flying;
			printf("camera     %s\n", flying ? "flying" : "orbit");
			fflush(stdout);
		}

		// Asked every frame rather than on the change, because a lock is
		// a request the window system may have taken away — losing focus
		// takes it — and asking again is how it comes back.
		voe_platform_input_lock_pointer(window, flying);

		now_locked = voe_platform_input_pointer_locked(window);
		if (now_locked != locked) {
			locked = now_locked;
			printf("locked     %s\n", locked ? "yes" : "no");
			fflush(stdout);
		}

		// The window's size is handed over every frame rather than
		// stored anywhere: it is the only thing that notices a resize,
		// and render rebuilds its swapchain from it when the two
		// disagree. The camera input goes the same way and for the same
		// reason — render holds no window and asks it nothing.
		if (!voe_render_device_frame(gpu, now_size,
					     camera_input(window, flying))) {
			fprintf(stderr, "the GPU stopped answering\n");
			break;
		}
	}

	voe_render_device_destroy(gpu);
	voe_platform_window_destroy(window);
	printf("closed\n");
	return 0;
}
