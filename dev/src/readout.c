// The block printed every reporting period and the readout laid out every
// frame, out of the samples main.c takes. See readout.h for who calls what and
// when.
//
// NOTHING HERE MEASURES ANYTHING. Every number comes in through the struct the
// loop fills; this file only decides how a period's numbers are worded, and
// what a line says for the one frame a period has no sample of its own.
#include "readout.h"

#include <3d/mesh_component.h>
#include <base/assert.h>
#include <platform/input.h>

#include <stdio.h>

// The string the readout holds is formatted into a buffer this long, which is
// well over the seven short lines it now holds.
#define READOUT_CHARS 192

// The legend, once, at startup. It is here and not repeated in every block
// because the four names never change and the block is meant to be glanced at,
// and it is printed at all because a number whose meaning is ambiguous is worse
// than no number.
void voe_dev_say_what_is_measured(void)
{
	printf("timing     four numbers every %.0f s, each averaged over that period with its worst\n",
	       VOE_DEV_REPORT_SECONDS);
	printf("           frame   one top of the loop to the next. The rate is its reciprocal\n");
	printf("           update  the poll, the input and the three systems\n");
	printf("           draw    the loop's draw phase, begin to end: the wait for the frame\n");
	printf("                   slot, the acquire, the readout's layout and upload, the\n");
	printf("                   recording, the submit and the present. On fifo the wait\n");
	printf("                   for the display is in here and is most of it\n");
	printf("           gpu     the graphics card's own clock, over that frame's commands\n");
	printf("                   only — the wait for the display is not in it. It runs two\n");
	printf("                   frames behind, and is absent on a card that cannot time\n");
	printf("           P switches between fifo and mailbox. Numbers from both are what\n");
	printf("           the frame-pacing decision wants; the mode is on every block below\n");
	printf("           The same five numbers stand top-left of the view as the period's\n");
	printf("           running averages, laid out again every frame\n");
}

