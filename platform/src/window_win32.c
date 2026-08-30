// The Win32 window. See include/platform/window.h for the API; this file is
// everything below that line and is invisible to callers.
//
// Ordinary Win32: register a class, CreateWindowExW, and a PeekMessage pump that
// takes what is queued and returns. The Unicode entry points throughout —
// CreateWindowExW, not CreateWindowExA — so a title with a character outside the
// active code page survives, which means the UTF-8 title the API takes is
// converted on the way in.
//
// The window procedure needs to reach the voe_platform_window behind an HWND, so
// the pointer is put in the window's own user data. It is not there yet during
// the first few messages CreateWindowExW sends, so the procedure copes with not
// finding one and defers to DefWindowProcW.
//
// WM_CLOSE is answered by recording that the user asked and returning — the
// window is not destroyed there. Destroying it is voe_platform_window_destroy's
// job, and a window that tore itself down inside poll would leave the caller
// holding a pointer to nothing until it next thought to ask.
#include <platform/window.h>

#include <base/assert.h>

#include <windows.h>

#include <stdlib.h>

// One class for every window this process opens, registered on the first one.
#define WINDOW_CLASS L"voe_platform_window"

// The title is converted onto the stack rather than allocated, because _new does
// not take an arena and the engine has no global one to reach for. A title
// longer than this is a call-site mistake, not a runtime condition.
#define TITLE_MAX 256

struct voe_platform_window {
	HWND hwnd;
	HINSTANCE instance;
	int width;
	int height;
	bool should_close;
};

static voe_platform_window *window_of(HWND hwnd)
{
	return (voe_platform_window *)(uintptr_t)GetWindowLongPtrW(hwnd,
								   GWLP_USERDATA);
}

static LRESULT CALLBACK window_proc(HWND hwnd, UINT message, WPARAM wparam,
				    LPARAM lparam)
{
	voe_platform_window *window = window_of(hwnd);

	// Messages sent by CreateWindowExW before the user data is set.
	if (window == NULL)
		return DefWindowProcW(hwnd, message, wparam, lparam);

	switch (message) {
	case WM_CLOSE:
		window->should_close = true;
		return 0;
	case WM_SIZE:
		window->width = LOWORD(lparam);
		window->height = HIWORD(lparam);
		return 0;
	default:
		return DefWindowProcW(hwnd, message, wparam, lparam);
	}
}

// Registering a class that is already registered fails with
// ERROR_CLASS_ALREADY_EXISTS, which is the second window opening and not a
// failure.
static bool class_ready(HINSTANCE instance)
{
	WNDCLASSEXW description = {
		.cbSize = sizeof(description),
		.style = CS_HREDRAW | CS_VREDRAW,
		.lpfnWndProc = window_proc,
		.hInstance = instance,
		.hCursor = LoadCursorW(NULL, IDC_ARROW),
		.lpszClassName = WINDOW_CLASS,
	};

	if (RegisterClassExW(&description) != 0)
		return true;
	return GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
}

voe_platform_window *voe_platform_window_new(int width, int height,
					     const char *title)
{
	voe_platform_window *window;
	wchar_t wide_title[TITLE_MAX];
	RECT wanted = { 0, 0, width, height };
	DWORD style = WS_OVERLAPPEDWINDOW;

	VOE_BASE_DEBUG_ASSERT(width > 0 && height > 0, "a window needs a size");
	VOE_BASE_DEBUG_ASSERT(title != NULL, "a window needs a title");

	VOE_BASE_ASSERT(MultiByteToWideChar(CP_UTF8, 0, title, -1, wide_title,
					    TITLE_MAX) != 0,
			"the window title is not UTF-8, or is longer than TITLE_MAX");

	window = calloc(1, sizeof(*window));
	VOE_BASE_ASSERT(window != NULL, "out of memory opening a window");

	window->instance = GetModuleHandleW(NULL);
	window->width = width;
	window->height = height;

	if (!class_ready(window->instance)) {
		free(window);
		return NULL;
	}

	// width and height are the client area, and CreateWindowExW is given the
	// outer size, so the decorations have to be added on.
	AdjustWindowRect(&wanted, style, FALSE);

	window->hwnd = CreateWindowExW(0, WINDOW_CLASS, wide_title, style,
				       CW_USEDEFAULT, CW_USEDEFAULT,
				       wanted.right - wanted.left,
				       wanted.bottom - wanted.top,
				       NULL, NULL, window->instance, NULL);
	if (window->hwnd == NULL) {
		free(window);
		return NULL;
	}

	SetWindowLongPtrW(window->hwnd, GWLP_USERDATA, (LONG_PTR)(uintptr_t)window);
	ShowWindow(window->hwnd, SW_SHOW);

	return window;
}

void voe_platform_window_destroy(voe_platform_window *window)
{
	VOE_BASE_DEBUG_ASSERT(window != NULL, "destroying a NULL window");

	if (window->hwnd != NULL)
		DestroyWindow(window->hwnd);
	free(window);
}

void voe_platform_window_poll(voe_platform_window *window)
{
	MSG message;

	VOE_BASE_DEBUG_ASSERT(window != NULL, "polling a NULL window");

	while (PeekMessageW(&message, NULL, 0, 0, PM_REMOVE) != 0) {
		TranslateMessage(&message);
		DispatchMessageW(&message);
	}
}

bool voe_platform_window_should_close(voe_platform_window *window)
{
	VOE_BASE_DEBUG_ASSERT(window != NULL, "asking a NULL window");

	return window->should_close;
}

voe_platform_size voe_platform_window_size(voe_platform_window *window)
{
	VOE_BASE_DEBUG_ASSERT(window != NULL, "asking a NULL window");

	return (voe_platform_size){ window->width, window->height };
}

voe_platform_native voe_platform_window_native(voe_platform_window *window)
{
	VOE_BASE_DEBUG_ASSERT(window != NULL, "asking a NULL window");

	return (voe_platform_native){ (uintptr_t)window->instance,
				      (uintptr_t)window->hwnd };
}
