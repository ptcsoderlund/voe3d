// voe_dev — the one program a person runs to see what the engine can currently
// do. Today it opens a window with two cubes in it and a camera orbiting them.
// There is one of these and it always shows the current state, so what is here
// now is expected to be deleted rather than kept behind a flag when the next
// thing lands.
//
// NO ENGINE LOGIC LIVES HERE. This file is a call site. Anything in it that
// starts to look worth keeping belongs in a folder, with a test — the moment it
// is worth testing it is in the wrong place. Nothing below decides where the
// camera is or how fast it goes: that is render's, and card 016 is the card that
// takes it off render and puts it on a keyboard.
//
// NOT ONE #ifdef. If this file ever needs to know which operating system it is
// on, the API in platform/window.h or render/device.h has a hole and that is the
// finding, not a reason to reach for the preprocessor.
//
// WHAT IT SHOULD LOOK LIKE. A flat blue-green background with two one-metre
// cubes standing on nothing, each corner of each cube a different colour so that
// every face is a gradient and no two faces read the same. It prints a line
// whenever something about the window changes — its size, or who is drawing its
// frame — and exits zero when the window is closed.
//
// THREE MOTIONS, AND ALL THREE HAVE TO BE THERE. This is the thing to look at
// and the reason there are two cubes rather than one:
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
// WHAT THERE IS TO TRY, since nothing here reads a key and nothing can:
//
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
//     no area has no frame to draw and the frame is skipped. The scene does not
//     advance while it is away, because the clock counts frames drawn.
//   - Close it. It should print `closed` and exit zero. Alt+F4 works with or
//     without a titlebar.
//
// It will spin a core while it is open. Presenting waits for the display, so a
// visible window costs one frame's worth of work per refresh — but a minimised
// one presents nothing, and _poll returns immediately because platform has no way
// to wait yet. That is also why the orbit's speed follows the refresh rate: the
// engine counts frames rather than measuring them, and card 020 is the card that
// changes that.
#include <base/arena.h>
#include <base/error.h>
#include <platform/window.h>
#include <render/device.h>

#include <stdio.h>

// Scratch for the questions starting the GPU asks the driver — how many cards,
// which queue families, which surface formats. It is handed over, used and
// destroyed here, because nothing the device keeps comes out of it.
#define STARTUP_SCRATCH (64 * 1024)

int main(void)
{
	voe_platform_window *window;
	voe_base_arena *scratch;
	voe_render_device *gpu;
	voe_base_error error;
	voe_platform_size size;
	bool decorated;

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
	fflush(stdout);

	while (!voe_platform_window_should_close(window)) {
		voe_platform_size now_size;
		bool now_decorated;

		voe_platform_window_poll(window);

		// Poll, then report what changed. Both are asked every frame
		// because platform hands out state, not events — there is
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

		// The window's size is handed over every frame rather than
		// stored anywhere: it is the only thing that notices a resize,
		// and render rebuilds its swapchain from it when the two
		// disagree.
		if (!voe_render_device_frame(gpu, now_size)) {
			fprintf(stderr, "the GPU stopped answering\n");
			break;
		}
	}

	voe_render_device_destroy(gpu);
	voe_platform_window_destroy(window);
	printf("closed\n");
	return 0;
}