// The readout's text this frame, out of what the period has measured so far,
// laid out through the transient path and put on the readout entity. Called
// between _begin and the draw system, which is the only place it can be called.
//
// AND SINCE CARD 028 IT DRAWS THEM TOO. The same numbers stand in the top-left
// of the view as the period's running averages, rebuilt every frame as a text
// block through voe_text_block_create_transient and never cached: a readout that
// remembered its string until it changed would, the day its invalidation missed,
// show yesterday's numbers and look exactly like a frozen program. The console
// block stays, because a period's worst is a different, still-useful thing.
//
// And in the top-left of the view, lines of numbers that change as you watch:
// the frame rate, the four timings — the same ones the console prints — the
// pointer, and what the last frame cost in draw commands and element records.
// Laid out again every frame.
//
// THE DRAWS LINE IS A PERIOD METRIC LIKE THE FOUR ABOVE IT, average and worst
// over the same window, and the element records beside it are the last
// completed frame's. It is built before anything is drawn, so the one frame
// after each console block has no sample yet and shows the previous period's;
// a program that has completed no frame at all says "no frame yet" rather than
// showing a nought that reads as a claim.
//
// AND THE PAIR ON IT IS ADR-0092'S CLAIM RATHER THAN A NUMBER STANDING FOR IT.
// Ninety-odd element records against thirty-odd draw commands is what "many
// small things in one draw" means; watch the records climb while the commands
// hold still. The count includes the draw that put this readout on screen,
// which is why counting the things you can see gives one fewer.
//
// AND IT COUNTS BOTH PASSES, WHICH IS WHY IT IS ABOUT TWICE WHAT IS ON SCREEN.
// The monitor walks the same world into its own target before the window's pass
// runs, so nearly everything visible is drawn twice a frame. The console line
// printed once at startup says how many of the frame's commands the window's own
// walk was, which is the half that matches what a person can count.
//
// THE WORST COLUMN IS THE POINT OF MAKING IT A METRIC. In this demo the scene
// is identical every frame, so the average and the worst are both the same
// number and the column looks like decoration. The day anything varies what is
// drawn — culling, streaming, an interface that grows — the worst frame in the
// period is the number that matters and the average is the one that hides it.
//
// THE NUMBERS ARE THE RUNNING AVERAGES OF THE CURRENT PERIOD. The console block
// prints a period's average and worst once the period is over; this prints the
// average so far, every frame, so it settles over the first few hundred
// milliseconds of a period and starts again with the next block. The rate is the
// reciprocal of the frame average rather than a count over elapsed time, because
// the frame sample always holds at least this frame. For the one frame after
// each console block the other three read nought — the period has no sample of
// them yet — and printing last period's number instead would be the cache this
// card forbids in a smaller coat.
//
// THERE IS NO CACHE AND NO "HAS IT CHANGED". The string is formatted and laid
// out every frame whether or not a digit moved. It is a few dozen glyphs and
// well under a tenth of a millisecond, and it is the design that was decided; a
// readout that stops changing is therefore a bug in the transient path and never
// a cache being clever.
//
// `glyphs` comes back as how many characters were laid out, so that what the
// readout actually consumed can be printed once beside what was asked for.
//
// What is wrong if it looks wrong:
//
//   - THE READOUT'S NUMBERS FROZEN WHILE THE CONSOLE BLOCKS KEEP COMING — the
//     transient path. Either the block is not being rebuilt each frame, or a
//     stale id is being drawn and refused on stderr every frame. There is no
//     cache anywhere that could make them merely slow to update, so frozen is
//     always a bug and never a saving.
//   - The readout's left edge jumping sideways as a number gains a digit — it
//     is being centred on its width. It is meant to be left-aligned, which is
//     why its placement asks nothing about its size.
//
// What there is to try:
//
//   - READ THE SAME NUMBERS OFF THE SCREEN. The readout shows the period's
//     running averages, so it settles over the first few hundred milliseconds
//     after each console block and should then agree with the block that
//     follows, to within the averaging. Watch a few periods go by: it resets
//     with every block, and the frame rate moves the moment the window is
//     resized or minimised and restored. The `gpu` line must not flash `no
//     measurement` at the reset — it holds the last period's average for the
//     one frame the new period has no sample yet. If it does flash, the
//     three-case fallback in build_the_readout has been collapsed to two.
//   - WATCH THE `mouse` LINE OF THE READOUT (card 029). It is the pointer's
//     position in the window's own pixels and the three buttons, live. Move
//     the pointer to each corner: top-left should read close to 0 0 and
//     bottom-right one less than the `size` line in both numbers, with neither
//     overshooting nor inverting — a swapped or mirrored axis is a backend
//     mistake. Press the buttons: the dots become L, M and R. Hold one and drag
//     out of the window: the numbers go negative or past the size and keep
//     following, which is the drag both window systems promise. Move the
//     pointer off the window with nothing held and the line says `away`.
//     Press Tab to fly and it says `locked`: the camera has the pointer and
//     there is nothing at its position to point at, as include/platform/input.h
//     says. Tab back and move, and the numbers are live again.
bool voe_dev_build_the_readout(voe_ecs_world *world, voe_render_device *gpu,
			       const voe_text_font *font, voe_base_arena *arena,
			       voe_ecs_entity readout,
			       voe_platform_window *window,
			       const struct voe_dev_timing *timing,
			       uint32_t *glyphs, voe_base_error *error)
{
	char gpu_line[READOUT_CHARS];
	char draws_line[READOUT_CHARS];
	char mouse_line[READOUT_CHARS];
	char text[READOUT_CHARS];
	voe_text_block block;
	double frame = voe_base_samples_average(&timing->frame);
	// The same two cases gpu has below, and for the same reason: the period
	// was reset with this readout still running, so for one frame per period
	// there is no sample of the draw phase yet and last period's average is
	// what holds the line still. Before the first console block there is no
	// last one either, and nought is then the truth rather than a stale
	// number.
	double draw = timing->draw.count > 0 ?
			      voe_base_samples_average(&timing->draw) :
			      timing->draw_last;
	uint32_t drawn = 0;
	voe_platform_pointer pointer = voe_platform_input_pointer(window);
	// The buttons as three letters in the order they sit on a mouse, a dot
	// for one that is up. Read every frame like everything else here: this
	// is a call site showing what platform hands out, not a GUI.
	char left = voe_platform_input_button_down(window,
						   VOE_PLATFORM_BUTTON_LEFT) ?
			    'L' :
			    '.';
	char middle = voe_platform_input_button_down(
			      window, VOE_PLATFORM_BUTTON_MIDDLE) ?
			      'M' :
			      '.';
	char right = voe_platform_input_button_down(
			     window, VOE_PLATFORM_BUTTON_RIGHT) ?
			     'R' :
			     '.';

	// THREE CASES AND NOT TWO, BECAUSE THE PERIOD IS RESET WITH THE READOUT
	// STILL RUNNING. A period with samples shows its running average. A
	// period with none yet — the one frame after each console block, since
	// the timestamp is asked for after the draw and this is built before it
	// — shows the last full period's average, so the line holds still
	// rather than flashing the fallback below for a single frame that
	// mailbox sometimes presents. Only a card that has never answered gets
	// the fallback, which is then the truth.
	if (timing->gpu.count > 0)
		snprintf(gpu_line, sizeof gpu_line, "gpu    %6.2f ms",
			 voe_base_samples_average(&timing->gpu) * 1000.0);
	else if (timing->gpu_timed)
		snprintf(gpu_line, sizeof gpu_line, "gpu    %6.2f ms",
			 timing->gpu_last * 1000.0);
	else
		snprintf(gpu_line, sizeof gpu_line, "gpu    no measurement");

	// THREE CASES, THE SAME THREE THE GPU LINE HAS AND FOR THE SAME REASON.
	// A period with samples shows its running average and worst. The one
	// frame after each console block has neither yet — the count is added
	// after the draw and this is built before it — and shows the last full
	// period's, so the line holds still rather than flashing. Only a program
	// that has not completed a frame at all gets the third case, which is
	// then the truth.
	//
	// AND THE COUNT INCLUDES THE DRAW THAT PUT THIS READOUT ON SCREEN. The
	// readout is a mesh like any other and is drawn like any other, so a
	// person counting the things they can see will find one fewer than this
	// says. That is correct and it is not adjusted for: subtracting it would
	// make this a number about something other than what the frame did.
	//
	// THE RECORDS BESIDE IT ARE ONE FRAME'S AND THE COMMANDS ARE A PERIOD'S,
	// which is a mixture on one line and is deliberate — see struct voe_dev_timing.
	// The pair only means something if both halves describe the same drawing.
	if (timing->draws.count > 0)
		snprintf(draws_line, sizeof draws_line,
			 "draws  %5.1f avg %4.0f worst  %u elements",
			 voe_base_samples_average(&timing->draws),
			 timing->draws.worst,
			 (unsigned)timing->drawn_elements);
	else if (timing->draws_measured)
		snprintf(draws_line, sizeof draws_line,
			 "draws  %5.1f avg %4.0f worst  %u elements",
			 timing->draws_last, timing->draws_last_worst,
			 (unsigned)timing->drawn_elements);
	else
		snprintf(draws_line, sizeof draws_line, "draws  no frame yet");

	// Three states and three words, because a number that looked live
	// while the pointer was locked or gone is exactly what the platform
	// header refuses to hand out. The position is printed whole: Wayland
	// reports fractions and they are not interesting to a person.
	if (pointer.over)
		snprintf(mouse_line, sizeof mouse_line,
			 "mouse  %5.0f %5.0f %c%c%c", pointer.x, pointer.y,
			 left, middle, right);
	else if (voe_platform_input_pointer_locked(window))
		snprintf(mouse_line, sizeof mouse_line, "mouse  locked      %c%c%c",
			 left, middle, right);
	else
		snprintf(mouse_line, sizeof mouse_line, "mouse  away        %c%c%c",
			 left, middle, right);

	snprintf(text, sizeof text,
		 "%5.0f fps\nframe  %6.2f ms\nupdate %6.2f ms\ndraw   %6.2f ms\n%s\n%s\n%s",
		 frame > 0.0 ? 1.0 / frame : 0.0, frame * 1000.0,
		 voe_base_samples_average(&timing->update) * 1000.0,
		 draw * 1000.0, gpu_line, draws_line, mouse_line);

	// Spaces and newlines lay out nothing; every other character in this
	// string is a glyph the font carries.
	for (const char *at = text; *at != '\0'; at++)
		if (*at != ' ' && *at != '\n')
			drawn++;
	*glyphs = drawn;

	if (!voe_text_block_create_transient(font, gpu, arena, text, VOE_DEV_READOUT_EM,
					     &block, error))
		return false;

	// The entity is this program's and is never destroyed, so a mesh that
	// is not there is a bug here and not a thing to handle.
	VOE_BASE_ASSERT(voe_3d_mesh_set_geometry(world, readout, block.geometry),
			"the readout entity has lost its mesh");
	return true;
}

