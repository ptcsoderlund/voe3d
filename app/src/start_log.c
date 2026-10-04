// The four calls of app/include/app/start_log.h: the marks, the sum, and the
// block composed once in scratch, then put on stderr and appended to the file.
// The reasoning is in the header.
#include <app/start_log.h>

#include <base/assert.h>
#include <platform/clock.h>
#include <platform/file.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

void voe_app_start_log_begin(voe_app_start_log *log)
{
	double launched;
	double now;

	VOE_BASE_ASSERT(log != NULL, "a start log needs somewhere to live");
	*log = (voe_app_start_log){ 0 };
	now = voe_platform_clock_now();
	if (voe_platform_clock_launched(&launched)) {
		log->steps[0].name = "before main";
		log->steps[0].seconds = now > launched ? now - launched : 0.0;
		log->count = 1;
	}
	log->mark = now;
	VOE_BASE_ASSERT(log->count <= 1, "begin records at most one step");
}

void voe_app_start_log_step(voe_app_start_log *log, const char *name)
{
	double now = voe_platform_clock_now();

	VOE_BASE_ASSERT(log != NULL && name != NULL, "a step needs a log and a name");
	VOE_BASE_ASSERT(log->count < VOE_APP_START_STEPS,
			"a start log holds VOE_APP_START_STEPS steps");
	log->steps[log->count].name = name;
	log->steps[log->count].seconds = now - log->mark;
	log->count++;
	log->mark = now;
}

double voe_app_start_log_total(const voe_app_start_log *log)
{
	double total = 0.0;

	VOE_BASE_ASSERT(log != NULL, "a total needs a log");
	VOE_BASE_ASSERT(log->count <= VOE_APP_START_STEPS, "a log past its steps");
	for (size_t i = 0; i < log->count; i++)
		total += log->steps[i].seconds;
	return total;
}

// One line per step and the total, into text when it is not NULL; the length
// either way, so the first call sizes the second.
static size_t compose_block(const voe_app_start_log *log, const char *program,
			    char *text, size_t room)
{
	size_t length = 0;
	int wrote;

	for (size_t i = 0; i <= log->count; i++) {
		const char *name = i < log->count ? log->steps[i].name : "total";
		double seconds = i < log->count ? log->steps[i].seconds
						: voe_app_start_log_total(log);

		wrote = snprintf(text == NULL ? NULL : text + length,
				 text == NULL ? 0 : room - length,
				 "%s: %s %.3f s\n", program, name, seconds);
		VOE_BASE_ASSERT(wrote >= 0, "a start line always formats");
		length += (size_t)wrote;
	}
	VOE_BASE_ASSERT(text == NULL || length < room, "the block fits its sizing");
	return length;
}

// The file's old text, or none when it is not there, could not be read, or is
// over the limit — then the new block starts it afresh.
static const uint8_t *read_kept(const char *path, voe_base_arena *scratch,
				size_t *count)
{
	const uint8_t *old = NULL;

	*count = 0;
	if (voe_platform_file_exists(path))
		old = voe_platform_file_read(path, scratch, count, NULL);
	if (old == NULL || *count > VOE_APP_START_LOG_LIMIT) {
		*count = 0;
		return NULL;
	}
	return old;
}

bool voe_app_start_log_write(const voe_app_start_log *log, const char *program,
			     const char *path, voe_base_arena *scratch)
{
	struct voe_base_arena_mark mark;
	const uint8_t *old;
	size_t old_count;
	size_t length;
	char *text;
	bool written;

	VOE_BASE_ASSERT(log != NULL && program != NULL && scratch != NULL,
			"a write needs a log, a program and scratch");
	mark = voe_base_arena_mark(scratch);
	length = compose_block(log, program, NULL, 0);
	old = path == NULL ? NULL : read_kept(path, scratch, &old_count);
	if (old == NULL)
		old_count = 0;

	text = voe_base_arena_push(scratch, old_count + length + 1);
	if (old_count > 0)
		memcpy(text, old, old_count);
	compose_block(log, program, text + old_count, length + 1);
	// A stderr that will not take the lines leaves nobody to tell.
	(void)fwrite(text + old_count, 1, length, stderr);

	written = path == NULL ||
		  voe_platform_file_write(path, (const uint8_t *)text,
					  old_count + length, NULL);
	voe_base_arena_rewind(scratch, mark);
	return written;
}
