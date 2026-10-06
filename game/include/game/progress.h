// The progress a splash wait's worker reports and its main thread shows
// (decision 0370 point 3): a phase, a count done of a total, and a stop flag.
//
//     // worker, before every step
//     voe_game_progress_set(progress, "Preparing shaders", step, steps);
//     if (voe_game_progress_stopped(progress))
//             return false;               // the window is closing
//     // main thread, every frame
//     voe_game_progress_line(progress, line, sizeof(line));
//
// WHO WRITES WHAT. The worker writes phase, done and total; the main thread
// reads them and writes stop, which the worker reads. Two threads touch the
// record at once, so every field is atomic: no lock, and neither side ever
// waits on the other. The three fields are stored one by one, so a line read
// mid-set may pair a new count with the old phase for one frame; it only
// shows.
//
// Constraints: the phase is a string the worker keeps alive while the record
// is read, a literal in practice, and in the font's characters (text/font.h).
// The record is the wait's (game/starting.h); a worker given NULL reports
// nothing and is never stopped.
#pragma once

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct voe_game_progress {
	_Atomic(const char *) phase;
	atomic_uint done;
	atomic_uint total;
	atomic_bool stop;
} voe_game_progress;

// The phase and the count done of total; total 0 is a phase with no count.
// Nothing on a NULL progress.
void voe_game_progress_set(voe_game_progress *progress, const char *phase,
			   unsigned done, unsigned total);

// Whether the main thread has asked the worker to stop; false on NULL.
bool voe_game_progress_stopped(voe_game_progress *progress);

// "phase done/total" into buffer, or the phase alone when total is 0, cut to
// size with its terminator. A NULL phase writes the empty line.
void voe_game_progress_line(voe_game_progress *progress, char *buffer,
			    size_t size);
