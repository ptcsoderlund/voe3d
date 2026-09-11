// A run of measurements, and the three things anything in this engine ever asks
// one: how many there were, what they averaged, and what the worst of them was.
//
// It is compact pure data with support functions and it has no owner, the same
// standing a maths type has: the fields are read directly and there is no system
// behind it.
//
//     voe_base_samples frame = { 0 };
//
//     voe_base_samples_add(&frame, seconds);          // once per frame
//     ...
//     printf("%.2f ms average, %.2f ms worst, over %llu\n",
//            voe_base_samples_average(&frame) * 1000.0,
//            frame.worst * 1000.0,
//            (unsigned long long)frame.count);
//     voe_base_samples_reset(&frame);                 // start the next period
//
// IT IS A PERIOD AND NOT A SLIDING WINDOW, AND THAT IS THE WHOLE REASON THERE IS
// NO RING BUFFER IN HERE. A sliding window needs somewhere to keep the last N
// values so the oldest can be dropped, which is a capacity to pick, a wrap to
// get wrong, and a `count` that stops meaning what it says the moment the ring
// fills. What a reporting loop actually wants is "since I last printed", and
// three numbers answer that exactly: reset, add for a while, read, reset. The
// average is then over precisely the frames `count` says it is over, and the
// worst is the worst of precisely those. If something later needs a value that
// decays rather than one that starts again, that is a different type and not a
// flag on this one.
//
// `worst` IS THE LARGEST VALUE AND THAT PRESUMES A DIRECTION. Most things
// measured with one of these are durations, and a longer duration is a worse
// one, but it is the direction that matters and not the unit: `dev`'s readout
// measures a frame's draw command count with one, and more draw commands is
// worse. Naming it `max` would be more general and would say less. A quantity
// where small is bad does not belong in this type.
//
// AN EMPTY RUN AVERAGES ZERO RATHER THAN REFUSING. A reporting period during
// which nothing happened is an ordinary thing — a minimised window, a first
// period that ended early — and a caller that has to guard every read is a
// caller that will forget once. Zero is also what `worst` reads as, so an empty
// run prints as three noughts and says what it is.
#pragma once

#include <stdint.h>

typedef struct {
	// How many values have been added since the last reset.
	uint64_t count;
	// Their total. Kept rather than an average kept up to date, because
	// adding to a running average needs a division per sample to say what
	// one division at the end says.
	double sum;
	// The largest value added since the last reset, and 0 when none has
	// been.
	double worst;
} voe_base_samples;

// Back to empty. A zeroed struct is already reset, so this is for starting the
// next period rather than for the first one.
void voe_base_samples_reset(voe_base_samples *samples);

// One measurement. A negative value is the caller's bug and asserts: every
// quantity this type is for is a duration, and a duration that ran backwards
// means the clock was read in the wrong order somewhere upstream.
void voe_base_samples_add(voe_base_samples *samples, double value);

// The mean of everything added since the last reset, and 0 when nothing has
// been.
double voe_base_samples_average(const voe_base_samples *samples);
