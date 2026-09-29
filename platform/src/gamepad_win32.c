// The Windows window's gamepads (ADR-0292 point 5): XInput for Xbox pads and HID
// raw input for the rest, their state handed to src/gamepad.h, which owns the
// slots and the mapping. window_win32.c calls _open after the mouse's
// registration, routes WM_INPUT_DEVICE_CHANGE to _device_change, seat_win32.c
// hands a RIM_TYPEHID WM_INPUT to _hid, _poll runs after each message pump and
// _close after the window is destroyed. Written, not verified: no Windows
// machine has run it (ADR-0130).
//
// BOTH DLLS ARE LOADED AT RUN TIME, as sound loads ole32 (ADR-0266), so the
// build links nothing new: xinput1_4.dll, else xinput9_1_0.dll, and hid.dll. A
// missing one, or one lacking a call, is those pads absent and not a failure;
// without hid.dll no raw input is registered for pads at all.
//
// HID PADS ARRIVE AS MESSAGES. Raw input for generic desktop usages 4 and 5 is
// registered with RIDEV_DEVNOTIFY, which posts GIDC_ARRIVAL for every device
// already plugged in as well, so those are taken at the first poll; and with
// RIDEV_INPUTSINK, so pads read without focus as on Linux (0292 point 3). A
// device whose name holds IG_ is XInput's own HID face and is skipped, or an
// Xbox pad would take two slots.
//
// AN UNCONNECTED XINPUT USER IS ASKED AT MOST ONCE A SECOND (0292 point 5):
// XInputGetState on an empty user is slow enough to cost a frame when asked
// every poll. A connected user is asked every poll.
//
// HID VALUES ARE SIGN-EXTENDED WHEN THE RANGE IS SIGNED. HidP_GetUsageValue
// hands back the field's raw bits; a logical minimum below 0 means they are a
// two's-complement number BitSize wide. A range whose maximum reads below a
// non-negative minimum is a device writing an unsigned maximum into a signed
// field, and is taken as all BitSize bits.
//
// Constraints: at most four HID pads, one per slot, each with at most
// VOE_PLATFORM_GAMEPAD_HID_VALUES value caps (one declaring more is not taken)
// and BUTTON_USAGES buttons down at once (a report with more moves no button).
// Preparsed data is allocated on arrival and freed on removal.
#include "window_win32.h"

#include <base/assert.h>

#include <stdlib.h>
#include <wchar.h>

#define UNCONNECTED_ASK_MS 1000
#define DEVICE_NAME_CHARS 512
#define BUTTON_USAGES 64
#define REPORTS_PER_INPUT 16
#define USAGES_PER_CAP 64
#define PAGE_GENERIC_DESKTOP 0x01
#define PAGE_BUTTON 0x09
#define USAGE_JOYSTICK 0x04
#define USAGE_GAMEPAD 0x05

// hid.dll and its four calls, or nothing loaded when any is missing.
static void hid_load(struct voe_platform_gamepad_devices *devices)
{
	VOE_BASE_DEBUG_ASSERT(devices->hid == NULL, "loading hid.dll twice");

	devices->hid = voe_platform_library_new("hid.dll");
	if (devices->hid == NULL)
		return;
	devices->get_caps = (voe_platform_hidp_get_caps_fn)
		voe_platform_library_symbol(devices->hid, "HidP_GetCaps");
	devices->get_value_caps = (voe_platform_hidp_get_value_caps_fn)
		voe_platform_library_symbol(devices->hid, "HidP_GetValueCaps");
	devices->get_usage_value = (voe_platform_hidp_get_usage_value_fn)
		voe_platform_library_symbol(devices->hid, "HidP_GetUsageValue");
	devices->get_usages = (voe_platform_hidp_get_usages_fn)
		voe_platform_library_symbol(devices->hid, "HidP_GetUsages");
	if (devices->get_caps == NULL || devices->get_value_caps == NULL ||
	    devices->get_usage_value == NULL || devices->get_usages == NULL) {
		voe_platform_library_destroy(devices->hid);
		devices->hid = NULL;
	}
	VOE_BASE_DEBUG_ASSERT(devices->hid == NULL || devices->get_usages != NULL,
			      "hid.dll loaded without its calls");
}

