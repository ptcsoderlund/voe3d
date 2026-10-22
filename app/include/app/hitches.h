// A stalled frame told from its mean, and one line naming what took the time
// (decision 0458 points 1 and 2). A frame loop hands over each frame's wall
// interval with its named CPU stretches, and each measured card frame's total
// with render's pass times; a hitch comes back true with its line written.
//
//     voe_app_hitches hitches = { 0 };
//     char line[VOE_APP_HITCH_LINE];
//     if (voe_app_hitches_frame(&hitches, tick.elapsed, stretches, 5, line,
//                               sizeof line))
//             fprintf(stderr, "editor: hitch: %s\n", line);
//
// THE RULE. Each of the two keeps an exponential mean of weight 1/60, seeded
// by its first sample. None is a hitch before 60 samples. A sample is a hitch
// when it is above 1.5 x the mean taken before it and above 1/60 s. Every
// sample is folded into its mean, a hitch too, so a scene that turns slower
// for good stops being reported once the mean has caught up.
//
// AGAINST ITS OWN MEAN, not a fixed budget: a card or a scene of any speed has
// its own ordinary frame, and what is wanted is the frame that broke from it.
// The 1/60 s floor keeps a fast scene's small wobbles quiet.
//
// THE LINES, in milliseconds to one decimal, the five largest parts, largest
// first (ties in the order given):
//     frame 52.3 ms (mean 16.6): views 31.0, open 12.4
//     card 41.2 ms (mean 9.1): bounce sun shadow 4x 30.1, bounce relight 6.0
// On the card's line, passes of one name are summed and shown with their
// count; a span (`<pass>: <span>`) is a name of its own. Stretches are listed
// as given, each on its own: they may nest, which is the caller's choice. The
// line is cut to `room`, its NUL inside it; nothing is written when false.
//
// Constraints: pure, no file and no stderr; the caller writes the line. A NULL
// line with a room, or NULL arrays with a count, asserts. Passes are summed by
// name with a scan of every pass against every other, quadratic in `count`;
// fine for a frame's few dozen, and a table by name would lift it.
#pragma once

#include <render/device.h>

#include <stddef.h>
#include <stdint.h>

// The bytes a line needs with its NUL: five parts of VOE_RENDER_PASS_NAME each.
#define VOE_APP_HITCH_LINE 256

// One named CPU stretch of a frame, as the caller measured it, in seconds.
typedef struct {
	const char *name;
	double seconds;
} voe_app_stretch;

// Zeroed is nothing seen yet. This module's; read it, change it only through
// the calls below.
typedef struct {
	double frame_mean;
	uint32_t frame_samples;
	double card_mean;
	uint32_t card_samples;
} voe_app_hitches;

// Folds `elapsed` (seconds) into the frame mean; true, with the line written,
// when it was a hitch.
bool voe_app_hitches_frame(voe_app_hitches *hitches, double elapsed,
			   const voe_app_stretch *stretches, uint32_t count,
			   char *line, size_t room);

// The same for a measured card frame's `total` and its passes.
bool voe_app_hitches_card(voe_app_hitches *hitches, double total,
			  const voe_render_pass_time *passes, uint32_t count,
			  char *line, size_t room);
