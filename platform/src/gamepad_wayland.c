// The Linux window's gamepads (ADR-0292 point 4): /dev/input/event* nodes found,
// opened and read, their bytes handed to src/gamepad.h, which owns the slots and
// the mapping. window_wayland.c calls _open once the window is up, _poll after
// each Wayland dispatch and _close before tearing the window down.
//
// NOTHING HERE IS WAYLAND DESPITE THE NAME. A pad is no window-system object:
// the compositor never sees it, and this file talks only to the kernel. The name
// says Linux, as every _wayland file in this folder does.
//
// WHY EVDEV AND NOT JOYDEV. evdev is the kernel's own input interface and every
// common pad's driver maps onto its standard codes (BTN_SOUTH, ABS_X, ABS_HAT0X),
// so a button is known by its place. joydev is older, numbers buttons and axes
// in probe order and loses those names; no library sits between, as none is
// needed for this much.
//
// WHY ATTRIBUTE CHANGES ARE WATCHED. udev makes the node first and grants the
// seat's user access a moment later. Opening on IN_CREATE alone would fail on
// permission and never be tried again; IN_ATTRIB is the retry. So a node that
// will not open is skipped silently: it is not a pad, or not ours yet.
//
// A device is kept when it reports BTN_GAMEPAD or BTN_JOYSTICK and ABS_X and a
// slot attaches; its keys and axes are read back at once (EVIOCGKEY, EVIOCGABS),
// so a stick held while plugging in reads from the first frame, and again after
// SYN_DROPPED once the next SYN_REPORT closes the lost packet. A read failing
// with anything but EAGAIN (ENODEV is a pad pulled out) closes the device and
// frees its slot. A failing inotify means pads plugged in later are not seen;
// the window still opens.
//
// Constraints: at most four devices, one per slot, in a fixed table beside the
// window; no allocation. Each poll reads at most READS_PER_POLL buffers of events
// per device and of inotify, and a directory scan looks at SCAN_ENTRIES names;
// a device flooding past that is read on the next poll.
#define _GNU_SOURCE

#include "window_wayland.h"

#include <base/assert.h>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <linux/input.h>
#include <stdio.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

#define READS_PER_POLL 16
#define SCAN_ENTRIES 1024
#define LONG_BITS (sizeof(unsigned long) * CHAR_BIT)
#define BIT_LONGS(max) ((max) / LONG_BITS + 1)

static bool bit_set(const unsigned long *bits, unsigned int code)
{
	return (bits[code / LONG_BITS] >> (code % LONG_BITS)) & 1UL;
}

// Reads every axis's range and value and every key's state back from the
// device and feeds them to the slot, as if each had just been reported.
static int device_sync(voe_platform_window *window,
		       struct voe_platform_gamepad_device *device)
{
	voe_platform_gamepad *pad = &window->input.gamepads.slots[device->slot];
	unsigned long abs_bits[BIT_LONGS(ABS_MAX)] = { 0 };
	unsigned long keys[BIT_LONGS(KEY_MAX)] = { 0 };
	struct input_absinfo info;

	VOE_BASE_DEBUG_ASSERT(device->fd >= 0, "syncing a closed device");
	VOE_BASE_DEBUG_ASSERT(pad->connected, "syncing a device with no slot");

	if (ioctl(device->fd, EVIOCGBIT(EV_ABS, sizeof(abs_bits)), abs_bits) < 0 ||
	    ioctl(device->fd, EVIOCGKEY(sizeof(keys)), keys) < 0)
		return -1;
	device->evdev.has_trigger_axes = bit_set(abs_bits, ABS_Z) ||
					 bit_set(abs_bits, ABS_RZ);
	for (unsigned int code = 0; code <= ABS_MAX; code++) {
		if (!bit_set(abs_bits, code))
			continue;
		if (ioctl(device->fd, EVIOCGABS(code), &info) < 0)
			return -1;
		device->evdev.abs[code] = (struct voe_platform_gamepad_range){
			info.minimum, info.maximum };
		voe_platform_gamepad_evdev_abs(pad, &device->evdev, code, info.value);
	}
	for (unsigned int code = 0; code <= KEY_MAX; code++)
		voe_platform_gamepad_evdev_key(pad, &device->evdev, code,
					       bit_set(keys, code));
	return 0;
}

