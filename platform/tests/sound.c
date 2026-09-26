// What platform/sound.h promises, checked from outside without waiting on a
// device: a machine with no sound library or device gets NULL, and destroying
// NULL is a no-op; with a device, room never passes the queue and writing that
// many frames of silence succeeds.
//
// With no device the NULL open prints a reported line to stderr on purpose, and
// the rest is skipped, said on stdout.
#include <platform/sound.h>

#include <testing/test.h>

#include <stdio.h>

static float silence[VOE_PLATFORM_SOUND_QUEUE * VOE_PLATFORM_SOUND_CHANNELS];

int main(void)
{
	voe_platform_sound *sound;
	uint32_t room;

	voe_platform_sound_destroy(NULL);
	sound = voe_platform_sound_new();
	if (sound == NULL) {
		printf("sound: no device, skipped the device checks\n");
		return voe_test_result();
	}
	room = voe_platform_sound_room(sound);
	VOE_TEST_CHECK(room <= VOE_PLATFORM_SOUND_QUEUE);
	VOE_TEST_CHECK(voe_platform_sound_write(sound, silence, room));
	voe_platform_sound_destroy(sound);
	return voe_test_result();
}
