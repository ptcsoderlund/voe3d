// The mean folded, the rule asked, and on a hitch the five largest parts
// ranked and written. The reasoning is in app/include/app/hitches.h.
#include <app/hitches.h>

#include <base/assert.h>

#include <stdio.h>
#include <string.h>

#define HITCH_SAMPLES 60
#define HITCH_RATIO 1.5
#define HITCH_FLOOR (1.0 / 60.0)
#define HITCH_PARTS 5

typedef struct {
	const char *name;
	int name_length;
	double seconds;
	uint32_t count;
} hitch_part;

typedef struct {
	hitch_part parts[HITCH_PARTS];
	uint32_t count;
} hitch_rank;

// True when `sample` is a hitch against the mean before it; folds it in either
// way. The first sample seeds the mean, so 60 ordinary frames make an ordinary
// mean rather than one still climbing from nought.
static bool hitch_fold(double *mean, uint32_t *samples, double sample)
{
	VOE_BASE_ASSERT(mean != NULL && samples != NULL, "a fold needs its mean");

	const double before = *samples == 0 ? sample : *mean;
	const bool hitch = *samples >= HITCH_SAMPLES &&
			   sample > HITCH_RATIO * before && sample > HITCH_FLOOR;

	*mean = before + (sample - before) / HITCH_SAMPLES;
	if (*samples < UINT32_MAX)
		*samples += 1;
	VOE_BASE_ASSERT(*samples > 0, "a folded mean has a sample");
	return hitch;
}

// Keeps the five largest, largest first; an equal one stays behind those given
// before it.
static void hitch_rank_add(hitch_rank *rank, hitch_part part)
{
	uint32_t at = rank->count;

	VOE_BASE_ASSERT(rank->count <= HITCH_PARTS, "a rank holds five");
	while (at > 0 && rank->parts[at - 1].seconds < part.seconds)
		at--;
	if (at >= HITCH_PARTS)
		return;
	if (rank->count < HITCH_PARTS)
		rank->count++;
	memmove(&rank->parts[at + 1], &rank->parts[at],
		(rank->count - 1 - at) * sizeof(rank->parts[0]));
	rank->parts[at] = part;
	VOE_BASE_ASSERT(rank->count <= HITCH_PARTS, "a rank holds five");
}

// Appends `text` to `line` at `*used`, cut to `room` with its NUL inside.
static void hitch_append(char *line, size_t room, size_t *used,
			 const char *text)
{
	size_t length = strlen(text);

	VOE_BASE_ASSERT(room > 0 && *used < room, "an append needs room");
	if (length > room - 1 - *used)
		length = room - 1 - *used;
	memcpy(line + *used, text, length);
	*used += length;
	line[*used] = '\0';
	VOE_BASE_ASSERT(*used < room, "the line ends inside its room");
}

static void hitch_write(const char *what, double seconds, double mean,
			const hitch_rank *rank, char *line, size_t room)
{
	size_t used = 0;
	char piece[96];

	VOE_BASE_ASSERT(what != NULL && rank != NULL, "a line needs its parts");
	if (room == 0)
		return;
	line[0] = '\0';
	snprintf(piece, sizeof piece, "%s %.1f ms (mean %.1f):", what,
		 seconds * 1000.0, mean * 1000.0);
	hitch_append(line, room, &used, piece);
	for (uint32_t i = 0; i < rank->count; i++) {
		const hitch_part *part = &rank->parts[i];
		const char *gap = i == 0 ? " " : ", ";

		if (part->count > 1)
			snprintf(piece, sizeof piece, "%s%.*s %ux %.1f", gap,
				 part->name_length, part->name,
				 (unsigned)part->count, part->seconds * 1000.0);
		else
			snprintf(piece, sizeof piece, "%s%.*s %.1f", gap,
				 part->name_length, part->name,
				 part->seconds * 1000.0);
		hitch_append(line, room, &used, piece);
	}
	VOE_BASE_ASSERT(used < room, "the line ends inside its room");
}

bool voe_app_hitches_frame(voe_app_hitches *hitches, double elapsed,
			   const voe_app_stretch *stretches, uint32_t count,
			   char *line, size_t room)
{
	hitch_rank rank = { 0 };

	VOE_BASE_ASSERT(hitches != NULL, "a frame needs its hitches");
	VOE_BASE_ASSERT(stretches != NULL || count == 0, "stretches with a count");
	VOE_BASE_ASSERT(line != NULL || room == 0, "a line with a room");
	const double mean = hitches->frame_mean;

	if (!hitch_fold(&hitches->frame_mean, &hitches->frame_samples, elapsed))
		return false;
	for (uint32_t i = 0; i < count; i++) {
		VOE_BASE_ASSERT(stretches[i].name != NULL, "a stretch has a name");
		hitch_rank_add(&rank, (hitch_part){
			.name = stretches[i].name,
			.name_length = (int)strlen(stretches[i].name),
			.seconds = stretches[i].seconds,
			.count = 1,
		});
	}
	hitch_write("frame", elapsed, mean, &rank, line, room);
	return true;
}

static bool hitch_same_name(const voe_render_pass_time *a,
			    const voe_render_pass_time *b)
{
	VOE_BASE_ASSERT(a != NULL && b != NULL, "two passes to compare");
	return strncmp(a->name, b->name, VOE_RENDER_PASS_NAME) == 0;
}

bool voe_app_hitches_card(voe_app_hitches *hitches, double total,
			  const voe_render_pass_time *passes, uint32_t count,
			  char *line, size_t room)
{
	hitch_rank rank = { 0 };

	VOE_BASE_ASSERT(hitches != NULL, "a card frame needs its hitches");
	VOE_BASE_ASSERT(passes != NULL || count == 0, "passes with a count");
	VOE_BASE_ASSERT(line != NULL || room == 0, "a line with a room");
	const double mean = hitches->card_mean;

	if (!hitch_fold(&hitches->card_mean, &hitches->card_samples, total))
		return false;
	for (uint32_t i = 0; i < count; i++) {
		const char *end = memchr(passes[i].name, '\0',
					 VOE_RENDER_PASS_NAME);
		hitch_part part = {
			.name = passes[i].name,
			.name_length = end != NULL ? (int)(end - passes[i].name)
						   : VOE_RENDER_PASS_NAME,
		};
		bool seen = false;

		// A name met before was summed when it was first met.
		for (uint32_t j = 0; j < i && !seen; j++)
			seen = hitch_same_name(&passes[j], &passes[i]);
		if (seen)
			continue;
		for (uint32_t j = i; j < count; j++) {
			if (!hitch_same_name(&passes[j], &passes[i]))
				continue;
			part.seconds += passes[j].seconds;
			part.count++;
		}
		hitch_rank_add(&rank, part);
	}
	hitch_write("card", total, mean, &rank, line, room);
	return true;
}