// Whether the open node says it is a pad or a joystick with a stick.
static bool device_is_pad(int fd)
{
	unsigned long key_bits[BIT_LONGS(KEY_MAX)] = { 0 };
	unsigned long abs_bits[BIT_LONGS(ABS_MAX)] = { 0 };

	VOE_BASE_DEBUG_ASSERT(fd >= 0, "asking a closed node");

	if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(key_bits)), key_bits) < 0 ||
	    ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(abs_bits)), abs_bits) < 0)
		return false;
	return (bit_set(key_bits, BTN_GAMEPAD) || bit_set(key_bits, BTN_JOYSTICK)) &&
	       bit_set(abs_bits, ABS_X);
}

// Opens /dev/input/<name> and keeps it when it is a new pad and a slot is
// free. Anything else, a node that will not open included, is left alone.
static void device_try(voe_platform_window *window, const char *name)
{
	struct voe_platform_gamepad_devices *devices = &window->gamepad_devices;
	struct voe_platform_gamepad_device *device;
	char path[64];
	struct stat node;
	int fd;
	int slot;

	VOE_BASE_DEBUG_ASSERT(name != NULL, "trying a node with no name");

	if (strncmp(name, "event", 5) != 0 ||
	    devices->count >= VOE_PLATFORM_GAMEPAD_SLOTS)
		return;
	if (snprintf(path, sizeof(path), "/dev/input/%s", name) >= (int)sizeof(path) ||
	    stat(path, &node) < 0)
		return;
	for (int i = 0; i < devices->count; i++) {
		if (devices->open[i].node == node.st_rdev)
			return;
	}
	fd = open(path, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
	if (fd < 0)
		return;
	if (!device_is_pad(fd)) {
		close(fd);
		return;
	}
	slot = voe_platform_gamepad_attach(&window->input.gamepads);
	if (slot < 0) {
		close(fd);
		return;
	}
	device = &devices->open[devices->count];
	*device = (struct voe_platform_gamepad_device){
		.fd = fd, .node = node.st_rdev, .slot = slot };
	if (device_sync(window, device) < 0) {
		voe_platform_gamepad_detach(&window->input.gamepads, slot);
		close(fd);
		return;
	}
	devices->count++;
	VOE_BASE_DEBUG_ASSERT(devices->count <= VOE_PLATFORM_GAMEPAD_SLOTS,
			      "more devices than slots");
}

// Closes the device at index, frees its slot, and moves the last one into its
// place.
static void device_drop(voe_platform_window *window, int index)
{
	struct voe_platform_gamepad_devices *devices = &window->gamepad_devices;

	VOE_BASE_DEBUG_ASSERT(index >= 0 && index < devices->count,
			      "dropping a device not in the table");

	close(devices->open[index].fd);
	voe_platform_gamepad_detach(&window->input.gamepads, devices->open[index].slot);
	devices->count--;
	devices->open[index] = devices->open[devices->count];
}

// One event onto the slot. Returns -1 when a lost packet could not be read
// back, which is the device failing.
static int device_event(voe_platform_window *window,
			struct voe_platform_gamepad_device *device,
			const struct input_event *event)
{
	voe_platform_gamepad *pad = &window->input.gamepads.slots[device->slot];

	VOE_BASE_DEBUG_ASSERT(event != NULL, "no event");

	if (event->type == EV_SYN && event->code == SYN_DROPPED) {
		device->dropped = true;
		return 0;
	}
	if (device->dropped) {
		if (event->type != EV_SYN || event->code != SYN_REPORT)
			return 0;
		device->dropped = false;
		return device_sync(window, device);
	}
	if (event->type == EV_KEY)
		voe_platform_gamepad_evdev_key(pad, &device->evdev, event->code,
					       event->value);
	else if (event->type == EV_ABS)
		voe_platform_gamepad_evdev_abs(pad, &device->evdev, event->code,
					       event->value);
	return 0;
}

// Reads the device until EAGAIN. Returns -1 when it failed and must go.
static int device_read(voe_platform_window *window,
		       struct voe_platform_gamepad_device *device)
{
	struct input_event events[64];

	VOE_BASE_DEBUG_ASSERT(device->fd >= 0, "reading a closed device");

	for (int reads = 0; reads < READS_PER_POLL; reads++) {
		ssize_t got = read(device->fd, events, sizeof(events));

		if (got < 0 && errno == EAGAIN)
			return 0;
		if (got < 0 && errno == EINTR)
			continue;
		if (got <= 0)
			return -1;
		for (size_t i = 0; i < (size_t)got / sizeof(events[0]); i++) {
			if (device_event(window, device, &events[i]) < 0)
				return -1;
		}
	}
	return 0;
}

// Tries every name inotify has for /dev/input since the last poll.
static void arrivals_read(voe_platform_window *window)
{
	alignas(struct inotify_event) char buffer[4096];
	int fd = window->gamepad_devices.inotify;

	VOE_BASE_DEBUG_ASSERT(fd >= 0, "reading no inotify");

	for (int reads = 0; reads < READS_PER_POLL; reads++) {
		ssize_t got = read(fd, buffer, sizeof(buffer));
		size_t at = 0;

		if (got <= 0)
			return;
		while (at + sizeof(struct inotify_event) <= (size_t)got) {
			const struct inotify_event *event = (const void *)(buffer + at);

			if (event->len > 0)
				device_try(window, event->name);
			at += sizeof(struct inotify_event) + event->len;
		}
	}
}

void voe_platform_gamepads_open(voe_platform_window *window)
{
	struct voe_platform_gamepad_devices *devices = &window->gamepad_devices;
	DIR *folder;
	const struct dirent *entry;

	VOE_BASE_DEBUG_ASSERT(window != NULL, "opening pads for no window");
	VOE_BASE_DEBUG_ASSERT(devices->count == 0, "opening pads twice");

	devices->inotify = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
	if (devices->inotify >= 0 &&
	    inotify_add_watch(devices->inotify, "/dev/input", IN_CREATE | IN_ATTRIB) < 0) {
		close(devices->inotify);
		devices->inotify = -1;
	}

	folder = opendir("/dev/input");
	if (folder == NULL)
		return;
	for (int seen = 0; seen < SCAN_ENTRIES; seen++) {
		entry = readdir(folder);
		if (entry == NULL)
			break;
		device_try(window, entry->d_name);
	}
	closedir(folder);
}

void voe_platform_gamepads_poll(voe_platform_window *window)
{
	struct voe_platform_gamepad_devices *devices = &window->gamepad_devices;

	VOE_BASE_DEBUG_ASSERT(window != NULL, "polling pads for no window");

	if (devices->inotify >= 0)
		arrivals_read(window);
	for (int i = devices->count - 1; i >= 0; i--) {
		if (device_read(window, &devices->open[i]) < 0)
			device_drop(window, i);
	}
	VOE_BASE_DEBUG_ASSERT(devices->count >= 0, "a negative device count");
}

void voe_platform_gamepads_close(voe_platform_window *window)
{
	struct voe_platform_gamepad_devices *devices = &window->gamepad_devices;

	VOE_BASE_DEBUG_ASSERT(window != NULL, "closing pads for no window");

	for (int i = 0; i < devices->count; i++)
		close(devices->open[i].fd);
	devices->count = 0;
	if (devices->inotify >= 0)
		close(devices->inotify);
	devices->inotify = -1;
}
