// A start's steps, timed and written out: where a slow start went (decision
// 0345). A program begins the log first thing in main, names each step as it
// finishes, and writes the block once its first normal frame is up.
//
//     voe_app_start_log log;
//     voe_app_start_log_begin(&log);
//     ... open the window ...
//     voe_app_start_log_step(&log, "window");
//     ... prepare the pipelines ...
//     voe_app_start_log_step(&log, "pipelines");
//     if (!voe_app_start_log_write(&log, "editor", path, scratch))
//             ;       // the file was not written; platform said why
//
// THE FIRST STEP IS THE TIME BEFORE THE PROGRAM'S OWN CODE RAN. When the OS can
// say when it started the process (voe_platform_clock_launched), begin records
// a step "before main" from that moment to now: loading, a scan of a new
// program, everything main cannot see. When it cannot, there is no such step and
// the log starts at begin. The launch is known to the OS's tick (10 ms on
// Linux), so that step is clamped at nought rather than reported as negative.
//
// THE STEPS ADD UP. Each step is the time since the last mark, and every call
// marks now, so the steps tile the time from the launch (or begin) to the last
// mark with no gap, and the total is their sum.
//
// A LINE is the program's name, the step's name and its seconds to the
// millisecond: `editor: window 0.042 s`; the block ends with `editor: total
// 1.318 s`. Every line goes to stderr; with a path, the same block is appended
// to that file.
//
// Constraints: at most VOE_APP_START_STEPS steps, and a step past that asserts.
// Names are the caller's strings and must outlive the log. The file is read
// whole, emptied first when over VOE_APP_START_LOG_LIMIT bytes, and written
// whole through platform/file.h's atomic write, so a crash leaves the old log;
// a log that grew past the limit is lost in one piece, which a log may be.
// The file's folder must exist. Write uses scratch and keeps nothing in it.
#pragma once

#include <base/arena.h>

#include <stddef.h>

#define VOE_APP_START_STEPS 16
#define VOE_APP_START_LOG_LIMIT ((size_t)64 * 1024)

typedef struct {
	const char *name;
	double seconds;
} voe_app_start_step;

// The log's own; read it, but change it only through the calls below.
typedef struct {
	voe_app_start_step steps[VOE_APP_START_STEPS];
	size_t count;
	double mark;
} voe_app_start_log;

// Zeroes log, records "before main" when the OS can say, and marks now.
void voe_app_start_log_begin(voe_app_start_log *log);

// The time since the last mark becomes step `name`; marks now.
void voe_app_start_log_step(voe_app_start_log *log, const char *name);

// The sum of the steps' seconds.
double voe_app_start_log_total(const voe_app_start_log *log);

// Writes the block to stderr and, when path is not NULL, appends it to path.
// False only when the file write failed, with platform's line on stderr.
[[nodiscard]] bool voe_app_start_log_write(const voe_app_start_log *log,
					   const char *program,
					   const char *path,
					   voe_base_arena *scratch);
