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
// Run it and it should open a window, print the size whenever it changes, and
// exit zero when the window is closed. On Linux there is no titlebar and no
// border — Wayland does not guarantee decorations and nothing here draws them —
// so use the compositor's own shortcuts. On KWin that is Meta+Up to resize and
// Alt+F4 to close. It will spin a core while it is open: _poll returns
// immediately and platform has no way to wait yet.
#include <platform/window.h>

#include <stdio.h>

int main(void)
{
	voe_platform_window *window;
	voe_platform_size size;

	window = voe_platform_window_new(960, 540, "voe3d — platform window");
	if (window == NULL) {
		fprintf(stderr, "could not open a window\n");
		return 1;
	}

	size = voe_platform_window_size(window);
	printf("opened %dx%d\n", size.width, size.height);
	fflush(stdout);

	while (!voe_platform_window_should_close(window)) {
		voe_platform_size now;

		voe_platform_window_poll(window);

		now = voe_platform_window_size(window);
		if (now.width != size.width || now.height != size.height) {
			size = now;
			printf("resized %dx%d\n", size.width, size.height);
			fflush(stdout);
		}
	}

	voe_platform_window_destroy(window);
	printf("closed\n");
	return 0;
}