// XInputGetState from the first xinput DLL that has it, or nothing loaded.
static void xinput_load(struct voe_platform_gamepad_devices *devices)
{
	static const char *const NAMES[] = { "xinput1_4.dll", "xinput9_1_0.dll" };

	VOE_BASE_DEBUG_ASSERT(devices->xinput == NULL, "loading xinput twice");

	for (size_t i = 0; i < sizeof(NAMES) / sizeof(NAMES[0]); i++) {
		devices->xinput = voe_platform_library_new(NAMES[i]);
		if (devices->xinput == NULL)
			continue;
		devices->get_state = (voe_platform_xinput_get_state_fn)
			voe_platform_library_symbol(devices->xinput, "XInputGetState");
		if (devices->get_state != NULL)
			return;
		voe_platform_library_destroy(devices->xinput);
		devices->xinput = NULL;
	}
	VOE_BASE_DEBUG_ASSERT(devices->xinput == NULL, "xinput kept without its call");
}

void voe_platform_gamepads_open(voe_platform_window *window)
{
	struct voe_platform_gamepad_devices *devices = &window->gamepad_devices;
	RAWINPUTDEVICE pads[2] = {
		{ .usUsagePage = PAGE_GENERIC_DESKTOP, .usUsage = USAGE_JOYSTICK,
		  .dwFlags = RIDEV_INPUTSINK | RIDEV_DEVNOTIFY, .hwndTarget = window->hwnd },
		{ .usUsagePage = PAGE_GENERIC_DESKTOP, .usUsage = USAGE_GAMEPAD,
		  .dwFlags = RIDEV_INPUTSINK | RIDEV_DEVNOTIFY, .hwndTarget = window->hwnd },
	};

	VOE_BASE_DEBUG_ASSERT(window != NULL, "opening pads for no window");
	VOE_BASE_DEBUG_ASSERT(devices->count == 0, "opening pads twice");

	for (int user = 0; user < XUSER_MAX_COUNT; user++)
		devices->users[user] = (struct voe_platform_gamepad_xinput_user){ .slot = -1 };
	xinput_load(devices);
	hid_load(devices);
	// Not checked: refused, it is HID pads absent, as a missing hid.dll is.
	if (devices->hid != NULL)
		RegisterRawInputDevices(pads, 2, sizeof(pads[0]));
}

// Whether the device is a HID joystick or gamepad and not XInput's.
static bool device_wanted(HANDLE handle)
{
	RID_DEVICE_INFO info = { .cbSize = sizeof(info) };
	UINT size = sizeof(info);
	wchar_t name[DEVICE_NAME_CHARS];
	UINT chars = DEVICE_NAME_CHARS;

	VOE_BASE_DEBUG_ASSERT(handle != NULL, "asking no device");

	if (GetRawInputDeviceInfoW(handle, RIDI_DEVICEINFO, &info, &size) == (UINT)-1 ||
	    info.dwType != RIM_TYPEHID || info.hid.usUsagePage != PAGE_GENERIC_DESKTOP ||
	    (info.hid.usUsage != USAGE_JOYSTICK && info.hid.usUsage != USAGE_GAMEPAD))
		return false;
	if (GetRawInputDeviceInfoW(handle, RIDI_DEVICENAME, name, &chars) == (UINT)-1)
		return false;
	name[DEVICE_NAME_CHARS - 1] = L'\0';
	return wcsstr(name, L"IG_") == NULL;
}

// Reads the device's preparsed data and value caps into device. -1 when it
// cannot, and then nothing is left allocated.
static int device_describe(const struct voe_platform_gamepad_devices *devices,
			   struct voe_platform_gamepad_hid_device *device)
{
	HIDP_CAPS caps;
	UINT size = 0;

	VOE_BASE_DEBUG_ASSERT(device->preparsed == NULL, "describing a device twice");

	if (GetRawInputDeviceInfoW(device->handle, RIDI_PREPARSEDDATA, NULL, &size) != 0 ||
	    size == 0)
		return -1;
	device->preparsed = malloc(size);
	if (device->preparsed == NULL)
		return -1;
	if (GetRawInputDeviceInfoW(device->handle, RIDI_PREPARSEDDATA, device->preparsed,
				   &size) == (UINT)-1 ||
	    devices->get_caps(device->preparsed, &caps) != HIDP_STATUS_SUCCESS ||
	    caps.NumberInputValueCaps > VOE_PLATFORM_GAMEPAD_HID_VALUES)
		goto free_preparsed;
	device->value_count = caps.NumberInputValueCaps;
	if (device->value_count > 0 &&
	    devices->get_value_caps(HidP_Input, device->values, &device->value_count,
				    device->preparsed) != HIDP_STATUS_SUCCESS)
		goto free_preparsed;
	VOE_BASE_DEBUG_ASSERT(device->value_count <= VOE_PLATFORM_GAMEPAD_HID_VALUES,
			      "more value caps than room");
	return 0;

free_preparsed:
	free(device->preparsed);
	device->preparsed = NULL;
	return -1;
}

