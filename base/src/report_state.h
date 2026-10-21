// One thread's report state, saved whole and loaded whole on another thread.
// Internal to base, as report_line.h is: nothing outside base includes it.
//
// It exists for the pool (ADR-0448 point 1): a task on a pool thread runs under
// its caller's subjects (ADR-0445), and hands back the error it kept so the
// caller can take it. The pool saves the caller's state, loads it on the thread
// that runs the task, and saves that thread's state again when the task is done.
//
// Subjects are copied as pointers, not as text: the caller's strings must
// outlive every task that runs under them. The kept error is copied as text.
#pragma once

#include "report_line.h"

#include <stddef.h>

struct voe_base_report_state {
	const char *subjects[VOE_BASE_REPORT_SUBJECTS];
	size_t subject_count;
	char first_error[VOE_BASE_REPORT_LINE_CAPACITY];
	bool first_error_kept;
};

// Copies the calling thread's subjects and kept first error into `out`.
void voe_base_report_state_save(struct voe_base_report_state *out);

// Makes the calling thread's subjects and kept first error exactly `in`'s:
// what it had before is gone, a kept error included.
void voe_base_report_state_load(const struct voe_base_report_state *in);
