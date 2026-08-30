// voe_dev — the one program a person runs to see what the engine can currently
// do. Today it opens a window and reports its size. Later it clears a frame,
// then draws a triangle. There is one of these and it always shows the current
// state, so what is here now is expected to be deleted rather than kept behind a
// flag when the next thing lands.
//
// NO ENGINE LOGIC LIVES HERE. This file is a call site. Anything in it that
// starts to look worth keeping belongs in a folder, with a test — the moment it
// is worth testing it is in the wrong place.
//
// NOT ONE #ifdef. If this file ever needs to know which operating system it is
// on, the API in platform/window.h has a hole and that is the finding, not a
// reason to reach for the preprocessor.
//
// Run it and it should open a window and print a line whenever something about
// it changes — its size, or who is drawing its frame — then exit zero when the
// window is closed.
//
// WHAT THERE IS TO TRY, since nothing here reads a key and nothing can:
//
//   - Resize it. A `size` line should follow. On KWin, Meta+Up maximises.
//   - Toggle the frame off and on. On KWin: right-click the titlebar ->
//     More Actions -> No Borders, or Alt+F3 for the same menu. A `size` line
//     follows and a `decorated` line does not. That is the measured answer and
//     not a gap: KWin does re-answer on every toggle, and every answer is the
//     same one, because it is responsible for the frame whether or not it draws
//     it. The `decorated` check is live — it sees the events — it just never
//     sees the value move. A compositor that answers differently when it draws
//     nothing would print here, which is what makes this worth keeping until a
//     second compositor has been tried.
//   - Close it. It should print `closed` and exit zero. Alt+F4 works with or
//     without a titlebar.
//
// It will spin a core while it is open: _poll returns immediately and platform
// has no way to wait yet.
#include <platform/window.h>

#include <stdio.h>

int main(void)
{
	voe_platform_window *window;
	voe_platform_size size;
	bool decorated;

	window = voe_platform_window_new(960, 540, "voe3d — platform window");
	if (window == NULL) {
		fprintf(stderr, "could not open a window\n");
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
	}

	voe_platform_window_destroy(window);
	printf("closed\n");
	return 0;
}
