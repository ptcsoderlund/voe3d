// Opens a window and does nothing with it. This is the whole verification card
// 004 has: opening a window needs a display and a person, so a test would either
// need a desktop session or lie about having one.
//
// Run it, and it should open a window, print the size whenever it changes, and
// exit zero when the window is closed. On Linux there is no titlebar and no
// border — Wayland does not guarantee decorations and nothing here draws them —
// so move and resize it with the compositor's own shortcuts.
//
// NOT ONE #ifdef. If this file ever needs to know which operating system it is
// on, the API in platform/window.h has a hole and that is the finding, not a
// reason to reach for the preprocessor.
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