// Takes a newly arrived device when it is a pad, not held yet, and a slot is
// free.
static void device_arrive(voe_platform_window *window, HANDLE handle)
{
	struct voe_platform_gamepad_devices *devices = &window->gamepad_devices;
	struct voe_platform_gamepad_hid_device *device;

	VOE_BASE_DEBUG_ASSERT(devices->hid != NULL, "a HID pad without hid.dll");

	if (devices->count >= VOE_PLATFORM_GAMEPAD_SLOTS || !device_wanted(handle))
		return;
	for (int i = 0; i < devices->count; i++) {
		if (devices->open[i].handle == handle)
			return;
	}
	device = &devices->open[devices->count];
	*device = (struct voe_platform_gamepad_hid_device){ .handle = handle };
	if (device_describe(devices, device) < 0)
		return;
	device->slot = voe_platform_gamepad_attach(&window->input.gamepads);
	if (device->slot < 0) {
		free(device->preparsed);
		device->preparsed = NULL;
		return;
	}
	devices->count++;
	VOE_BASE_DEBUG_ASSERT(devices->count <= VOE_PLATFORM_GAMEPAD_SLOTS,
			      "more HID pads than slots");
}

// The table's index for handle, or -1.
static int device_find(const struct voe_platform_gamepad_devices *devices, HANDLE handle)
{
	for (int i = 0; i < devices->count; i++) {
		if (devices->open[i].handle == handle)
			return i;
	}
	return -1;
}

// Frees the device at index, lets go of its slot, and moves the last one into
// its place.
static void device_drop(voe_platform_window *window, int index)
{
	struct voe_platform_gamepad_devices *devices = &window->gamepad_devices;

	VOE_BASE_DEBUG_ASSERT(index >= 0 && index < devices->count,
			      "dropping a device not in the table");

	free(devices->open[index].preparsed);
	voe_platform_gamepad_detach(&window->input.gamepads, devices->open[index].slot);
	devices->count--;
	devices->open[index] = devices->open[devices->count];
	VOE_BASE_DEBUG_ASSERT(devices->count >= 0, "a negative device count");
}

void voe_platform_gamepads_device_change(voe_platform_window *window, WPARAM wparam,
					 LPARAM lparam)
{
	HANDLE handle = (HANDLE)lparam;
	int index;

	VOE_BASE_DEBUG_ASSERT(window != NULL, "a device change for no window");

	if (window->gamepad_devices.hid == NULL || handle == NULL)
		return;
	if (wparam == GIDC_ARRIVAL) {
		device_arrive(window, handle);
		return;
	}
	index = device_find(&window->gamepad_devices, handle);
	if (wparam == GIDC_REMOVAL && index >= 0)
		device_drop(window, index);
}

// The field's raw bits as the number its range means; see the header.
static int32_t value_signed(ULONG raw, const HIDP_VALUE_CAPS *cap)
{
	if (cap->LogicalMin < 0 && cap->BitSize > 0 && cap->BitSize < 32 &&
	    (raw & (1UL << (cap->BitSize - 1))) != 0)
		return (int32_t)(raw | ~((1UL << cap->BitSize) - 1));
	return (int32_t)raw;
}

static struct voe_platform_gamepad_range value_range(const HIDP_VALUE_CAPS *cap)
{
	struct voe_platform_gamepad_range range = { cap->LogicalMin, cap->LogicalMax };

	if (range.min >= 0 && range.max < range.min && cap->BitSize > 0 && cap->BitSize < 32)
		range.max = (int32_t)((1UL << cap->BitSize) - 1);
	return range;
}