// One block, and then the period starts again. `seconds` is how long the period
// really lasted rather than VOE_DEV_REPORT_SECONDS, because a frame straddles the end of
// one and the rate has to be over the time actually covered.
//
// THE GPU LINE IS ABSENT RATHER THAN NOUGHT WHEN THERE IS NO MEASUREMENT. A card
// that cannot write timestamps would otherwise report a graphics card that takes
// no time at all, which is the most misleading thing this could print.
//
// What there is to try:
//
//   - WATCH THE TIMING BLOCKS, AND WATCH THEM MOVE. On fifo, `frame` should sit
//     within a few tenths of a millisecond of the display's refresh interval —
//     16.7 ms at sixty hertz — and the reciprocal printed beside it should be
//     the refresh rate. `draw` should be nearly all of it and `update` almost
//     none: the program is waiting for the display, which is what fifo means.
//     Then make something happen: drag the window bigger and `gpu` should go up
//     with the pixel count, minimise it and the blocks should stop until it is
//     shown again (0215).
//     A number that never moves is a number that is not being measured.
//   - Hold the window still and read the `worst` column. It should be close to
//     the average. A worst several times the average is stutter, and stutter is
//     the thing a person actually notices — which is why it is printed at all.
void voe_dev_report(struct voe_dev_timing *timing, double seconds,
		    voe_render_present present)
{
	printf("timing     %llu frames in %.2f s — %.1f per second, %s\n",
	       (unsigned long long)timing->frame.count, seconds,
	       seconds > 0.0 ? (double)timing->frame.count / seconds : 0.0,
	       present == VOE_RENDER_PRESENT_MAILBOX ? "mailbox" : "fifo");
	printf("           frame  %7.2f ms avg  %7.2f ms worst\n",
	       voe_base_samples_average(&timing->frame) * 1000.0,
	       timing->frame.worst * 1000.0);
	printf("           update %7.2f ms avg  %7.2f ms worst\n",
	       voe_base_samples_average(&timing->update) * 1000.0,
	       timing->update.worst * 1000.0);
	printf("           draw   %7.2f ms avg  %7.2f ms worst\n",
	       voe_base_samples_average(&timing->draw) * 1000.0,
	       timing->draw.worst * 1000.0);
	if (timing->gpu.count > 0)
		printf("           gpu    %7.2f ms avg  %7.2f ms worst\n",
		       voe_base_samples_average(&timing->gpu) * 1000.0,
		       timing->gpu.worst * 1000.0);
	else
		printf("           gpu        no measurement — this card or its queue cannot write timestamps\n");
	// The one line here that is not milliseconds, in the same two columns as
	// the four that are. What varies it is the scene rather than the
	// machine, so in this demo the average and the worst are the same number
	// — and the day they differ is the day something started drawing more in
	// some frames than others, which is exactly what a worst column is for.
	printf("           draws  %7.1f    avg  %7.0f    worst\n",
	       voe_base_samples_average(&timing->draws), timing->draws.worst);
	fflush(stdout);

	// What the readout shows until this new period has a sample of its own.
	// Only the two measured after it is built need it; see struct voe_dev_timing.
	if (timing->draw.count > 0)
		timing->draw_last = voe_base_samples_average(&timing->draw);
	if (timing->gpu.count > 0)
		timing->gpu_last = voe_base_samples_average(&timing->gpu);
	// Both halves of this one, because the readout shows both.
	if (timing->draws.count > 0) {
		timing->draws_last = voe_base_samples_average(&timing->draws);
		timing->draws_last_worst = timing->draws.worst;
	}

	voe_base_samples_reset(&timing->frame);
	voe_base_samples_reset(&timing->update);
	voe_base_samples_reset(&timing->draw);
	voe_base_samples_reset(&timing->gpu);
	voe_base_samples_reset(&timing->draws);
}
