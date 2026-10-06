// The splash wait's progress record, as game/include/game/progress.h gives.
// Stores and loads are sequentially consistent, the atomics' default; the
// record is touched a few times a frame, so nothing weaker is worth the reason.
#include <game/progress.h>

#include <base/assert.h>

#include <stdio.h>
#include <string.h>

void voe_game_progress_set(voe_game_progress *progress, const char *phase,
			   unsigned done, unsigned total)
{
	if (progress == NULL)
		return;
	atomic_store(&progress->phase, phase);
	atomic_store(&progress->total, total);
	atomic_store(&progress->done, done);
}

bool voe_game_progress_stopped(voe_game_progress *progress)
{
	return progress != NULL && atomic_load(&progress->stop);
}

void voe_game_progress_line(voe_game_progress *progress, char *buffer,
			    size_t size)
{
	const char *phase;
	unsigned total;

	VOE_BASE_ASSERT(progress != NULL && buffer != NULL && size > 0,
			"a progress line with no record or no room");

	phase = atomic_load(&progress->phase);
	total = atomic_load(&progress->total);
	if (phase == NULL)
		phase = "";
	if (total == 0)
		(void)snprintf(buffer, size, "%s", phase);
	else
		(void)snprintf(buffer, size, "%s %u/%u", phase,
			       atomic_load(&progress->done), total);
	VOE_BASE_ASSERT(memchr(buffer, '\0', size) != NULL,
			"a progress line not terminated");
}