// One report's generic desktop values and buttons onto pad.
static void report_read(const struct voe_platform_gamepad_devices *devices,
			const struct voe_platform_gamepad_hid_device *device,
			voe_platform_gamepad *pad, CHAR *report, ULONG length)
{
	USAGE buttons[BUTTON_USAGES];
	ULONG count = BUTTON_USAGES;

	VOE_BASE_DEBUG_ASSERT(report != NULL, "reading no report");

	for (USHORT i = 0; i < device->value_count; i++) {
		const HIDP_VALUE_CAPS *cap = &device->values[i];
		USAGE first = cap->IsRange ? cap->Range.UsageMin : cap->NotRange.Usage;
		USAGE last = cap->IsRange ? cap->Range.UsageMax : cap->NotRange.Usage;

		if (cap->UsagePage != PAGE_GENERIC_DESKTOP)
			continue;
		for (int step = 0; step < USAGES_PER_CAP && first + step <= last; step++) {
			USAGE usage = (USAGE)(first + step);
			ULONG raw;

			if (devices->get_usage_value(HidP_Input, cap->UsagePage,
						     cap->LinkCollection, usage, &raw,
						     device->preparsed, report,
						     length) != HIDP_STATUS_SUCCESS)
				continue;
			voe_platform_gamepad_hid_value(pad, usage, value_signed(raw, cap),
						       value_range(cap));
		}
	}
	if (devices->get_usages(HidP_Input, PAGE_BUTTON, 0, buttons, &count,
				device->preparsed, report, length) == HIDP_STATUS_SUCCESS)
		voe_platform_gamepad_hid_buttons(pad, buttons, count);
}

void voe_platform_gamepads_hid(voe_platform_window *window, const RAWINPUT *raw)
{
	const struct voe_platform_gamepad_devices *devices = &window->gamepad_devices;
	const struct voe_platform_gamepad_hid_device *device;
	const BYTE *reports = raw->data.hid.bRawData;
	DWORD size = raw->data.hid.dwSizeHid;
	int index;

	VOE_BASE_DEBUG_ASSERT(raw->header.dwType == RIM_TYPEHID, "a HID read of another type");

	index = device_find(devices, raw->header.hDevice);
	if (index < 0)
		return;
	device = &devices->open[index];
	for (DWORD i = 0; i < raw->data.hid.dwCount && i < REPORTS_PER_INPUT; i++)
		report_read(devices, device, &window->input.gamepads.slots[device->slot],
			    (CHAR *)(reports + (size_t)i * size), size);
}

// Asks one user, if due, and moves its slot by the answer.
static void user_poll(voe_platform_window *window, DWORD user, ULONGLONG now)
{
	struct voe_platform_gamepad_xinput_user *asked = &window->gamepad_devices.users[user];
	XINPUT_STATE state;
	DWORD result;

	VOE_BASE_DEBUG_ASSERT(user < XUSER_MAX_COUNT, "an XInput user past the four");

	if (asked->slot < 0 && now < asked->ask_at)
		return;
	result = window->gamepad_devices.get_state(user, &state);
	if (result == ERROR_SUCCESS) {
		if (asked->slot < 0)
			asked->slot = voe_platform_gamepad_attach(&window->input.gamepads);
		if (asked->slot < 0) {
			asked->ask_at = now + UNCONNECTED_ASK_MS;
			return;
		}
		voe_platform_gamepad_xinput(&window->input.gamepads.slots[asked->slot],
					    state.Gamepad.wButtons, state.Gamepad.bLeftTrigger,
					    state.Gamepad.bRightTrigger, state.Gamepad.sThumbLX,
					    state.Gamepad.sThumbLY, state.Gamepad.sThumbRX,
					    state.Gamepad.sThumbRY);
		return;
	}
	if (result == ERROR_DEVICE_NOT_CONNECTED && asked->slot >= 0) {
		voe_platform_gamepad_detach(&window->input.gamepads, asked->slot);
		asked->slot = -1;
	}
	if (asked->slot < 0)
		asked->ask_at = now + UNCONNECTED_ASK_MS;
}

void voe_platform_gamepads_poll(voe_platform_window *window)
{
	ULONGLONG now = GetTickCount64();

	VOE_BASE_DEBUG_ASSERT(window != NULL, "polling pads for no window");

	if (window->gamepad_devices.get_state == NULL)
		return;
	for (DWORD user = 0; user < XUSER_MAX_COUNT; user++)
		user_poll(window, user, now);
}

void voe_platform_gamepads_close(voe_platform_window *window)
{
	struct voe_platform_gamepad_devices *devices = &window->gamepad_devices;

	VOE_BASE_DEBUG_ASSERT(window != NULL, "closing pads for no window");

	for (int i = devices->count - 1; i >= 0; i--)
		device_drop(window, i);
	if (devices->xinput != NULL)
		voe_platform_library_destroy(devices->xinput);
	if (devices->hid != NULL)
		voe_platform_library_destroy(devices->hid);
	devices->xinput = NULL;
	devices->hid = NULL;
	devices->get_state = NULL;
	VOE_BASE_DEBUG_ASSERT(devices->count == 0, "a pad left after close");
}
