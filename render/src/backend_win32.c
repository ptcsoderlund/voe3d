// The Windows backend. See backend.h for what a backend is.
//
// HOW THIS NAMES OS TYPES WITHOUT INCLUDING AN OS HEADER — the card's trap, and
// this is the side with work in it. vulkan_win32.h spells its fields with names
// from the Windows SDK and declares none of them: it assumes windows.h was
// included first, which render may not do. Unlike `struct wl_display *` on the
// Wayland side, HINSTANCE and HWND are typedefs, and C will not invent a typedef
// it has not seen.
//
// So the seven names that header uses are declared here, spelled exactly as the
// Windows SDK spells them. Two facts make that safe rather than a guess:
//
//   - DECLARE_HANDLE(x) in windef.h expands to `typedef struct x##__ *x`, so
//     HINSTANCE really is a pointer to a struct tagged HINSTANCE__. Naming the
//     same tag here produces the same type, not a lookalike — if a translation
//     unit ever saw both declarations they would agree rather than clash.
//   - Only four of the seven are used by anything this folder touches, and three
//     of those four are pointers. SECURITY_ATTRIBUTES is only ever pointed at,
//     so an incomplete type is enough and its layout never has to be guessed.
//
// DWORD is the one that is a value and not a pointer, and `unsigned long` is
// what the SDK says it is. It appears only in external-memory extension structs
// that nothing here creates, and it is declared so the header compiles at all.
//
// The alternative was declaring VkWin32SurfaceCreateInfoKHR ourselves and not
// vendoring vulkan_win32.h. That trades seven typedefs whose definitions are
// fixed by a published ABI for a hand-copied API struct that has to be kept in
// step with a header we already carry, which is the worse of the two.
#include "backend.h"

#include <base/report.h>

#include <stddef.h>

typedef unsigned long DWORD;
typedef void *HANDLE;
typedef struct HINSTANCE__ *HINSTANCE;
typedef struct HWND__ *HWND;
typedef struct HMONITOR__ *HMONITOR;
typedef const wchar_t *LPCWSTR;
typedef struct _SECURITY_ATTRIBUTES SECURITY_ATTRIBUTES;

#include "../vulkan/vulkan_win32.h"

const char *voe_render_backend_library(void)
{
	return "vulkan-1.dll";
}

const char *voe_render_backend_extension(void)
{
	return VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
}

VkSurfaceKHR voe_render_backend_surface_new(VkInstance instance,
					    voe_platform_native native)
{
	PFN_vkCreateWin32SurfaceKHR create;
	VkWin32SurfaceCreateInfoKHR info = {
		.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
		.hinstance = (HINSTANCE)native.context,
		.hwnd = (HWND)native.window,
	};
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	VkResult result;

	create = (PFN_vkCreateWin32SurfaceKHR)
		voe_render_vk.get_instance_proc_addr(instance,
						     "vkCreateWin32SurfaceKHR");
	if (create == NULL) {
		VOE_BASE_ERROR("render",
			       "the instance has no vkCreateWin32SurfaceKHR");
		return VK_NULL_HANDLE;
	}

	result = create(instance, &info, NULL, &surface);
	if (result != VK_SUCCESS) {
		VOE_BASE_ERROR("render",
			       "vkCreateWin32SurfaceKHR failed (VkResult %d)",
			       (int)result);
		return VK_NULL_HANDLE;
	}

	return surface;
}
